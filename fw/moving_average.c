/**
 * @file       moving_average.h
 * @brief      simple moving average filter for ADC
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

#include "moving_average.h"


/**
 * @brief Insert new item to SMA object
 */
void sma_insert(struct sma_buffer *obj, adcsample_t sample)
{
	if (obj->wr_idx > obj->len)
		obj->wr_idx = 0;

	obj->samples[obj->wr_idx++] = sample;
}

/**
 * @brief calculate moving average from SMA object
 */
adcsample_t sma_get(struct sma_buffer *obj)
{
	uint32_t accu = 0;

	for (size_t i = 0; i < obj->len; i++)
		accu += obj->samples[i];

	return accu / obj->len;
}

