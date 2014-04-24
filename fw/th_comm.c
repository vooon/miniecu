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

/* global parameters */

int32_t g_engine_id;
int32_t g_serial_baud;
int32_t g_status_period;

/* Thread */
static void send_status(void);
static void recv_time_reference(uint8_t msg_len);
static void recv_command(uint8_t msg_len);
static void recv_param_request(uint8_t msg_len);
static void recv_param_set(uint8_t msg_len);
static void recv_log_request(uint8_t msg_len);

/* Local varables */
static uint8_t msg_buf[256];


THD_FUNCTION(th_comm, arg ATTR_UNUSED)
{
	msg_t ret;
	uint8_t msgid;
	uint8_t in_msg_len;
	systime_t send_time = 0;

	param_init();
	pbstx_init();

	while (true) {
		if (chTimeElapsedSince(send_time) >= MS2ST(g_status_period)) {
			send_status();
			send_time = chTimeNow();
		}

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
	uint8_t local_msg_buf[68];
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
	debug_printf(DP_DEBUG, "serial1 baud change: %d", g_serial_baud);
}

static void send_status(void)
{
	pb_ostream_t outstream = pb_ostream_from_buffer(msg_buf, sizeof(msg_buf));
	miniecu_Status status;
	uint32_t flags = 0;

	if (time_is_known())		flags |= miniecu_Status_Flags_TIME_KNOWN;
	if (alert_check_error())	flags |= miniecu_Status_Flags_ERROR;
	if (batt_check_voltage())	flags |= miniecu_Status_Flags_UNDERVOLTAGE;

	memset(&status, 0, sizeof(status));
	status.engine_id = g_engine_id;
	status.status = flags;
	status.timestamp_ms = time_get_timestamp();

	status.battery.voltage = batt_get_voltage();
	/* status.battery.current = batt_get_current(); Not supported by hw_v2 */
	status.battery.has_remaining = batt_get_remaining(&status.battery.remaining);

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

static void recv_command(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_Command cmd;

	if (!pb_decode(&instream, miniecu_Command_fields, &cmd)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	/* TODO */
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

