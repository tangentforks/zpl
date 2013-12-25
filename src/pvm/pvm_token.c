/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "pvm3.h"
#include "md_zlib.h"
#include "pvm_zlib.h"

extern int *_tids;

void _ResetToken() {
  _BarrierSynch();
  while (pvm_nrecv(-1,_TOKENID) != 0) {
  }
  _BarrierSynch();
}


void _SendTokenWithValue(int next,long *val) {
  pvm_initsend(PvmDataRaw);
  pvm_pkbyte((char *)val,sizeof(long),1);
  pvm_send(_tids[next],_TOKENID);
}


void _RecvTokenWithValue(long *val) {
  pvm_recv(-1,_TOKENID);
  pvm_upkbyte((char *)val,sizeof(long),1);
}




