/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <mpp/shmem.h>
#include "md_zlib.h"

volatile static long printtoken=0;
static long tokenon=1;
static long tokenval;


void _ResetToken(void) {
  printtoken=0;
  _BarrierSynch();
}


void _SendTokenWithValue(int next,long *val) {
  shmem_put(&tokenval,val,1,next);
  shmem_put((long *)&printtoken,&tokenon,1,next);
}


void _RecvTokenWithValue(long *val) {
  while (printtoken != 1) {
  }
  printtoken=0;
  *val = tokenval;
}


