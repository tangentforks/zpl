/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md_zinc.h"
#include "ensemble.h"
#include "ironman.h"
#include "ironman_utils.h"
#include "mloop.h"
#include "mloop_sparse.h"
#include "priv_access.h"
#include "region.h"
#include "zerror.h"
#include "zlib.h"
#include "zmacros.h"
#include "zplglobal.h"

#ifdef PRINT_COMPS
#define PRINT_COMP printf
#else
#define PRINT_COMP(a,b,c,d,e,f,g)
#endif

#define PRINT_DIR(d,nd) \
  { \
    int k; \
    printf("["); \
    for (k=0;k<nd;k++) { \
      if (k) { \
        printf(","); \
      } \
      printf("%2d",(d)[k]); \
    } \
    printf("]"); \
  }

#define COMM_NON 0
#define COMM_REM 1
#define COMM_LCL 2 /* these numeric values are actually relied upon */

#define DIRPTR(dir,dim)      (dirptr[(dir)*_MAXDIM+(dim)])
#define DIRPTR_VECT(dir)     (&(dirptr[(dir)*_MAXDIM]))

#define ZPL_AT_COMMBANK(dir) (_PRIV_ACCESS(zpl_at_commbank)[dir])
#define AT_COMMBANK(cid,dir) (_PRIV_ACCESS(at_commbank)[cid*num_comm_dirs+dir])
#define IM_COMMBANK(cid,dir) (_PRIV_ACCESS(im_commbank)[cid*num_comm_dirs+dir])
#define IM_COMMBANK_VCT(cid) (&(_PRIV_ACCESS(im_commbank)[cid*num_comm_dirs]))
#define SPS_COMMBANK(cid)    (_PRIV_ACCESS(at_spsinfo)[cid])
#define AT_COMMPOOL(cid)     (_PRIV_ACCESS(at_commpool)[cid])
#define COMM_COUNT(cid)      (_PRIV_ACCESS(comm_count)[cid])
#define COMM_HANDLE(cid)     (_PRIV_ACCESS(commhandle)[cid])

static int num_comm_dirs;
static _atcomminfo* zpl_at_commbank;
static IM_commpair* im_commbank;
static comminfo* at_commbank;
static IM_comminfo** at_commpool;
static spscomminfo* at_spsinfo;
static commcount* comm_count;
static IM_info** commhandle;

static int numcomms;
static const int* enspercommid;
static int maxdirs;
static int maxwrapdirs;

void _InitAtComm(const int newnumcomms,const int newenspercommid[],
		 const int newmaxdirs, const int newmaxwrapdirs) {
  int maxens;
  int totens;
  int i, j;

  numcomms = newnumcomms;
  enspercommid = newenspercommid;
  maxdirs = newmaxdirs;
  maxwrapdirs = newmaxwrapdirs;

  num_comm_dirs = 1;
  for (i=0;i<_MAXDIM;i++) {
    num_comm_dirs *= 3;
  }
  num_comm_dirs--;

  maxens = 0;
  totens = 0;
  for (i=0;i<numcomms;i++) {
    if (enspercommid[i] > maxens) {
      maxens = enspercommid[i];
      totens += enspercommid[i];
    }
  }

  zpl_at_commbank =
    (_atcomminfo*)_zmalloc(num_comm_dirs*sizeof(_atcomminfo),"@-commbank");
  for (i=0; i<num_comm_dirs; i++) {
    ZPL_AT_COMMBANK(i).arrlist = 
      (_arrcomminfo*)_zmalloc(maxens*sizeof(_arrcomminfo),
				"@-commbank array list");
    for (j=0; j<maxens; j++) {
      ZPL_AT_COMMBANK(i).arrlist[j].dirlist =
	(_direction_pnc*)_zmalloc(maxdirs*sizeof(_direction_pnc),
				    "@-commank direction list");
    }
  }

  im_commbank = 
    (IM_commpair*)_zmalloc(numcomms*2*num_comm_dirs*sizeof(IM_commpair),
			     "Ironman commbank");
  at_commbank = 
    (comminfo*)_zmalloc(numcomms*num_comm_dirs*sizeof(comminfo),"@-commbank");
  at_commpool = 
    (IM_comminfo**)_zmalloc(numcomms*sizeof(IM_comminfo*),"@-commpool");
  at_spsinfo = 
    (spscomminfo*)_zmalloc(numcomms*sizeof(spscomminfo),"sparse comminfo");

  for (i=0;i<numcomms;i++) {
    for (j=0;j<2*num_comm_dirs;j++) {
      IM_COMMBANK(i,j).recvinfo = NULL;
      IM_COMMBANK(i,j).sendinfo = NULL;
    }
    for (j=0;j<num_comm_dirs;j++) {
      AT_COMMBANK(i,j).recvinfo = NULL;
      AT_COMMBANK(i,j).recvready = 0;
      AT_COMMBANK(i,j).sendinfo = NULL;
      AT_COMMBANK(i,j).sendready = 0;
    }

    AT_COMMPOOL(i) = 
      (IM_comminfo*)_zmalloc(2*num_comm_dirs*sizeof(IM_comminfo),
			       "@ commpool");
    for (j=0;j<2*num_comm_dirs;j++) {
      AT_COMMPOOL(i)[j].mbvect =
	(IM_memblock *)_zmalloc(enspercommid[i]*sizeof(IM_memblock),
				  "@ memory block vector");
    }

    SPS_COMMBANK(i).recvsps.numsps = 0;
    SPS_COMMBANK(i).recvsps.vect = 
      (spsinfo*)_zmalloc(totens*num_comm_dirs*sizeof(spsinfo),
			   "Sparse receiving vector");
    SPS_COMMBANK(i).sendsps.numsps = 0;
    SPS_COMMBANK(i).sendsps.vect = 
      (spsinfo*)_zmalloc(totens*num_comm_dirs*sizeof(spsinfo),
			   "Sparse sending vector");
  }

  comm_count = (commcount*)_zmalloc(numcomms*sizeof(commcount),
						  "Communication counts");
  for (i=0;i<numcomms;i++) {
    COMM_COUNT(i).remcomms = 0;
    COMM_COUNT(i).lclcomms = 0;
  }

  commhandle =
    (IM_info**)_zmalloc(numcomms*sizeof(IM_info*),"Communication handles");

  IM_Initialize(_INDEX,_PROCESSORS,numcomms,2*num_comm_dirs,maxens);
  _InitCommID();
}

