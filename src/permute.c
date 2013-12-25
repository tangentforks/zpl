/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"

/*#define _MPI_PERMUTE_DEBUG*/

_permmap** _pm;
int _numpermmaps;

void _MI_PERM_Init(const int numpermmaps) {
  int i;
  _numpermmaps = numpermmaps;
  _pm = (_permmap**)_zmalloc(_numpermmaps * sizeof(_permmap*), "permute map array");
  for (i = 0; i < _numpermmaps; i++) {
    _pm[i] = NULL;
  }
}

void _PERM_DeleteMaps(void) {
  int i;
  for (i = 0; i < _numpermmaps; i++) {
    _CALL_PERM_MD(_pm[i]);
  }
}

/** determine whether an istream is straight, and thus allow the remap
    direct optimization
**/
int _PERM_StraightIStream(const int * const restrict is, const int streams, const int levels, const int * const restrict bs) {
  int i, j;
  int zvec[_MAXRANK];

  if (!is[2]) {
    return 0;
  }
  if (is[0] != 3 + 2 * streams + levels + streams * levels) {
    return 0;
  }
  for (i = 0; i < streams; i++) {
    zvec[i] = 0;
  }
  for (j = 0; j < levels; j++) {
    for (i = 0; i < streams; i++) {
      if (bs[i] && bs[streams-j-1]) {
	if (is[3 + streams + (streams + 1) * j + streams] > 1) {
	  if (i != streams - j - 1) {
	    if (zvec[i] != is[3 + streams + (streams + 1) * j + i]) {
	      return 0;
	    }
	  }
	}
      }
    }
    zvec[streams - j - 1] = 0 - (is[3 + streams + (streams + 1) * j + streams] - 1) * is[3 + streams + (streams + 1) * j + streams - j - 1];
  }
  return 1;
}

void _PERM_CleanIndices(int * const restrict lsize,
			int * const restrict rsize,
			int * restrict * const restrict lind,
			int * restrict * const restrict rind,
			int ** lindbase,
			int ** rindbase) {
  int i, j;
  int tsize, toff;

  tsize = 0;
  for (i = 0; i < _PROCESSORS; i++) {
    tsize += lsize[i];
  }
  *lindbase = (int *)_zmalloc(tsize * sizeof(int), "perm ir lind");
  toff = 0;
  for (i = 0; i < _PROCESSORS; i++) {
    if (lsize[i]) {
      int * const tbase = &((*lindbase)[toff]);
      int * restrict tnew = tbase;
      int * restrict told = lind[i];
      
      for (j = 0; j < lsize[i]; j++) {
	*tnew++ = *told++;
      }
      toff += lsize[i];
      _zfree(lind[i], "perm ir istream");
      lind[i] = tbase;
    }
    else {
      lind[i] = NULL;
    }
  }
  tsize = 0;
  for (i = 0; i < _PROCESSORS; i++) {
    tsize += rsize[i];
  }
  *rindbase = (int *)_zmalloc(tsize * sizeof(int), "perm ir rind");
  toff = 0;
  for (i = 0; i < _PROCESSORS; i++) {
    if (rsize[i]) {
      rind[i] = &((*rindbase)[toff]);
      rind[i][0] = rsize[i];
      toff += rsize[i];
    }
    else {
      rind[i] = NULL;
    }
  }
}

