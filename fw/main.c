/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

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
#include "th_comm.h"
#include "th_adc.h"
#include "alert_led.h"
#include "rtc_time.h"

static THD_WORKING_AREA(wa_comm, 1024);
static THD_WORKING_AREA(wa_led, 128);
static THD_WORKING_AREA(wa_adc, 512);

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

	sdStart(&PBSTX_SD, NULL);
	alert_init();
	time_init();

	chThdCreateStatic(wa_led, sizeof(wa_led), LOWPRIO, th_led, NULL);
	chThdCreateStatic(wa_comm, sizeof(wa_comm), NORMALPRIO, th_comm, NULL);
	chThdCreateStatic(wa_adc, sizeof(wa_adc), NORMALPRIO + 1, th_adc, NULL);

	/* we use main thread as idle */
	chThdSetPriority(IDLEPRIO);

	/* This is now the idle thread loop, you may perform here a low priority
	   task but you must never try to sleep or wait in this loop. Note that
	   this tasks runs at the lowest priority level so any instruction added
	   here will be executed after all other tasks have been started.*/
	while (true) {
	}
}
