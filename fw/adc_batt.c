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

/* NOTE: this file includes in th_adc.c ! */

/* -*- parameters -*-  */

int32_t g_batt_cells;
char g_batt_type[PT_STRING_SIZE];

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
	float cell_volt = m_vbat / g_batt_cells;

	if (cell_volt > BATT_NiMH_MAXV)
		return 100;
	else if (cell_volt < BATT_NiMH_MINV)
		return 0;
	else
		/* like arduino map() */
		return (cell_volt - BATT_NiMH_MINV) * (100 - 0) / (BATT_NiMH_MAXV - BATT_NiMH_MINV) + 0;
}

/* TODO: add remaining functions to other type (if possible) */

static float m_batt_min_cell_volt = BATT_CELL_NiMH;
static uint32_t (*m_batt_remaining_func)(void) = batt_get_remaining_nimh;

/* -*- global -*- */

void on_batt_type_change(struct param_entry *p ATTR_UNUSED)
{
	if (strcasecmp(g_batt_type, "NiMH") == 0) {
		m_batt_min_cell_volt = BATT_CELL_NiMH;
		m_batt_remaining_func = batt_get_remaining_nimh;
	}
	else if (strcasecmp(g_batt_type, "NiCd") == 0) {
		m_batt_min_cell_volt = BATT_CELL_NiCd;
		m_batt_remaining_func = batt_get_remaining_nimh;
	}
	else if (strcasecmp(g_batt_type, "LiIon") == 0) {
		m_batt_min_cell_volt = BATT_CELL_LiIon;
		m_batt_remaining_func = NULL;
	}
	else if (strcasecmp(g_batt_type, "LiPo") == 0) {
		m_batt_min_cell_volt = BATT_CELL_LiPo;
		m_batt_remaining_func = NULL;
	}
	else if (strcasecmp(g_batt_type, "LiFePo") == 0) {
		m_batt_min_cell_volt = BATT_CELL_LiFePo;
		m_batt_remaining_func = NULL;
	}
	else if (strcasecmp(g_batt_type, "Pb") == 0) {
		m_batt_min_cell_volt = BATT_CELL_Pb;
		m_batt_remaining_func = NULL;
	}
	else {
		m_batt_min_cell_volt = 0.0;
		m_batt_remaining_func = NULL;
		strcpy(g_batt_type, "UNK");
		debug_printf(DP_ERROR, "unknown battery type");
	}
}

/**
 * Return battery viltage in [mV]
 */
uint32_t batt_get_voltage(void)
{
	return m_vbat * 1000;
}

/**
 * Check battery voltage
 * @return true if voltage is LOW
 */
bool batt_check_voltage(void)
{
	return (m_batt_min_cell_volt * g_batt_cells) > m_vbat;
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

static void adc_handle_battery(void)
{
	/* TODO: send event to Log */
}