void _InitAtCommPerGrid(_grid_fnc grid) {
  int i,j;
  _dir_def_dyn dirptrval;
  int dirnum;
  int distributed;
  int well_distributed;
  int null_dir;
  int num_useless_comm_dirs;
  int num_useful_comm_dirs;
  int num_useful_at_dirs;
  int *dirptr;

  null_dir = num_comm_dirs / 2;

  dirptr = (int*)_zmalloc((num_comm_dirs+1)*_MAXDIM*sizeof(int),
			  "@-comm direction buckets");

  num_useful_comm_dirs = 0;
  num_useless_comm_dirs = 0;
  num_useful_at_dirs = 0;

  for (i=0;i<_MAXDIM;i++) {
    dirptrval[i] = -1;
  }
  for (dirnum=0; dirnum<num_comm_dirs+1; dirnum++) {
    if (dirnum != null_dir) {  /* this is all 0's so skip it */
      distributed = 0;
      well_distributed = 1;
      for (j=0; j<_MAXDIM; j++) {
	if (dirptrval[j] != 0) {
	  if (_GRID_SIZE(grid, j) != 1) {
	    distributed = 1;
	  } else {
	    well_distributed = 0;
	  }
	}
      }
      if (distributed) {
	if (well_distributed) {
	  j = num_useful_at_dirs;
	  num_useful_at_dirs++;
	} else {
	  j = -1;
	}
      } else {
	num_useless_comm_dirs++;
	j = num_comm_dirs-num_useless_comm_dirs;
      }

      if (j != -1) {
	for (i=0;i<_MAXDIM;i++) {
	  DIRPTR(j,i) = dirptrval[i];
	}
      }
    }

    /* bump everything */
    i = _MAXDIM;
    do {
      i--;
      dirptrval[i]++;
      if (dirptrval[i] == 2) {
	dirptrval[i] = -1;
      }
    } while (dirptrval[i] == -1 && i);
  }
  for (i=0;i<_MAXDIM;i++) {
    dirptrval[i] = -1;
  }
  num_useful_comm_dirs = num_useful_at_dirs;
  for (dirnum=0; dirnum<num_comm_dirs+1; dirnum++) {
    if (dirnum != null_dir) {  /* this is all 0's so skip it */
      distributed = 0;
      well_distributed = 1;
      for (j=0; j<_MAXDIM; j++) {
	if (dirptrval[j] != 0) {
	  if (_GRID_SIZE(grid, j) != 1) {
	    distributed = 1;
	  } else {
	    well_distributed = 0;
	  }
	}
      }
      if (distributed && !well_distributed) {
	j = num_useful_comm_dirs;
	num_useful_comm_dirs++;
      } else {
	j = -1;
      }

      if (j != -1) {
	for (i=0;i<_MAXDIM;i++) {
	  DIRPTR(j,i) = dirptrval[i];
	}
      }
    }

    /* bump everything */
    i = _MAXDIM;
    do {
      i--;
      dirptrval[i]++;
      if (dirptrval[i] == 2) {
	dirptrval[i] = -1;
      }
    } while (dirptrval[i] == -1 && i);
  }

  if (maxwrapdirs == 0) {
    num_useful_comm_dirs = num_useful_at_dirs;
  }

  /*
  if (_INDEX == 0) {
    for (dirnum=0; dirnum<num_comm_dirs; dirnum++) {
      if (dirnum == num_useful_comm_dirs) {
	printf("-------\n");
      }
      if (dirnum == num_useful_at_dirs) {
	printf("=======\n");
      }
      PRINT_DIR(&(DIRPTR(dirnum,0)),_MAXDIM);
      printf("\n");
    }
  }
  */

  if (_GRID_DIRPTR(grid)) {
    _zfree(_GRID_DIRPTR(grid), "@-comm direction buckets");
  }
  _GRID_NUM_USEFUL_COMM_DIRS(grid) = num_useful_comm_dirs;
  _GRID_NUM_USEFUL_AT_DIRS(grid) = num_useful_at_dirs;
  _GRID_DIRPTR(grid) = dirptr;
}


#ifndef _AtComm_SR
static void PackData(const spsinfo* const info) {
  char* buffptr;
  const int numdims = _NUMDIMS(info->arr);
  _region reg = info->reg;
  const char* const Base = _ARR_ORIGIN(info->arr);
  const int elemdist = _ARR_BLK(info->arr,numdims-1);
  int spsindex;
  int regwalker[_MAXRANK];

  buffptr = info->buffer;
  _SPS_MLOOP_UP_V(reg,regwalker,numdims,info->lo,info->hi) {
    spsindex = _SPSNODE_ID(reg,_SPS_WLK_V(regwalker,numdims-1));
    memcpy(buffptr,Base + (spsindex*elemdist),info->elemsize);
    buffptr += info->elemsize;
    _ADVANCE_SPS_MLOOP_UP_V(reg,regwalker,numdims,info->lo,info->hi);
  }
  _END_SPS_MLOOP_UP_V();
}


static void PackAllSparse(const int numblocks,const spsinfo* const info) {
  int i;

  for (i=0;i<numblocks;i++) {
    PackData(&(info[i]));
  }
}
#endif


#ifndef _AtComm_DN
static void UnPackData(const spsinfo* const info) {
  char* buffptr;
  const int numdims = _NUMDIMS(info->arr);
  _region reg = info->reg;
  char* const Base = _ARR_ORIGIN(info->arr);
  const int elemdist = _ARR_BLK(info->arr,numdims-1);
  int spsindex;
  int regwalker[_MAXRANK];

  buffptr = info->buffer;
  _SPS_MLOOP_UP_V(reg,regwalker,numdims,info->lo,info->hi) {
    spsindex = _SPSNODE_ID(reg,_SPS_WLK_V(regwalker,numdims-1));
    memcpy(Base + (spsindex*elemdist),buffptr,info->elemsize);
    buffptr += info->elemsize;
    _ADVANCE_SPS_MLOOP_UP_V(reg,regwalker,numdims,info->lo,info->hi);
  }
  _END_SPS_MLOOP_UP_V();
}


static void UnPackAllSparse(const int numblocks,const spsinfo* const info) {
  int i;

  for (i=0;i<numblocks;i++) {
    UnPackData(&(info[i]));
  }
}
#endif


static void FreeAllSparse(const int numblocks,const spsinfo* const info) {
  int i;
  for (i=0;i<numblocks;i++) {
    _zfree(info[i].buffer,"sparse buffer");
  }
}


static int _SetupSparseMemInfo(IM_memblock* const memblock,_region reg,
			       _array ens,const int numdims,
			       const _vector lo,const _vector hi,
			       const unsigned int elemsize,
			       spsinfo* const spsinfo) {
  _vector loc_lo;
  _vector loc_hi;
  _region arr_reg = _ARR_DECL_REG(ens);
  int i;
  int numelems;
  char* buffer;
  int regwalker[_MAXRANK];

  for (i=0;i<numdims;i++) {
    loc_lo[i] = _SNAP_UP(lo[i],arr_reg,i);
    loc_hi[i] = _SNAP_DN(hi[i],arr_reg,i);
    if (loc_lo[i] > loc_hi[i]) {
      return 0;
    }
  }
  /*
  printf("[%d] [%d..%d,%d..%d]\n",_INDEX,loc_lo[0],loc_hi[0],
	 loc_lo[1],loc_hi[1]);
  */

  numelems = 0;
  _SPS_MLOOP_UP_V(arr_reg,regwalker,numdims,loc_lo,loc_hi) {
    /*
    printf("[%d] [%d,%d]\n",_INDEX,
	   _SPSNODE_INDEX(reg,_SPS_WLK_V(regwalker,0),0),
	   _SPSNODE_INDEX(reg,_SPS_WLK_V(regwalker,1),1));
    */
    numelems++;
    _ADVANCE_SPS_MLOOP_UP_V(arr_reg,regwalker,numdims,loc_lo,loc_hi);
  }
  _END_SPS_MLOOP_UP_V();
  if (numelems == 0) {
    return 0;
  }

  buffer = (char*)_zmalloc(elemsize*numelems,"sparse @-communication buffer");

  memblock->baseptr = buffer;
  memblock->elemsize = elemsize*numelems;
  memblock->diminfo[0].numelems = 1;
  memblock->diminfo[0].stride = 0;
  memblock->numdims = 1;

  spsinfo->reg = arr_reg;
  spsinfo->arr = ens;
  for (i=0;i<numdims;i++) {
    spsinfo->lo[i] = loc_lo[i];
    spsinfo->hi[i] = loc_hi[i];
  }
  spsinfo->elemsize = elemsize;
  spsinfo->buffer = buffer;

  return 1;
}


