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

#include "miniecu.pb.h"
#include "flash.pb.h"
#include "param.h"
#include <string.h>

/* -*- configuration -*- */

// Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0, MSb first).
static const SPIConfig spi1_cfg = {
	NULL,
	GPIOB,
	GPIOB_FLASH_CS,
	0,
	SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
};

static SST25Driver SST25_chip;
static const SST25Config flash_cfg = {
	.spip = &SPID1,
	.spicfg = &spi1_cfg
};

static SST25Driver SST25_config;
static SST25Driver SST25_error;
static SST25Driver SST25_log;

#define EPAGES	(4096/256)
static const struct sst25_partition init_parts[] = {
	{ &SST25_config, { .name = "config", .start_page = 0, .nr_pages = EPAGES * 4 /* 16 KiB */ } },
	{ &SST25_error, { .name = "error", .start_page = EPAGES * 4, .nr_pages = EPAGES * 16 /* 64 KiB */ } },
	{ &SST25_log, { .name = "log", .start_page = EPAGES * 4 + EPAGES * 16, .nr_pages = UINT32_MAX /* all above */ } },
	{ NULL }
};

static uint8_t m_rw_buff[256]; /* note: for sst25 */

static CONDVAR_DECL(m_cfg_operation);
static MUTEX_DECL(m_cfg_op_mtx);

static thread_t *thdp_log = NULL;
#define EVT_TIMEOUT			MS2ST(10000)
#define DO_SAVE_CFG_EVMASK		EVENT_MASK(3)
#define DO_LOAD_CFG_EVMASK		EVENT_MASK(4)
#define DO_ERASE_CFG_EVMASK		EVENT_MASK(5)
#define DO_ERASE_LOG_EVMASK		EVENT_MASK(6)

/* -*- submodules -*- */
#include "flash_param.c"

/* -*- public -*- */

bool memdump_ll_flash_readpage(uint32_t page, uint8_t *rbuff)
{
	if (blkGetDriverState(&SST25_chip) != BLK_ACTIVE)
		return HAL_FAILED;

	return blkRead(&SST25_chip, page, rbuff, 1);
}

THD_FUNCTION(th_flash_log, arg ATTR_UNUSED)
{
	bool is_first_loop = true;
	eventmask_t mask = 0;

	thdp_log = chThdGetSelfX();

	sst25ObjectInit(&SST25_chip);
	sst25Start(&SST25_chip, &flash_cfg);

	while (true) {
		if (is_first_loop)
			is_first_loop = false;
		else
			mask = chEvtWaitAnyTimeout(ALL_EVENTS, EVT_TIMEOUT);

		/* initialization */
		if (blkGetDriverState(&SST25_chip) != BLK_ACTIVE) {
			if (blkConnect(&SST25_chip) != HAL_SUCCESS) {
				alert_component(ALS_FLASH, AL_FAIL);
				debug_printf(DP_FAIL, "FLASH connection failed");
				continue;
			}

			alert_component(ALS_FLASH, AL_NORMAL);
			sst25InitPartitionTable(&SST25_chip, init_parts);
			mask = DO_LOAD_CFG_EVMASK;
		}

		if (mask & DO_LOAD_CFG_EVMASK) {
			debug_printf(DP_INFO, "requested load cfg");
			flash_param_load();
			chCondSignal(&m_cfg_operation);
		}

		if (mask & DO_SAVE_CFG_EVMASK) {
			debug_printf(DP_INFO, "requested save cfg");
			flash_param_save();
			chCondSignal(&m_cfg_operation);
		}

		if (mask & DO_ERASE_CFG_EVMASK) {
			debug_printf(DP_WARN, "requested erase cfg");
			mtdErase(&SST25_config, 0, UINT32_MAX);
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

