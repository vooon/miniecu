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
#include "param.h"
#include <string.h>

#ifndef BOARD_MINIECU_V2
# error "unsupported board"
#endif

/* -*- global parameters -*- */

float g_vbat_vd1_voltage_drop; // [V]

/* -*- private vars -*- */

static adcsample_t p_int_temp_vrtc_samples[2];
static adcsample_t p_temp_oilp_vbat_samples[3];
static adcsample_t p_flow_samples[1];

static float m_int_temp;	// [C°]
static float m_vrtc;		// [V]
static float m_temp_volt;	// [V] before conversion to temp
static float m_oilp_volt;	// [V] raw voltage on OIL_P
static float m_vbat;		// [V]
static float m_flow_volt;	// [V] before conversion to FLOW

static Thread *thdp_adc = NULL;
#define EVT_TIMEOUT	MS2ST(1000)
#define ADC1_EVMASK	EVENT_MASK(1)
#define SDADC1_EVMASK	EVENT_MASK(2)
#define SDADC3_EVMASK	EVENT_MASK(3)

/* -*- conversion functions -*- */

#define ADC_VREF	3.3
#define SDADC_VREF	3.3
#define SDADC_GAIN	1
/* given in DM00046749.pdf table 66. */
#define TEMP_V25	1.43	/* [V] */
#define TEMP_AVG_SLOPE	4.3	/* [mV/C°] */

/** convert sample to voltage (ADC)
 */
static float adc_to_voltage(adcsample_t adc)
{
	return ADC_VREF * adc / ((1 << 12) - 1);
}

static float adc_to_int_temp(adcsample_t adc)
{
	float temp_voltage = adc_to_voltage(adc);
	return (TEMP_V25 - temp_voltage) * 1000. / TEMP_AVG_SLOPE + 25.;
}

/** convert sample to voltage (SDADC in SE Zero)
 * @note see @a DM0007480.pdf Application Note
 */
static float sdadc_sez_to_voltage(adcsample_t adc)
{
	return (((int16_t) adc) + 32767) * SDADC_VREF / (SDADC_GAIN * 65535);
}

/* -*- private functions -*- */

static void adc_int_temp_vrtc_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	m_int_temp = adc_to_int_temp(buffer[0]);
	m_vrtc = 2 * adc_to_voltage(buffer[1]);

	/*if (thdp_adc != NULL) {
		chSysLockFromIsr();
		chEvtSignalI(thdp_adc, ADC1_EVMASK);
		chSysUnlockFromIsr();
	} not used */
}

static void adc_temp_oilp_vbat_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	// note: Vbat source after VD1, we add voltage drop
	m_vbat = g_vbat_vd1_voltage_drop +
		3 * sdadc_sez_to_voltage(buffer[0]);	// AIN4P
	m_oilp_volt = sdadc_sez_to_voltage(buffer[1]);	// AIN5P
	m_temp_volt = sdadc_sez_to_voltage(buffer[2]);	// AIN6P

	if (thdp_adc != NULL) {
		chSysLockFromIsr();
		chEvtSignalI(thdp_adc, SDADC1_EVMASK);
		chSysUnlockFromIsr();
	}
}

static void adc_flow_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	m_flow_volt = sdadc_sez_to_voltage(buffer[0]);	// AIN6P

	if (thdp_adc != NULL) {
		chSysLockFromIsr();
		chEvtSignalI(thdp_adc, SDADC3_EVMASK);
		chSysUnlockFromIsr();
	}
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
	.end_cb = adc_int_temp_vrtc_cb,
	.error_cb = adc_error_cb,
	.u.adc = {
		.cr1 = 0,
		.cr2 = ADC_CR2_SWSTART,
		.ltr = 0,
		.htr = 0,
		.smpr = {
			ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_239P5) |
				ADC_SMPR1_SMP_VBAT(ADC_SAMPLE_239P5),
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
	.end_cb = adc_temp_oilp_vbat_cb,
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

#include "adc_batt.c"
#include "adc_therm.c"

THD_FUNCTION(th_adc, arg ATTR_UNUSED)
{
	/* set listening thread */
	thdp_adc = currp;

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
	adcStartConversion(&ADCD1, &adc1group, p_int_temp_vrtc_samples, 1);
	adcStartConversion(&SDADCD1, &sdadc1group, p_temp_oilp_vbat_samples, 1);
	adcStartConversion(&SDADCD3, &sdadc3group, p_flow_samples, 1);

	while (true) {
		eventmask_t mask = chEvtWaitAnyTimeout(ALL_EVENTS, EVT_TIMEOUT);

		if (!(mask & (ADC1_EVMASK | SDADC1_EVMASK | SDADC3_EVMASK))) {
			/* notify ADC timed out */
			alert_component(ALS_ADC, AL_FAIL);
		}
		else {
			/* notify ADC running */
			alert_component(ALS_ADC, AL_NORMAL);
		}

		if (mask & ADC1_EVMASK) {
			/* CPU temp don't emit events */
			/* Vrtc currently not used */
		}
		if (mask & SDADC1_EVMASK) {
			adc_handle_battery();
			adc_handle_temperature();
			//debug_printf(DP_DEBUG, "Oilp V: %3d", (int)(m_oilp_volt * 1000));
		}
		if (mask & SDADC3_EVMASK) {
			//debug_printf(DP_DEBUG, "Flow V: %3d", (int)(m_flow_volt * 1000));
		}
	}
}