int _SetupDenseMemInfo(IM_memblock* const memblock,_region reg,_array ens,
		       const int numensdims,const _vector lo,const _vector hi,
		       const unsigned int elemsize) {
  _vector marker;
  int numelems;
  int w;
  int r;
  int stride;
  _vector loc_lo;
  _vector loc_hi;
  _region_pnc arr_reg;

  /* if referring to a coarse array on a fine region (using a mask or
     shattered control flow to filter what's actually being referred to)
     it's possible to come up with a rectangle that doesn't actually 
     match the array's declaration;
     Unfortunately, can't do this snapping outside of here because different
     arrays will have different stridednesses and they may be combined
     within one comm. */

  arr_reg = _ARR_DECL_REG(ens);
  for (r=0;r<numensdims;r++) {
    loc_lo[r] = _SNAP_UP(lo[r],arr_reg,r);
    loc_hi[r] = _SNAP_DN(hi[r],arr_reg,r);
    if (loc_lo[r] > loc_hi[r]) {
      return 0;
    }
  }

  memblock->baseptr = _GenAccess(ens,loc_lo);
  memblock->elemsize = elemsize;
      
  for (w=0;w<numensdims;w++) {
    marker[w] = loc_lo[w];
  }

  for (r=numensdims-1,w=0;r>=0;r--,w++) {
    /*    Although this seems to make sense, it doesn't work when north,
	  _x_ne and _x_nw are combined when the directions are stride 2
	  and the array is stride 1:
	  
	  stride = _REG_STRIDE(reg,r); 

	  So instead, use... (which may be overkill, but is safe)
    */

    stride = _ARR_STR(ens,r);


    numelems = (loc_hi[r]-loc_lo[r])/stride+1;
    memblock->diminfo[w].numelems = numelems;
    if (numelems == 1) {
      if (r!=0 || w!=0) {
	w--;
      }
    } else {
      marker[r] += stride;
      memblock->diminfo[w].stride = _GenAccessDistance(ens,loc_lo,marker);
      marker[r] -= stride;
    }
  }
  memblock->numdims = w;
  return 1;
}


static int _ComputeCommFlatDim(_region reg,const int dim,const int comp,
			       int* const lo,int* const hi) {
  *lo = _REG_MYLO(reg,dim)+comp;
  *hi = _REG_MYHI(reg,dim)+comp;

  return 0;
}


static int _ComputeCommZeroComp(_region reg,const int dim,int* const lo,
				int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  loc_lo = _REG_MYLO(reg,dim);
  loc_hi = _REG_MYHI(reg,dim);
  if (loc_lo > loc_hi) {
    loc_proc = -1;
  } else {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _GRID_LOC(grid, dim);
  }
  return loc_proc;
}


static int _ComputeCommZeroCompPosRoot(_region reg,const int dim,const int comp,
				       int* const lo,int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  loc_lo = _SNAP_UP(_DIST_MYLO(dist, dim),reg,dim);
  loc_lo = max(loc_lo,_REG_GLOB_LO(reg,dim))+comp;
  loc_hi = _SNAP_DN(_DIST_MYHI(dist, dim),reg,dim);
  loc_hi = min(loc_hi,_REG_GLOB_HI(reg,dim))+comp;
  while (loc_hi > _DIST_MYHI(dist, dim)) {
    loc_hi -= _REG_STRIDE(reg,dim);
  }
  if ((loc_lo <= loc_hi) && (_DIST_MYLO(dist, dim)+comp <= loc_hi)) {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _GRID_LOC(grid, dim);
  } else {
    loc_proc = -1;
  }
  return loc_proc;
}


static int _ComputeCommZeroCompNegRoot(_region reg,const int dim,const int comp,
				       int* const lo,int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  loc_lo = _SNAP_UP(_DIST_MYLO(dist, dim),reg,dim);
  loc_lo = max(loc_lo,_REG_GLOB_LO(reg,dim))+comp;
  while (loc_lo < _DIST_MYLO(dist, dim)) {
    loc_lo += _REG_STRIDE(reg,dim);
  }
  loc_hi = _SNAP_DN(_DIST_MYHI(dist, dim),reg,dim);
  loc_hi = min(loc_hi,_REG_GLOB_HI(reg,dim))+comp;
  if ((loc_lo <= loc_hi) && (loc_lo <= _DIST_MYHI(dist, dim)+comp)) {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _GRID_LOC(grid, dim);
  } else {
    loc_proc = -1;
  }
  return loc_proc;
}


static int _ComputeSendPosComp(_region reg,const int dim,const int comp,
			       int* const lo,int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  int snapped_lo;
  int next_lo;
  _distribution dist = _REG_DIST(reg);

  snapped_lo = _SNAP_DN(_DIST_MYLO(dist, dim)-1,reg,dim);
  next_lo = snapped_lo+min(_REG_STRIDE(reg,dim),comp);
  if (next_lo < _DIST_MYLO(dist, dim)) {
    next_lo += _REG_STRIDE(reg,dim);
  }
  loc_lo = max(next_lo,_REG_GLOB_LO(reg,dim)+comp);
  loc_hi = min(snapped_lo,_REG_GLOB_HI(reg,dim))+comp;
  if ((loc_lo <= loc_hi) && (loc_lo <= _DIST_MYHI(dist, dim))) {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _DIST_TO_PROC(unknown, dist, loc_lo-comp,dim);
    if (loc_proc < 0) {
      loc_proc = 0;
    }
  } else {
    loc_proc = -1;
  }
  return loc_proc;
}


static int _ComputeRecvPosComp(_region reg,const int dim,const int comp,
			       int* const lo,int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  int snapped_hi;
  int next_hi;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  snapped_hi = _SNAP_DN(_DIST_MYHI(dist, dim),reg,dim);
  next_hi = snapped_hi+min(_REG_STRIDE(reg,dim),comp);
  if (next_hi <= _DIST_MYHI(dist, dim)) {
    next_hi += _REG_STRIDE(reg,dim);
  }
  loc_lo = max(next_hi,_REG_GLOB_LO(reg,dim)+comp);
  loc_hi = min(snapped_hi,_REG_GLOB_HI(reg,dim))+comp;
  if ((loc_lo <= loc_hi) && (_DIST_MYLO(dist, dim)+comp<=loc_hi)) {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _DIST_TO_PROC(unknown, dist, loc_lo,dim);
    if (loc_proc >= _GRID_SIZE(grid, dim)) {
      loc_proc = _GRID_SIZE(grid, dim)-1;
    }
  } else {
    loc_proc = -1;
  }
  return loc_proc;
}


