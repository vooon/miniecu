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
#include "moving_average.h"

#ifndef BOARD_MINIECU_V2
# error "unsupported board"
#endif

/* -*- private vars -*- */

static adcsample_t int_term_vrtc_samples[2];
static adcsample_t term_oilp_vbat_samples[3];
static adcsample_t flow_samples[1];

STATIC_SMA_BUFFER_DECL(m_int_term, 4);
STATIC_SMA_BUFFER_DECL(m_vrtc, 4);
STATIC_SMA_BUFFER_DECL(m_term, 4);
STATIC_SMA_BUFFER_DECL(m_oilp, 4);
STATIC_SMA_BUFFER_DECL(m_vbat, 4);
STATIC_SMA_BUFFER_DECL(m_flow, 4);

/* -*- private functions -*- */

static void adc_int_term_vrtc_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	//sma_insert(&m_int_term, buffer[0]);
	//sma_insert(&m_vrtc, buffer[1]);
	/* TODO: send event */
}

static void adc_term_oilp_vbat_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	//sma_insert(&m_vbat, buffer[0]);	// AIN4P
	//sma_insert(&m_oilp, buffer[1]);	// AIN5P
	//sma_insert(&m_term, buffer[2]);	// AIN6P
	/* SAME */
}

static void adc_flow_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	//sma_insert(&m_flow, buffer[0]);	// AIN6P
	/* same... */
}

static void adc_error_cb(ADCDriver *adcd ATTR_UNUSED, adcerror_t err ATTR_UNUSED)
{
	alert_component(ALS_ADC, AL_FAIL);
}

/* -*- configuration -*- */

/* internal chip temperature, V rtc config */
static const ADCConversionGroup adc1group = {
	.circular = TRUE,
	.num_channels = 2,
	.end_cb = adc_int_term_vrtc_cb,
	.error_cb = adc_error_cb,
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
				ADC_SQR3_SQ2_N(ADC_CHANNEL_VBAT) |
				0
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
	.circular = TRUE,
	.num_channels = 3,
	.end_cb = adc_term_oilp_vbat_cb,
	.error_cb = adc_error_cb,
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
	.circular = TRUE,
	.num_channels = 1,
	.end_cb = adc_flow_cb,
	.error_cb = adc_error_cb,
	.u.sdadc = {
		.cr2 = SDADC_CR2_JSWSTART | SDADC_CR2_FAST,
		.jchgr = SDADC_JCHGR_CH(6),
		.confchr = {
			SDADC_CONFCHR1_CH6(0),
			0
		}
	}
};

/* -*- module thread -*- */

/** helper function: convert average to voltage (ADC) */
static float sma_get_adc_voltage(struct sma_buffer *obj)
{
	return 3.3 * sma_get(obj) / ((1 << 12) - 1);
}

/** helper function: convert average to voltage (SDADC in SE Zero)
 * @note see @a DM0007480.pdf Application Note
 */
static float sma_get_sdadc_voltage(struct sma_buffer *obj)
{
	int16_t sdadc_val = (int16_t) sma_get(obj);
	return (sdadc_val + 32767) * 3.3 / (1 * 65535); // for GAIN = 1
}

static float get_internal_temp(void)
{
	/* TODO: use TS_CAL? */
	/* given in DM00046749.pdf table 66. */
#define STM32_TEMP_V25		1.43	/* [V] */
#define STM32_TEMP_AVG_SLOPE	4.3	/* [mV/C°] */

	float temp_voltage = sma_get_adc_voltage(&m_int_term);
	return (STM32_TEMP_V25 - temp_voltage) * 1000. / STM32_TEMP_AVG_SLOPE + 25.;
}

static int get_v(adcsample_t adc)
{
	return (((int16_t) adc) + 32767) * 3.3 / (1 * 65535) * 1000;
}

THD_FUNCTION(th_adc, arg ATTR_UNUSED)
{
	/* ADC1 */
	adcStart(&ADCD1, NULL);
	adcSTM32Calibrate(&ADCD1);
	adcSTM32EnableTSVREFE(); /* enable thermometer */
	adcSTM32EnableVBATE(); /* enable Vrtc bat, TODO: enable once in 10-sec */

	/* SDADC1 */
	adcStart(&SDADCD1, &sdadc1cfg);
	adcSTM32Calibrate(&SDADCD1);

	/* SDADC3 */
	adcStart(&SDADCD3, &sdadc3cfg);
	adcSTM32Calibrate(&SDADCD3);

	/* Start continous conversions */
	adcStartConversion(&ADCD1, &adc1group, int_term_vrtc_samples, 1);
	adcStartConversion(&SDADCD1, &sdadc1group, term_oilp_vbat_samples, 1);
	adcStartConversion(&SDADCD3, &sdadc3group, flow_samples, 1);

	while (true) {
		chThdSleepMilliseconds(1000);
		//debug_printf(DP_INFO, "temperature: %3d V_rtc: %3d",
		//		(int)(get_internal_temp() * 1000),
		//		(int)(sma_get_adc_voltage(&m_vrtc) * 1000 * 2));
		//debug_printf(DP_INFO, "V_bat: %d", (int)(sma_get_sdadc_voltage(&m_vbat) * 1000));
		//debug_printf(DP_INFO, "term: %d", (int)(sma_get_sdadc_voltage(&m_term) * 1000));
		//debug_printf(DP_INFO, "oilp: %d", (int)(sma_get_sdadc_voltage(&m_oilp) * 1000));
		//debug_printf(DP_INFO, "flow: %d", (int)(sma_get_sdadc_voltage(&m_flow) * 1000));

		debug_printf(DP_INFO, "raw_v: %6d %6d %6d %6d",
				get_v(term_oilp_vbat_samples[0]),
				get_v(term_oilp_vbat_samples[1]),
				get_v(term_oilp_vbat_samples[2]),
				get_v(flow_samples[0]));

		/* notify ADC running */
		alert_component(ALS_ADC, AL_NORMAL);
	}
}

