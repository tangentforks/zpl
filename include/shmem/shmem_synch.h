/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __SHMEM_SYNCH_
#define __SHMEM_SYNCH_

void _SHMEMSetupSynch(void);
void _SignalRecvReady(int,long);
void _WaitRecvReady(int,long);
void _SignalSendDone(int,long);
void _WaitSendDone(int,long);

#endif
