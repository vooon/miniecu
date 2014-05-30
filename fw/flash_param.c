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

//static bool flash_pb_istream_cb(pb_istream_t *stream, uint8_t *buf, size_t count)
//{
//	return false;
//}

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
			/* finalize buffer */
			if (!flash_pb_ostream_finalize(stream))
				return false;
		}

		size_ret += ret;
	}

	return true;
}

/* -*- public -*- */

void flash_param_load(void)
{
//static pb_istream_t ostream = {flash_pb_istream, NULL, 0, SIZE_MAX};
}

void flash_param_save(void)
{
	//flash_ParamStorageArray param_array;
	flash_pb_state_t state = { 0 };
	flash_param_header_t header = { PARAM_SIGNATURE, PARAM_VERSION, 0, {0,0} };

	msObjectInit(&state.buffer, m_rw_buff, sizeof(m_rw_buff), 0);
	pb_ostream_t ostream = {flash_pb_ostream_cb, &state, mtdGetSize(&SST25_config), 0};

	/* erase flash */
	mtdErase(&SST25_config, 0, UINT32_MAX);

	header.counter = ++m_flash_param_cnt;
	pb_write(&ostream, (const uint8_t *)&header, sizeof(header));

	//param_array.vars.funcs.encode = NULL;

	/* finalize stream */
	flash_pb_ostream_finalize(&ostream);
	debug_printf(DP_INFO, "parameters saved #%u, %u bytes", m_flash_param_cnt, ostream.bytes_written);
}