static int _ComputeSendNegComp(_region reg,const int dim,const int comp,
			       int* const lo,int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  int snapped_hi;
  int next_hi;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  
  snapped_hi = _SNAP_UP(_DIST_MYHI(dist, dim)+1,reg,dim);
  next_hi = snapped_hi+max(-_REG_STRIDE(reg,dim),comp);
  if (next_hi > _DIST_MYHI(dist, dim)) {
    next_hi -= _REG_STRIDE(reg,dim);
  }
  loc_lo = max(snapped_hi,_REG_GLOB_LO(reg,dim))+comp;
  loc_hi = min(next_hi,_REG_GLOB_HI(reg,dim)+comp);
  if ((loc_lo <= loc_hi) && (_DIST_MYLO(dist, dim) <= loc_hi)) {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _DIST_TO_PROC(unknown, dist, loc_hi-comp,dim);
    if (loc_proc >= _GRID_SIZE(grid, dim)) {
      loc_proc = _GRID_SIZE(grid, dim)-1;
    }
  } else {
    loc_proc = -1;
  }
  return loc_proc;
}


static int _ComputeRecvNegComp(_region reg,const int dim,const int comp,
			       int* const lo,int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  int snapped_lo;
  int next_lo;
  _distribution dist = _REG_DIST(reg);

  snapped_lo = _SNAP_UP(_DIST_MYLO(dist, dim),reg,dim);
  next_lo = snapped_lo+max(-_REG_STRIDE(reg,dim),comp);
  if (next_lo >= _DIST_MYLO(dist, dim)) {
    next_lo -= _REG_STRIDE(reg,dim);
  }
  loc_lo = max(snapped_lo,_REG_GLOB_LO(reg,dim))+comp;
  loc_hi = min(next_lo,_REG_GLOB_HI(reg,dim)+comp);
  if ((loc_lo <= loc_hi) && (loc_lo <= _DIST_MYHI(dist, dim)+comp)) {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _DIST_TO_PROC(unknown, dist, loc_hi,dim);
    if (loc_proc < 0) {
      loc_proc = 0;
    }
  } else {
    loc_proc = -1;
  }
  return loc_proc;
}


static int _ComputeCommZeroCompPosRootWrap(_region reg,const int dim,
					   const int comp,int* const lo,
					   int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  loc_lo = _SNAP_UP(_DIST_MYLO(dist, dim),reg,dim);
  loc_lo = max(loc_lo,_REG_GLOB_LO(reg,dim))+comp;
  loc_hi = _SNAP_DN(_DIST_MYHI(dist, dim),reg,dim);
  loc_hi = min(loc_hi,_REG_GLOB_HI(reg,dim))+comp;
  while (loc_hi > _REG_MYHI(reg,dim)) {
    loc_hi -= _REG_STRIDE(reg,dim);
  }
  if ((loc_lo <= loc_hi) && (_DIST_MYLO(dist, dim)+comp <= loc_hi)) {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _GRID_LOC(grid, dim);
  } else {
    loc_proc = -1;
  }
  return loc_proc;
}


static int _ComputeCommZeroCompNegRootWrap(_region reg,const int dim,
					   const int comp,int* const lo,
					   int* const hi) {
  int loc_lo;
  int loc_hi;
  int loc_proc;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  loc_lo = _SNAP_UP(_DIST_MYLO(dist, dim),reg,dim);
  loc_lo = max(loc_lo,_REG_GLOB_LO(reg,dim))+comp;
  while (loc_lo < _REG_MYLO(reg,dim)) {
    loc_lo += _REG_STRIDE(reg,dim);
  }
  loc_hi = _SNAP_DN(_DIST_MYHI(dist, dim),reg,dim);
  loc_hi = min(loc_hi,_REG_GLOB_HI(reg,dim))+comp;
  if ((loc_lo <= loc_hi) && (loc_lo <= _DIST_MYHI(dist, dim)+comp)) {
    *lo = loc_lo;
    *hi = loc_hi;
    loc_proc = _GRID_LOC(grid, dim);
  } else {
    loc_proc = -1;
  }
  return loc_proc;
}


static int _ComputeSendPosCompWrap(_region reg,_region arrreg,const int dim,
				   const int comp,int* const lo,int* const hi) {
  int rem_lo;
  int rem_hi;
  int mylo;
  int wraparound;
  int loc_lo;
  int loc_hi;
  int rem_proc = -1;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  rem_lo = _REG_MYLO(arrreg,dim)-comp;
  rem_hi = _REG_MYHI(arrreg,dim)-comp;
  if (rem_lo <= rem_hi) {
    if (_INV_BEG_OF_REG(dist, arrreg,dim)) {
      mylo = _REG_LO(arrreg,dim);
    } else {
      mylo = _DIST_MYLO(dist, dim);
    }
    if (rem_hi >= mylo) {
      rem_hi = mylo-1;
    }
    if (rem_lo <= rem_hi) {
      wraparound = _REG_HI(reg,dim)+comp - _REG_HI(arrreg,dim);
      if (wraparound < 0) {
	wraparound = 0;
      }
      if (wraparound && (rem_lo <= _REG_LO(arrreg,dim) - comp)) {
	rem_lo = _SNAP_UP_WRAP(_REG_LO(arrreg,dim)-comp,reg,dim,arrreg);
      } else if (wraparound && (rem_lo <= _REG_LO(arrreg,dim) - 1)) {
	rem_lo = _SNAP_UP_WRAP(rem_lo,reg,dim,arrreg);
      } else if (rem_lo < _REG_LO(reg,dim)) {
	rem_lo = _REG_LO(reg,dim);
      } else {
	rem_lo = _SNAP_UP(rem_lo,reg,dim);
      }
      if (rem_hi > _REG_HI(reg,dim)) {
	rem_hi = _REG_HI(reg,dim);
      } else if (rem_hi >= _REG_LO(reg,dim)) {
	rem_hi = _SNAP_DN(rem_hi,reg,dim);
      } else if (wraparound) {
	rem_hi = _SNAP_DN_WRAP(_REG_LO(arrreg,dim)-1,reg,dim,arrreg);
      }
      if (rem_lo <= rem_hi) {
	loc_lo = rem_lo+comp;
	loc_hi = rem_hi+comp;
	*lo = loc_lo;
	*hi = loc_hi;
	if (rem_hi < _REG_LO(arrreg,dim)) {
	  rem_lo += _REG_SPAN(arrreg,dim);
	  rem_hi += _REG_SPAN(arrreg,dim);
	}
	rem_proc = _DIST_TO_PROC(unknown, dist, rem_hi,dim);
	if (rem_proc >= _GRID_SIZE(grid, dim)) {
	  rem_proc = _GRID_SIZE(grid, dim)-1;
	} else if (rem_proc < 0) {
	  rem_proc = 0;
	}
      }
    }
  }
  PRINT_COMP("[%d] sending %d..%d to %d (to be recvd at %d..%d)\n",_INDEX,
	     loc_lo,loc_hi,rem_proc,rem_lo,rem_hi);
  return rem_proc;
}


