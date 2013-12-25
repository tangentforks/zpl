/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ZTIMER_H_
#define __ZTIMER_H_

#include <md_timer.h>  /* search from deep dirs, not here */
#include <sys/time.h>

typedef struct _timer_struct {
  _timervalue start;
  double elapse;
  int running;
} timer;

void _InitTimer(void);
void ClearTimer(timer* const);
void StartTimer(timer* const, int);
void StopTimer(timer* const);
double ReadTimer(const timer* const);

#endif
