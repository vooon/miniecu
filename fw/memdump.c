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
#include <string.h>

int32_t memdump_int_ram(uint32_t address, void *buffer, size_t size)
{
	void *ptr = (void *) address;
	memmove(buffer, ptr, size);
	return size;
}

int32_t memdump_ext_flash(uint32_t address ATTR_UNUSED, void *buffer ATTR_UNUSED, size_t size ATTR_UNUSED)
{
	/* TODO */
	return -1;
}

