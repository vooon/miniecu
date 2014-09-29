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
#include "th_flash_log.h"
#include "flash-mtd.h"

static uint8_t m_rw_buff[256]; /* note: for sst25 */

static CONDVAR_DECL(m_cfg_operation);
static MUTEX_DECL(m_cfg_op_mtx);

static thread_t *thdp_log = NULL;
#define EVT_TIMEOUT			MS2ST(10000)
#define DO_SAVE_CFG_EVMASK		EVENT_MASK(3)
#define DO_LOAD_CFG_EVMASK		EVENT_MASK(4)
#define DO_ERASE_CFG_EVMASK		EVENT_MASK(5)
#define DO_ERASE_LOG_EVMASK		EVENT_MASK(6)

/* -*- public -*- */

THD_FUNCTION(th_flash_log, arg ATTR_UNUSED)
{
	bool is_first_loop = true;
	eventmask_t mask = 0;

	thdp_log = chThdGetSelfX();


	while (true) {
		if (is_first_loop)
			is_first_loop = false;
		else
			mask = chEvtWaitAnyTimeout(ALL_EVENTS, EVT_TIMEOUT);


		if (mask & DO_LOAD_CFG_EVMASK) {
			debug_printf(DP_INFO, "requested load cfg");
			//flash_param_load();
			chCondSignal(&m_cfg_operation);
		}

		if (mask & DO_SAVE_CFG_EVMASK) {
			debug_printf(DP_INFO, "requested save cfg");
			//flash_param_save();
			chCondSignal(&m_cfg_operation);
		}

		if (mask & DO_ERASE_CFG_EVMASK) {
			debug_printf(DP_WARN, "requested erase cfg");
			//mtdErase(&SST25_config, 0, UINT32_MAX);
			chCondSignal(&m_cfg_operation);
		}

		/* TODO */
	}
}

/* -*- functions for th_command -*- */

bool flash_do_load_cfg(systime_t timeout)
{
	if (thdp_log == NULL)
		return false;

	chMtxLock(&m_cfg_op_mtx);
	chEvtSignal(thdp_log, DO_LOAD_CFG_EVMASK);
	if (chCondWaitTimeout(&m_cfg_operation, timeout) != MSG_TIMEOUT) {
		chMtxUnlock(&m_cfg_op_mtx);
		return true;
	}
	else
		return false;
}

bool flash_do_save_cfg(systime_t timeout)
{
	if (thdp_log == NULL)
		return false;

	chMtxLock(&m_cfg_op_mtx);
	chEvtSignal(thdp_log, DO_SAVE_CFG_EVMASK);
	if (chCondWaitTimeout(&m_cfg_operation, timeout) != MSG_TIMEOUT) {
		chMtxUnlock(&m_cfg_op_mtx);
		return true;
	}
	else
		return false;
}

bool flash_do_erase_cfg(systime_t timeout)
{
	if (thdp_log == NULL)
		return false;

	chMtxLock(&m_cfg_op_mtx);
	chEvtSignal(thdp_log, DO_ERASE_CFG_EVMASK);
	if (chCondWaitTimeout(&m_cfg_operation, timeout) != MSG_TIMEOUT) {
		chMtxUnlock(&m_cfg_op_mtx);
		return true;
	}
	else
		return false;
}

