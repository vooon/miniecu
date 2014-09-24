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
int32_t g_status_period;
bool g_debug_enable_adc_raw;
bool g_debug_enable_memdump;

/* PBStx class */

typedef struct {
	PBStxDev dev;
	pbstx_message_t msg;
} PBStxComm;

#define MAX_INSTANCES	2
PBStxComm *m_instances[MAX_INSTANCES] = {};

/* PBStx methods */
static void send_status(PBStxComm *self);
//static void recv_time_reference(uint8_t msg_len);
//static void recv_command(uint8_t msg_len);
//static void recv_param_request(uint8_t msg_len);
//static void recv_param_set(uint8_t msg_len);
//static void recv_log_request(uint8_t msg_len);
//static void recv_memory_dump_request(uint8_t msg_len);

//#define MEMDUMP_SIZE	64
//int32_t memdump_int_ram(uint32_t address, void *buffer, size_t size);
//int32_t memdump_ext_flash(uint32_t address, void *buffer, size_t size);

// -*- helpers -*-

/**
 * This helper function encodes union-like message miniecu.Message and send
 *
 * Based on nanopb example using_union_messages/encode.c
 *
 * @param dev		PBStx proto object
 * @param msg		message buffer
 * @param messagetype	submessage type defenition
 * @param message	submessage struct
 *
 * @return MSG_OK on success
 */
msg_t pbstxEncodeSend(PBStxDev *dev, pbstx_message_t *msg, const pb_field_t messagetype[], const void *message)
{
	pb_ostream_t outstream = pb_ostream_from_buffer(msg->payload, PBSTX_PAYLOAD_BYTES);

	const pb_field_t *field;
	for (field = miniecu_Message_fields; field->tag != 0; field++) {
		if (field->ptr == messagetype) {
			if (!pb_encode_tag_for_field(&outstream, field))
				goto err_out;

			if (!pb_encode_submessage(&outstream, messagetype, message))
				goto err_out;

			msg->size = outstream.bytes_written;
			return pbstxSend(dev, msg);
		}
	}

err_out:
	alert_component(ALS_COMM, AL_FAIL);
	return MSG_RESET;
}

/**
 * Variation of @a pbstxEncodeSend for PBStxComm objects
 */
msg_t pbstxEncodeSendComm(PBStxComm *self, const pb_field_t messagetype[], const void *message)
{
	return pbstxEncodeSend(&self->dev, &self->msg, messagetype, message);
}

// -*- thread main -*-

THD_FUNCTION(th_comm, arg)
{
	osalDbgCheck(arg != NULL);

	msg_t ret;
	int instance_id;
	systime_t send_time = 0;
	PBStxComm self;

	chRegSetThreadName("pbstx");
	pbstxObjectInit(&self.dev, (BaseChannel*)arg);

	// store instance m_instances
	for (instance_id = 0; instance_id < MAX_INSTANCES; instance_id++) {
		if (m_instances[instance_id] == NULL) {
			m_instances[instance_id] = &self;
			break;
		}
	}
	osalDbgAssert(instance_id < ARRAY_SIZE(m_instances), "pbstx: no space");

	while (!chThdShouldTerminateX()) {
		if (chVTTimeElapsedSinceX(send_time) >= MS2ST(g_status_period)) {
			send_status(&self);
			send_time = osalOsGetSystemTimeX();
		}

		ret = pbstxReceive(&self.dev, &self.msg);
		if (ret == MSG_OK) {
			// TODO
		}
	}

	if (m_instances[instance_id] != NULL)
		m_instances[instance_id] = NULL;

	return MSG_OK;
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
#if 0
	// XXX: relocate it to better module
	// XXX:
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
#endif
}

static void send_status(PBStxComm *self)
{
	miniecu_Status status = miniecu_Status_init_default;
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

	status.engine_id = g_engine_id;
	status.status = flags;
	status.timestamp_ms = time_get_timestamp();
	status.rpm = rpm_get_filtered();

	/* battery */
	status.battery.voltage = batt_get_voltage();
	status.battery.has_remaining = batt_get_remaining(&status.battery.remaining);

	/* temperature */
	status.temperature.engine1 = temp_get_temperature();
	status.temperature.has_engine2 = oilp_get_temperature(&status.temperature.engine2);

	/* CPU status */
	//status.cpu.load = 0; /* TODO */
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

	pbstxEncodeSendComm(self, miniecu_Status_fields, &status);
}

#if 0
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
	time_ref.system_time = ST2MS(osalOsGetSystemTimeX());
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

#endif
