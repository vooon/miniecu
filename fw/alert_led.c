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
#include "hw/led.h"

/* local variables */

static enum alert_status al_status[ALS_MAX];
static THD_WORKING_AREA(wa_led, LED_WASZ);

/* thread */

static THD_FUNCTION(th_led, arg ATTR_UNUSED)
{
	chRegSetThreadName("led");

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
			led_init_toggle();
			chThdSleepMilliseconds(150);
			break;

		case AL_FAIL:
			led_fail_toggle();
			chThdSleepMilliseconds(500);
			break;

		case AL_NORMAL:
			led_normal_toggle();
			chThdSleepMilliseconds(500);
			break;
		}
	}

	return MSG_RESET;
}

/* public interface */

/** Set component status
 */
void alert_component(enum alert_source src, enum alert_status st)
{
	osalDbgAssert((src < ALS_MAX), "alert source");

	al_status[src] = st;
}

/** Check that there no component in error state.
 */
bool alert_check_error(void)
{
	for (int i = 0; i < ALS_MAX; i++)
		if (al_status[i] == AL_FAIL)
			return true;

	return false;
}

/** Start alert led subsytem
 *
 * Starts @a th_led thread
 */
void alert_led_init(void)
{
	for (int i = 0; i < ALS_MAX; i++)
		al_status[i] = AL_INIT;

	chThdCreateStatic(wa_led, sizeof(wa_led), LED_PRIO, th_led, NULL);
}
