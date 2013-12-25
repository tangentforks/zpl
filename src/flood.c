/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <string.h>
#include "stand_inc.h"
#include "priv_access.h"

static void _GlobalFlood(_region dstreg,_region srcreg,_array dstens,
			 _array srcens,unsigned int elemsize,int commid,
			 int floodtype) {
  int numdims;
  _vector lorect;
  _vector hirect;
  _vector temp;
  IM_comminfo src;
  IM_comminfo *srcptr;
  IM_memblock _srcmem;
  IM_memblock *srcmem=NULL;
  IM_comminfo dst;
  IM_comminfo *dstptr;
  IM_memblock _dstmem;
  IM_memblock *dstmem=&(_dstmem);
  int w;
  int r;
  unsigned int origelemsize;
  int rootloc[_MAXRANK];
  int dimbit;
  int rootproc;
  _distribution dist = _REG_DIST(dstreg);
  _grid grid = _DIST_GRID(dist);
  _PRIV_TID_DECL;

  numdims = _NUMDIMS(dstreg);
  if (!_REG_I_OWN(srcreg) && !_REG_I_OWN(dstreg)) {
    dstptr = NULL;
    srcptr = NULL;
  } else {
    for (w=0;w<numdims;w++) {
      if ((_FLOODED_DIM(dstreg,w) && !_REG_I_OWN(srcreg)) &&
	  !_FLOODED_DIM(srcreg,w)) {
	lorect[w] = _REG_GLOB_LO(srcreg,w);
	hirect[w] = _REG_GLOB_HI(srcreg,w);
      } else {
	lorect[w] = _REG_MYLO(srcreg,w);
	hirect[w] = _REG_MYHI(srcreg,w);
      }
      temp[w] = lorect[w];
    }
    
    dstptr = &dst;
    dst.numblocks = 1;
    dst.mbvect = dstmem;
    dstmem->baseptr = _GenAccess(dstens,lorect);
    if (_REG_I_OWN(srcreg)) {
      if (srcens == dstens) {
	srcptr = dstptr;
      } else {
	srcptr = &src;
	src.numblocks = 1;
	srcmem = &(_srcmem);
	src.mbvect = srcmem;
	srcmem->baseptr = _GenAccess(srcens,lorect);
      }
    } else {
      srcptr = NULL;
    }
    
    origelemsize = elemsize;
    
    for (r=numdims-1,w=0;r>=0;r--,w++) {
      dstmem->diminfo[w].numelems = hirect[r]-lorect[r]+1;
      if (srcmem) {
	srcmem->diminfo[w].numelems = dstmem->diminfo[w].numelems;
      }
      if (dstmem->diminfo[w].numelems == 1) {
	if (r==0 && w==0) {
	} else {
	  w--;
	}
      } else {
	lorect[r]++;
	
	dstmem->diminfo[w].stride = _GenAccessDistance(dstens,temp,lorect);
	if (srcmem) {
	  srcmem->diminfo[w].stride = _GenAccessDistance(srcens,temp,lorect);
	}
	
	if (w <= 1) {
	  if (dstmem->diminfo[w].stride == origelemsize &&
	      (srcmem == NULL || srcmem->diminfo[w].stride == origelemsize)) {
	    elemsize *= dstmem->diminfo[w].numelems;
	    dstmem->diminfo[w].stride *= dstmem->diminfo[w].numelems;
	    if (srcmem) {
	      srcmem->diminfo[w].stride *= dstmem->diminfo[w].numelems;
	    }
	    if (w == 0) {
	      dstmem->diminfo[w].numelems=1;
	      if (srcmem) {
		srcmem->diminfo[w].numelems=1;
	      }
	    } else {
	      w--;
	    }
	    hirect[r]=lorect[r]-1; /* we've essentially collapsed this dim */
	  }
	} else {
	  if (dstmem->diminfo[w].stride == dstmem->diminfo[w-1].stride &&
	      (srcmem == NULL || 
	       srcmem->diminfo[w].stride == srcmem->diminfo[w-1].stride)) {
	    dstmem->diminfo[w-1].numelems += dstmem->diminfo[w].numelems;
	    if (srcmem) {
	      srcmem->diminfo[w-1].numelems += srcmem->diminfo[w].numelems;
	    }
	    w--;
	  }
	}
	lorect[r]--;
      }
      temp[r]=hirect[r];
    }
  }
  dstmem->elemsize = elemsize;
  dstmem->numdims = w;
  if (srcmem) {
    srcmem->elemsize = elemsize;
    srcmem->numdims = w;
  }
  
  dimbit = 1;
  for (r=0; r<numdims; r++) {
    if (floodtype & dimbit) {
      rootloc[r] = 0;
    } else {
      rootloc[r] = _MIN_PROC(srcreg,r);
    }
    dimbit = dimbit << 1;
  }
  rootproc = _PosToProcInSlice(grid, numdims,rootloc,floodtype);

  _BroadcastToSlice(grid, dstptr,srcptr,rootproc,1,commid,floodtype);
}


