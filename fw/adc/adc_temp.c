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

#include "th_adc.h"
#include "ntc.h"
#include "param_table.h"

/* -*- parameters -*-  */
int32_t gp_temp_r;
float gp_temp_overheat;
float gp_temp_sh_a;
float gp_temp_sh_b;
float gp_temp_sh_c;


/* -*- module variables -*- */
#define TEMP_NTC_R	10e3
#define TEMP_AVCC	3.3

static float m_temp;	// [C°]


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
	return m_temp > gp_temp_overheat || adc_getflt_int_temp() > 90.0;
}

void adc_handle_temperature(void)
{
	float ntc_r;

	if (gp_temp_r == TEMP_R__R1)
		ntc_r = ntc_get_R1(adc_getflt_temp(), TEMP_AVCC, TEMP_NTC_R);
	else
		ntc_r = ntc_get_R2(adc_getflt_temp(), TEMP_AVCC, TEMP_NTC_R);

	m_temp = ntc_K_to_C(ntc_get_K(ntc_r, gp_temp_sh_a, gp_temp_sh_b, gp_temp_sh_c));

	/* TODO: send event to Log */
}

