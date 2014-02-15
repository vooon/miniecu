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
# error "RT kernel not supported!"

#elif defined(USE_NIL_KERNEL)
# include "nil.h"

# define ST2MS(st) (st * 1000 / NIL_CFG_ST_FREQUENCY)

#else
# error "Please choose kernel: Nil or RT"
#endif

#include "hal.h"

#define ATTR_UNUSED __attribute__((unused))

#endif /* FW_COMMON_H */
