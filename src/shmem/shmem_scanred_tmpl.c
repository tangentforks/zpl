/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <string.h>
#include <mpp/shmem.h>
#include "stand_inc.h"
#include "shmem_scanred.h"

#define _GSR(t)       _Scan##t
#define GSR(t)        _GSR(t)
#define GLB_SCAN_ROUT GSR(TYPE)

#define _GRR(t)       _Reduce##t
#define GRR(t)        _GRR(t)
#define GLB_RED_ROUT  GRR(TYPE)

#define _LR(t,o)      shmem_##t##_##o##_to_all
#define LR(t,o)       _LR(t,o)
#define LIB_ADD       LR(TYPE,sum)
#define LIB_MULT      LR(TYPE,prod)
#define LIB_AND       LR(TYPE,and)
#define LIB_OR        LR(TYPE,or)
#define LIB_MAX       LR(TYPE,max)
#define LIB_MIN       LR(TYPE,min)
#define LIB_BAND      LR(TYPE,and)
#define LIB_BOR       LR(TYPE,or)
#define LIB_BXOR      LR(TYPE,xor)

void GLB_SCAN_ROUT(TYPE input[], TYPE output[], _region R, int dim, int numelms,
		   int dir,int op,TYPE ident) {
  _RT_ALL_FATAL0("Scans not yet implemented for shmem -- "
		 "bug us: zpl-info@cs.washington.edu\n");
}


void GLB_RED_ROUT(_grid grid, TYPE* data, int numelems, int op, int redtype,
		  int destproc, int bcast) {
  int start;
  int stride;
  int procs;
  int involved=1;
  long *pSync;
  char* sym_input;
  char* sym_output;
  void* workspace;
  int i;
  int bitvect;
  int reddim=-1;

  if (redtype == 0) {
    start=0;
    stride=0;
    procs=_PROCESSORS;
  } else {
    bitvect = 1;
    for (i=0; i<_MAXDIM; i++) {
      if (((redtype & bitvect) != 0) && (_GRID_SIZE(grid, i) == 1)) {
	redtype -= bitvect;
      }
      bitvect = bitvect << 1;
    }
    if (redtype == 0) {
      start=0;
      stride=0;
      procs=_PROCESSORS;
    } else {
      bitvect = 1;
      for (i=0; i<_MAXDIM; i++) {
	if ((redtype & bitvect) == 0) {
	  if (reddim == -1) {
	    reddim = i;
	  } else {
	    fprintf(stderr,
		    "ERROR: brad hasn't implemented multidim reductions yet\n");
	    return;
	  }
	}
	bitvect = bitvect << 1;
      }
      if (reddim == 0) {
	if (_GRID_SIZE(grid, 0) == 1) {
	  return;
	}
	start=_GRID_LOC(grid, 1);
	stride=0;
	bitvect = _GRID_SIZE(grid, 1);
	while (bitvect != 1) {
	  bitvect = bitvect >> 1;
	  stride++;
	}
	procs=_GRID_SIZE(grid, 0);
      } else if (reddim == 1) {
	if (_GRID_SIZE(grid, 1) == 1) {
	  return;
	}
	start= _GRID_LOC(grid, 0) * _GRID_SIZE(grid, 1);
	stride=0;
	procs=_GRID_SIZE(grid, 1);
      } else {
	fprintf(stderr,
		"ERROR: brad hasn't implemented higher dimensional reductions yet\n");
	return;
      }
    }
  }

  _GetRedBuffs(numelems,sizeof(TYPE),&pSync,&sym_input,&sym_output,&workspace);
  if (involved) {
    memcpy(sym_input,data,numelems*sizeof(TYPE));

    switch (op) {
    case _OP_ADD:
      LIB_ADD((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	      (TYPE *)workspace,pSync);
      break;
#ifndef COMPLEX_TYPE
    case _OP_MULT:
      LIB_MULT((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	       (TYPE *)workspace,pSync);
      break;
    case _OP_MAX:
      LIB_MAX((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	      (TYPE *)workspace,pSync);
      break;
    case _OP_MIN:
      LIB_MIN((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	      (TYPE *)workspace,pSync);
      break;
#ifndef FLOAT_TYPE
    case _OP_AND:
      LIB_AND((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	      (TYPE *)workspace,pSync);
      break;
    case _OP_OR:
      LIB_OR((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	     (TYPE *)workspace,pSync);
      break;
    case _OP_BAND:
      LIB_BAND((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	       (TYPE *)workspace,pSync);
      break;
    case _OP_BOR:
      LIB_BOR((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	      (TYPE *)workspace,pSync);
      break;
    case _OP_XOR:
      LIB_BXOR((TYPE *)sym_output,(TYPE *)sym_input,numelems,start,stride,procs,
	       (TYPE *)workspace,pSync);
      break;
#else
    case _OP_AND:
    case _OP_OR:
      _RT_ALL_FATAL0("float & and | reductions are not yet implemented for "
		     "shmem --\n"
		     "bug us: zpl-info@cs.washington.edu\n");
      break;
#endif
#endif
    }
  }
  /* if complete reduce but not all processors involved, broadcast result */
  if (redtype == 0 && bcast) {
    _BroadcastSimpleToSlice(grid, sym_output,numelems*sizeof(TYPE),0,1,0,
			    redtype);
  }
  memcpy(data,sym_output,numelems*sizeof(TYPE));
}


