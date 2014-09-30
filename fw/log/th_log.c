/**
 * @file       th_flash_log.c
 * @brief      FLASH task
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
#include "th_log.h"
#include "flash-mtd.h"

#define INIT_TIMEOUT	MS2ST(5000)

/* -*- local data -*- */
static MUTEX_DECL(m_init_mtx);
static CONDVAR_DECL(m_log_init_done);
static THD_WORKING_AREA(wa_log, LOG_WASZ);


/* -*- thread -*- */
static THD_FUNCTION(th_log, arg ATTR_UNUSED)
{
	/* TODO */

	chCondSignal(&m_log_init_done);
	while (true) {
		/* TODO */
		chThdSleepMilliseconds(1000);
	}
}


/* -*- public functions -*- */

void log_init(void)
{
	chThdCreateStatic(wa_log, sizeof(wa_log), LOG_PRIO, th_log, NULL);

	chMtxLock(&m_init_mtx);
	chCondWaitTimeout(&m_log_init_done, INIT_TIMEOUT);
	chMtxUnlock(&m_init_mtx);
}
