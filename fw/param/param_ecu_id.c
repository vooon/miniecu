/**
 * @file       param_ecu_id.c
 * @brief      ECU identification parameters
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

#include "param.h"

/* special variables for host system (for identifying ECU) */
char gp_engine_name[PT_STRING_SIZE];
char gp_engine_serial_no[PT_STRING_SIZE];
char gp_ecu_serial_no[PT_STRING_SIZE];
char gp_ecu_fw_version[PT_STRING_SIZE];
char gp_ecu_hw_version[PT_STRING_SIZE];


void roinit_ecu_serial_no(struct param_entry *self ATTR_UNUSED)
{
#define STM32F37x_UID_BASE	0x1FFFF7AC
	uint32_t uid0 = *((__I uint32_t *) (STM32F37x_UID_BASE + 0x00));
	uint32_t uid1 = *((__I uint32_t *) (STM32F37x_UID_BASE + 0x04));
	uint32_t uid2 = *((__I uint32_t *) (STM32F37x_UID_BASE + 0x08));

	/* same number as calculated in USB DFU bootloader
	 * See @a https://my.st.com/public/STe2ecommunities/mcu/Tags.aspx?tags=id%20unique%20meaning%20shortening
	 */
	uint32_t sn_hi = (((uid2 >> 24) & 0xff) << 8) |
		(((uid2 >> 16) & 0xff) + ((uid0 >> 16) & 0xff));

	uint32_t sn_lo = (((uid2 >> 8) & 0xff) << 24) |
		(((uid2 & 0xff) + (uid0 & 0xff)) << 16) |
		(((uid1 >> 24) & 0xff) << 8) |	/* following two values don't match to DFU */
		(((uid1 >> 16) & 0xff));	/* it was uid2[31:24] and uid2[23:16] */

	chsnprintf(gp_ecu_serial_no, PT_STRING_SIZE, "SN%04x%08x", sn_hi, sn_lo);
}

void roinit_ecu_hw_version(struct param_entry *self ATTR_UNUSED)
{
	strncpy(gp_ecu_hw_version, BOARD_NAME, PT_STRING_SIZE);
}
