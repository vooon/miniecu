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
#include "alert_led.h"
#include "comm/th_comm_pbstx.h"
#include "adc/th_adc.h"
#include "log/th_log.h"
#include "th_rpm.h"
#include "param.h"
#include "hw/led.h"
#include "hw/usb_vcom.h"
#include "hw/rtc_time.h"
#include "hw/ext_flash.h"
#include "hw/ectl_pads.h"
#include "param_table.h"
#include <string.h>


/* -*- main parameters -*- */
bool gp_rtc_init_ignore_alert_led;
char gp_serial1_proto[PT_STRING_SIZE];


/* -*- main module -*- */

/**
 * @brief safety hook
 * Called from SYSTEM_HALT_HOOK() macro.
 */
void system_halt_hook(void)
{
	/* safe gpio state */
	ctl_ignition_set(false);
	ctl_starter_set(false);

	/* indication */
	led_halt_state();
}

/**
 * @brief start serial1 communication thread
 */
static void serial1_comm_create(void)
{
#define SERIAL1_PROTO_IS(proto) \
	(strcasecmp(gp_serial1_proto, SERIAL1_PROTO__ ## proto) == 0)

	if (SERIAL1_PROTO_IS(PBStx))
		pbstxCreate(&SERIAL1_SD, PBSTX_WASZ, PBSTX_PRIO);

#undef SERIAL1_PROTO_IS
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
	flash_init();
	param_init();
	serial1_comm_create();
	// start logging after pbstx, so we can hear errors
	log_init();
	rpm_init();
	adc_init();

	// force change RTC mode to normal if ignore required
	if (gp_rtc_init_ignore_alert_led)
		alert_component(ALS_RTC, AL_NORMAL);

	vcom_connect();
	chThdSetPriority(LOWPRIO);

	thread_t *usb_comm = NULL;
	while (true) {
		// release terminated thread
		if (usb_comm != NULL && chThdTerminatedX(usb_comm)) {
			chThdRelease(usb_comm);
			usb_comm = NULL;
		}

		// start/stop PBStxComm on USB serial device
		if (vcom_is_connected()) {
			if (usb_comm == NULL)
				usb_comm = pbstxCreate(&SDU1, PBSTX_WASZ, PBSTX_PRIO);
		}
		else if (usb_comm != NULL) {
			chThdTerminate(usb_comm);
		}

		chThdSleepMilliseconds(500);
	}
}
