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
#include "th_comm_pbstx.h"
#include "pbstx.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "param.h"
#include "rtc_time.h"
#include "th_adc.h"
#include "th_rpm.h"
#include "th_command.h"

/* global parameters */

int32_t gp_engine_id;
int32_t gp_status_period;
bool gp_debug_enable_adc_raw;
bool gp_debug_enable_memdump;

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
static void recv_param_request(PBStxComm *self, pb_istream_t *instream);
static void recv_param_set(PBStxComm *self, pb_istream_t *instream);
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
static msg_t pbstxEncodeSend(PBStxDev *dev, pbstx_message_t *msg, const pb_field_t messagetype[], const void *message)
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
static msg_t pbstxEncodeSendComm(PBStxComm *self, const pb_field_t messagetype[], const void *message)
{
	return pbstxEncodeSend(&self->dev, &self->msg, messagetype, message);
}

/**
 * Encode and send message via all available channels
 *
 * @return MSG_OK if no errors on send.
 *         or last send error.
 */
static msg_t pbstxEncodeSendBroadcast(pbstx_message_t *msg, const pb_field_t messagetype[], const void *message)
{
	msg_t ret = MSG_OK;
	msg_t sret;

	msg->size = 0; // force encode
	for (int i = 0; i < MAX_INSTANCES; i++)
		if (m_instances[i] != NULL) {
			if (msg->size == 0)
				sret = pbstxEncodeSend(&m_instances[i]->dev, msg, messagetype, message);
			else
				sret = pbstxSend(&m_instances[i]->dev, msg);

			if (sret < 0)
				ret = sret;
		}

	return ret;
}

/**
 * This helper function decodes type of miniecu.Message
 *
 * Based on nanopb example using_union_messages/decode.c
 */
static const pb_field_t *pbstxDecodeType(pb_istream_t *stream)
{
	pb_wire_type_t wire_type;
	uint32_t tag;
	bool eof;

	while (pb_decode_tag(stream, &wire_type, &tag, &eof)) {
		if (wire_type == PB_WT_STRING) {
			const pb_field_t *field;
			for (field = miniecu_Message_fields; field->tag != 0; field++) {
				if (field->tag == tag && (field->type & PB_LTYPE_SUBMESSAGE))
					/* Found our field. */
					return field->ptr;
			}
		}

		/* Wasn't our field.. */
		pb_skip_field(stream, wire_type);
	}

	return NULL;
}

/** Decode message content
 */
static bool pbstxDecodeMessage(pb_istream_t *stream, const pb_field_t messagetype[], void *dest_struct)
{
	pb_istream_t substream;
	bool status;
	if (!pb_make_string_substream(stream, &substream))
		return false;

	status = pb_decode(&substream, messagetype, dest_struct);
	pb_close_string_substream(stream, &substream);
	return status;
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
	pbstx_message_t msg;
	miniecu_StatusText st;

	msObjectInit(&ms, (uint8_t *)st.text, sizeof(st.text), 0);
	chp = (BaseSequentialStream *)&ms;

	st.engine_id = gp_engine_id;
	st.severity = severity;
	va_start(ap, fmt);
	chvprintf(chp, fmt, ap);
	va_end(ap);

	/* final zero */
	chSequentialStreamPut(chp, 0);

	pbstxEncodeSendBroadcast(&msg, miniecu_StatusText_fields, &st);
}


/** PBStxComm thread
 * @param[in] arg	pointer to BaseChannel device
 */
static THD_FUNCTION(th_comm_pbstx, arg)
{
	osalDbgCheck(arg != NULL);

	msg_t ret;
	int instance_id;
	systime_t send_time = 0;
	PBStxComm self;

	chRegSetThreadName("pbstx");
	pbstxObjectInit(&self.dev, (BaseChannel*)arg);

	// store instance m_instances for broadcast messages
	for (instance_id = 0; instance_id < MAX_INSTANCES; instance_id++) {
		if (m_instances[instance_id] == NULL) {
			m_instances[instance_id] = &self;
			break;
		}
	}
	if (instance_id >= MAX_INSTANCES)
		return MSG_RESET;

	alert_component(ALS_COMM, AL_NORMAL);

	//debug_printf(DP_DEBUG, "pbstx%d: started", instance_id);
	while (!chThdShouldTerminateX()) {
		if (chVTTimeElapsedSinceX(send_time) >= MS2ST(gp_status_period)) {
			send_status(&self);
			send_time = osalOsGetSystemTimeX();
		}

		ret = pbstxReceive(&self.dev, &self.msg);
		if (ret != MSG_OK)
			continue;

		pb_istream_t instream = pb_istream_from_buffer(self.msg.payload, self.msg.size);
		const pb_field_t *field = pbstxDecodeType(&instream);

		if (field == miniecu_ParamRequest_fields)
			recv_param_request(&self, &instream);
		else if (field == miniecu_ParamSet_fields)
			recv_param_set(&self, &instream);
	}

	if (m_instances[instance_id] != NULL)
		m_instances[instance_id] = NULL;

	debug_printf(DP_DEBUG, "pbstx%d: terminated", instance_id);
	return MSG_OK;
}

