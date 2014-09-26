/**
 * @file       th_adc.h
 * @brief      ADC task
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

#ifndef TH_ADC_H
#define TH_ADC_H

#include "fw_common.h"

THD_FUNCTION(th_adc, arg ATTR_UNUSED);

/* subsystem functions */

uint32_t batt_get_voltage(void);
bool batt_check_voltage(void);
bool batt_get_remaining(uint32_t *out);

int32_t temp_get_int_temperature(void);
int32_t temp_get_temperature(void);
bool temp_check_temperature(void);

bool oilp_get_pressure(int32_t *out);
bool oilp_get_temperature(int32_t *out);

bool flow_get_flow(uint32_t *out);
uint32_t flow_get_used_ml(void);
bool flow_check_fuel(void);
bool flow_get_remaining(uint32_t *out);

float adc_getll_temp(void);
float adc_getll_oilp(void);
float adc_getll_flow(void);
float adc_getll_vbat(void);
float adc_getll_vrtc(void);

#endif /* TH_ADC_H */
