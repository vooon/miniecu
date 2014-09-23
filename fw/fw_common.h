/**
 * @file       fw_common.h
 * @brief      common defenitions
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

#ifndef FW_COMMON_H
#define FW_COMMON_H

#include "fw_config.h"

#if defined(USE_RT_KERNEL)
# include "ch.h"

//# define OSAL_ST_FREQUENCY		CH_FREQUENCY
//# define THD_FUNCTION(name, arg)	msg_t name(void *arg)
//# define THD_WORKING_AREA(name, size)	WORKING_AREA(name, size)
//# define MSG_OK				RDY_OK
//# define MSG_RESET			RDY_RESET

#elif defined(USE_NIL_KERNEL)
# include "nil.h"
# error "ChibiOS/Nil not supported by now. "\
	"(some strange bug on F050, so we switch back to stable ChibiOS 2"
#else
# error "Please choose kernel: Nil or RT"
#endif

#include "hal.h"
#include "chprintf.h"
#include "memstreams.h"
#include <inttypes.h>

//#define ST2MS(st) (st * 1000 / OSAL_ST_FREQUENCY)

#define ATTR_UNUSED __attribute__((unused))

#define ARRAY_SIZE(_arr)	(sizeof((_arr)) / sizeof((_arr)[0]))

/* NOTE same as in miniecu.proto */
enum severity {
	DP_DEBUG = 0,
	DP_INFO,
	DP_WARN,
	DP_ERROR,
	DP_FAIL
};

void debug_printf(enum severity severity, char *fmt, ...)
	__attribute__((format (printf, 2, 3)));

/* flash-mtd driver messages */
#define MTD_DEBUG(fmt, args...)		debug_printf(DP_DEBUG, fmt, args)
#define MTD_INFO(fmt, args...)		debug_printf(DP_INFO, fmt, args)

#define arduino_map(x, in_min, in_max, out_min, out_max) \
	(((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + out_min)

#define chTimeElapsedSince(t)	(osalOsGetSystemTimeX() - t)

#endif /* FW_COMMON_H */
