/**
 * @file       flash_param.c
 * @brief      FLASH Parameter storage submodule
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

#include "pb_encode.h"
#include "pb_decode.h"
#include "pios_crc.h"

// For big endian:
//#define PARAM_SIGNATURE		0x706172616d763130
#define PARAM_SIGNATURE		0x3031766d61726170
#define PARAM_VERSION		0x30303100

typedef struct {
	uint64_t signature;
	uint32_t version;
	uint32_t counter;
	uint64_t reserved[2];
} flash_param_header_t;

typedef struct {
	uint32_t page;
	MemoryStream buffer;
} flash_pb_state_t;

/* -*- submodule data -*- */

static uint32_t m_flash_param_cnt = 0;

/* -*- pb_istream_t flash read functions -*- */

static bool flash_pb_istream_read_next(pb_istream_t *stream)
{
	flash_pb_state_t *state = stream->state;

	state->buffer.eos = state->buffer.offset = 0;

	if (blkRead(&SST25_config, state->page, state->buffer.buffer, 1) != CH_SUCCESS)
		return false;

	state->buffer.eos = mtdGetPageSize(&SST25_config);
	state->page += 1;
	return true;
}

static bool flash_pb_istream_cb(pb_istream_t *stream, uint8_t *buf, size_t count)
{
	flash_pb_state_t *state = stream->state;
	BaseSequentialStream *chp = (BaseSequentialStream *)&state->buffer;
	size_t size_ret = 0;

	if (state->buffer.eos == 0)
		if (!flash_pb_istream_read_next(stream))
			return false;

	while (size_ret < count) {
		size_t ret = chSequentialStreamRead(chp, buf, count - size_ret);
		if (ret < count - size_ret) {
			if (!flash_pb_istream_read_next(stream))
				return false;
		}

		size_ret += ret;
	}

	return true;
}

static bool flash_decode_repeated_parameter_storage(pb_istream_t *stream,
		const pb_field_t *field ATTR_UNUSED, void **arg ATTR_UNUSED)
{
	flash_ParamStorage storage;

	memset(&storage, 0, sizeof(storage));
	if (!pb_decode_noinit(stream, flash_ParamStorage_fields, &storage)) {
		debug_printf(DP_FAIL, "parameter decode error");
		return false;
	}

	uint8_t crc8 = PIOS_CRC_updateCRC(0, (uint8_t *)storage.param_id, PT_ID_SIZE);
	crc8 = PIOS_CRC_updateCRC(crc8, (uint8_t *)&storage.value, sizeof(storage.value));

	if (crc8 != storage.crc8) {
		debug_printf(DP_FAIL, "parameter CRC error '%16s'", storage.param_id);
		return true; /* we read corrupted parameter, but try check next items */
	}

	if (param_set(storage.param_id, &storage.value) != PARAM_OK)
		debug_printf(DP_WARN, "parameter '%s' set error", storage.param_id);

	return true;
}

/* -*- pb_ostream_t flash write functions -*- */

static bool flash_pb_ostream_finalize(pb_ostream_t *stream)
{
	flash_pb_state_t *state = stream->state;
	BaseSequentialStream *chp = (BaseSequentialStream *)&state->buffer;

	/* check buffer is empty */
	if (state->buffer.eos == 0)
		return true;

	/* fill tail */
	while (chSequentialStreamPut(chp, 0xFF) == RDY_OK);

	if (blkWrite(&SST25_config, state->page, state->buffer.buffer, 1) != CH_SUCCESS)
		return false;

	state->page += 1;
	state->buffer.eos = 0;
	return true;
}

static bool flash_pb_ostream_cb(pb_ostream_t *stream, const uint8_t *buf, size_t count)
{
	flash_pb_state_t *state = stream->state;
	BaseSequentialStream *chp = (BaseSequentialStream *)&state->buffer;
	size_t size_ret = 0;

	while (size_ret < count) {
		size_t ret = chSequentialStreamWrite(chp, buf, count - size_ret);
		if (ret < count - size_ret) {
			if (!flash_pb_ostream_finalize(stream))
				return false;
		}

		size_ret += ret;
	}

	return true;
}

static bool flash_encode_repeated_parameter_storage(pb_ostream_t *stream,
		const pb_field_t *field, void * const *arg ATTR_UNUSED)
{
	flash_ParamStorage storage;
	size_t idx, count = param_count();
	for (idx = 0; idx < count; idx++) {
		memset(&storage, 0, sizeof(storage));
		if (param_get_by_idx(idx, storage.param_id, &storage.value) != PARAM_OK)
			continue;

		uint8_t crc8 = PIOS_CRC_updateCRC(0, (uint8_t *)storage.param_id, PT_ID_SIZE);
		storage.crc8 = PIOS_CRC_updateCRC(crc8, (uint8_t *)&storage.value, sizeof(storage.value));

		if (!pb_encode_tag_for_field(stream, field))
			return false;

		if (!pb_encode_submessage(stream, flash_ParamStorage_fields, &storage))
			return false;
	}

	return true;
}

/* -*- submodule functions -*- */

static void flash_param_load(void)
{
	flash_ParamStorageArray param_array;
	flash_pb_state_t state = { 0 };
	flash_param_header_t header;

	msObjectInit(&state.buffer, m_rw_buff, sizeof(m_rw_buff), 0);
	pb_istream_t istream = {flash_pb_istream_cb, &state, mtdGetSize(&SST25_config)};

	if (!pb_read(&istream, (uint8_t *)&header, sizeof(header))) {
		alert_component(ALS_FLASH, AL_FAIL);
		debug_printf(DP_FAIL, "parameter read error");
		return;
	}

	if (header.signature != PARAM_SIGNATURE || header.version != PARAM_VERSION) {
		debug_printf(DP_WARN, "unknown parameter header");
		return;
	}

	m_flash_param_cnt = header.counter;

	param_array.vars.funcs.decode = flash_decode_repeated_parameter_storage;
	if (!pb_decode(&istream, flash_ParamStorageArray_fields, &param_array)) {
		alert_component(ALS_FLASH, AL_FAIL);
		debug_printf(DP_FAIL, "parameter load error");
		return;
	}

	debug_printf(DP_INFO, "parameters loaded #%u", m_flash_param_cnt);
}

static void flash_param_save(void)
{
	flash_ParamStorageArray param_array;
	flash_pb_state_t state = { 0 };
	flash_param_header_t header = { PARAM_SIGNATURE, PARAM_VERSION, 0, {0,0} };
	uint64_t null_terminator = 0;

	msObjectInit(&state.buffer, m_rw_buff, sizeof(m_rw_buff), 0);
	pb_ostream_t ostream = {flash_pb_ostream_cb, &state, mtdGetSize(&SST25_config), 0};

	/* erase flash */
	mtdErase(&SST25_config, 0, UINT32_MAX);

	header.counter = ++m_flash_param_cnt;
	if (!pb_write(&ostream, (const uint8_t *)&header, sizeof(header)))
		return;

	param_array.vars.funcs.encode = flash_encode_repeated_parameter_storage;
	if (!pb_encode(&ostream, flash_ParamStorageArray_fields, &param_array)) {
		alert_component(ALS_FLASH, AL_FAIL);
		return;
	}

	/* finalize stream */
	if (!pb_write(&ostream, (const uint8_t *)&null_terminator, sizeof(null_terminator)))
		return;

	if (!flash_pb_ostream_finalize(&ostream))
		return;

	debug_printf(DP_INFO, "parameters saved #%u, %u bytes", m_flash_param_cnt, ostream.bytes_written);
}

