/**
 * @file       th_command.c
 * @brief      Command task
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

#include "command.h"
#include "alert_led.h"
#include "hw/ectl_pads.h"
#include "hw/ext_flash.h"
#include "miniecu.pb.h"
#include "param.h"


uint32_t command_request(uint32_t cmdid)
{
	switch (cmdid) {
	case miniecu_Command_Operation_EMERGENCY_STOP:
		// TODO: stop other modules (if needed)
		ctl_ignition_set(false);
		ctl_starter_set(false);
		break;

	case miniecu_Command_Operation_IGNITION_ENABLE:
	case miniecu_Command_Operation_IGNITION_DISABLE:
		ctl_ignition_set(cmdid == miniecu_Command_Operation_IGNITION_ENABLE);
		return miniecu_Command_Response_ACK;

	case miniecu_Command_Operation_STARTER_ENABLE:
	case miniecu_Command_Operation_STARTER_DISABLE:
		ctl_starter_set(cmdid == miniecu_Command_Operation_STARTER_ENABLE);
		return miniecu_Command_Response_ACK;

	//case miniecu_Command_Operation_DO_ENGINE_START:
	//	break;
	//case miniecu_Command_Operation_STOP_ENGINE_START:
	//	break;

	case miniecu_Command_Operation_REFUEL_DONE:
		// XXX: wait flow module
		break;

	case miniecu_Command_Operation_SAVE_CONFIG:
		if (flash_connect() != MSG_OK)
			return miniecu_Command_Response_NAK;

		//param_save();
		return miniecu_Command_Response_ACK;

	case miniecu_Command_Operation_LOAD_CONFIG:
		if (flash_connect() != MSG_OK)
			return miniecu_Command_Response_NAK;

		//param_load();
		return miniecu_Command_Response_ACK;

	case miniecu_Command_Operation_DO_ERASE_CONFIG:
		mtdErase(&FLASHD1_config, 0, UINT32_MAX);
		return miniecu_Command_Response_ACK;

	// XXX TODO: call flash log thread to do work
	case miniecu_Command_Operation_DO_ERASE_LOG:
		mtdErase(&FLASHD1_error, 0, UINT32_MAX);
		mtdErase(&FLASHD1_log, 0, UINT32_MAX);
		return miniecu_Command_Response_ACK;

	case miniecu_Command_Operation_DO_REBOOT:
		/* TODO */
		break;

	default:
		break;
	}

	return miniecu_Command_Response_NAK;
}
