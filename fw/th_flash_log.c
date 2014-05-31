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

/* -*- submodules -*- */
#include "flash_param.c"

/* -*- public -*- */

bool_t memdump_ll_flash_readpage(uint32_t page, uint8_t *rbuff)
{
	if (blkGetDriverState(&SST25_chip) != BLK_ACTIVE)
		return CH_FAILED;

	return blkRead(&SST25_chip, page, rbuff, 1);
}

THD_FUNCTION(th_flash_log, arg ATTR_UNUSED)
{
	spiStart(&SPID1, &spi1_cfg);

	sst25ObjectInit(&SST25_chip);
	sst25Start(&SST25_chip, &flash_cfg);

	while (true) {
		chThdSleepMilliseconds(1000);

		if (blkGetDriverState(&SST25_chip) != BLK_ACTIVE) {
			if (blkConnect(&SST25_chip) != CH_SUCCESS) {
				alert_component(ALS_FLASH, AL_FAIL);
				debug_printf(DP_FAIL, "FLASH connection failed");
				continue;
			}

			alert_component(ALS_FLASH, AL_NORMAL);
			sst25InitPartitionTable(&SST25_chip, init_parts);

			flash_param_load();
			flash_param_save();
		}
	}
}

