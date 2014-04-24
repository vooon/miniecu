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

#endif /* TH_ADC_H */
