/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "pvm3.h"
#include "stand_inc.h"
#include "pvm_scanred.h"
#include "pvm_zlib.h"

#define _GSR(t)       _Scan##t
#define GSR(t)        _GSR(t)
#define GLB_SCAN_ROUT GSR(TYPE)

#define _GRR(t)       _Reduce##t
#define GRR(t)        _GRR(t)
#define GLB_RED_ROUT  GRR(TYPE)

#define _PVM_TYPE(t)  _PVM_TYPE_##t
#define PVM_TYPE(t)   _PVM_TYPE(t)

#ifndef COMPLEX_TYPE
static void PVMAndFunc(int *datatype,void *x,void *y,int *num,int *info) {
  int i;

  for (i=0;i<*num;i++) {
    ((TYPE *)x)[i] = ((TYPE *)x)[i] && ((TYPE *)y)[i];
  }
}


static void PVMOrFunc(int *datatype,void *x,void *y,int *num,int *info) {
  int i;

  for (i=0;i<*num;i++) {
    ((TYPE *)x)[i] = ((TYPE *)x)[i] || ((TYPE *)y)[i];
  }
}

#ifndef FLOAT_TYPE
static void PVMBandFunc(int *datatype,void *x,void *y,int *num,int *info) {
  int i;

  for (i=0;i<*num;i++) {
    ((TYPE *)x)[i] &= ((TYPE *)y)[i];
  }
}


static void PVMBorFunc(int *datatype,void *x,void *y,int *num,int *info) {
  int i;

  for (i=0;i<*num;i++) {
    ((TYPE *)x)[i] |= ((TYPE *)y)[i];
  }
}


static void PVMXorFunc(int *datatype,void *x,void *y,int *num,int *info) {
  int i;

  for (i=0;i<*num;i++) {
    ((TYPE *)x)[i] ^= ((TYPE *)y)[i];
  }
}

#endif
#else
static void PVMComplexProduct(int *datatype,void *x,void *y,int *num,int *info){
  int i;

  for (i=0;i<(*num)/2;i++) {
    _MULT_STMT_CMPLX_HELP(((TYPE *)x)[i],((TYPE *)y)[i],TYPE);
  }
}
#endif


static void (*opvect[])(int *,void *,void *,int *,int *) = {NULL,PvmSum,
#ifndef COMPLEX_TYPE
							      PvmProduct,
							      PvmMax,PvmMin,
							      PVMAndFunc,
							      PVMOrFunc
#ifndef FLOAT_TYPE
								,PVMBandFunc,
								PVMBorFunc,
								PVMXorFunc
#endif
#else
							      PVMComplexProduct
#endif
			     };

void GLB_SCAN_ROUT(TYPE input[], TYPE output[], _region R, int dim, int numelms,
		   int dir,int op,TYPE ident) {
  int i,recv_proc,send_proc;
  int off[_MAXRANK] = {0,0,0,0,0,0};
  int read=0,combine=0,write=0,init=0;
  int loc[_MAXRANK];
  _distribution dist = _REG_DIST(R);
  _grid grid = _DIST_GRID(dist);

  if (_REG_I_OWN(R)) {
    
    if (dir != 1 && dir != -1) {
      printf("Whoops: brad dir mis-assumption\n");
      return;
    }
    
    off[dim] = 1 * dir;
    
    for (i=0; i<_MAXDIM; i++) {
      loc[i] = _GRID_LOC(grid, i)-off[i];
    }
    recv_proc = _GRID_TO_PROC(grid, _MAXDIM,loc);

    for (i=0; i<_MAXDIM; i++) {
      loc[i] = _GRID_LOC(grid, i)+off[i];
    }
    send_proc = _GRID_TO_PROC(grid, _MAXDIM,loc);
    
    if (!_DIM_DIST(R,dim) || (_FLOODED_DIM(R,dim) == _DIM_FLOODED)) {
      init=1;
    } else if (_REG_MYLO(R, dim) == _REG_GLOB_LO(R, dim)) {
      if (dir>=0) {
	write=1;
	init=1;
      } else {
	read=1;
	combine=1;
      }
    } else if (_REG_MYHI(R, dim) == _REG_GLOB_HI(R, dim)) {
      if (dir>=0) {
	read=1;
	combine=1;
      } else {
	write=1;
	init=1;
      }
    } else {
      read=1;
      combine=1;
      write=1;
    }
  
    if (read) {
      _ReadPort(recv_proc, _PORT_SCAN, _WPROC_COMMID(100, recv_proc),
		output, numelms*sizeof(TYPE));
    }
    if (combine) {
      for (i=0; i<numelms; i++) {
	DO_OP(op,input[i],output[i])
      }
    }
    if (write) {
      _WritePort(send_proc, _PORT_SCAN, _WPROC_COMMID(100, _INDEX), input,
		 numelms*sizeof(TYPE));
    }
    if (init) {
      for (i=0; i<numelms; i++) {
	output[i] = ident;
      }
    }
  }
}


void GLB_RED_ROUT(_grid grid, TYPE* data,int numelms,int op,int redtype,int destproc,
		  int bcast) {
  char *groupname;

  groupname = _GetSliceName(grid, redtype,_groupname);
#ifdef COMPLEX_TYPE
  pvm_reduce(opvect[op],data,2*numelms,PVM_TYPE(TYPE),_GetDynamicCommID(),groupname,destproc);
#else
  pvm_reduce(opvect[op],data,numelms,PVM_TYPE(TYPE),_GetDynamicCommID(),groupname,destproc);
#endif
  if (bcast) {
    _BroadcastSimpleToSlice(grid, data,numelms*sizeof(TYPE),1,destproc,0,redtype);
  }
}
