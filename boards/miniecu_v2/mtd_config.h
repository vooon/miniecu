#ifndef _MTD_CONFIG_H_
#define _MTD_CONFIG_H_

#include "fw_common.h"

#define MTD_USE_SST25

/* Note: Maximum SPI speed on STM32F373 is 18 MHz
 *       so we can use SLOW_READ (< 25 MHz).
 *       But enable FAST_WRITE (SST25 AAI) for better performance.
 */

//#define SST25_FAST_READ
#define SST25_SLOW_READ
#define SST25_FAST_WRITE
//#define SST25_SLOW_WRITE

#endif /* _MTD_CONFIG_H_ */