static void _LocalFlood(_region dstreg,_region srcreg,_array dstens,
			_array srcens,unsigned int elemsize) {
  int numdims;
  _vector lorect;
  _vector hirect;
  _vector temp;
  _vector origin;
  char *srcptr;
  char *dstptr;
  _vector numelems;
  _vector srcstride;
  _vector dststride;
  int w;
  int cont;
  _PRIV_TID_DECL;

  numdims = _NUMDIMS(dstreg);
  if (_REG_I_OWN(srcreg) && _REG_I_OWN(dstreg)) {
    for (w=0;w<numdims;w++) {
      origin[w] = 0;
      temp[w] = 0;
      lorect[w] = 0;
      hirect[w] = _REG_MYLO(srcreg,w);
      numelems[w] = _REG_NUMELMS(srcreg,w);
    }
    
    dstptr = (char *)_GenAccess(dstens,hirect);
    srcptr = (char *)_GenAccess(srcens,hirect);
    
    for (w=numdims-1;w>=0;w--) {
      if (numelems[w] == 1) {
	numdims--;
      } else {
	temp[w] = _REG_STRIDE(srcreg,w);
	dststride[w] = _GenAccessDistance(dstens,origin,temp);
	srcstride[w] = _GenAccessDistance(srcens,origin,temp);
	temp[w] = 0;
	if ((srcstride[w]==elemsize) && (dststride[w]==elemsize)) {
	  elemsize *= numelems[w];
	  numelems[w]=1;
	  numdims--;
	} else {
	  origin[w] = _REG_MYHI(srcreg,w);
	  temp[w] = _REG_MYLO(srcreg,w);
	  w--;
	  break;
	}
      }
    }

    for (;w>=0;w--) {
      temp[w] = _REG_STRIDE(srcreg,w);
      dststride[w] = _GenAccessDistance(dstens,origin,temp);
      srcstride[w] = _GenAccessDistance(srcens,origin,temp);
      origin[w] = _REG_MYHI(srcreg,w);
      temp[w] = _REG_MYLO(srcreg,w);
    }
    
    if (!numdims) {
      numdims = 1;
    }
    cont = 1;
    while (cont) {
      memcpy(dstptr,srcptr,elemsize);
      
      lorect[numdims-1]++;
      for (w=numdims-1;w>=0;w--) {
	if (lorect[w] == numelems[w]) {
	  if (w) {
	    lorect[w-1]++;
	    lorect[w]=0;
	  } else {
	    cont = 0;
	  }
	} else {
	  dstptr += dststride[w];
	  srcptr += srcstride[w];
	  break;
	}
      }
    }
  }
}

void _FloodGuts(_region dstreg,_region srcreg,_array dstens,
		_array srcens,unsigned int elemsize,int commid) {
  const int numdims = _NUMDIMS(dstreg);
  int floodtype = _UNIPROC(numdims) ^ _UNIPROC(_MAXDIM_GRID);
  int dimbit;
  int bcast_needed;
  int i;
  _distribution dist = _REG_DIST(dstreg);
  _grid grid = _DIST_GRID(dist);
  _PRIV_TID_DECL;
  
  dimbit = 1;
  bcast_needed = 0;
  for (i=0; i<numdims; i++) {
    if (_FLOODED_DIM(dstreg,i) && !_FLOODED_DIM(srcreg,i) && _GRID_SIZE(grid, i) > 1) {
      bcast_needed = 1;
    } else {
      floodtype |= dimbit;
    }
    
    dimbit = dimbit << 1;
  }

  if (bcast_needed) {      /* broadcast required */
    _GlobalFlood(dstreg,srcreg,dstens,srcens,elemsize,commid,floodtype);
  } else {
    _LocalFlood(dstreg,srcreg,dstens,srcens,elemsize);
  }
}


