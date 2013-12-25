/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"

#define _GRR(t)       _Reduce_##t
#define GRR(t)        _GRR(t)
#define GLB_RED_ROUT  GRR(TYPE)

#define _MGRR(t)         _Reduce##t
#define MGRR(t)          _MGRR(t)
#define MD_GLB_RED_ROUT  MGRR(TYPE)

#ifndef _MD_STUBS_REDS

void GLB_RED_ROUT(TYPE data[],int numelms,_region dstR,_region srcR,int op) {
  int i;
  int dimbit;
  int redtype=0;
  int bcast=0;
  int destproc;
  int toploc[_MAXRANK];

  if (_PROCESSORS > 1) {
    if (dstR == NULL && srcR == NULL) {
      MD_GLB_RED_ROUT(_DefaultGrid, data, numelms, op, 0, 0, 1);
      return;
    }
    {
      const int numdims = _NUMDIMS(srcR);
      _grid grid = _DIST_GRID(_REG_DIST(srcR));
      if (dstR == NULL) { /* complete reduce */
	dimbit = 1;
	for (i=0; i<numdims; i++) {
	  if (_GRID_SIZE(grid, i) == 1 || _FLOODED_DIM(srcR,i) == _DIM_FLOODED) {
	    redtype |= dimbit;
	  }
	  dimbit = dimbit << 1;
	}
	bcast = 1;
	destproc = 0;
      } else {
	dimbit = 1;
	for (i=0; i<numdims; i++) {
	  if (_GRID_SIZE(grid, i) > 1 && _RED_DIM(srcR,dstR,i)) {
	    if (_BCS_DIM(srcR,dstR,i)) {
	      bcast = 1;
	    }
	    toploc[i] = _MIN_PROC(dstR,i);
	  } else {
	    redtype |= dimbit;
	    toploc[i] = 0;
	  }
	  dimbit = dimbit << 1;
	}
	destproc = _PosToProcInSlice(grid, numdims,toploc,redtype);
      }
      if (redtype != _UNIPROC(numdims)) {  /* if reduction is not local */
	MD_GLB_RED_ROUT(grid, data,numelms,op,redtype,destproc,bcast);
      }
    }
  }
}

#endif

