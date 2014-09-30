/**
 * @file       ectl_pads.h
 * @brief      Engine control
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

#ifndef HW_ECTL_PADS_H
#define HW_ECTL_PADS_H

#include "fw_common.h"

static inline bool ctl_ignition_state(void)
{
	return palReadPad(GPIOE, GPIOE_IGN_EN);
}

static inline bool ctl_starter_state(void)
{
	return palReadPad(GPIOE, GPIOE_STARTER);
}

/** Set ignition state
 *
 * @param state  true: On, flase: Off
 */
static inline void ctl_ignition_set(bool state)
{
	if (state)
		palSetPad(GPIOE, GPIOE_IGN_EN);
	else
		palClearPad(GPIOE, GPIOE_IGN_EN);
}

/** Set starter state
 *
 * @param state  true: On, flase: Off
 */
static inline void ctl_starter_set(bool state)
{
	if (state)
		palSetPad(GPIOE, GPIOE_STARTER);
	else
		palClearPad(GPIOE, GPIOE_STARTER);
}

#endif /* HW_ECTL_PADS_H */