static int _ComputeRecvPosCompWrap(_region reg,_region arrreg,const int dim,
				   const int comp,int* const lo,int* const hi) {
  int rem_lo;
  int rem_hi;
  int myhi;
  int rem_proc = -1;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  rem_lo = _REG_MYLO(reg,dim)+comp;
  rem_hi = _REG_MYHI(reg,dim)+comp;
  if (rem_lo <= rem_hi) {
    if (_INV_END_OF_REG(dist, arrreg,dim)) {
      myhi = _REG_HI(arrreg,dim);
    } else {
      myhi = _DIST_MYHI(dist, dim);
    }
    if (rem_lo <= myhi) {
      rem_lo = _SNAP_UP(myhi+1-comp,reg,dim)+comp;
    }
    if (rem_lo <= rem_hi) {
      *lo = rem_lo;
      *hi = rem_hi;
      if (rem_lo > _REG_HI(arrreg,dim)) {
	rem_lo -= _REG_SPAN(arrreg,dim);
	rem_hi -= _REG_SPAN(arrreg,dim);
      }
      rem_proc = _DIST_TO_PROC(unknown, dist, rem_hi,dim);
      if (rem_proc >= _GRID_SIZE(grid, dim)) {
	rem_proc = _GRID_SIZE(grid, dim)-1;
      } else if (rem_proc < 0) {
	rem_proc = 0;
      }
    }
  }
  PRINT_COMP("[%d] recving at %d..%d fm %d (to be sent fm %d..%d)\n",_INDEX,*lo,
	     *hi,rem_proc,rem_lo,rem_hi);
  return rem_proc;
}


static int _ComputeSendNegCompWrap(_region reg,_region arrreg,const int dim,
				   const int comp,int* const lo,int* const hi) {
  int rem_lo;
  int rem_hi;
  int myhi;
  int wraparound;
  int loc_lo;
  int loc_hi;
  int rem_proc = -1;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  rem_lo = _REG_MYLO(arrreg,dim)-comp;
  rem_hi = _REG_MYHI(arrreg,dim)-comp;
  if (rem_lo <= rem_hi) {
    if (_INV_END_OF_REG(dist, reg,dim)) {
      myhi = _REG_HI(reg,dim);
    } else {
      myhi = _DIST_MYHI(dist, dim);
    }
    if (rem_lo <= myhi) {
      rem_lo = myhi+1;
    }
    if (rem_lo <= rem_hi) {
      wraparound = _REG_LO(arrreg,dim) - (_REG_LO(reg,dim)+comp);
      if (wraparound < 0) {
	wraparound = 0;
      }
      if (rem_lo < _REG_LO(reg,dim)) {
	rem_lo = _REG_LO(reg,dim);
      } else if (rem_lo <= _REG_HI(reg,dim)) {
	rem_lo = _SNAP_UP(rem_lo,reg,dim);
      } else if (wraparound) {
	rem_lo = _SNAP_UP_WRAP(_REG_HI(arrreg,dim)+1,reg,dim,arrreg);
      }
      if (wraparound && (rem_hi >= _REG_HI(arrreg,dim) - comp)) {
	rem_hi = _SNAP_DN_WRAP(_REG_HI(arrreg,dim)-comp,reg,dim,arrreg);
      } else if (wraparound && (rem_hi >= _REG_HI(arrreg,dim)+1)) {
	rem_hi = _SNAP_DN_WRAP(rem_hi,reg,dim,arrreg);
      } else if (rem_hi > _REG_HI(reg,dim)) {
	rem_hi = _REG_HI(reg,dim);
      } else {
	rem_hi = _SNAP_DN(rem_hi,reg,dim);
      }
      if (rem_lo <= rem_hi) {
	loc_lo = rem_lo+comp;
	loc_hi = rem_hi+comp;
	*lo = loc_lo;
	*hi = loc_hi;
	if (rem_lo > _REG_HI(arrreg,dim)) {
	  rem_lo -= _REG_SPAN(arrreg,dim);
	  rem_hi -= _REG_SPAN(arrreg,dim);
	}
	rem_proc = _DIST_TO_PROC(unknown, dist, rem_hi,dim);
	if (rem_proc >= _GRID_SIZE(grid, dim)) {
	  rem_proc = _GRID_SIZE(grid, dim)-1;
	} else if (rem_proc < 0) {
	  rem_proc = 0;
	}
      }
    }
  }
  PRINT_COMP("[%d] sending %d..%d to %d (to be recvd at %d..%d)\n",_INDEX,
	     loc_lo,loc_hi,rem_proc,rem_lo,rem_hi);
  return rem_proc;
}


static int _ComputeRecvNegCompWrap(_region reg,_region arrreg,const int dim,
				   const int comp,int* const lo,int* const hi) {
  int rem_lo;
  int rem_hi;
  int mylo;
  int rem_proc = -1;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  rem_lo = _REG_MYLO(reg,dim)+comp;
  rem_hi = _REG_MYHI(reg,dim)+comp;
  if (rem_lo <= rem_hi) {
    if (_INV_BEG_OF_REG(dist, arrreg,dim)) {
      mylo = _REG_LO(arrreg,dim);
    } else {
      mylo = _DIST_MYLO(dist, dim);
    }
    if (rem_hi >= mylo) {
      rem_hi = _SNAP_DN(mylo-1-comp,reg,dim)+comp;
    }
    if (rem_lo <= rem_hi) {
      *lo = rem_lo;
      *hi = rem_hi;
      if (rem_hi < _REG_LO(arrreg,dim)) {
	rem_lo += _REG_SPAN(arrreg,dim);
	rem_hi += _REG_SPAN(arrreg,dim);
      }
      rem_proc = _DIST_TO_PROC(unknown, dist, rem_hi,dim);
      if (rem_proc >= _GRID_SIZE(grid, dim)) {
	rem_proc = _GRID_SIZE(grid, dim)-1;
      } else if (rem_proc < 0) {
	rem_proc = 0;
      }
    }
  }
  PRINT_COMP("[%d] recving at %d..%d fm %d (to be sent fm %d..%d)\n",_INDEX,*lo,
	     *hi,rem_proc,rem_lo,rem_hi);
  return rem_proc;
}


