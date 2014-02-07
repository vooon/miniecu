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

#include "ch.h"
#include "hal.h"
#include "evtimer.h"
#include "th_comm.h"

#include <string.h>

#include "pbstx.h"
#include "pb_encode.h"
#include "pb_decode.h"


/* Thread */
static WORKING_AREA(wa_comm, 512);
static msg_t thd_comm(void *arg __attribute__((unused)));
static void send_status(void);

/* Local varables */
static EvTimer status_et;
static uint8_t msg_buf[256];

#define STATUS_TIMEOUT	MS2ST(1000)


void th_comm_init(void)
{
	chThdCreateStatic(wa_comm, sizeof(wa_comm), HIGHPRIO, thd_comm, NULL);
}

static msg_t thd_comm(void *arg __attribute__((unused)))
{
	msg_t ret;
	uint8_t msgid;
	uint8_t in_msg_len;
	EventListener el0;

	chRegSetThreadName("comm");

	evtInit(&status_et, STATUS_TIMEOUT);
	chEvtRegister(&status_et.et_es, &el0, 0);

	evtStart(&status_et);

	while (!chThdShouldTerminate()) {
		eventmask_t mask = chEvtGetAndClearEvents(ALL_EVENTS);

		if (mask & EVENT_MASK(0)) {
			send_status();
		}

		ret = pbstx_receive(&msgid, msg_buf, &in_msg_len);
		if (ret == RDY_OK) {
			/* TODO: respond to requests */
		}
	}

	return 0;
}

static void send_status(void)
{
	pb_ostream_t outstream = pb_ostream_from_buffer(msg_buf, sizeof(msg_buf));
	miniecu_Status status;

	memset(&status, 0, sizeof(status));
	memcpy(status.engine_id, "eng2", 4);
	status.timestamp_ms = chTimeNow() * 1000 / CH_FREQUENCY; /* systime -> msec */

	/* TODO: Fill status */

	pb_encode(&outstream, miniecu_Status_fields, &status);

	pbstx_send(miniecu_MessageId_STATUS,
			msg_buf, outstream.bytes_written);
}

