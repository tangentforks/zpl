/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __SHMEM_IRONMAN_H_
#define __SHMEM_IRONMAN_H_

#ifndef DEFINE_SHMEM_IRON_GLOB
#define DEFINE_SHMEM_IRON_GLOB extern
#endif

#define _MAX_SHMEM_PROCS 64

#include "zplglobal.h"
#include "shmem_scanred.h"

typedef struct _meminfo {
  void * ptr;
  unsigned int elemsize;
  int numdims;
  _vector numelems;
  _vector stride;
} meminfo;

typedef struct _fullmeminfo {
  int num;
  meminfo * membank;
  int numalloced;
} fullmeminfo;

typedef struct _comminfo {
  int isend;
  int irecv;
  int dyn_commid;
  
  fullmeminfo sendinfo;
  fullmeminfo recvinfo;
} comminfo;

extern const int _max_ens_comm;
extern int _shmem_max_ens_comm;

#define _SHMEM_ACCESS_MEM(cid,proc,ens) (remmembuff[cid*_PROCESSORS*_shmem_max_ens_comm+proc*_shmem_max_ens_comm+ens])
#define _SHMEM_REM_PTR(cid,proc,ens) (_SHMEM_ACCESS_MEM(cid,proc,ens).ptr)

DEFINE_SHMEM_IRON_GLOB comminfo *commbank;
DEFINE_SHMEM_IRON_GLOB meminfo *remmembuff;


#endif
