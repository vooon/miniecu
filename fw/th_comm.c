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


/* Thread */
static void send_status(void);
static void recv_time_reference(uint8_t msg_len);
static void recv_command(uint8_t msg_len);
static void recv_param_request(uint8_t msg_len);
static void recv_param_set(uint8_t msg_len);
static void recv_log_request(uint8_t msg_len);

/* Local varables */
static uint8_t msg_buf[256];

#define STATUS_TIMEOUT	MS2ST(1000) // TODO: make it settable


THD_FUNCTION(th_comm, arg ATTR_UNUSED)
{
	msg_t ret;
	uint8_t msgid;
	uint8_t in_msg_len;
	systime_t send_time = 0;

	pbstx_init();

	while (true) {
		if (chTimeElapsedSince(send_time) >= STATUS_TIMEOUT) {
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

static void send_status(void)
{
	pb_ostream_t outstream = pb_ostream_from_buffer(msg_buf, sizeof(msg_buf));
	miniecu_Status status;

	memset(&status, 0, sizeof(status));
	status.engine_id = 1;
	status.timestamp_ms = ST2MS(chTimeNow());

	/* TODO: Fill status */

	if (!pb_encode(&outstream, miniecu_Status_fields, &status)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
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

	/* TODO: set RTC time, calculate diff, return current time */

	pb_ostream_t outstream = pb_ostream_from_buffer(msg_buf, sizeof(msg_buf));

	time_ref.engine_id = 1;
	time_ref.has_system_time = true;
	time_ref.system_time = ST2MS(chTimeNow());
	time_ref.has_timediff = true;
	time_ref.timediff = 9000;

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

static void recv_param_request(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_ParamRequest param_req;

	if (!pb_decode(&instream, miniecu_ParamRequest_fields, &param_req)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	/* TODO */
}

static void recv_param_set(uint8_t msg_len)
{
	pb_istream_t instream = pb_istream_from_buffer(msg_buf, msg_len);
	miniecu_ParamSet param_set;

	if (!pb_decode(&instream, miniecu_ParamSet_fields, &param_set)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	/* TODO */
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

