/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"

void _CalcRedReg(_region_fnc new,_region dst,_region src) {
  const int numdims = _NUMDIMS(dst);
  int i;

  _NUMDIMS(new) = _NUMDIMS(src);
  _REG_DIST(new) = _REG_DIST(src);
  _REG_ARRLIST(new) = NULL;
  for (i=0; i<numdims; i++) {
    if (_RED_DIM(src,dst,i) || _FLOODED_DIM(src, i)) {
      _SET_GLOB_BOUNDS_FLOOD(new,i,0,0,_DIM_RGRID);
    } else {
      _SET_GLOB_BOUNDS(new,i,_REG_GLOB_LO(src,i),_REG_GLOB_HI(src,i),
		       _REG_STRIDE(src,i));
    }
  }
  _REG_SET_SETUP_ON(new);
  _InitRegion(new, 0);
}

#ifndef _MD_STUBS_REDS

void _Reduce_user(void *data, int numelms, _region dstR, _region srcR, void (*userfn)(void*,void*), size_t size) {
  int i;
  int dimbit;
  int redtype=0;
  int bcast=0;
  int destproc;
  int toploc[_MAXRANK];

  if (_PROCESSORS > 1) {
    if (dstR == NULL && srcR == NULL) {
      _Reduceuser(_DefaultGrid, data, numelms, 0, 0, bcast, userfn, size);
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
	_Reduceuser(grid, data, numelms, redtype, destproc, bcast, userfn, size);
      }
    }
  }
}

#endif
