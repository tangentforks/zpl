/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "pvm3.h"
#include "md_zinc.h"
#include "pvm_scanred.h"
#include "stand_inc.h"
#include "pvm_zlib.h"

static void (*pvm_scanred_userfn)(void*,void*);
static int data_multiple;

void _InitScanRed() {
}


void _Scan_zquad(_zquad* a,_zquad *b,_region c,int d,int e,int f,int g,
		 _zquad h) {
  _RT_ALL_FATAL0("PVM can't handle scans of quad values\n");
}


void _Scanqcomplex(qcomplex *a,qcomplex *b,_region c,int d,int e,int f,int g,
		    qcomplex h) {
  _RT_ALL_FATAL0("PVM can't handle scans of qcomplex values\n");
}


void _Reduce_zquad(_grid grid, _zquad* a,int b,int c,int d,int e,int f) {
  _RT_ALL_FATAL0("PVM can't handle reductions of quad values\n");
}


void _Reduceqcomplex(_grid grid, qcomplex *a,int b,int c,int d,int e,int f) {

  _RT_ALL_FATAL0("PVM can't handle reductions of qcomplex values\n");
}

static void PVMUserFunc(int* datatype, void* x, void* y, int* num, int* info) {
  int honest_num;

  honest_num = *num / data_multiple;
  if (*num % data_multiple != 0) {
    _RT_ALL_FATAL0("pvm user reduction error - data not fully delivered to user function");
    exit(1);
  }
  if (honest_num != 1) {
    _RT_ALL_FATAL0("pvm user reduction error - arrays not handled");
    exit(1);
  }
  pvm_scanred_userfn(y, x);
}

void _Reduceuser(_grid grid, void* data,int numelms,int redtype,int destproc, int bcast,
		 void (*userfn)(void*,void*), size_t size) {

  char *groupname;

  groupname = _GetSliceName(grid, redtype,_groupname);
  data_multiple = size / sizeof(int);
  if (size % sizeof(int) != 0) {
    _RT_ALL_FATAL0("pvm user reduction error - datatype size must be multiple of integer size");
    exit(1);
  }
  pvm_scanred_userfn = userfn;
  pvm_reduce(PVMUserFunc, data, data_multiple*numelms, PVM_INT, _GetDynamicCommID(), groupname, destproc);
  if (bcast) {
    _BroadcastSimpleToSlice(grid, data,numelms*size,1,destproc,0,redtype);
  }
}
