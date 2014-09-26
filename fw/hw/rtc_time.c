/**
 * @file       rtc_time.c
 * @brief      RTC helpers
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

#include "rtc_time.h"
#include "chrtclib.h"

/* -*- local -*- */

static bool m_time_is_known = false;

/* -*- global -*- */

/* XXX: RTC driver still not ported to v3 */

/** Initialize RTC module
 */
void rtc_time_init(void)
{
	/* TODO:
	 * - chack that RTC configured (RTC battery exists)
	 */

	m_time_is_known = false;
	//rtcSetPeriodicWakeup_v2(&RTCD1, NULL);
}

/** Get timestamp state flag
 */
bool time_is_known(void)
{
	return m_time_is_known;
}

/** Get timestamp for comm and log
 * @return sys time or RTC time
 */
uint64_t time_get_timestamp(void)
{
	if (!m_time_is_known) {
		return ST2MS(osalOsGetSystemTimeX());
	}
	else {
		return 0;//rtcGetTimeUnixUsec(&RTCD1) / 1000;
	}
}

/**
 * Set current timestamp
 * @note current RTC Lib don't support subsecond Set
 *
 * @return time diff
 */
int32_t time_set_timestamp(uint64_t ts)
{
	uint64_t curr_ts;

	//curr_ts = (m_time_is_set)? rtcGetTimeUnixUsec(&RTCD1) / 1000 : ts;
	// rtcSetTimeUnixUsec(&RTCD1, ts * 1000);
	//rtcSetTimeUnixSec(&RTCD1, ts / 1000);

	//m_time_is_known = true;

	return curr_ts - ts;
}
