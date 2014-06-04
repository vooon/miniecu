/**
 * @file       th_comm.c
 * @brief      communication thread
 * @author     Vladimir Ermakov Copyright (C) 2014.
 * @see        The GNU Public License (GPL) Version 3
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "alert_led.h"
#include "th_comm.h"
#include "pbstx.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "param.h"
#include "rtc_time.h"
#include "th_adc.h"
#include "th_rpm.h"
#include "th_command.h"

/* global parameters */

int32_t g_engine_id;
int32_t g_serial_baud;
int32_t g_status_period;
bool g_debug_enable_adc_raw;
bool g_debug_enable_memdump;

/* Thread */
static void send_status(void);
static void recv_time_reference(uint8_t msg_len);
static void recv_command(uint8_t msg_len);
static void recv_param_request(uint8_t msg_len);
static void recv_param_set(uint8_t msg_len);
static void recv_log_request(uint8_t msg_len);
static void recv_memory_dump_request(uint8_t msg_len);


#define MEMDUMP_SIZE	64
int32_t memdump_int_ram(uint32_t address, void *buffer, size_t size);
int32_t memdump_ext_flash(uint32_t address, void *buffer, size_t size);

/* Local varables */
static uint8_t msg_buf[256];
static SerialConfig serial1_cfg = {
	.speed = SERIAL_DEFAULT_BITRATE,
	.cr1 = 0,
	/* 8N1, autobaud mode 1 */
	.cr2 = USART_CR2_STOP1_BITS | USART_CR2_ABREN | USART_CR2_ABRMODE_0,
	.cr3 = 0
};


THD_FUNCTION(th_comm, arg ATTR_UNUSED)
{
	msg_t ret;
	uint8_t msgid;
	uint8_t in_msg_len;
	systime_t send_time = 0;

	while (true) {
		if (chTimeElapsedSince(send_time) >= MS2ST(g_status_period)) {
			send_status();
			send_time = chTimeNow();
		}

		pbstx_check_usb();
		ret = pbstx_receive(&msgid, msg_buf, &in_msg_len);
		if (ret == MSG_OK) {
			switch (msgid) {
				case miniecu_MessageId_TIME_REFERENCE:
					recv_time_reference(in_msg_len);
					break;
				case miniecu_MessageId_COMMAND:
					recv_command(in_msg_len);
					break;
				case miniecu_MessageId_PARAM_REQUEST:
					recv_param_request(in_msg_len);
					break;
				case miniecu_MessageId_PARAM_SET:
					recv_param_set(in_msg_len);
					break;
				case miniecu_MessageId_LOG_REQUEST:
					recv_log_request(in_msg_len);
					break;
				case miniecu_MessageId_MEMORY_DUMP_REQUEST:
					if (g_debug_enable_memdump)
						recv_memory_dump_request(in_msg_len);
					break;

				default:
					/* ALARM? */
					break;
			}
		}
	}
}

/**
 * @brief Send STATUS_TEXT message
 * @param severity message level
 * @param fmt formatting string @a chprintf()
 *
 * NOTE: baed on @a chsnprintf()
 */
