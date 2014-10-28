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
#include "lib/lowpassfilter2p.h"

#ifndef BOARD_MINIECU_V2
# error "unsupported board"
#endif

/* Enables toggling pads in adc handlers:
 * ADC1: PC13
 * SDADC1: PA1
 * SDADC3: PA2
 */
#define DEBUG_ADC_FREQ	TRUE

/* -*- parameters -*- */
// None


/* -*- private data -*- */
// ADC sample buffers
static adcsample_t p_int_temp_vrtc_samples[2];
static adcsample_t p_temp_oilp_vbat_samples[3];
static adcsample_t p_flow_samples[1];

// raw values
static float r_int_temp;	// [C°]
static float r_vrtc;		// [V]
static float r_temp_volt;	// [V] before conversion to temp
static float r_oilp_volt;	// [V] raw voltage on OIL_P
static float r_vbat;		// [V] on ADC input (VD1 drop added later)
static float r_flow_volt;	// [V] before conversion to FLOW

// filtered values
static float f_int_temp;
static float f_vrtc;
static float f_temp_volt;
static float f_oilp_volt;
static float f_vbat;
static float f_flow_volt;

// filters
static LowPassFilter2p fo_int_temp;
static LowPassFilter2p fo_vrtc;
static LowPassFilter2p fo_temp_volt;
static LowPassFilter2p fo_oilp_volt;
static LowPassFilter2p fo_vbat;
static LowPassFilter2p fo_flow_volt;

// thread
static THD_WORKING_AREA(wa_adc, ADC_WASZ);


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

/* -*- callback functions -*- */

static void adc_int_temp_vrtc_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	r_int_temp = adc_to_int_temp(buffer[0]);
	r_vrtc = 2 * adc_to_voltage(buffer[1]);

	f_int_temp = lpf2pApply(&fo_int_temp, r_int_temp);
	f_vrtc = lpf2pApply(&fo_vrtc, r_vrtc);

#if DEBUG_ADC_FREQ
	palTogglePad(GPIOC, GPIOC_XP2_PC13);
#endif
}

static void adc_temp_oilp_vbat_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	r_vbat = 3 * sdadc_sez_to_voltage(buffer[0]);	// AIN4P
	r_oilp_volt = sdadc_sez_to_voltage(buffer[1]);	// AIN5P
	r_temp_volt = sdadc_sez_to_voltage(buffer[2]);	// AIN6P

	f_vbat = lpf2pApply(&fo_vbat, r_vbat);
	f_oilp_volt = lpf2pApply(&fo_oilp_volt, r_oilp_volt);
	f_temp_volt = lpf2pApply(&fo_temp_volt, r_temp_volt);

#if DEBUG_ADC_FREQ
	palTogglePad(GPIOA, GPIOA_XP2_PA1);
#endif
}

static void adc_flow_cb(ADCDriver *adcp ATTR_UNUSED,
		adcsample_t *buffer, size_t n ATTR_UNUSED)
{
	r_flow_volt = sdadc_sez_to_voltage(buffer[0]);	// AIN6P

	f_flow_volt = lpf2pApply(&fo_flow_volt, r_flow_volt);

#if DEBUG_ADC_FREQ
	palTogglePad(GPIOA, GPIOA_XP2_PA2);
#endif
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
		0
	}
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
		.cr2 = SDADC_CR2_JSWSTART /* | SDADC_CR2_FAST */,
		.jchgr = SDADC_JCHGR_CH(6),
		.confchr = {
			SDADC_CONFCHR1_CH6(0),
			0
		}
	}
};

/* -*- low-level getters -*- */

#define MAKE_GETTER(name, var)			\
	float adc_getflt_ ## name (void) {	\
		return f_ ## var;		\
	}					\
	float adc_getraw_ ## name (void) {	\
		return r_ ## var;		\
	}

MAKE_GETTER(temp, temp_volt);
MAKE_GETTER(oilp, oilp_volt);
MAKE_GETTER(flow, flow_volt);
MAKE_GETTER(vbat, vbat);
MAKE_GETTER(vrtc, vrtc);
MAKE_GETTER(int_temp, int_temp);

#undef MAKE_GETTER

/* -*- module thread -*- */

void adc_handle_battery(void);
void adc_handle_temperature(void);
void adc_handle_oilp(void);
void adc_handle_flow(void);


static THD_FUNCTION(th_adc, arg ATTR_UNUSED)
{
	chRegSetThreadName("adc");

#if DEBUG_ADC_FREQ
	debug_printf(DP_INFO, "ADC freq outputs enabled");
	palSetPadMode(GPIOA, GPIOA_XP2_PA1, PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOA, GPIOA_XP2_PA2, PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOC, GPIOC_XP2_PC13, PAL_MODE_OUTPUT_PUSHPULL);
#endif

	/* Init low pass filters */
	lpf2pObjectInit(&fo_int_temp);
	lpf2pObjectInit(&fo_vrtc);
	lpf2pObjectInit(&fo_temp_volt);
	lpf2pObjectInit(&fo_oilp_volt);
	lpf2pObjectInit(&fo_vbat);
	lpf2pObjectInit(&fo_flow_volt);

	/* Experimental freq's:
	 * PC13:	8.935 kHz	111 uS
	 * PA1:		2.78 kHz	350 uS
	 * PA2:		8.34 kHz	120 uS
	 *
	 * So actual sampling freq's:
	 * 		(measured)	(calced)
	 * ADC1:	17.78 kHz	2 * 17.1 us => 29.24 kHz
	 * SDADC1:	5.56 kHz	16600 / 3 => 5.5(3) kHz
	 * SDADC3:	16.68 kHz	16600 / 1 => 16.6 kHz
	 */

	/* SAR ADC1 */
	lpf2pSetCutoffFrequency(&fo_int_temp, 17780.0, 50.0);
	lpf2pSetCutoffFrequency(&fo_vrtc, 17780.0, 50.0);
	/* SD ADC1 */
	lpf2pSetCutoffFrequency(&fo_temp_volt, 5560.0, 10.0);
	lpf2pSetCutoffFrequency(&fo_oilp_volt, 5560.0, 10.0);
	lpf2pSetCutoffFrequency(&fo_vbat, 5560.0, 10.0);
	/* SD ADC3 */
	lpf2pSetCutoffFrequency(&fo_flow_volt, 16680.0, 50.0);

	/* ADC1 */
	adcStart(&ADCD1, NULL);
	adcSTM32Calibrate(&ADCD1);
	adcSTM32EnableTSVREFE();	/* enable thermometer */
	adcSTM32EnableVBATE();		/* enable Vrtc bat, TODO: enable once in 10-sec */

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

	alert_component(ALS_ADC, AL_NORMAL);
	while (true) {
		chThdSleepMilliseconds(20);

		adc_handle_battery();
		adc_handle_temperature();
		adc_handle_oilp();
		adc_handle_flow();
	}

	return MSG_OK;
}

void adc_init(void)
{
	chThdCreateStatic(wa_adc, sizeof(wa_adc), ADC_PRIO, th_adc, NULL);
}
