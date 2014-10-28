/**
 * @file       adc_cpu.c
 * @brief      ADC CPU funcs
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


/**
 * Return CPU temp in [mC°]
 */
int32_t cpu_get_temperature(void)
{
	return adc_getflt_int_temp() * 1000;
}

/**
 * Return CPU RTC battery voltage [mV]
 *
 * @return true if voltage higher than 1.0 volt
 */
bool cpu_get_rtc_voltage(uint32_t *out)
{
	*out = adc_getflt_vrtc() * 1000;
	return *out > 1000;
}
