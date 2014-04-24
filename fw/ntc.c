/**
 * @file       ntc.c
 * @brief      NTC thermistor calculation functions
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

#include "ntc.h"
#include <math.h>

#define KELV	273.15

/*
 * See @a http://en.wikipedia.org/wiki/Voltage_divider
 */

/**
 * Get R1 resistance in Voltage Divider
 */
float ntc_get_R1(float Vout, float Vin, float R2)
{
	return R2 * Vin / Vout - R2;
}

/**
 * Get R2 resistance in Voltage Divider
 */
float ntc_get_R2(float Vout, float Vin, float R1)
{
	if (Vout == 0)
		return NAN;
	return R1 / (Vin / Vout - 1);
}

/*
 * See @a http://en.wikipedia.org/wiki/Steinhart-Hart_equation
 * For calibration use this calculator:
 *  @a http://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
 */

/**
 * Convert NTC resistance to Kelvin
 * Using Shteinhart-Hart equation
 */
float ntc_get_K(float R, float sh_a, float sh_b, float sh_c)
{
	float logR = logf(R);
	return 1.0 / (sh_a + sh_b * logR + sh_c * logR * logR * logR);
}

/**
 * Convert Kelvin to Celsius
 */
float ntc_K_to_C(float K)
{
	return K - KELV;
}

