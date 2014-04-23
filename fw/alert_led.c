/**
 * @file       alert_led.c
 * @brief      led alarm functions
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

/* local variables */
static enum alert_status al_status[ALS_MAX];

#if defined(BOARD_MINIECU_V1)

static bool _led_state = false;

# define _LED_xxx_op(operation)	do {					\
	palSetPadMode(GPIOF, GPIOF_LED, PAL_MODE_OUTPUT_PUSHPULL);	\
	pal ## operation ## Pad(GPIOF, GPIOF_LED);			\
	_led_state = true;						\
} while(0)

# define LED_ALL_OFF()		do {					\
	palSetPadMode(GPIOF, GPIOF_LED, PAL_MODE_INPUT);		\
	_led_state = false;						\
} while(0)

# define LED_FAIL_ON()		_LED_xxx_op(Set)
# define LED_FAIL_TOGGLE()	if (_led_state) {			\
	LED_ALL_OFF();							\
} else {								\
	LED_FAIL_ON();							\
}

# define LED_NORMAL_ON()	_LED_xxx_op(Clear)
# define LED_NORMAL_TOGGLE()	if (_led_state) {			\
	LED_ALL_OFF();							\
} else {								\
	LED_NORMAL_ON();						\
}

#elif defined(BOARD_MINIECU_V2)

# define LED_ALL_OFF() do { \
	palClearPad(GPIOA_LED_R); \
	palClearPad(GPIOA_LED_G); \
} while (0)

# define _LED_op_xy(op, ledop, ledoff) do { \
	palClearPad(GPIOA, ledoff); \
	pal ## op ## Pad(GPIOA, ledop); \
} while (0)

# define LED_FAIL_ON()		_LED_op_xy(Set, GPIOA_LED_R, GPIOA_LED_G)
# define LED_FAIL_TOGGLE()	_LED_op_xy(Toggle, GPIOA_LED_R, GPIOA_LED_G)

# define LED_NORMAL_ON()	_LED_op_xy(Set, GPIOA_LED_G, GPIOA_LED_R)
# define LED_NORMAL_TOGGLE()	_LED_op_xy(Toggle, GPIOA_LED_G, GPIOA_LED_R)

#else
# error "unknown board"
#endif

/* public interface */

void alert_component(enum alert_source src, enum alert_status st)
{
	chDbgAssert((src < ALS_MAX), "alert_component", "alert source");

	al_status[src] = st;
	/* TODO: signall led thread */
}

void alert_init(void)
{
	for (int i = 0; i < ALS_MAX; i++)
		al_status[i] = AL_INIT;

	LED_FAIL_ON();
}

bool alert_check_error(void)
{
	for (int i = 0; i < ALS_MAX; i++)
		if (al_status[i] == AL_FAIL)
			return true;

	return false;
}

/* local functions */
THD_FUNCTION(th_led, arg ATTR_UNUSED)
{
	//alert_init();

	while (true) {
		enum alert_status st = AL_NORMAL;
		for (int i = 0; i < ALS_MAX; i++) {
			if (al_status[i] == AL_FAIL)
				st = AL_FAIL;
			else if (al_status[i] == AL_INIT && st != AL_FAIL)
				st = AL_INIT;
		}

		switch (st) {
		case AL_INIT:
			LED_FAIL_TOGGLE();
			chThdSleepMilliseconds(150);
			break;

		case AL_FAIL:
			LED_FAIL_ON();
			chThdSleepMilliseconds(500);
			break;

		case AL_NORMAL:
			LED_NORMAL_TOGGLE();
			chThdSleepMilliseconds(500);
			break;
		}

	}
}