void debug_printf(enum severity severity, char *fmt, ...)
{
	va_list ap;
	MemoryStream ms;
	BaseSequentialStream *chp;
	uint8_t local_msg_buf[82];
	pb_ostream_t outstream = pb_ostream_from_buffer(local_msg_buf,
			sizeof(local_msg_buf));
	miniecu_StatusText st;

	msObjectInit(&ms, (uint8_t *)st.text, sizeof(st.text), 0);
	chp = (BaseSequentialStream *)&ms;

	st.engine_id = g_engine_id;
	st.severity = severity;
	va_start(ap, fmt);
	chvprintf(chp, fmt, ap);
	va_end(ap);

	/* final zero */
	chSequentialStreamPut(chp, 0);

	if (!pb_encode(&outstream, miniecu_StatusText_fields, &st)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	pbstx_send(miniecu_MessageId_STATUS_TEXT,
			local_msg_buf, outstream.bytes_written);
}

void on_serial1_change(const struct param_entry *p ATTR_UNUSED)
{
	switch (g_serial_baud) {
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 115200:
	case 230400:
	case 460800:
	case 921600:
		debug_printf(DP_WARN, "serial baud change: %d", g_serial_baud);
		serial1_cfg.speed = g_serial_baud;
		sdStart(&SD1, &serial1_cfg);
		break;

	default:
		g_serial_baud = serial1_cfg.speed;
		break;
	}
}

static void send_status(void)
{
	pb_ostream_t outstream = pb_ostream_from_buffer(msg_buf, sizeof(msg_buf));
	miniecu_Status status;
	uint32_t flags = 0;

	if (time_is_known())		flags |= miniecu_Status_Flags_TIME_KNOWN;
	if (command_check_ignition())	flags |= miniecu_Status_Flags_IGNITION_ENABLED;
	if (command_check_starter())	flags |= miniecu_Status_Flags_STARTER_ENABLED;
	if (rpm_check_engine_running())	flags |= miniecu_Status_Flags_ENGINE_RUNNING;

	if (alert_check_error())	flags |= miniecu_Status_Flags_ERROR;
	if (batt_check_voltage())	flags |= miniecu_Status_Flags_UNDERVOLTAGE;
	if (temp_check_temperature())	flags |= miniecu_Status_Flags_OVERHEAT;
	if (rpm_check_limit())		flags |= miniecu_Status_Flags_HIGH_RPM;
	if (flow_check_fuel())		flags |= miniecu_Status_Flags_LOW_FUEL;

	memset(&status, 0, sizeof(status));
	status.engine_id = g_engine_id;
	status.status = flags;
	status.timestamp_ms = time_get_timestamp();
	status.rpm = rpm_get_filtered();

	/* battery */
	status.battery.voltage = batt_get_voltage();
	// status.battery.current = batt_get_current(); /* Not supported by hw_v2 */
	status.battery.has_remaining = batt_get_remaining(&status.battery.remaining);

	/* temperature */
	status.temperature.engine1 = temp_get_temperature();
	status.temperature.has_engine2 = oilp_get_temperature(&status.temperature.engine2);

	/* CPU status */
	status.cpu.load = 0; /* TODO */
	status.cpu.temperature = temp_get_int_temperature();

	/* Oil pressure */
	status.has_oil_pressure = oilp_get_pressure(&status.oil_pressure);

	/* Fuel flow status */
	if ((status.has_fuel = flow_get_flow(&status.fuel.flow_ml)) == true) {
		status.fuel.total_used_ml = flow_get_used_ml();
		status.fuel.has_remaining = flow_get_remaining(&status.fuel.remaining);
	}

	if (g_debug_enable_adc_raw) {
		status.has_adc_raw = true;
		status.adc_raw.temp = adc_getll_temp();
		status.adc_raw.oilp = adc_getll_oilp();
		status.adc_raw.flow = adc_getll_flow();
		status.adc_raw.battv = adc_getll_vbat();
		status.adc_raw.rtc_batt = adc_getll_vrtc();
	}

	/* TODO: Fill status */

	if (!pb_encode(&outstream, miniecu_Status_fields, &status)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}
	else {
		alert_component(ALS_COMM, AL_NORMAL);
	}

	pbstx_send(miniecu_MessageId_STATUS,
			msg_buf, outstream.bytes_written);
}

static void recv_time_reference(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_TimeReference time_ref;

	if (!pb_decode(&instream, miniecu_TimeReference_fields, &time_ref)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	/* answer to broadcast too */
	if (time_ref.engine_id != (unsigned)g_engine_id
			&& time_ref.engine_id != 0)
		return;

	pb_ostream_t outstream = pb_ostream_from_buffer(msg_buf, sizeof(msg_buf));

	time_ref.engine_id = g_engine_id;
	time_ref.has_system_time = true;
	time_ref.system_time = ST2MS(chTimeNow());
	time_ref.has_timediff = true;
	time_ref.timediff = time_set_timestamp(time_ref.timestamp_ms);

	if (!pb_encode(&outstream, miniecu_TimeReference_fields, &time_ref)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	pbstx_send(miniecu_MessageId_TIME_REFERENCE,
			msg_buf, outstream.bytes_written);
}

void send_command_response(uint32_t operation, uint32_t response)
{
	uint8_t local_msg_buf[32];
	pb_ostream_t outstream = pb_ostream_from_buffer(local_msg_buf, sizeof(local_msg_buf));
	miniecu_Command cmd;

	cmd.engine_id = g_engine_id;
	cmd.operation = operation;
	cmd.has_response = true;
	cmd.response = response;

	if (!pb_encode(&outstream, miniecu_Command_fields, &cmd)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	pbstx_send(miniecu_MessageId_COMMAND,
			local_msg_buf, outstream.bytes_written);
}

static void recv_command(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_Command cmd;

	if (!pb_decode(&instream, miniecu_Command_fields, &cmd)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	if (cmd.engine_id != (unsigned)g_engine_id)
		return;

	/* note: th_command can send response later */
	send_command_response(cmd.operation, command_request(cmd.operation));
}

static void send_param_value(miniecu_ParamValue *pv_msg)
{
	pb_ostream_t outstream = pb_ostream_from_buffer(msg_buf, sizeof(msg_buf));

	if (!pb_encode(&outstream, miniecu_ParamValue_fields, pv_msg)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	pbstx_send(miniecu_MessageId_PARAM_VALUE,
			msg_buf, outstream.bytes_written);
}

static void recv_param_request(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_ParamRequest param_req;
	miniecu_ParamValue param_value;
	size_t idx, count = param_count();

	if (!pb_decode(&instream, miniecu_ParamRequest_fields, &param_req)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	/* we answer to broadcast reqest too */
	if (param_req.engine_id != (unsigned)g_engine_id &&
			param_req.engine_id != 0)
		return;

	if (param_req.has_param_id) {
		/* request one param */
		if (param_get(param_req.param_id, &param_value.value, &idx) != PARAM_OK)
			return;

		param_value.engine_id = g_engine_id;
		param_value.param_index = idx;
		param_value.param_count = count;
		strncpy(param_value.param_id, param_req.param_id, PT_ID_SIZE);

		send_param_value(&param_value);
	}
	else {
		/* request all */
		for (idx = 0; idx < count; idx++) {
			if (param_get_by_idx(idx, param_value.param_id, &param_value.value) != PARAM_OK)
				continue;

			param_value.engine_id = g_engine_id;
			param_value.param_index = idx;
			param_value.param_count = count;

			send_param_value(&param_value);
		}
	}
}

static void recv_param_set(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_ParamSet _param_set;
	miniecu_ParamValue param_value;
	size_t idx, count = param_count();

	if (!pb_decode(&instream, miniecu_ParamSet_fields, &_param_set)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	if (_param_set.engine_id != (unsigned)g_engine_id)
		return;

	msg_t ret = param_set(_param_set.param_id, &_param_set.value);
	if (ret != PARAM_OK && ret != PARAM_LIMIT)
		return;

	if (param_get(_param_set.param_id, &param_value.value, &idx) != PARAM_OK)
		return;

	param_value.engine_id = g_engine_id;
	param_value.param_index = idx;
	param_value.param_count = count;
	strncpy(param_value.param_id, _param_set.param_id, PT_ID_SIZE);

	send_param_value(&param_value);
}

static void recv_log_request(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_LogRequest log_req;

	if (!pb_decode(&instream, miniecu_LogRequest_fields, &log_req)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	/* TODO */
}

static void send_memory_dump_page(miniecu_MemoryDumpPage *page_msg)
{
	pb_ostream_t outstream = pb_ostream_from_buffer(msg_buf, sizeof(msg_buf));

	if (!pb_encode(&outstream, miniecu_MemoryDumpPage_fields, page_msg)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	pbstx_send(miniecu_MessageId_MEMORY_DUMP_PAGE,
			msg_buf, outstream.bytes_written);
}

static void recv_memory_dump_request(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_MemoryDumpRequest dump_req;

	if (!pb_decode(&instream, miniecu_MemoryDumpRequest_fields, &dump_req)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	/* NOTE: ssize_t missing in chibios so we use int32_t instead */
	miniecu_MemoryDumpPage page_msg;
	uint32_t address = dump_req.address;
	int32_t bytes_rem = dump_req.size;
	int32_t (*memdump)(uint32_t address, void *buffer, size_t size) = NULL;

	switch (dump_req.type) {
	case miniecu_MemoryDumpRequest_Type_RAM:
		memdump = memdump_int_ram;
		break;
	case miniecu_MemoryDumpRequest_Type_FLASH:
		memdump = memdump_ext_flash;
		break;

	default:
		debug_printf(DP_ERROR, "MemDump: unknown type");
		return;
	};

	while (bytes_rem > 0) {
		int32_t ret = memdump(address,
				page_msg.page.bytes,
				(bytes_rem > MEMDUMP_SIZE)? MEMDUMP_SIZE : bytes_rem);

		if (ret <= 0) {
			debug_printf(DP_ERROR, "MemDump: read error");
			return;
		}

		page_msg.engine_id = g_engine_id;
		page_msg.stream_id = dump_req.stream_id;
		page_msg.address = address;
		page_msg.page.size = ret;

		address += ret;
		bytes_rem -= ret;

		send_memory_dump_page(&page_msg);
	}
}

