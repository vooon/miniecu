/*
    ChibiOS - Copyright (C) 2006-2014 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "fw_common.h"
#include "th_comm_pbstx.h"
#include "th_adc.h"
#include "th_rpm.h"
#include "th_flash_log.h"
#include "th_command.h"
#include "alert_led.h"
#include "pbstx.h"
#include "param.h"
#include "hw/led.h"
#include "hw/usb_vcom.h"
#include "hw/rtc_time.h"


static THD_WORKING_AREA(wa_adc, 512);
static THD_WORKING_AREA(wa_rpm, 256);
static THD_WORKING_AREA(wa_flash_log, 1024);
static THD_WORKING_AREA(wa_command, 256);

#define COMM_PRIO	(NORMALPRIO - 5)
#define PBSTX_WASZ	1024*2

/**
 * @brief safety hook
 * Called from SYSTEM_HALT_HOOK() macro.
 */
void system_halt_hook(void)
{
	/* safe gpio state */
	palClearPad(GPIOE, GPIOE_IGN_EN);
	palClearPad(GPIOE, GPIOE_STARTER);

	/* indication */
	led_halt_state();
}

/*
 * Application entry point.
 */
int main(void) {

	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();
	chSysInit();

	sdStart(&SERIAL1_SD, NULL);
	alert_led_init();
	vcom_init();
	rtc_time_init();
	param_init();

	//chThdCreateStatic(wa_flash_log, sizeof(wa_flash_log), NORMALPRIO - 2, th_flash_log, NULL);
	//chThdCreateStatic(wa_adc, sizeof(wa_adc), NORMALPRIO + 1, th_adc, NULL);
	//chThdCreateStatic(wa_rpm, sizeof(wa_rpm), NORMALPRIO - 1, th_rpm, NULL);
	//chThdCreateStatic(wa_command, sizeof(wa_command), NORMALPRIO - 2, th_command, NULL);

	// TODO: check serial1 protocol selector
	pbstxCreate(&SERIAL1_SD, PBSTX_WASZ, COMM_PRIO);

	vcom_connect();
	chThdSetPriority(LOWPRIO);

	thread_t *usb_comm = NULL;
	while (true) {
		// start/stop PBStxComm on USB serial device
		if (vcom_is_connected()) {
			if (usb_comm == NULL) {
				usb_comm = pbstxCreate(&SDU1, PBSTX_WASZ, COMM_PRIO);
			}
		}
		else if (usb_comm != NULL) {
			if (chThdTerminatedX(usb_comm)) {
				chThdRelease(usb_comm);
				usb_comm = NULL;
			}
			else
				chThdTerminate(usb_comm);
		}

		chThdSleepMilliseconds(500);
	}
}
