/**
 * @file       hw/ext_flash.h
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

#ifndef HW_EXT_FLASH_H
#define HW_EXT_FLASH_H

#include "fw_common.h"
#include "flash-mtd.h"


extern SST25Driver FLASHD1;
extern SST25Driver FLASHD1_config;
extern SST25Driver FLASHD1_error;
extern SST25Driver FLASHD1_log;


void flash_init(void);
msg_t flash_connect(void);

#endif /* HW_EXT_FLASH_H */
