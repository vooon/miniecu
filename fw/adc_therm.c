/**
 * @file       adc_therm.c
 * @brief      ADC temperature subtask
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

/* NOTE: this file includes in th_adc.c ! */

#include "ntc.h"

/* -*- parameters -*-  */

enum {
	TEMPR_R1 = 1,
	TEMPR_R2 = 2
};

int32_t g_temp_r;
float g_temp_overheat;
float g_temp_sh_a;
float g_temp_sh_b;
float g_temp_sh_c;

/* -*- module variables -*- */

#define TEMP_NTC_R	10e3
#define TEMP_AVCC	3.3

static float m_temp;	// [C°]

/**
 * Return CPU temp in [mC°]
 */
int32_t temp_get_int_temperature(void)
{
	return m_int_temp * 1000;
}

/**
 * Return engine temp in [mC°]
 */
int32_t temp_get_temperature(void)
{
	return m_temp * 1000;
}

/**
 * Check engine temperature
 * @return true if temperature is HIGH
 */
bool temp_check_temperature(void)
{
	return m_temp > g_temp_overheat || m_int_temp > 90.0;
}

static void adc_handle_temperature(void)
{
	float ntc_r;

	if (g_temp_r == TEMPR_R1)
		ntc_r = ntc_get_R1(m_temp_volt, TEMP_AVCC, TEMP_NTC_R);
	else
		ntc_r = ntc_get_R2(m_temp_volt, TEMP_AVCC, TEMP_NTC_R);

	m_temp = ntc_K_to_C(ntc_get_K(ntc_r, g_temp_sh_a, g_temp_sh_b, g_temp_sh_c));

	/* TODO: send event to Log */
}

