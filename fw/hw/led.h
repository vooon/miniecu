/**
 * @file       led.h
 * @brief      Led functions.
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

#ifndef HW_LED_H
#define HW_LED_H

#include "fw_common.h"

#if !defined(BOARD_MINIECU_V2)
# error "unknown board"
#endif

static inline void led_all_off(void)
{
	palClearPad(GPIOA, GPIOA_LED_R);
	palClearPad(GPIOA, GPIOA_LED_G);
}

/** LED Halt state: Red
 */
static inline void led_halt_state(void)
{
	palSetPad(GPIOA, GPIOA_LED_R);
	palClearPad(GPIOA, GPIOA_LED_G);
}

/** LED Init state: blink Red
 */
static inline void led_init_toggle(void)
{
	palClearPad(GPIOA, GPIOA_LED_G);
	palTogglePad(GPIOA, GPIOA_LED_R);
}

/** LED Fail state: blink Green + Red
 */
static inline void led_fail_toggle(void)
{
	palSetPad(GPIOA, GPIOA_LED_R);
	palTogglePad(GPIOA, GPIOA_LED_G);
}

/** LED Normal state: blink Green
 */
static inline void led_normal_toggle(void)
{
	palClearPad(GPIOA, GPIOA_LED_R);
	palTogglePad(GPIOA, GPIOA_LED_G);
}

#endif /* HW_LED_H */
