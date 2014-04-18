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

#ifndef MOVING_AVERAGE_H
#define MOVING_AVERAGE_H

#include "fw_common.h"

struct sma_buffer {
	uint8_t len;
	uint8_t wr_idx;
	adcsample_t *samples;
};

#define SMA_BUFFER_DECL(name, size)			\
	adcsample_t name ## _sma_samples[size];		\
	struct sma_buffer name = { size, 0, name ## _sma_samples }

#define STATIC_SMA_BUFFER_DECL(name, size)		\
	static adcsample_t name ## _sma_samples[size];	\
	static struct sma_buffer name = { size, 0, name ## _sma_samples }

void sma_insert(struct sma_buffer *obj, adcsample_t sample);
adcsample_t sma_get(struct sma_buffer *obj);

#endif /* MOVING_AVERAGE_H */