/** PBStxComm constructor
 * This function starts @a th_comm_pbstx thread
 *
 * @param chn	BaseChannel device pointer
 */
thread_t *pbstxCreate(void *chn, size_t size, tprio_t prio)
{
	return chThdCreateFromHeap(NULL, size, prio, th_comm_pbstx, chn);
}

/** PBStxComm methods
 * @{
 */

/** Send miniecu.Status message
 */
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

	status.engine_id = gp_engine_id;
	status.status = flags;
	status.timestamp_ms = time_get_timestamp();
	status.rpm = rpm_get_filtered();

	/* battery */
	status.battery.voltage = batt_get_voltage();
	status.battery.has_remaining = batt_get_remaining(&status.battery.remaining);

	/* temperature */
	status.temperature.engine = temp_get_temperature();
	status.temperature.has_oilp = oilp_get_temperature(&status.temperature.oilp);

	/* CPU status */
	//status.cpu.load = 0; /* TODO */
	status.cpu.has_temperature = true;
	status.cpu.temperature = temp_get_int_temperature();

	/* Oil pressure */
	//status.has_oil_pressure = oilp_get_pressure(&status.oil_pressure);

	/* Fuel flow status */
	if ((status.has_fuel = flow_get_flow(&status.fuel.flow_ml)) == true) {
		status.fuel.total_used_ml = flow_get_used_ml();
		status.fuel.has_remaining = flow_get_remaining(&status.fuel.remaining);
	}

	if (gp_debug_enable_adc_raw) {
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
#endif

/** Broadcasts miniecu.ParamValue
 */
static void send_param_value(pbstx_message_t *msg, miniecu_ParamValue *pv_msg)
{
	pbstxEncodeSendBroadcast(msg, miniecu_ParamValue_fields, pv_msg);
}

static void recv_param_request(PBStxComm *self, pb_istream_t *instream)
{
	miniecu_ParamRequest param_req;
	miniecu_ParamValue param_value;
	size_t idx, count = param_count();

	if (!pbstxDecodeMessage(instream, miniecu_ParamRequest_fields, &param_req)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	/* we answer to broadcast reqest too */
	if (param_req.engine_id != (unsigned)gp_engine_id &&
			param_req.engine_id != 0)
		return;

	if (param_req.has_param_id) {
		/* request one param */
		if (param_get(param_req.param_id, &param_value.value, &idx) != PARAM_OK)
			return;

		param_value.engine_id = gp_engine_id;
		param_value.param_index = idx;
		param_value.param_count = count;
		strncpy(param_value.param_id, param_req.param_id, PT_ID_SIZE);

		send_param_value(&self->msg, &param_value);
	}
	else {
		/* request all */
		for (idx = 0; idx < count; idx++) {
			if (param_get_by_idx(idx, param_value.param_id, &param_value.value) != PARAM_OK)
				continue;

			param_value.engine_id = gp_engine_id;
			param_value.param_index = idx;
			param_value.param_count = count;

			send_param_value(&self->msg, &param_value);
		}
	}
}

static void recv_param_set(PBStxComm *self, pb_istream_t *instream)
{
	miniecu_ParamSet param_set_;
	miniecu_ParamValue param_value;
	size_t idx, count = param_count();

	if (!pbstxDecodeMessage(instream, miniecu_ParamSet_fields, &param_set_)) {
		alert_component(ALS_COMM, AL_FAIL);
		return;
	}

	if (param_set_.engine_id != (unsigned)gp_engine_id)
		return;

	msg_t ret = param_set(param_set_.param_id, &param_set_.value);
	if (ret != PARAM_OK && ret != PARAM_LIMIT)
		return;

	if (param_get(param_set_.param_id, &param_value.value, &idx) != PARAM_OK)
		return;

	param_value.engine_id = gp_engine_id;
	param_value.param_index = idx;
	param_value.param_count = count;
	strncpy(param_value.param_id, param_set_.param_id, PT_ID_SIZE);

	send_param_value(&self->msg, &param_value);
}

#if 0
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
