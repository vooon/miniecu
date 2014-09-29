/**
 * @file       memdump.c
 * @brief      memdump functions
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

#include "fw_common.h"
#include "hw/ext_flash.h"
#include <string.h>

int32_t memdump_int_ram(uint32_t address, void *buffer, size_t size)
{
	void *ptr = (void *) address;
	memmove(buffer, ptr, size);
	return size;
}

int32_t memdump_ext_flash(uint32_t address, void *buffer, size_t size)
{
	uint8_t rd_buff[mtdGetPageSize(&FLASHD1)];	// C99 dynamic array
	int32_t size_ret = 0;

	if (blkGetDriverState(&FLASHD1) != BLK_ACTIVE)
		return -1;

	while (size_ret < size) {
		uint32_t page = address / sizeof(rd_buff);
		uint32_t off = address % sizeof(rd_buff);
		int32_t sz = sizeof(rd_buff) - off;

		if (sz > size - size_ret)
			sz = size - size_ret;
		if (blkRead(&FLASHD1, page, rd_buff, 1) != HAL_SUCCESS)
			return -1;

		memcpy(buffer, rd_buff + off, sz);
		address += sz;
		buffer += sz;
		size_ret += sz;
	}

	return size_ret;
}

