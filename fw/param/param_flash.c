/**
 * @file       param_flash.c
 * @brief      FLASH Parameter storage
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
#include "lib_crc16.h"
#include "memstreams.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "flash.pb.h"
#include "param_table.h"
#include "hw/ext_flash.h"

// uint64_t representation of 'paramv10' in big endian format (reversed)
#define PARAM_SIGNATURE		0x3031766d61726170

//! Flash parameter storage header
typedef struct {
	uint64_t signature;
	uint32_t format_version;
	int32_t counter;
	uint64_t reserved[2];
} flash_param_header_t;

//! State for FLASH-nanoPB streams
typedef struct {
	uint32_t page;
	MemoryStream buffer;
} flash_pb_state_t;

/* -*- parameters -*- */
int32_t gp_param_save_cnt = 0;


/* -*- pb_istream_t flash read functions -*- */

static bool pb_istream_read_next(pb_istream_t *stream)
{
	flash_pb_state_t *state = stream->state;

	state->buffer.eos = state->buffer.offset = 0;

	if (blkRead(&FLASHD1_config, state->page, state->buffer.buffer, 1) != HAL_SUCCESS)
		return false;

	state->buffer.eos = mtdGetPageSize(&FLASHD1_config);
	state->page += 1;
	return true;
}

static bool pb_istream_cb(pb_istream_t *stream, uint8_t *buf, size_t count)
{
	flash_pb_state_t *state = stream->state;
	BaseSequentialStream *chp = (BaseSequentialStream *)&state->buffer;
	size_t size_ret = 0;

	if (state->buffer.eos == 0)
		if (!pb_istream_read_next(stream))
			return false;

	while (size_ret < count) {
		size_t ret = chSequentialStreamRead(chp, buf + size_ret, count - size_ret);
		if (ret < count - size_ret) {
			if (!pb_istream_read_next(stream))
				return false;
		}

		size_ret += ret;
	}

	return true;
}

/* -*- pb_ostream_t flash write functions -*- */

static bool pb_ostream_finalize(pb_ostream_t *stream)
{
	flash_pb_state_t *state = stream->state;
	BaseSequentialStream *chp = (BaseSequentialStream *)&state->buffer;

	/* check buffer is empty */
	if (state->buffer.eos == 0)
		return true;

	/* fill tail */
	while (chSequentialStreamPut(chp, 0xFF) == MSG_OK);

	if (blkWrite(&FLASHD1_config, state->page, state->buffer.buffer, 1) != HAL_SUCCESS)
		return false;

	state->page += 1;
	state->buffer.eos = 0;
	return true;
}

static bool pb_ostream_cb(pb_ostream_t *stream, const uint8_t *buf, size_t count)
{
	flash_pb_state_t *state = stream->state;
	BaseSequentialStream *chp = (BaseSequentialStream *)&state->buffer;
	size_t size_ret = 0;

	while (size_ret < count) {
		size_t ret = chSequentialStreamWrite(chp, buf + size_ret, count - size_ret);
		if (ret < count - size_ret) {
			if (!pb_ostream_finalize(stream))
				return false;
		}

		size_ret += ret;
	}

	return true;
}

/* -*- enc/dec repeated ParamStorage -*- */

static bool decode_repeated_ParamStorage(pb_istream_t *stream,
		const pb_field_t *field ATTR_UNUSED, void **arg ATTR_UNUSED)
{
	flash_ParamStorage storage = flash_ParamStorage_init_default;

	if (!pb_decode_noinit(stream, flash_ParamStorage_fields, &storage)) {
		debug_printf(DP_FAIL, "parameter decode error");
		return false;
	}

	uint16_t crc = crc16((uint8_t*)storage.param_id, PT_ID_SIZE);
	crc = crc16part((uint8_t*)&storage.value, sizeof(storage.value), crc);

	if (crc != storage.crc16) {
		debug_printf(DP_FAIL, "parameter CRC error '%16s'", storage.param_id);
		return true; /* we read corrupted parameter, but try check next items */
	}

	if (param_set(storage.param_id, &storage.value) != PARAM_OK)
		debug_printf(DP_WARN, "parameter '%s' set error", storage.param_id);

	return true;
}

