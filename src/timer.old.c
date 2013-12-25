/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** FILE: timer.c
 ***       Generic timing routines
 ***
 ***/

#include "stand_inc.h"


static timer globalTimer;
static timer checkpointTimer;


void _InitOldTimer(void) {
  ClearTimer(&globalTimer);
  ClearTimer(&checkpointTimer);
}


/* for backwards compatibility */


double ResetTimer() {
  double val;

  _BarrierSynch();
  val = ReadTimer(&globalTimer);
  _Reducedouble(_DefaultGrid, &val, 1, _OP_MAX, 0, 0, 1);
  ClearTimer(&globalTimer);
  StartTimer(&globalTimer, 0);

  return val;
}  


double CheckTimer() {
  double val = ReadTimer(&globalTimer);
  
  _Reducedouble(_DefaultGrid, &val, 1, _OP_MAX, 0, 0, 1);

  return val;
}


double ResetCheckpointTimer() {
  double val = ReadTimer(&globalTimer);

  ClearTimer(&checkpointTimer);
  StartTimer(&checkpointTimer, 1);

  return val;
}


double CheckCheckpointTimer() {
  return ReadTimer(&checkpointTimer);
}
