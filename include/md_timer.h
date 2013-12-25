/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __DEFAULT_MD_TIMER_H_
#define __DEFAULT_MD_TIMER_H_

#include <stdlib.h>
#include <sys/time.h>

#define _global_timer_decls

#define _timerinit() NULL

#define _timervalue struct timeval

#define _now(set) \
  { \
    struct timezone tz; \
    gettimeofday(&(set), &tz); \
  }

#define _getdiff(check,set) \
        ((double) (((check).tv_sec*1e+6+(check).tv_usec) -\
	           ((set).tv_sec*1e+6+(set).tv_usec)) / 1e+6)

#endif
