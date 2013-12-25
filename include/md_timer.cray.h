/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _MD_TIMER_CRAY_H_
#define _MD_TIMER_CRAY_H_

/* new timers */
#define _MD_TIMERS
#define _global_timer_decls static long long _cray_clock_rate = -1
#define _timerinit() _cray_clock_rate = _CRAY_CLOCK_RATE
#define _timervalue long long
#define _now(set) set = _rtc()
#define _getdiff(check,set) (((double)((check)-(set)))/_cray_clock_rate)

#endif
