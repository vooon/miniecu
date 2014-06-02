/**
 * @file       adc_oilp.c
 * @brief      OILP multifunctional ADC pin subtask
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
#include <math.h>

/* -*- parameters -*-  */

enum {
	OILP_R1 = 1,
	OILP_R2 = 2
};

char g_oilp_mode[PT_STRING_SIZE];
int32_t g_oilp_r;
float g_oilp_sh_a;
float g_oilp_sh_b;
float g_oilp_sh_c;

/* -*- module variables -*- */

#define OILP_NTC_R	10e3
#define OILP_AVCC	3.3

static float m_oilp_temp = NAN;	// [C°]
static void (*m_oilp_handle_func)(void) = NULL;

/* -*- handle funcs -*- */

static void oilp_handle_ntc10k(void)
{
	float ntc_r;

	if (g_oilp_r == OILP_R1)
		ntc_r = ntc_get_R1(m_oilp_volt, OILP_AVCC, OILP_NTC_R);
	else
		ntc_r = ntc_get_R2(m_oilp_volt, OILP_AVCC, OILP_NTC_R);

	m_oilp_temp = ntc_K_to_C(ntc_get_K(ntc_r, g_oilp_sh_a, g_oilp_sh_b, g_oilp_sh_c));
}

/* -*- global -*- */

void on_oilp_mode_change(struct param_entry *p ATTR_UNUSED)
{
	m_oilp_temp = NAN;
	if (strcasecmp(g_oilp_mode, "Disabled") == 0) {
		m_oilp_handle_func = NULL;
	}
	else if (strcasecmp(g_oilp_mode, "NTC10k") == 0) {
		m_oilp_handle_func = oilp_handle_ntc10k;
	}
	else {
		m_oilp_handle_func = NULL;
		strcpy(g_oilp_mode, "Disabled");
		debug_printf(DP_ERROR, "unknown oilp mode");
	}
}

/**
 * Return OILP temp in [mC°] if in NTC mode
 */
bool oilp_get_temperature(int32_t *out)
{
	if (!isnan(m_oilp_temp)) {
		*out = m_oilp_temp * 1000;
		return true;
	}
	else
		return false;
}

/**
 * Return engine oil pressure
 */
bool oilp_get_pressure(int32_t *out ATTR_UNUSED)
{
	/* TODO */
	return false;
}

static void adc_handle_oilp(void)
{
	if (m_oilp_handle_func != NULL)
		m_oilp_handle_func();
}

