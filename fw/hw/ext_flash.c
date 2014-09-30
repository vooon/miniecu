/**
 * @file       hw/ext_flash.c
 * @brief      FLASH driver configuration
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
#include "ext_flash.h"

/* -*- global -*- */

//! SST25 Flash chip driver
SST25Driver FLASHD1;

SST25Driver FLASHD1_config;	//!< Config partition
SST25Driver FLASHD1_error;	//!< Error log partition
SST25Driver FLASHD1_log;	//!< Log partition


/* -*- local -*- */

//! Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0, MSb first).
static const SPIConfig spi1_cfg = {
	NULL,
	GPIOB,
	GPIOB_FLASH_CS,
	0,
	SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
};

//! SST25 Flash configuration
static const SST25Config flash1_cfg = {
	.spip = &SPID1,
	.spicfg = &spi1_cfg
};

//! SST25 pages per erase sector
#define EPAGES	(4096/256)

/** SST25 partition table
 *
 * Partitions:
 * - config: 16 KiB
 * - error: 64 KiB
 * - log: chip size - config - error
 */
static const struct sst25_partition init_parts[] = {
	{ &FLASHD1_config, { .name = "config", .start_page = 0, .nr_pages = EPAGES * 4 /* 16 KiB */ } },
	{ &FLASHD1_error, { .name = "error", .start_page = EPAGES * 4, .nr_pages = EPAGES * 16 /* 64 KiB */ } },
	{ &FLASHD1_log, { .name = "log", .start_page = EPAGES * 4 + EPAGES * 16, .nr_pages = UINT32_MAX /* all above */ } },
	{ NULL }
};


/* -*- initializer -*- */

void flash_init(void)
{
	sst25ObjectInit(&FLASHD1);
	sst25ObjectInit(&FLASHD1_config);
	sst25ObjectInit(&FLASHD1_error);
	sst25ObjectInit(&FLASHD1_log);
	sst25Start(&FLASHD1, &flash1_cfg);
}

msg_t flash_connect(void)
{
	if (blkGetDriverState(&FLASHD1) != BLK_ACTIVE) {
		if (blkConnect(&FLASHD1) != HAL_SUCCESS) {
			alert_component(ALS_FLASH, AL_FAIL);
			debug_printf(DP_FAIL, "FLASH connection failed");
			return MSG_RESET;
		}

		sst25InitPartitionTable(&FLASHD1, init_parts);
	}

	alert_component(ALS_FLASH, AL_NORMAL);
	return MSG_OK;
}
