/**
 * @file       th_comm.c
 * @brief      communication thread
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
#include "param.h"

/* global parameters */
int32_t gp_serial1_baud;
char gp_serial1_proto[PT_STRING_SIZE];


static SerialConfig serial1_cfg = {
	.speed = SERIAL_DEFAULT_BITRATE,
	.cr1 = 0,
	/* 8N1, autobaud mode 1 */
	.cr2 = USART_CR2_STOP1_BITS | USART_CR2_ABREN | USART_CR2_ABRMODE_0,
	.cr3 = 0
};

void on_change_serial1_baud(const struct param_entry *p ATTR_UNUSED)
{
	switch (gp_serial1_baud) {
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 115200:
	case 230400:
	case 460800:
	case 921600:
		debug_printf(DP_WARN, "serial1 baud change: %i", gp_serial1_baud);
		serial1_cfg.speed = gp_serial1_baud;
		sdStart(&SERIAL1_SD, &serial1_cfg);
		break;

	default:
		gp_serial1_baud = serial1_cfg.speed;
		break;
	}
}