void _SetupTempFloodEnsemble(_array_fnc E,_region_fnc R,unsigned int elemsize) {
  int numdims;
  int i;

  numdims = _NUMDIMS(R);
  for (i=0;i<numdims;i++) {
    _ARR_STR(E,i)=_REG_STRIDE(R,i);
    _ARR_OFF(E,i)=_REG_MYLO(R,i);
  }
  _ARR_ELEMSIZE(E) = elemsize;
  _ARR_BLK(E,numdims-1) = _ARR_ELEMSIZE(E);
  for (i=numdims-2;i>=0;i--) {
    _ARR_BLK(E,i) = _ARR_BLK(E,i+1)*(_REG_MYHI(R,i+1)-_REG_MYLO(R,i+1)+1);
  }
  E->numdims = numdims;
  E->decl_regptr=R;
  E->size=(_ARR_BLK(E,0)*(_REG_MYHI(R,0)-_REG_MYLO(R,0)+1));
  for (i=0;i<numdims;i++) {
    if (_FLOODED_DIM(R,i) || _REG_MYHI(R,i) == _REG_MYLO(R,i)) {
      _ARR_BLK(E,i) = 0;
    }
  }
  E->data=(char *)_zmalloc(E->size,"Temp Flood Array");
  E->origin = E->data;
  for (i=0;i<numdims;i++) {
    E->origin = ((char *)(E->origin)) + (int)((-_REG_MYLO(R,i))*_ARR_BLK(E,i));
  }
  E->Writefn = NULL;
  E->Readfn  = NULL;  
}


void _DestroyTempFloodEnsemble(_array E) {
  _zfree(E->data,"Temp Flood Array");
}


void _CreateFloodRegion(_region_fnc DR,_region dstreg,_region srcreg,
			int numdims) {
  int i;
  _PRIV_TID_DECL;

  _NUMDIMS(DR) = _NUMDIMS(dstreg);
  _REG_DIST(DR) = _REG_DIST(dstreg);
  _REG_ARRLIST(DR) = NULL;

  for (i=0;i<numdims;i++) {
    if (!_FLOODED_DIM(dstreg,i) &&
	_REG_GLOB_LO(dstreg,i) == _REG_GLOB_LO(srcreg,i) &&
	_REG_GLOB_HI(dstreg,i) == _REG_GLOB_HI(srcreg,i)) {
      _INHERIT_GLOB_BOUNDS(DR,dstreg,i);
    } else {
      _SET_GLOB_BOUNDS_FLOOD(DR,i,0,0,_DIM_FLOODED);
    }
  }
  _SPS_REGION(DR) = 0;
  _REG_SET_SETUP_ON(DR);
  _InitRegion(DR, 0);
}


int _Flood(_region dstreg,_region srcreg,_array dstens,_array srcens,
	   unsigned int elemsize,int commid) {
  int numdims;
  int i;
  int dstfloodable = 1;
  _reg_info DRinfo;
  _region_fnc DR = &DRinfo;
  _arr_info FEinfo;
  _array_fnc FE = &FEinfo;
  int j[_MAXRANK];

  numdims = _NUMDIMS(dstreg);
  for (i=0;i<numdims;i++) {
    if (!_FLOODED_DIM(dstreg,i) && 
	(_REG_GLOB_LO(dstreg,i) != _REG_GLOB_LO(srcreg,i) ||
	 _REG_GLOB_HI(dstreg,i) != _REG_GLOB_HI(srcreg,i))) {
      if (!_FLOODED_DIM(srcreg,i) && 
	  _REG_GLOB_LO(srcreg,i) != _REG_GLOB_HI(srcreg,i)) {
	return i+1;
      }
      dstfloodable = 0;
    }
  }
  
  if (dstfloodable) {
    _FloodGuts(dstreg,srcreg,dstens,srcens,elemsize,commid);
    return 0;
  }

  _CreateFloodRegion(DR,dstreg,srcreg,numdims);

  _SetupTempFloodEnsemble(FE,DR,elemsize);

  _FloodGuts(DR,srcreg,FE,srcens,elemsize,commid);

  if (_REG_I_OWN(dstreg)) {

    for (i=0;i<numdims;i++) {
      j[i] = _REG_MYLO(dstreg,i);
    }

    while (1) {
      memcpy(_GenAccess(dstens,j),_GenAccess(FE,j),elemsize);
    
      j[numdims-1]++;
      for (i = numdims-1;i>0;i--) {
	if (j[i] > _REG_MYHI(dstreg,i)) {
	  j[i-1]++;
	  j[i] = _REG_MYLO(dstreg,i);
	} else {
	  break;
	}
      }
      if (j[0] > _REG_MYHI(dstreg,0)) {
	break;
      }
    }
  }

  _DestroyTempFloodEnsemble(FE);

  return 0;
}
