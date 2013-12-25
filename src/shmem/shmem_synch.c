/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "mpp/shmem.h"
#include "shmem_synch.h"
#include "stand_inc.h"

static long volatile *sendbuff;


void _SHMEMSetupSynch() {
  int i;

  sendbuff = (long *)shmalloc(sizeof(long)*_PROCESSORS);
  for (i=0;i<_PROCESSORS;i++) {
    sendbuff[i]=0;
  }
}


void _SignalSendDone(int proc,long commid) {
  shmem_put((long *)&(sendbuff[_INDEX]),&commid,1,proc);
}


void _WaitSendDone(int proc,long commid) {
  while (sendbuff[proc] < commid) {
  }
}