static int _FindRect(_region reg,const _arrcomminfo* const arrinfo,
		     const int* const ptr,_vector lo,_vector hi,const int sr) {
  _region arrreg = _ARR_DECL_REG(arrinfo->arr);
  const int numwrapdirs = arrinfo->numwrapdirs;
  const int numdirs = arrinfo->numdirs;
  const _direction_pnc* dirvect = arrinfo->dirlist;
  const int numdims = _NUMDIMS(reg);
  _direction_pnc dir;
  int dirnum;
  int comm;
  int i;
  int proc[_MAXRANK] = {0,0,0,0,0,0};
  int loc_proc = -1;
  int comp;
  int first;
  int tmp_lo;
  int tmp_hi;
  int tmp_proc;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  for (i=0; i<_MAXDIM_GRID; i++) {
    if (i >= numdims && _GRID_LOC(grid, i) != 0) {
      return -1;
    }
  }

  first = 1;
  for (dirnum=0; dirnum<numwrapdirs; dirnum++) {
    dir = dirvect[dirnum];
    comm = 1;
    for (i=0; i<numdims && comm; i++) {
      comp = _DIRCOMP(dir,i);
      if (ptr[i] == 0) {
	if (comp == 0) {
	  tmp_proc = _ComputeCommZeroComp(reg,i,&tmp_lo,&tmp_hi);
	} else if (comp < 0) {
	  tmp_proc = _ComputeCommZeroCompNegRootWrap(arrreg,i,comp,&tmp_lo,
						     &tmp_hi);
	} else {
	  tmp_proc = _ComputeCommZeroCompPosRootWrap(arrreg,i,comp,&tmp_lo,
						     &tmp_hi);
	}
      } else {
	if (comp < 0) {
	  if (sr == _SEND) {
	    tmp_proc=_ComputeSendNegCompWrap(reg,arrreg,i,comp,&tmp_lo,&tmp_hi);
	  } else {
	    tmp_proc=_ComputeRecvNegCompWrap(reg,arrreg,i,comp,&tmp_lo,&tmp_hi);
	  }
	} else {
	  if (sr == _SEND) {
	    tmp_proc=_ComputeSendPosCompWrap(reg,arrreg,i,comp,&tmp_lo,&tmp_hi);
	  } else {
	    tmp_proc=_ComputeRecvPosCompWrap(reg,arrreg,i,comp,&tmp_lo,&tmp_hi);
	  }
	}
      }
      if (tmp_proc == -1) {
	comm = 0;
      } else {
	proc[i] = tmp_proc;
	if (first) {
	  lo[i] = tmp_lo;
	  hi[i] = tmp_hi;
	} else {
	  if (tmp_lo < lo[i]) {
	    lo[i] = tmp_lo;
	  }
	  if (tmp_hi > hi[i]) {
	    hi[i] = tmp_hi;
	  }
	}
      }
    }
    if (comm) {
      tmp_proc = _GRID_TO_PROC(grid, numdims,proc);
      if (first) {
	first = 0;
	loc_proc = tmp_proc;
      } else {
	if (tmp_proc != loc_proc) {
	  _RT_ANY_FATAL0("Illegally combining communication.");
	}
      }
    }
  }
  for (; dirnum<numdirs; dirnum++) {
    dir = dirvect[dirnum];
    comm = 1;
    for (i=0; i<numdims && comm; i++) {
      comp = _DIRCOMP(dir,i);
      if (_GRID_SIZE(grid, i) == 1) {
	tmp_proc = _ComputeCommFlatDim(reg,i,comp,&tmp_lo,&tmp_hi);
      } else if (ptr[i] == 0) {
	if (comp == 0) {
	  tmp_proc = _ComputeCommZeroComp(reg,i,&tmp_lo,&tmp_hi);
	} else if (comp < 0) {
	  tmp_proc = _ComputeCommZeroCompNegRoot(reg,i,comp,&tmp_lo,&tmp_hi);
	} else {
	  tmp_proc = _ComputeCommZeroCompPosRoot(reg,i,comp,&tmp_lo,&tmp_hi);
	}
      } else {
	if (comp < 0) {
	  if (sr == _SEND) {
	    tmp_proc=_ComputeSendNegComp(reg,i,comp,&tmp_lo,&tmp_hi);
	  } else {
	    tmp_proc=_ComputeRecvNegComp(reg,i,comp,&tmp_lo,&tmp_hi);
	  }
	} else {
	  if (sr == _SEND) {
	    tmp_proc=_ComputeSendPosComp(reg,i,comp,&tmp_lo,&tmp_hi);
	  } else {
	    tmp_proc=_ComputeRecvPosComp(reg,i,comp,&tmp_lo,&tmp_hi);
	  }
	}
      }
      if (tmp_proc != -1) {
	proc[i] = tmp_proc;
	if (first) {
	  lo[i] = tmp_lo;
	  hi[i] = tmp_hi;
	} else {
	  if (tmp_lo < lo[i]) {
	    lo[i] = tmp_lo;
	  }
	  if (tmp_hi > hi[i]) {
	    hi[i] = tmp_hi;
	  }
	}
      } else {
	comm = 0;
      }
    }
    if (comm) {
      tmp_proc = _GRID_TO_PROC(grid, numdims,proc);
      if (first) {
	first = 0;
	loc_proc = tmp_proc;
      } else {
	if (tmp_proc != loc_proc) {
	  _RT_ANY_FATAL0("Illegally combining communication.");
	}
      }
    }
  }

  if (loc_proc == _INDEX) {
    if (numwrapdirs == 0) {
      loc_proc = -1;
    }
  }

#ifdef PRINT_COMM
  if (loc_proc == -1) {
    printf("[%d] comm: none\n",_INDEX);
  } else {
    switch (numdims) {
    case 1:
      printf("[%d] comm w/ %d: [%d..%d] (%d)\n",_INDEX,loc_proc,
	     lo[0],hi[0],sr);
      break;
    case 2:
      printf("[%d] comm w/ %d: [%d..%d,%d..%d] (%d)\n",_INDEX,loc_proc,
	     lo[0],hi[0],lo[1],hi[1],sr);
      break;
    case 3:
      printf("[%d] comm w/ %d: [%d..%d,%d..%d,%d..%d] (%d)\n",_INDEX,loc_proc,
	     lo[0],hi[0],lo[1],hi[1],lo[2],hi[2],sr);
      break;
    }
  }
#endif

  return loc_proc;
}


static int _SetupCommInfo(IM_comminfo* const comminfo,_region reg,
			  const _atcomminfo* const dirinfo,const int* const ptr,
			  const int sr,spsinfovect* const spsinfo) {
  const int numarrs = dirinfo->numarr;
  const _arrcomminfo* const arrlist = dirinfo->arrlist;
  int numblocks;
  IM_memblock *memblock;
  int i;
  int retval = 0;
  int proc;
  _vector lo;
  _vector hi;
  int contains_data;
  _array_pnc arr;
  int elemsize;

  numblocks=0;
  memblock = comminfo->mbvect;
  for (i=0;i<numarrs;i++) {
    proc = _FindRect(reg,&(arrlist[i]),ptr,lo,hi,sr);
    if (proc != -1) {
      elemsize = arrlist[i].elemsize;
      arr = arrlist[i].arr;
      if (_ARR_DENSE(arr)) {
	contains_data = _SetupDenseMemInfo(memblock,reg,arr,_NUMDIMS(reg),lo,hi,
					   elemsize);
      } else {
	contains_data = _SetupSparseMemInfo(memblock,reg,arr,_NUMDIMS(reg),lo,
					    hi,elemsize,
					    &(spsinfo->vect[spsinfo->numsps]));
	spsinfo->numsps += contains_data;
      }
      if (contains_data) {
	numblocks++;
	if (numblocks == 1) {
	  comminfo->procnum = proc;
	  retval = 1 + (proc == _INDEX);  /* set LCL_BIT or REM_BIT */
	} else {
	  if (comminfo->procnum != proc) {
	    _RT_ANY_FATAL0("Illegally combining communication.");
	  }
	}
	memblock++;
      }
    }
  }
  comminfo->numblocks = numblocks;

  return retval;
}


