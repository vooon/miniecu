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

#include <time.h>
#include "rtc_time.h"
#include "alert_led.h"

/* -*- local -*- */

static bool m_time_is_known = false;


static void rtc_set_unix_msec(uint64_t ts)
{
	RTCDateTime timeval;
	struct tm tm;
	time_t ts_sec = ts / 1000;

	localtime_r(&ts_sec, &tm);

	timeval.year = tm.tm_year - 80;	// tm starts at 1900, chibios at 1980
	timeval.month = tm.tm_mon + 1;	// tm 0..11, rtc 1..12
	timeval.dstflag = 0;		// not used by RTCv2 driver
	timeval.dayofweek = tm.tm_wday + 1;	// tm mon(0)..sun(6), rtc mon(1)..sun(7)
	timeval.day = tm.tm_mday;
	timeval.millisecond = ts % 86400000;	// 86400 sec == 1 day

	//debug_printf(DP_DEBUG, "ts: %u-%u-%u %u %u", timeval.year, timeval.month, timeval.day, timeval.dayofweek, timeval.millisecond);
	rtcSetTime(&RTCD1, &timeval);
}

static uint64_t rtc_get_unix_msec(void)
{
	RTCDateTime timeval;
	struct tm tm;
	time_t date_ts;

	rtcGetTime(&RTCD1, &timeval);
	//debug_printf(DP_DEBUG, "tg: %u-%u-%u %u %u", timeval.year, timeval.month, timeval.day, timeval.dayofweek, timeval.millisecond);

	// same rules as in rtc_set_unix_msec()
	tm.tm_year = timeval.year + 80;
	tm.tm_mon = timeval.month - 1;
	tm.tm_isdst = -1;	// not known
	tm.tm_wday = timeval.dayofweek - 1;
	tm.tm_mday = timeval.day;
	// time set to 0:00:00
	tm.tm_yday = tm.tm_hour = tm.tm_min = tm.tm_sec = 0;

	// get date time stamp
	date_ts = mktime(&tm);

	return ((uint64_t)date_ts * 1000) + timeval.millisecond;
}


/* -*- global -*- */

/** Initialize RTC module
 */
void rtc_time_init(void)
{
	// if rtc time > 1000000000.000 sec: Sun Sep  9 05:46:40 MSD 2001
	m_time_is_known = (rtc_get_unix_msec() > 1000000000000)? true : false;
	if (m_time_is_known)
		alert_component(ALS_RTC, AL_NORMAL);
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
		return rtc_get_unix_msec();
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

	curr_ts = (m_time_is_known)? rtc_get_unix_msec() : ts;
	rtc_set_unix_msec(ts);

	m_time_is_known = true;
	alert_component(ALS_RTC, AL_NORMAL);
	return curr_ts - ts;
}