void _PERM_INFinish(_permmap* const restrict pm,
		    const int scatter,
		    const int eltsize,
		    _array dst,
		    _array src,
		    _region reg,
		    const int prilevels,
		    const int dstsrcconflict,
		    const int dstdead,
		    const int srcdead,
		    const int dstdirect,
		    const int srcdirect) {
  int optvec = 0;
  int i;

  if (!scatter) {
    int nogosrc = 0;
    int nogodst = 0;
    if (srcdirect) {
      char* last = 0;
      for (i = 0; i < _PROCESSORS; i++) {
	if (pm->rind[i]) {
	  if (!(_PERM_StraightIStream(pm->rind[i], pm->dims, prilevels, src->blocksize))) {
	    nogosrc = 1;
	    break;
	  }
	}
      }
    }
    if (dstdirect && !nogodst) {
      optvec |= _PERM_DSTDIRECT;
    }
    if (srcdirect && !nogosrc) {
      optvec |= _PERM_SRCDIRECT;
    }
  }
  pm->optsvector = optvec;

  {
    int tlcnt;
    int trcnt;
    
    tlcnt = 0;
    trcnt = 0;
    for (i=0; i<_PROCESSORS; i++) {
      tlcnt += _PERM_LCNT(pm, i);
      trcnt += _PERM_RCNT(pm, i);
    }
    pm->tlcnt = tlcnt;
    pm->trcnt = trcnt;
  }
}

void _FastComputeRemapMap(_permmap* const restrict pm, int scatter, _region reg, _distribution dist, int* _index_maps) {
  int regrank = _NUMDIMS(reg);

  if (scatter) {
    _RT_ANY_FATAL0("Scatter fast remaps not supported");
  }
  printf("_FastComputeRemapMap\n");


}

