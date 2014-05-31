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

#include "alert_led.h"
#include "th_comm.h"
#include "th_flash_log.h"
#include "miniecu.pb.h"

static Thread *thdp_cmd;
#define EVT_TIMEOUT		MS2ST(10000)
#define CFG_OP_TIMEOUT		MS2ST(1000)
#define ESTOP_EVMASK		EVENT_MASK(1)
#define DO_START_EVMASK		EVENT_MASK(2)
#define SAVE_CFG_EVMASK		EVENT_MASK(3)
#define LOAD_CFG_EVMASK		EVENT_MASK(4)
#define DO_ERASE_CFG_EVMASK	EVENT_MASK(5)
#define DO_ERASE_LOG_EVMASK	EVENT_MASK(6)

THD_FUNCTION(th_command, arg ATTR_UNUSED)
{
	thdp_cmd = chThdSelf();

	while (true) {
		eventmask_t mask = chEvtWaitAnyTimeout(ALL_EVENTS, EVT_TIMEOUT);

		if (mask & LOAD_CFG_EVMASK) {
			send_command_response(miniecu_Command_Operation_LOAD_CONFIG,
					(flash_do_load_cfg(CFG_OP_TIMEOUT))?
						miniecu_Command_Response_ACK :
						miniecu_Command_Response_NACK);
		}

		if (mask & SAVE_CFG_EVMASK) {
			send_command_response(miniecu_Command_Operation_SAVE_CONFIG,
					(flash_do_save_cfg(CFG_OP_TIMEOUT))?
						miniecu_Command_Response_ACK :
						miniecu_Command_Response_NACK);
		}

		if (mask & DO_ERASE_CFG_EVMASK) {
			send_command_response(miniecu_Command_Operation_DO_ERASE_CONFIG,
					(flash_do_erase_cfg(CFG_OP_TIMEOUT))?
						miniecu_Command_Response_ACK :
						miniecu_Command_Response_NACK);
		}
	}
}

uint32_t command_request(uint32_t cmdid)
{
	eventmask_t mask = 0;

	switch (cmdid) {
	case miniecu_Command_Operation_EMERGENCY_STOP:
		//mask = ESTOP_EVMASK;
		break;

	case miniecu_Command_Operation_IGNITION_ENABLE:
		palSetPad(GPIOE, GPIOE_IGN_EN);
		return miniecu_Command_Response_ACK;
	case miniecu_Command_Operation_IGNITION_DISABLE:
		palClearPad(GPIOE, GPIOE_IGN_EN);
		return miniecu_Command_Response_ACK;

	case miniecu_Command_Operation_STARTER_ENABLE:
		palSetPad(GPIOE, GPIOE_STARTER);
		return miniecu_Command_Response_ACK;
	case miniecu_Command_Operation_STARTER_DISABLE:
		palClearPad(GPIOE, GPIOE_STARTER);
		return miniecu_Command_Response_ACK;

	case miniecu_Command_Operation_DO_ENGINE_START:
	case miniecu_Command_Operation_STOP_ENGINE_START:
		//mask = DO_START_EVMASK;
		break;

	case miniecu_Command_Operation_REFUEL_DONE:
		break;

	case miniecu_Command_Operation_SAVE_CONFIG:
		mask = SAVE_CFG_EVMASK;
		break;
	case miniecu_Command_Operation_LOAD_CONFIG:
		mask = LOAD_CFG_EVMASK;
		break;

	case miniecu_Command_Operation_DO_ERASE_CONFIG:
		mask = DO_ERASE_CFG_EVMASK;
		break;
	case miniecu_Command_Operation_DO_ERASE_LOG:
		//mask = DO_ERASE_LOG_EVMASK;
		break;

	case miniecu_Command_Operation_DO_REBOOT:
		/* TODO */
		break;

	default:
		break;
	}

	if (mask && thdp_cmd != NULL) {
		chEvtSignal(thdp_cmd, mask);
		return miniecu_Command_Response_IN_PROGRESS;
	}

	return miniecu_Command_Response_NACK;
}

bool command_check_ignition(void)
{
	return palReadPad(GPIOE, GPIOE_IGN_EN);
}

bool command_check_starter(void)
{
	return palReadPad(GPIOE, GPIOE_STARTER);
}

