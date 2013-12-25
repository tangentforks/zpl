/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** FILE: timer.c
 ***       Generic timing routines
 ***
 ***/

#include "barrier.h"
#include "timer.h"

_global_timer_decls;


void _InitTimer(void) {
  _timerinit();
  _InitOldTimer();
}


void ClearTimer(timer* const t) {
  t->elapse = 0.0;
  t->running = 0;
}


void StartTimer(timer* const t, int sync) {
  if (!(t->running)) {
    if (sync) {
      _BarrierSynch();
    }
    t->running = 1;
    _now(t->start);
  }
  else {
    printf("Attemp to start a timer that has not been stopped.");
  }
}


void StopTimer(timer* const t) {
  if (t->running) {
    _timervalue tv;
    _now(tv);
    t->elapse += _getdiff(tv, t->start);
    t->running = 0;
  }
  else {
    printf("Attempt to stop a timer that has not been started.");
  }
}


double ReadTimer(const timer* const t) {
  if (t->running) {
    _timervalue tv;
    _now(tv);
    return t->elapse + _getdiff(tv, t->start);
  } else {
    return t->elapse;
  }
}
