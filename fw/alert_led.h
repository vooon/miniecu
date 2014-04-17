/**
 * @file       alert_led.h
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

#ifndef ALERT_LED_H
#define ALERT_LED_H

#include "fw_common.h"

enum alert_status {
	AL_INIT = 0,
	AL_FAIL,
	AL_NORMAL
};

enum alert_source {
	ALS_COMM = 0,
	ALS_ADC,
	//ALS_RTC,
	ALS_MAX /* max size of alert array */
};

THD_FUNCTION(th_led, arg ATTR_UNUSED);
void alert_init(void);
void alert_component(enum alert_source src, enum alert_status st);

#endif /* ALERT_LED_H */
