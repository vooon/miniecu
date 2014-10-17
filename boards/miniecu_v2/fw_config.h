#ifndef _FW_CONFIG_H_
#define _FW_CONFIG_H_

#define SERIAL1_SD	SD1

#define USE_RT_KERNEL

// threads priority
#define PBSTX_PRIO	(NORMALPRIO - 5)
#define LOG_PRIO	(NORMALPRIO - 2)
#define LED_PRIO	(LOWPRIO)
#define ADC_PRIO	(NORMALPRIO + 2)
#define RPM_PRIO	(NORMALPRIO + 1)
#define PARAMLD_PRIO	(NORMALPRIO)

// threads stack size
#define PBSTX_WASZ	2048
#define LOG_WASZ	1024
#define LED_WASZ	128
#define ADC_WASZ	512
#define RPM_WASZ	256*10
#define PARAMLD_WASZ	2048

#endif /* _FW_CONFIG_H_ */
