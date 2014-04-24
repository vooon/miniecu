/**
 * @file       ntc.h
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

#ifndef NTC_H
#define NTC_H

#include "fw_common.h"

float ntc_get_R1(float Vout, float Vin, float R2);
float ntc_get_R2(float Vout, float Vin, float R1);
float ntc_get_K(float R, float sh_a, float sh_b, float sh_c);
float ntc_K_to_C(float K);

#endif /* NTC_H */
