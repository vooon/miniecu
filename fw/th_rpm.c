/**
 * @file       th_rpm.c
 * @brief      RPM task
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
#include "th_rpm.h"
#include "param.h"
#include <string.h>

#ifndef BOARD_MINIECU_V2
# error "unsupported board"
#endif

/* -*- module settings -*- */
int32_t gp_pulses_per_revolution;
int32_t gp_rpm_limit;
int32_t gp_rpm_min_idle;

/* -*- private data -*- */

#define PERIODS_MAX	10
static uint32_t m_periods[PERIODS_MAX];
static size_t m_period_idx;
static size_t m_period_cnt;
static bool m_is_overflow;
static float m_curr_rpm;

static thread_t *thdp_rpm = NULL;
#define EVT_TIMEOUT	MS2ST(1000)
#define RPM_EVMASK	EVENT_MASK(1)
#define RPM_TO_EVMASK	EVENT_MASK(2)


static void rpm_period_cb(ICUDriver *icup)
{
	if (m_period_idx >= PERIODS_MAX)
		m_period_idx = 0;

	if (m_period_cnt < PERIODS_MAX)
		m_period_cnt++;

	m_is_overflow = false;
	m_periods[m_period_idx++] = icuGetPeriodX(icup);

	if (thdp_rpm != NULL) {
		chSysLockFromISR();
		chEvtSignalI(thdp_rpm, RPM_EVMASK);
		chSysUnlockFromISR();
	}
}

static void rpm_overflow_cb(ICUDriver *icup ATTR_UNUSED)
{
	/* if overflow: clear filter */
	m_is_overflow = true;
	m_period_idx = 0;
	m_period_cnt = 0;

	if (thdp_rpm != NULL) {
		chSysLockFromISR();
		chEvtSignalI(thdp_rpm, RPM_TO_EVMASK);
		chSysUnlockFromISR();
	}
}

static uint32_t rpm_get_average(void)
{
	uint64_t accu = 0;

	for (size_t i = 0; i < m_period_cnt; i++)
		accu += m_periods[i];

	return accu / m_period_cnt;
}

static const ICUConfig tim2_cfg = {
	.mode = ICU_INPUT_ACTIVE_LOW,
	.frequency = 1000000,		// 1 MHz
	.width_cb = NULL,
	.period_cb = rpm_period_cb,
	.overflow_cb = rpm_overflow_cb,
	.channel = ICU_CHANNEL_1,
	.dier = 0
};

/* -*- public functions -*- */

uint32_t rpm_get_filtered(void)
{
	return m_curr_rpm;
}

bool rpm_check_limit(void)
{
	return m_curr_rpm > gp_rpm_limit;
}

bool rpm_check_engine_running(void)
{
	return m_curr_rpm > gp_rpm_min_idle;
}

THD_FUNCTION(th_rpm, arg ATTR_UNUSED)
{
	/* set listening thread */
	thdp_rpm = chThdGetSelfX();

	/* clear filter data */
	m_period_idx = 0;
	m_period_cnt = 0;
	memset(m_periods, 0, sizeof(m_periods));

	/* Start input capture
	 *
	 * HACK: TIM2 32-bit timer, but driver uses it as 16-bit
	 * by setting ARR to 0xffff.
	 */
	icuStart(&ICUD2, &tim2_cfg);
	//ICUD2.tim->ARR = 0xffffffff;
	ICUD2.tim->ARR = 2000000; // 2 sec overflow
	icuEnableNotifications(&ICUD2); // XXX: try new polled API

	while (true) {
		eventmask_t mask = chEvtWaitAnyTimeout(ALL_EVENTS, EVT_TIMEOUT);

		if (mask & (RPM_EVMASK | RPM_TO_EVMASK)) {
			uint32_t period = rpm_get_average();

			if (period > 100) /* 100 usec RPM over 90000 */
				m_curr_rpm = 1e6 / period * 60.0f * gp_pulses_per_revolution;
			else
				m_curr_rpm = 0.0f;
		}
	}
}