void _LocalComm(IM_comminfo* const recvinfo, IM_comminfo* const sendinfo) {
  int i;
  IM_memblock* recvblock;
  IM_memblock* sendblock;
  int j;
  int numdims;
  int elemsize;
  _vector k;
  char* recvptr;
  char* sendptr;
 
  _IMU_Pack(recvinfo,recvinfo);
  _IMU_Pack(sendinfo,sendinfo);
  recvblock = recvinfo->mbvect;
  sendblock = sendinfo->mbvect;
  for (i=0; i<recvinfo->numblocks; i++) {
    sendptr = sendblock->baseptr;
    recvptr = recvblock->baseptr;
    numdims = recvblock->numdims;
    elemsize = recvblock->elemsize;
    for (j=0; j<numdims; j++) {
      k[j] = 0;
    }
    while (k[numdims-1] < recvblock->diminfo[numdims-1].numelems) {
      memcpy(recvptr,sendptr,elemsize);
      
      k[0]++;
      for (j=0;j<numdims-1;j++) {
	if (k[j] == recvblock->diminfo[j].numelems) {
	  k[j] = 0;
	  k[j+1]++;
	} else {
	  break;
	}
      }
      recvptr += recvblock->diminfo[j].stride;
      sendptr += sendblock->diminfo[j].stride;
    }

    recvblock++;
    sendblock++;
  }
}


void _AtComm_New(_region reg,const int numarr,const _arrcomminfo inputinfo[],
		 const int commid) {
  const int numdims = _NUMDIMS(reg);
  int dirnum;
  _direction_pnc dir;
  int i;
  int j;
  int jhi;
  int jhi2;
  int k;
  int khi;
  int d;
  int compatible;
  int arrsused;
  int numwrapdirs;
  int numdirs;
  int has_wrap_dirs=0;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  int num_useful_comm_dirs = _GRID_NUM_USEFUL_COMM_DIRS(grid);
  int num_useful_at_dirs =  _GRID_NUM_USEFUL_AT_DIRS(grid);
  int* dirptr = _GRID_DIRPTR(grid);

  for (dirnum=0; dirnum<num_comm_dirs; dirnum++) {
    arrsused = 0;
    for (i=0; i<numarr; i++) {
      numdirs = 0;
      jhi = inputinfo[i].numwrapdirs;
      jhi2 = inputinfo[i].numdirs;
      if (dirnum < num_useful_at_dirs) {
	khi = 2; /* loop over wrapdirs and normal dirs */
      } else {
	khi = 1; /* loop over wrapdirs only */
      }
      j = 0;
      for (k=0; k<khi; k++) {
	for (; j<jhi; j++) {
	  dir = inputinfo[i].dirlist[j]; 
	  compatible = 1;
	  for (d=0; d<numdims; d++) {
	    compatible &= ((DIRPTR(dirnum,d) == 0) ||
			   ((DIRPTR(dirnum,d) > 0) && (dir[d] > 0)) ||
			   ((DIRPTR(dirnum,d) < 0) && (dir[d] < 0)));
	  }
	  
	  if (compatible) {
	    ZPL_AT_COMMBANK(dirnum).arrlist[arrsused].dirlist[numdirs] = 
	      inputinfo[i].dirlist[j];
	    
	    numdirs++;
	  }
	}
	if (k == 0) {
	  numwrapdirs = numdirs;
	  jhi = jhi2;
	}
      }
      if (numdirs > 0) {
	if (numwrapdirs > 0 && dirnum >= num_useful_at_dirs && 
	    dirnum < num_useful_comm_dirs) {
	  has_wrap_dirs=1;
	}
	ZPL_AT_COMMBANK(dirnum).arrlist[arrsused].numwrapdirs = numwrapdirs;
	ZPL_AT_COMMBANK(dirnum).arrlist[arrsused].numdirs = numdirs;
	ZPL_AT_COMMBANK(dirnum).arrlist[arrsused].arr = inputinfo[i].arr;
	ZPL_AT_COMMBANK(dirnum).arrlist[arrsused].lhscomm =
	  inputinfo[i].lhscomm;
	ZPL_AT_COMMBANK(dirnum).arrlist[arrsused].elemsize = 
	  inputinfo[i].elemsize;
	arrsused++;
      }
    }
    ZPL_AT_COMMBANK(dirnum).numarr = arrsused;
  }

  /*
  if (_INDEX == 0) {
    for (dirnum=0; dirnum<num_comm_dirs; dirnum++) {
      int first = 1;
      int arrnum;

      if (dirnum == num_useful_comm_dirs) {
        printf("--------\n");
      }
      if (dirnum == num_useful_at_dirs) {
	printf("=======\n");
      }
      arrnum = ZPL_AT_COMMBANK(dirnum).numarr;
      for (i=0;i<arrnum;i++) {
        numdirs = ZPL_AT_COMMBANK(dirnum).arrlist[i].numdirs;
        if (numdirs > 0) { 
          if (first) {
            printf("comp ");
            PRINT_DIR(&(DIRPTR(dirnum,0)),numdims);
            printf(":\n");
            first = 0;
          }
          printf("  array %d:\n",i);
          for (j=0;j<numdirs;j++) {
            printf("    ");
            PRINT_DIR(ZPL_AT_COMMBANK(dirnum).arrlist[i].dirlist[j],numdims);
            printf("\n");
          }
        }
      }
    }
  }
  */

  {
    IM_comminfo* freeinfo = AT_COMMPOOL(commid);
    IM_comminfo* recvinfo = freeinfo++;
    IM_comminfo* sendinfo = freeinfo++;
    int numlclcomms = 0;
    int numremcomms = 0;
    int hi_comm_dir;

    if (has_wrap_dirs) {
      hi_comm_dir = num_useful_comm_dirs;
    } else {
      hi_comm_dir = num_useful_at_dirs;
    }

    /* iterate over the distributed dimensions */
    for (dirnum=0; dirnum<hi_comm_dir; dirnum++) {
      if (ZPL_AT_COMMBANK(dirnum).numarr != 0) {
	int recv, send;
	if (ZPL_AT_COMMBANK(dirnum).arrlist[0].lhscomm == 0) {
	  recv=_SetupCommInfo(recvinfo,reg,&(ZPL_AT_COMMBANK(dirnum)),
			      DIRPTR_VECT(dirnum),_RECV,
			      &(SPS_COMMBANK(commid).recvsps));
	  send=_SetupCommInfo(sendinfo,reg,&(ZPL_AT_COMMBANK(dirnum)),
			      DIRPTR_VECT(dirnum),_SEND,
			      &(SPS_COMMBANK(commid).sendsps));
	} else {
	  /*** lhs @, so reverse recv and send ***/
	  recv=_SetupCommInfo(recvinfo,reg,&(ZPL_AT_COMMBANK(dirnum)),
			      DIRPTR_VECT(dirnum),_SEND,
			      &(SPS_COMMBANK(commid).recvsps));
	  send=_SetupCommInfo(sendinfo,reg,&(ZPL_AT_COMMBANK(dirnum)),
			      DIRPTR_VECT(dirnum),_RECV,
			      &(SPS_COMMBANK(commid).sendsps));
	}
	
	switch (recv) {
	case COMM_NON:
	  if (send) {
	    IM_COMMBANK(commid,numremcomms).sendinfo = sendinfo;
	    sendinfo = freeinfo++;
	  }
	  break;
	case COMM_REM:
	  IM_COMMBANK(commid,numremcomms).recvinfo = recvinfo;
	  recvinfo = freeinfo++;
	  if (send) {
	    IM_COMMBANK(commid,numremcomms).sendinfo = sendinfo;
	    sendinfo = freeinfo++;
	  }
	  break;
	case COMM_LCL:
	  AT_COMMBANK(commid,numlclcomms).recvinfo = recvinfo;
	  recvinfo = freeinfo++;
	  AT_COMMBANK(commid,numlclcomms).sendinfo = sendinfo;
	  sendinfo = freeinfo++;
	  numlclcomms++;
	  break;
	}
	numremcomms++;
	/* Why always bump this?  Because it might be local for me,
	   but "nothing" for others (for example, if I have a wrap-@
	   on a very thinly defined array that lands only on one
	   column of processors).  Yet "nothing" for others isn't a
	   way to distinguish between local or remote, so they would
	   have to bump remote to be safe; and therefore I do too so
	   that we all call into Ironman identically. */
      }
    }

    /* these are guaranteed to be local */
    for (; dirnum<num_comm_dirs; dirnum++) {
      if (ZPL_AT_COMMBANK(dirnum).numarr != 0) {
	int recv;
	if (ZPL_AT_COMMBANK(dirnum).arrlist[0].lhscomm == 0) {
	  recv = _SetupCommInfo(recvinfo,reg,&(ZPL_AT_COMMBANK(dirnum)),
				DIRPTR_VECT(dirnum),_RECV,
				&(SPS_COMMBANK(commid).recvsps));
	  _SetupCommInfo(sendinfo,reg,&(ZPL_AT_COMMBANK(dirnum)),
			 DIRPTR_VECT(dirnum),_SEND,
			 &(SPS_COMMBANK(commid).sendsps));
	} else {
	  recv = _SetupCommInfo(recvinfo,reg,&(ZPL_AT_COMMBANK(dirnum)),
				DIRPTR_VECT(dirnum),_SEND,
				&(SPS_COMMBANK(commid).recvsps));
	  _SetupCommInfo(sendinfo,reg,&(ZPL_AT_COMMBANK(dirnum)),
			 DIRPTR_VECT(dirnum),_RECV,
			 &(SPS_COMMBANK(commid).sendsps));
	}
	if (recv) {
	  AT_COMMBANK(commid,numlclcomms).recvinfo = recvinfo;
	  recvinfo = freeinfo++;
	  AT_COMMBANK(commid,numlclcomms).sendinfo = sendinfo;
	  sendinfo = freeinfo++;
	  numlclcomms++;
	}
      }
    }

    if (numremcomms) {
      COMM_HANDLE(commid) = IM_New(grid,numremcomms,IM_COMMBANK_VCT(commid));
    } else {
      COMM_HANDLE(commid) = NULL;
    }

    COMM_COUNT(commid).remcomms = numremcomms;
    COMM_COUNT(commid).lclcomms = numlclcomms;
    /*
    printf("[%d] numrem: %2d numlcl: %2d\n",_INDEX,numremcomms,numlclcomms);
    */
  }
}


