/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <macros.h>
#include <mpp/shmem.h>
#include "ironman.h"
#include "stand_inc.h"
#include "shmem_ironman.h"

#pragma _CRI cache_align barrierspace,barriernum,barrieron

volatile static long barrierspace[_MAX_SHMEM_PROCS][2];
volatile static int barriernum[_MAX_SHMEM_PROCS];
volatile static long barrieron=1;


void _InitBiBarrier() {
  int i;
  int j;
  
  for (i=0;i<_N_PES;i++) {
    barriernum[i] = 0;
    barrierspace[i][0] = 0;
    barrierspace[i][1] = 0;
  }
}


void _StartBiBarrier(int proc) {
  if (proc >= 0) {
    shmem_put((long *)&(barrierspace[_INDEX][barriernum[proc]]),
	      (long *)&barrieron,1,proc);
  }
}


void _FinishBiBarrier(int proc) {
  int bnum;

  if (proc >= 0) {
    bnum = barriernum[proc];
    if (_CommsInProgress()) {
      shmem_udcflush_line((long *)&(barrierspace[proc][bnum]));
    }
    while (barrierspace[proc][bnum] != 1) {
    }
    barrierspace[proc][bnum] = 0;
    barriernum[proc]=(bnum+1)%2;
  }
}
