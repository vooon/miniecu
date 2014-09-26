/**
 * @file       adc_flow.c
 * @brief      ADC Flow sensor subtask
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

#include <math.h>

/* -*- parameters -*-  */

bool g_flow_enable;
float g_flow_v0;	// V
float g_flow_dia1;	// mm
float g_flow_dia2;	// mm
float g_flow_cd;
float g_flow_ro;	// kg/m3
float g_flow_tank_ml;	// mL
float g_flow_low_ml;	// mL

/* -*- private variabled -*- */

static float m_C;
static float m_A2;		// m2
static double m_total_used_ml;
static float m_flow_mlsec;	// mL3/sec

#define FLOW_MAXV	3.3	// V
#define MP3V5004DP_MINP	0.0	// Pa
#define MP3V5004DP_MAXP	3920.0	// Pa

/* -*- global -*- */

void on_flow_params_change(struct param_entry *p ATTR_UNUSED)
{
	m_A2 = M_PI * powf(g_flow_dia2 / 1000.0, 2) / 4.0;
	m_C = g_flow_cd / sqrtf(1 - powf(g_flow_dia2 / g_flow_dia1, 4));
}

/**
 * Return current flow [ml3/min]
 */
bool flow_get_flow(uint32_t *out)
{
	/* out in 0.1 mL/min */
	*out = m_flow_mlsec * 600.0;
	return g_flow_enable;
}

/**
 * Return fuel usage [ml]
 */
uint32_t flow_get_used_ml(void)
{
	return m_total_used_ml;
}

/**
 * Check fuel reserve
 * @return true if fuel level is LOW
 */
bool flow_check_fuel(void)
{
	if (g_flow_low_ml == 0.0 || g_flow_tank_ml == 0.0)
		return false;

	return (g_flow_tank_ml - m_total_used_ml) <= g_flow_low_ml;
}

/**
 * Calculate fuel gauge (if possible)
 *
 * @param[out] *out calculated value
 * @return true if calculation possible
 */
bool flow_get_remaining(uint32_t *out)
{
	if (g_flow_tank_ml == 0.0)
		return false;

	float rem_ml = g_flow_tank_ml - m_total_used_ml;
	if (rem_ml > g_flow_tank_ml)	rem_ml = g_flow_tank_ml;
	else if (rem_ml < 0)		rem_ml = 0;

	*out = arduino_map(rem_ml, 0.0, g_flow_tank_ml, 0, 100);
	return true;
}

static void adc_handle_flow(void)
{
	static bool is_inited = false;
	if (!is_inited) {
		on_flow_params_change(NULL);
		is_inited = true;
	}

	if (!g_flow_enable)
		return;

	/* TODO average filter */

	/* Notes: equation for orifice plate from
	 * http://en.wikipedia.org/wiki/Orifice_plate
	 */

	float dP = arduino_map(m_flow_volt, g_flow_v0, FLOW_MAXV, MP3V5004DP_MINP, MP3V5004DP_MAXP);
	float Q = m_C * m_A2 * sqrtf(2.0 * dP / g_flow_ro);

	m_flow_mlsec = Q * 1e6;

	/* TODO: total fuel used calculation */
}

