/**
 * @file       adc_batt.c
 * @brief      ADC battery subtask
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
#include "param_table.h"
#include <string.h>


/* -*- parameters -*-  */
int32_t gp_batt_cells;
char gp_batt_type[PT_STRING_SIZE];
float gp_batt_vd1_voltage_drop;


/**
 * Get corrected battery voltage.
 * (VD1 voltage drop)
 */
static float get_vbat(void)
{
	return gp_batt_vd1_voltage_drop + adc_getll_vbat();
}

/* -*- battery types -*- */

#define BATT_CELL_NiMH		1.0
#define BATT_CELL_NiCd		1.0
#define BATT_CELL_LiIon		3.0
#define BATT_CELL_LiPo		3.0
#define BATT_CELL_LiFePo	2.8
#define BATT_CELL_Pb		1.66

#define BATT_NiMH_MINV	1.15
#define BATT_NiMH_MAXV	1.25

static uint32_t batt_get_remaining_nimh(void)
{
	float cell_volt = get_vbat() / gp_batt_cells;

	if (cell_volt > BATT_NiMH_MAXV)
		return 100;
	else if (cell_volt < BATT_NiMH_MINV)
		return 0;
	else
		return arduino_map(cell_volt, BATT_NiMH_MINV, BATT_NiMH_MAXV, 0, 100);
}

/* TODO: add remaining functions to other type (if possible) */

static float m_batt_min_cell_volt = BATT_CELL_NiMH;
static uint32_t (*m_batt_remaining_func)(void) = batt_get_remaining_nimh;

/* -*- global -*- */

void on_change_batt_type(struct param_entry *p)
{
#define BATT_TYPE_IS(type)	\
	(strcasecmp(gp_batt_type, BATT_TYPE__ ## type) == 0)

	m_batt_remaining_func = NULL;
	if (BATT_TYPE_IS(NiMH)) {
		m_batt_min_cell_volt = BATT_CELL_NiMH;
		m_batt_remaining_func = batt_get_remaining_nimh;
	}
	else if (BATT_TYPE_IS(NiCd)) {
		m_batt_min_cell_volt = BATT_CELL_NiCd;
		m_batt_remaining_func = batt_get_remaining_nimh;
	}
	else if (BATT_TYPE_IS(LiIon)) {
		m_batt_min_cell_volt = BATT_CELL_LiIon;
	}
	else if (BATT_TYPE_IS(LiPo)) {
		m_batt_min_cell_volt = BATT_CELL_LiPo;
	}
	else if (BATT_TYPE_IS(LiFePo)) {
		m_batt_min_cell_volt = BATT_CELL_LiFePo;
	}
	else if (BATT_TYPE_IS(Pb)) {
		m_batt_min_cell_volt = BATT_CELL_Pb;
	}
	else {
		m_batt_min_cell_volt = 0.0;
		strcpy(gp_batt_type, p->default_value.s);
		debug_printf(DP_ERROR, "BATT: unknown battery type");
	}

#undef BATT_TYPE_IS
}

/**
 * Return battery viltage in [mV]
 */
uint32_t batt_get_voltage(void)
{
	return get_vbat() * 1000;
}

/**
 * Check battery voltage
 * @return true if voltage is LOW
 */
bool batt_check_voltage(void)
{
	return (m_batt_min_cell_volt * gp_batt_cells) > get_vbat();
}

/**
 * Calculate battery fuel gauge (if possible)
 *
 * @param[out] *out calculated value
 * @return true if calculation possible
 */
bool batt_get_remaining(uint32_t *out)
{
	if (m_batt_remaining_func == NULL)
		return false;

	*out = m_batt_remaining_func();
	return true;
}

void adc_handle_battery(void)
{
	/* TODO: send event to Log */
}

