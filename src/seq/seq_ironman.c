/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include "ironman.h"
#include "zerror.h"

void IM_Initialize(int procnum,int procs,int comms,int maxblockspercomm) {
}


IM_info *IM_New(IM_comminfo *recvinfo,IM_comminfo *sendinfo,int commid) {
  if (recvinfo != NULL || sendinfo != NULL) {
    _RT_ANY_FATAL0("Trying to do communication in uniprocessor case");
  }
  return NULL;
}


void IM_Old(IM_info *old) {
}
