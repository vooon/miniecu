/**
 * @file       th_adc.c
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

#include "alert_led.h"
#include "th_adc.h"

#ifndef BOARD_MINIECU_V2
# error "unsupported board"
#endif

/* -*- private vars -*- */

static adcsample_t int_term_vrtc_samples[2];
static adcsample_t term_oilp_vbat_samples[3];
static adcsample_t flow_samples[1];


/* -*- private functions -*- */

static void adc_int_term_vrtc_cb(ADCDriver *adcp ATTR_UNUSED, adcsample_t *buffer, size_t n)
{
}

static void adc_term_oilp_vbat_cb(ADCDriver *adcp ATTR_UNUSED, adcsample_t *buffer, size_t n)
{
}

static void adc_flow_cb(ADCDriver *adcp ATTR_UNUSED, adcsample_t *buffer, size_t n)
{
}

/* -*- configuration -*- */

/* internal chip temperature, V rtc config */
static const ADCConversionGroup adc1group = {
	.circular = 1,
	.num_channels = 2,
	.end_cb = adc_int_term_vrtc_cb,
	.u.adc = {
		.cr1 = 0,
		.cr2 = ADC_CR2_SWSTART,
		.ltr = 0,
		.htr = 0,
		.smpr = {
			ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_41P5) |
				ADC_SMPR1_SMP_VBAT(ADC_SAMPLE_41P5),
			0
		},
		.sqr = {
			0,
			0,
			ADC_SQR3_SQ1_N(ADC_CHANNEL_SENSOR) |
				ADC_SQR3_SQ1_N(ADC_CHANNEL_VBAT)
		}
	}
};

/* V bat, engine therm 1 & 2 (OIL) */
static const ADCConfig sdadc1cfg = {
	.cr1 = 0,
	.confxr = {
		SDADC_CONFR_GAIN_1X | SDADC_CONFR_SE_ZERO_VOLT | SDADC_CONFR_COMMON_VSSSD,
		0,
		0}
};

static const ADCConversionGroup sdadc1group = {
	.circular = 1,
	.num_channels = 3,
	.end_cb = adc_term_oilp_vbat_cb,
	.u.sdadc = {
		.cr2 = SDADC_CR2_JSWSTART,
		.jchgr = SDADC_JCHGR_CH(6) |
			SDADC_JCHGR_CH(5) |
			SDADC_JCHGR_CH(4),
		.confchr = {
			SDADC_CONFCHR1_CH6(0) |
				SDADC_CONFCHR1_CH5(0) |
				SDADC_CONFCHR1_CH4(0),
			0
		}
	}
};

/* Flow sensor */
static const ADCConfig sdadc3cfg = {
	.cr1 = 0,
	.confxr = {
		SDADC_CONFR_GAIN_1X | SDADC_CONFR_SE_ZERO_VOLT | SDADC_CONFR_COMMON_VSSSD,
		0,
		0}
};

static const ADCConversionGroup sdadc3group = {
	.circular = 1,
	.num_channels = 1,
	.end_cb = adc_flow_cb,
	.u.sdadc = {
		.cr2 = SDADC_CR2_JSWSTART,
		.jchgr = SDADC_JCHGR_CH(6),
		.confchr = {
			SDADC_CONFCHR1_CH6(0),
			0
		}
	}
};

/* -*- module thread -*- */

THD_FUNCTION(th_adc, arg ATTR_UNUSED)
{
	/* ADC1 */
	adcStart(&ADCD1, NULL);
	adcSTM32Calibrate(&ADCD1);
	adcSTM32EnableTSVREFE(); /* enable sensor */
	adcSTM32EnableVBATE(); /* enable Vrtc bat */

	/* SDADC1 */
	//adcStart(&SDADCD1, &sdadc1cfg);
	//adcSTM32Calibrate(&SDADCD1);

	/* SDADC3 */
	//adcStart(&SDADCD3, &sdadc3cfg);
	//adcSTM32Calibrate(&SDADCD3);

	/* Start continous conversions */
	//adcStartConversion(&ADCD1, &adc1group, int_term_vrtc_samples, 1);
	//adcStartConversion(&SDADCD1, &sdadc1group, term_oilp_vbat_samples, 1);
	//adcStartConversion(&SDADCD3, &sdadc3group, flow_samples, 1);

	/* notify ADC running */
	alert_component(ALS_ADC, AL_NORMAL);

	while (true) {
		chThdSleepMilliseconds(1100);
		debug_printf(DP_DEBUG, "ADC value: %d", 10);
	}
}