void _AtComm_DR(_region reg, const int commid) {
  int i;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  const int numlclcomms = COMM_COUNT(commid).lclcomms;
  IM_info* const handle = COMM_HANDLE(commid);

  if (handle) {
    IM_DR(grid, handle);
  }
  for (i=0; i<numlclcomms; i++) {
    if (AT_COMMBANK(commid,i).sendready) {
      AT_COMMBANK(commid,i).sendready = 0;
      _LocalComm(AT_COMMBANK(commid,i).recvinfo,AT_COMMBANK(commid,i).sendinfo);
    } else {
      AT_COMMBANK(commid,i).recvready = 1;
    }
  }
}


void _AtComm_SR(_region reg, const int commid) {
  int i;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  const int numlclcomms = COMM_COUNT(commid).lclcomms;
  const int numsps = SPS_COMMBANK(commid).sendsps.numsps;
  IM_info* const handle = COMM_HANDLE(commid);

  if (numsps) {
    PackAllSparse(numsps,SPS_COMMBANK(commid).sendsps.vect);
  }
  if (handle) {
    IM_SR(grid, handle);
  }
  for (i=0; i<numlclcomms; i++) {
    if (AT_COMMBANK(commid,i).recvready) {
      AT_COMMBANK(commid,i).recvready = 0;
      _LocalComm(AT_COMMBANK(commid,i).recvinfo,AT_COMMBANK(commid,i).sendinfo);
    } else {
      AT_COMMBANK(commid,i).sendready = 1;
    }
  }
}


#ifndef _AtComm_DN
void _AtComm_DN(_region reg, const int commid) {
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  const int numsps = SPS_COMMBANK(commid).recvsps.numsps;
  IM_info* const handle = COMM_HANDLE(commid);

  if (handle) {
    IM_DN(grid, handle);
  }
  if (numsps) {
    UnPackAllSparse(numsps,SPS_COMMBANK(commid).recvsps.vect);
  }
}
#endif


#ifndef _AtComm_SV
void _AtComm_SV(_region reg, const int commid) {
  int i;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  IM_info* const handle = COMM_HANDLE(commid);

  if (handle) {
    IM_SV(grid, handle);
  }
}
#endif


void _AtComm_Old(_region reg, const int commid) {
  int i;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  int num_useful_comm_dirs = _GRID_NUM_USEFUL_COMM_DIRS(grid);
  const int numremcomms = COMM_COUNT(commid).remcomms;
  const int numrecvsps = SPS_COMMBANK(commid).recvsps.numsps;
  const int numsendsps = SPS_COMMBANK(commid).sendsps.numsps;
  IM_info* const handle = COMM_HANDLE(commid);

  if (handle) {
    IM_Old(handle);
  }
  for (i=0; i<numremcomms; i++) {
    IM_COMMBANK(commid,i).recvinfo = NULL;
    IM_COMMBANK(commid,i).sendinfo = NULL;
  }
  if (numrecvsps) {
    FreeAllSparse(numrecvsps,SPS_COMMBANK(commid).recvsps.vect);
    SPS_COMMBANK(commid).recvsps.numsps = 0;
  }
  if (numsendsps) {
    FreeAllSparse(numsendsps,SPS_COMMBANK(commid).sendsps.vect);
    SPS_COMMBANK(commid).sendsps.numsps = 0;
  }
}