static bool encode_repeated_ParamStorage(pb_ostream_t *stream,
		const pb_field_t *field, void * const *arg ATTR_UNUSED)
{
	flash_ParamStorage storage;
	size_t idx, count = param_count();

	for (idx = 0; idx < count; idx++) {
		// check PT_NSAVE flag
		msg_t flags = param_get_flags_by_idx(idx);
		if (flags < 0 || flags & PT_NSAVE)
			continue;

		memset(&storage, 0, sizeof(storage));
		if (param_get_by_idx(idx, storage.param_id, &storage.value) != PARAM_OK)
			continue;

		uint16_t crc = crc16((uint8_t*)storage.param_id, PT_ID_SIZE);
		storage.crc16 = crc16part((uint8_t*)&storage.value, sizeof(storage.value), crc);

		if (!pb_encode_tag_for_field(stream, field))
			return false;

		if (!pb_encode_submessage(stream, flash_ParamStorage_fields, &storage))
			return false;
	}

	return true;
}

/* -*- api functions -*- */

/** Load parameters from FLASHD1_config parfition
 */
void param_load(void)
{
	flash_ParamStorageArray param_array;
	flash_pb_state_t state = { 0 };
	flash_param_header_t header;
	uint8_t rd_buff[mtdGetPageSize(&FLASHD1_config)];

	msObjectInit(&state.buffer, rd_buff, sizeof(rd_buff), 0);
	pb_istream_t istream = { pb_istream_cb, &state, mtdGetSize(&FLASHD1_config) };

	/* read header */
	if (!pb_read(&istream, (uint8_t *)&header, sizeof(header))) {
		alert_component(ALS_FLASH, AL_FAIL);
		debug_printf(DP_FAIL, "parameter read error");
		return;
	}

	/* validate header */
	if (header.signature != PARAM_SIGNATURE ||
			header.format_version != param_format_version_be32) {
		debug_printf(DP_WARN, "unknown parameter header");
		return;
	}

	gp_param_save_cnt = header.counter;

	/* load param array */
	param_array.vars.funcs.decode = decode_repeated_ParamStorage;
	if (!pb_decode(&istream, flash_ParamStorageArray_fields, &param_array)) {
		alert_component(ALS_FLASH, AL_FAIL);
		debug_printf(DP_FAIL, "parameter load error");
		return;
	}

	debug_printf(DP_INFO, "parameters loaded #%i", gp_param_save_cnt);
}

/** Save parameters to FLASHD1_config partition
 */
void param_save(void)
{
	flash_ParamStorageArray param_array;
	flash_pb_state_t state = { 0 };
	flash_param_header_t header = { PARAM_SIGNATURE, param_format_version_be32, 0, {0,0} };
	uint64_t null_terminator = 0;
	uint8_t wr_buff[mtdGetPageSize(&FLASHD1_config)];

	msObjectInit(&state.buffer, wr_buff, sizeof(wr_buff), 0);
	pb_ostream_t ostream = { pb_ostream_cb, &state, mtdGetSize(&FLASHD1_config), 0 };

	/* erase flash */
	mtdErase(&FLASHD1_config, 0, UINT32_MAX);

	/* write header */
	header.counter = ++gp_param_save_cnt;
	if (!pb_write(&ostream, (const uint8_t *)&header, sizeof(header)))
		return;

	/* save param array */
	param_array.vars.funcs.encode = encode_repeated_ParamStorage;
	if (!pb_encode(&ostream, flash_ParamStorageArray_fields, &param_array)) {
		alert_component(ALS_FLASH, AL_FAIL);
		return;
	}

	/* finalize stream */
	if (!pb_write(&ostream, (const uint8_t *)&null_terminator, sizeof(null_terminator)))
		return;

	if (!pb_ostream_finalize(&ostream))
		return;

	debug_printf(DP_INFO, "parameters saved #%i, %u bytes", gp_param_save_cnt, ostream.bytes_written);
}

