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

int32_t g_batt_cells;
char g_batt_type[PT_STRING_SIZE];

/* TODO: check values */
#define BATT_CELL_NiMH		1.0
#define BATT_CELL_NiCd		1.0
#define BATT_CELL_LiIon		3.0
#define BATT_CELL_LiPo		3.0
#define BATT_CELL_LiFePo	2.8
#define BATT_CELL_Pb		1.66

static float m_batt_min_cell_volt = BATT_CELL_NiMH;

void on_batt_type_change(struct param_entry *p ATTR_UNUSED)
{
	if (strcasecmp(g_batt_type, "NiMH") == 0)
		m_batt_min_cell_volt = BATT_CELL_NiMH;
	else if (strcasecmp(g_batt_type, "NiCd") == 0)
		m_batt_min_cell_volt = BATT_CELL_NiCd;
	else if (strcasecmp(g_batt_type, "LiIon") == 0)
		m_batt_min_cell_volt = BATT_CELL_LiIon;
	else if (strcasecmp(g_batt_type, "LiPo") == 0)
		m_batt_min_cell_volt = BATT_CELL_LiPo;
	else if (strcasecmp(g_batt_type, "LiFePo") == 0)
		m_batt_min_cell_volt = BATT_CELL_LiFePo;
	else if (strcasecmp(g_batt_type, "Pb") == 0)
		m_batt_min_cell_volt = BATT_CELL_Pb;
	else {
		debug_printf(DP_ERROR, "unknown battery type");
		m_batt_min_cell_volt = 0.0;
		strcpy(g_batt_type, "UNK");
	}
}

/**
 * Return battery viltage in [mV]
 */
uint32_t batt_get_voltage(void)
{
	return m_vbat * 1000;
}

bool batt_check_voltage(void)
{
	return (m_batt_min_cell_volt * g_batt_cells) > m_vbat;
}

static void adc_handle_battery(void)
{
	/* TODO: send event to Log */
}

