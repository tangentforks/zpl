/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include "pvm3.h"

#include "md_zinc.h"
#include "ironman.h"
#include "ironman_utils.h"
#include "zlib.h"

#ifdef PRINT_WIRE
#define PRINT_W(t1,t2,t3,t4,t5) printf(t1,t2,t3,t4,t5)
#else
#define PRINT_W(t1,t2,t3,t4,t5)
#endif

extern int *_tids;

static IM_info *commbank;
static IM_info **freelist;
static int numfree;


void IM_Initialize(const int procnum,const int procs,const int comms,
		   const int maxdirs,const int maxblocks) {
  int i;
  int j;

  commbank = (IM_info *)_zmalloc(comms*sizeof(IM_info),"pvm commank");
  freelist = (IM_info **)_zmalloc(comms*sizeof(IM_info *),"pvm freelist");
  for (i=0;i<comms;i++) {
    commbank[comms-(i+1)].numrecvs = 0;
    commbank[comms-(i+1)].recvinfo = 
      (IM_percomm*)_zmalloc(maxdirs*sizeof(IM_percomm),"pvm recvinfo");
    commbank[comms-(i+1)].numsends = 0;
    commbank[comms-(i+1)].sendinfo = 
      (IM_percomm*)_zmalloc(maxdirs*sizeof(IM_percomm),"pvm sendinfo");
    for (j=0; j<maxdirs; j++) {
      commbank[comms-(i+1)].recvinfo[j].consecutive = 0;
      commbank[comms-(i+1)].recvinfo[j].dyn_commid = 0;
      commbank[comms-(i+1)].recvinfo[j].comminfo = NULL;
      commbank[comms-(i+1)].sendinfo[j].consecutive = 0;
      commbank[comms-(i+1)].sendinfo[j].dyn_commid = 0;
      commbank[comms-(i+1)].sendinfo[j].comminfo = NULL;
    }
    freelist[i] = &(commbank[comms-(i+1)]);
  }
  numfree = comms-1;
}


IM_info *IM_New(_grid grid,const int numcomms,const IM_commpair* const incomminfo) {
  IM_info* const newinfo = freelist[numfree--];
  int numrecvs = 0;
  int numsends = 0;
  IM_comminfo* info;
  int i;

  for (i=0; i<numcomms; i++) {
    const int dyn_commid = _GetDynamicCommID();

    info = incomminfo[i].recvinfo;
    if (info) {
      const int consecutive = _IMU_Pack(info,info);;

      newinfo->recvinfo[numrecvs].consecutive = consecutive;
      if (consecutive) {
	newinfo->recvinfo[numrecvs].buffsize = info->mbvect[0].elemsize;
	newinfo->recvinfo[numrecvs].buff = info->mbvect[0].baseptr;
      } else {
	newinfo->recvinfo[numrecvs].buff = 
	  _IMU_AllocBuffer(info,&(newinfo->recvinfo[numrecvs].buffsize));
      }
      newinfo->recvinfo[numrecvs].dyn_commid = dyn_commid;
      newinfo->recvinfo[numrecvs].comminfo = info;
      numrecvs++;
    }
    info = incomminfo[i].sendinfo;
    if (info) {
      const int consecutive = _IMU_Pack(info,info);;

      newinfo->sendinfo[numsends].consecutive = consecutive;
      if (consecutive) {
	newinfo->sendinfo[numsends].buffsize = info->mbvect[0].elemsize;
	newinfo->sendinfo[numsends].buff = info->mbvect[0].baseptr;
      } else {
	newinfo->sendinfo[numsends].buff = 
	  _IMU_AllocBuffer(info,&(newinfo->sendinfo[numsends].buffsize));
      }
      newinfo->sendinfo[numsends].dyn_commid = dyn_commid;
      newinfo->sendinfo[numsends].comminfo = info;
      numsends++;
    }
  }

  if (numrecvs == 0 && numsends == 0) {
    freelist[++numfree] = newinfo;
    return NULL;
  }

  newinfo->numrecvs = numrecvs;
  newinfo->numsends = numsends;
  
  return newinfo;
}


void IM_SR(_grid grid, IM_info* const in_info) {
  const IM_info* const info = in_info;
  const int numsends = info->numsends;
  int i;

  for (i=0; i<numsends; i++) {
    const int consecutive = info->sendinfo[i].consecutive;
    const int msgsize = info->sendinfo[i].buffsize;
    char* const buff = info->sendinfo[i].buff;
    const IM_comminfo* const sendinfo = info->sendinfo[i].comminfo;
    const int dyn_commid = info->sendinfo[i].dyn_commid;
    const int isend = sendinfo->procnum;

    if (!consecutive) {
      _IMU_Marshall(buff,sendinfo);
    }

    PRINT_W("[%d] sending %d to %d on %d\n",_INDEX,msgsize,isend,dyn_commid);
    pvm_initsend(PvmDataRaw);
    pvm_pkbyte(buff,msgsize,1);
    pvm_send(_tids[isend],dyn_commid);
  }
}


void IM_DN(_grid grid, IM_info* const in_info) {
  const IM_info* const info = in_info;
  const int numrecvs = info->numrecvs;
  int i;

  for (i=0; i<numrecvs; i++) {
    const int consecutive = info->recvinfo[i].consecutive;
    const int buffsize = info->recvinfo[i].buffsize;
    char* const buff = info->recvinfo[i].buff;
    const IM_comminfo* const recvinfo = info->recvinfo[i].comminfo;
    const int dyn_commid = info->recvinfo[i].dyn_commid;
    const int irecv = recvinfo->procnum;

    PRINT_W("[%d] recving %d fm %d on %d\n",_INDEX,buffsize,irecv,dyn_commid);
    pvm_recv(_tids[irecv],dyn_commid);
    pvm_upkbyte(buff,buffsize,1);
    
    if (!consecutive) {
      _IMU_Unmarshall(recvinfo,buff);
    }
  }
}


void IM_Old(IM_info* const oldinfo) {
  const IM_info* const info = oldinfo;
  const int numrecvs = info->numrecvs;
  const int numsends = info->numsends;
  int i;

  for (i=0; i<numrecvs; i++) {
    const int consecutive = info->recvinfo[i].consecutive;

    if (!consecutive) {
      char* const buff = info->recvinfo[i].buff;
      
      _IMU_FreeBuffer(buff);
    }
  }

  for (i=0; i<numsends; i++) {
    const int consecutive = info->sendinfo[i].consecutive;

    if (!consecutive) {
      char* const buff = info->sendinfo[i].buff;
      
      _IMU_FreeBuffer(buff);
    }
  }

  freelist[++numfree] = oldinfo;
}