void _PERM_SetupDirect(const _permmap* const pm,
		       _permdata* const pd,
		       const int scatter,
		       const int eltsize,
		       _array dst,
		       _array src,
		       _region reg,
		       const int prilevels,
		       const int dstsrcconflict,
		       const int dstdead,
		       const int srcdead,
		       const int dstdirect,
		       const int srcdirect) {
  char ** const ldata = pd->ldata;
  char ** const rdata = pd->rdata;
  const int tlcnt = pm->tlcnt;
  const int trcnt = pm->trcnt;
  const int tldcnt = scatter ? tlcnt : trcnt;
  const int trdcnt = scatter ? trcnt : tlcnt;
  int i, j;
  int optvec = 0;

#ifdef _MPI_PERMUTE_DEBUG
  sleep(_INDEX);
  printf("PROCESSOR %d\n", _INDEX);
  printf("  PROCMAP: size = %d, # elts = %d, encoded = %d :: ", pm->procmap[0], pm->procmap[1], pm->procmap[2]);
  for (j = 3; j < pm->procmap[0]; j++) {
    printf("%d ", pm->procmap[j]);
  }
  printf("\n");
  for (i = 0; i < _PROCESSORS; i++) {
    if (pm->rind[i] != 0) {
      printf("  REMOTE INDICES %d: ", i);
      printf("size = %d, # elts = %d, encoded = %d :: ", pm->rind[i][0], pm->rind[i][1], pm->rind[i][2]);
      for (j = 3; j < pm->rind[i][0]; j++) {
	printf("%d ", pm->rind[i][j]);
      }
      printf("\n");
    }
  }
  printf("\n");
#endif

  if (scatter) {
    if (!dstsrcconflict && dst && (dstdead || (!scatter && reg == _ARR_DECL_REG(dst))) && _ARR_DATA_SIZE(dst) >= tldcnt * eltsize) {
      pd->lbase = ldata[0] = _ARR_DATA(dst);
      if (_QueryVerboseRemap() && _INDEX == 0) {
	printf("REMAP OPT= NO LOCAL BUCKETS\n");
      }
    }
    else {
      pd->lbase = ldata[0] = (char*)_zmalloc(tldcnt*eltsize, "perm ds ldata");
      optvec |= _PERM_SRCBUCKET;
    }
    for (i=1;i<_PROCESSORS;i++) {
      ldata[i] = ldata[i-1] + (scatter ? _PERM_LCNT(pm, i-1) : _PERM_RCNT(pm, i-1)) * eltsize;
    }
    if (!dstsrcconflict && src && srcdead && _ARR_DATA_SIZE(src) >= trdcnt * eltsize) {
      pd->rbase = rdata[0] = _ARR_DATA(src);
      if (_QueryVerboseRemap() && _INDEX == 0) {
	printf("REMAP OPT= NO REMOTE BUCKETS\n");
      }
    }
    else {
      pd->rbase = rdata[0] = (char*)_zmalloc(trdcnt*eltsize, "perm ds rdata");
      optvec |= _PERM_DSTBUCKET;
    }
    for (i=1;i<_PROCESSORS;i++) {
      rdata[i] = rdata[i-1] + (scatter ? _PERM_RCNT(pm, i-1) : _PERM_LCNT(pm, i-1)) * eltsize;
    }
  }
  else {
    int nogosrc = 0;
    int nogodst = 0;
    if (pm->optsvector & _PERM_SRCDIRECT) {
      char* last = 0;
      for (i = 0; i < _PROCESSORS; i++) {
	if (pm->rind[i]) {
#ifdef _MPI_PERMUTE_DEBUG
	  printf("testing %d --> straight   = %d\n", i, _PERM_StraightIStream(pm->rind[i], pm->dims, prilevels, src->blocksize));
	  printf("           --> l - f      = %d, %d - %d, %d\n", pm->rind[i][pm->rind[i][0]-pm->dims], pm->rind[i][pm->rind[i][0]-pm->dims+1], pm->rind[i][3], pm->rind[i][4]);
	  printf("           --> l - f      = %d\n", (char*)_GenAccess(src, &(pm->rind[i][pm->rind[i][0]-pm->dims])) - (char*)_GenAccess(src, &(pm->rind[i][3])));
	  printf("           --> elts-1 *sz = %d\n", eltsize * (pm->rind[i][1] - 1));
#endif
	  if (!((char*)_GenAccess(src, &(pm->rind[i][pm->rind[i][0]-pm->dims])) - (char*)_GenAccess(src, &(pm->rind[i][3])) == eltsize * (pm->rind[i][1] - 1))) {
	    nogosrc = 1;
	    break;
	  }
	  else {
	    ldata[i] = (char*)_GenAccess(src, &(pm->rind[i][3]));
	    if (!last) {
	      for (j = 0; j < i; j++) {
		ldata[j] = last;
	      }
	      last = ldata[i];
	      pd->lbase = last;
	    }
	    else if (ldata[i] != last + (eltsize * pm->rind[i][1])) {
	      nogosrc = 1;
	      break;
	    }
	    else {
	      last = ldata[i];
	    }
	  }
	}
	else {
	  ldata[i] = last;
	}
      }
    }
    if ((pm->optsvector & _PERM_DSTDIRECT)) {
      char* last = 0;
      for (i = 0; i < _PROCESSORS; i++) {
	if (pm->lind[i]) {
#ifdef _MPI_PERMUTE_DEBUG
	  printf("testing %d --> l - f      = %d, %d - %d, %d\n", i, pm->lf[i*2*pm->dims+pm->dims + 0], pm->lf[i*2*pm->dims+pm->dims + 1], pm->lf[i*2*pm->dims], pm->lf[i*2*pm->dims + 1]);
	  printf("           --> l - f      = %d\n", (char*)_GenAccess(dst, &(pm->lf[i*2*pm->dims+pm->dims])) - (char*)_GenAccess(dst, &(pm->lf[i*2*pm->dims])));
	  printf("           --> elts-1 *sz = %d\n", eltsize * (pm->lind[i][1] - 1));
#endif
	  if (!((char*)_GenAccess(dst, &(pm->lf[i*2*pm->dims+pm->dims])) - (char*)_GenAccess(dst, &(pm->lf[i*2*pm->dims])) == eltsize * (pm->lind[i][1] - 1))) {
	    nogodst = 1;
	    break;
	  }
	  else {
	    rdata[i] = (char*)_GenAccess(dst, &(pm->lf[i*2*pm->dims]));
	    if (!last) {
	      for (j = 0; j < i; j++) {
		rdata[j] = last;
	      }
	      last = rdata[i];
	      pd->rbase = last;
	    }
	    else if (rdata[i] != last + (eltsize * pm->lind[i][1])) {
	      nogodst = 1;
	      break;
	    }
	    else {
	      last = rdata[i];
	    }
	  }
	}
	else {
	  rdata[i] = last;
	}
      }
    }
    if ((pm->optsvector & _PERM_SRCDIRECT) && !nogosrc) {
      if (_QueryVerboseRemap() && _INDEX == 0) {
	printf("REMAP OPT= SRC DIRECT\n");
      }
      optvec |= _PERM_SRCDIRECT;
    }
    if ((pm->optsvector & _PERM_DSTDIRECT) && !nogodst) {
      if (_QueryVerboseRemap() && _INDEX == 0) {
	printf("REMAP OPT= DST DIRECT\n");
      }
      optvec |= _PERM_DSTDIRECT;
    }
    if (!(optvec & _PERM_SRCDIRECT)) {
      pd->lbase = NULL;
      if (!dstsrcconflict && dst && (dstdead || reg == _ARR_DECL_REG(dst)) && _ARR_DATA_SIZE(dst) >= tldcnt * eltsize) {
	if (!(optvec & _PERM_DSTDIRECT)) {
	  pd->lbase = ldata[0] = _ARR_DATA(dst);
	  if (_QueryVerboseRemap() && _INDEX == 0) {
	    printf("REMAP OPT= NO LOCAL BUCKETS\n");
	  }
	}
	else if (src && srcdead && _ARR_DATA_SIZE(dst) == _ARR_DATA_SIZE(src)) {
	  char* thread = pd->lbase;
	  pd->lbase = ldata[0] = _ARR_DATA(dst);
	  pd->rbase += _ARR_DATA(src) - _ARR_DATA(dst);
	  for (i = 0; i < _PROCESSORS; i++) {
	    rdata[i] += _ARR_DATA(src) - _ARR_DATA(dst);
	  }
	  if (_QueryVerboseRemap() && _INDEX == 0) {
	    printf("REMAP OPT= NO LOCAL BUCKETS\n");
	  }
	  optvec |= _PERM_UNODIRECT;
	}
      }
      if (!(pd->lbase)) {
	pd->lbase = ldata[0] = (char*)_zmalloc(tldcnt*eltsize, "perm ds ldata");
	optvec |= _PERM_SRCBUCKET;
      }
      for (i=1;i<_PROCESSORS;i++) {
	ldata[i] = ldata[i-1] + _PERM_RCNT(pm, i-1) * eltsize;
      }
    }
    if (!(optvec & _PERM_DSTDIRECT)) {
      pd->rbase = 0;
      if (!dstsrcconflict && src && srcdead && _ARR_DATA_SIZE(src) >= trdcnt * eltsize) {
	if (!(optvec & _PERM_SRCDIRECT)) {
	  pd->rbase = rdata[0] = _ARR_DATA(src);
	  if (_QueryVerboseRemap() && _INDEX == 0) {
	    printf("REMAP OPT= NO REMOTE BUCKETS\n");
	  }
	}
	else if (dst && (dstdead || reg == _ARR_DECL_REG(dst)) && _ARR_DATA_SIZE(dst) == _ARR_DATA_SIZE(src)) {
	  pd->rbase = rdata[0] = _ARR_DATA(dst);
	  if (_QueryVerboseRemap() && _INDEX == 0) {
	    printf("REMAP OPT= NO REMOTE BUCKETS\n");
	  }
	  optvec |= _PERM_UNODIRECT;
	}
      }
      if (!(pd->rbase)) {
	pd->rbase = rdata[0] = (char*)_zmalloc(trdcnt*eltsize, "perm ds rdata");
	optvec |= _PERM_DSTBUCKET;
      }
      for (i=1;i<_PROCESSORS;i++) {
	rdata[i] = rdata[i-1] + _PERM_LCNT(pm, i-1) * eltsize;
      }
    }
  }
  pd->eltsize = eltsize;
  pd->optsvector = optvec;
}
