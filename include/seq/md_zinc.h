/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MD_ZINC_H_
#define __MD_ZINC_H_

#include <stdio.h>

#define _GetDynamicCommID() 0
#define _InitCommID()
#define _SetDynamicCommID(cid) 0
#define _InitScanRed()


/* STUB BROADCASTS and override defaults */

#define _MD_BCAST_NO_DEFAULT
#define _BroadcastSimple(g,w,x,y,z)
#define _BroadcastSimpleToSlice(g,u,v,w,x,y,z)


/* STUB REDUCTIONS */

#define _MD_STUBS_REDS

/* stub MI versions */
#define _Reduce_int
#define _Reduce__uint
#define _Reduce_long
#define _Reduce__ulong
#define _Reduce_float
#define _Reduce_double
#define _Reduce__zquad
#define _Reduce_fcomplex
#define _Reduce_dcomplex
#define _Reduce_qcomplex
#define _Reduce_user

/* stub MD versions */
#define _Reduceint
#define _Reduce_uint
#define _Reducelong
#define _Reduce_ulong
#define _Reducefloat
#define _Reducedouble
#define _Reduce_zquad
#define _Reducefcomplex
#define _Reducedcomplex
#define _Reduceqcomplex


#define _ResetToken()
#define _SendTokenWithValue(y,z)
#define _RecvTokenWithValue(z)

extern int _SRC_LOC;         /* MDD */
extern int _SRCSAMPL_NUM;    /* MDD */
extern int *_SRCSAMPL_ZPROF; /* MDD */

extern int _PRCNT_NUM;     /* MDD: number of basic bloc counters */
extern int *_PRCNT_ZPROF;  /* MDD: bb counter profile ds         */
extern int _PCCNT_NUM;     /* MDD: number of call-edge counters  */
extern int *_PCCNT_ZPROF;  /* MDD: call-edge profile ds          */

void zprof_ds_out(void);	   /* MDD */
void zprof_alarm_set(void);        /* MDD */

typedef struct {
  int optsvector;
  int tlcnt;
  int trcnt;
  int dims;
  int* lf;
  int** lind;
  int** rind;
  int* procmap;
  int* lindbase;
  int* rindbase;
} _permmap;

typedef struct {
  int optsvector;
  int eltsize;
  char** ldata;
  char** rdata;
  char* lbase;
  char* rbase;
} _permdata;

#endif
