/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "stand_inc.h"
#include "ironman_utils.h"

typedef struct {
  IM_info *commhandle;
  IM_comminfo recvinfo;
  IM_comminfo sendinfo;
} wrapinfo;

_PRIV_DECL(static, wrapinfo *, wrap_commbank);

void _SetupWrapReflect(_region reg, _region basereg, _direction dir) {
  int sendwrap;
  int recvwrap;
  int srcproc = _INDEX;
  int dstproc = _INDEX;
  int i;
  int numdims;
  int dstloc[_MAXRANK];
  int srcloc[_MAXRANK];
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  numdims = _NUMDIMS(reg);
  for (i=0; i<numdims; i++) {
    if (dir[i] < 0) {
      dstloc[i] = _MIN_PROC(reg,i);
      srcloc[i] = _MAX_PROC(basereg,i);
    } else if (dir[i] == 0) {
      dstloc[i] = _GRID_LOC(grid, i);
      srcloc[i] = _GRID_LOC(grid, i);
    } else {
      dstloc[i] = _MAX_PROC(reg,i);
      srcloc[i] = _MIN_PROC(basereg,i);
    }
  }
  dstproc = _GRID_TO_PROC(grid, numdims, dstloc);
  srcproc = _GRID_TO_PROC(grid, numdims, srcloc);

  sendwrap = (_INDEX == srcproc);
  recvwrap = (_INDEX == dstproc);

  /* This means "is it local for everyone?" not just "...for me?" */
  _WRAP_LOCAL(reg) = (dstproc == srcproc);

  if (sendwrap) {
    _I_SEND_WRAP(reg) = dstproc;
  } else {
    _I_SEND_WRAP(reg) = _NO_PROCESSOR;
  }
  if (recvwrap) {
    _I_RECV_WRAP(reg) = srcproc;
  } else {
    _I_RECV_WRAP(reg) = _NO_PROCESSOR;
  }
  for (i=0;i<numdims;i++) {
    if (dir[i] == 0) {
      _WRAP_SEND_LO(reg,i) = _REG_MYLO(basereg,i);
      _WRAP_SEND_HI(reg,i) = _REG_MYHI(basereg,i);
      _REFLECT_SRC_LO(reg,i) = _REG_MYLO(basereg,i);
      _REFLECT_SRC_HI(reg,i) = _REG_MYHI(basereg,i);
      _REFLECT_SRC_STEP(reg,i) = 1;
    } else if (dir[i] < 0) {
      _WRAP_SEND_LO(reg,i) = _SNAP_UP(_REG_MYHI(basereg,i)+dir[i]+1,basereg,i);
      _WRAP_SEND_HI(reg,i) = _SNAP_DN(_REG_MYHI(basereg,i),basereg,i);
      _REFLECT_SRC_LO(reg,i) = _REG_MYLO(basereg,i)- dir[i] - 1;
      _REFLECT_SRC_HI(reg,i) = _REG_MYLO(basereg,i);
      _REFLECT_SRC_STEP(reg,i) = -1;
    } else {
      _WRAP_SEND_LO(reg,i) = _SNAP_UP(_REG_MYLO(basereg,i),basereg,i);
      _WRAP_SEND_HI(reg,i) = _SNAP_DN(_REG_MYLO(basereg,i)+dir[i]-1,basereg,i);
      _REFLECT_SRC_LO(reg,i) = _REG_MYHI(basereg,i);
      _REFLECT_SRC_HI(reg,i) = _REG_MYHI(basereg,i) - dir[i] + 1;
      _REFLECT_SRC_STEP(reg,i) = -1;
    }
  }
}

static int _FindRect(_region reg,_vector lo,_vector hi,int sr) {
  int numdims;
  int i;

  numdims = _NUMDIMS(reg);
  if (sr == _RECV) {
    for (i=0;i<numdims;i++) {
      lo[i] = _WRAP_RECV_LO(reg,i);
      hi[i] = _WRAP_RECV_HI(reg,i);
    }
  } else {
    for (i=0;i<numdims;i++) {
      lo[i] = _WRAP_SEND_LO(reg,i);
      hi[i] = _WRAP_SEND_HI(reg,i);
    }
  }
  return numdims;
}


static void _SetupMemInfo(IM_comminfo *comminfo,_region reg,int num,
			  _array *ens,unsigned int *elemsize,int sr) {
  IM_memblock *memblock; 
  int numdims;
  int i;
  _vector lo;
  _vector hi;
  int contains_data;
  int numblocks;

  memblock = comminfo->mbvect;
  numdims = _FindRect(reg,lo,hi,sr);

  numblocks = 0;
  for (i=0;i<num;i++) {
    contains_data = _SetupDenseMemInfo(memblock,reg,ens[i],numdims,lo,hi,
				       elemsize[i]);
    if (contains_data) {
      numblocks++;
      memblock++;
    }
  }
  comminfo->numblocks = numblocks;
}


static IM_comminfo *_SetupWrapInfo(IM_comminfo *comminfo,_region reg,int num,
				   _array *ens,unsigned int *elemsize,
				   int sr) {
  int ido;
  IM_comminfo *retval;

  ido = _I_WRAP(reg,sr);
  if (ido < 0) {
    retval = NULL;
  } else {
    comminfo->procnum = ido;
    _SetupMemInfo(comminfo,reg,num,ens,elemsize,sr);
    retval = comminfo;
  }
  return retval;
}


void _InitWrap(const int numcomms,const int enspercommid[]) {
  int i;
  _PRIV_TID_DECL;

  _PRIV_ALLOC(wrapinfo *, wrap_commbank);
  _PRIV_ACCESS(wrap_commbank) = (wrapinfo *)_zmalloc(numcomms*sizeof(wrapinfo),
						       "wrap commbank");
  for (i=0;i<numcomms;i++) {
    _PRIV_ACCESS(wrap_commbank)[i].recvinfo.mbvect =
      (IM_memblock *)_zmalloc(enspercommid[i]*sizeof(IM_memblock),
				"wrap memblock recv vectors");
    _PRIV_ACCESS(wrap_commbank)[i].sendinfo.mbvect = 
      (IM_memblock *)_zmalloc(enspercommid[i]*sizeof(IM_memblock),
				"wrap memblock send vectors");
  }
  /* Ironman libraries initialized in at_comm.c */
}


void _WrapI(_region reg,int numens,_array *ens,unsigned int *elemsize,
	    int commid) {
  IM_comminfo *recvinfo;
  IM_comminfo *sendinfo;
  IM_commpair commpair;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  _SetupWrapReflect(reg, _REG_PREP_BASEREG(reg), _REG_PREP_DIR(reg));

  recvinfo = _SetupWrapInfo(&(_PRIV_ACCESS(wrap_commbank)[commid].recvinfo),
			    reg,numens,ens,elemsize,_RECV);
  sendinfo = _SetupWrapInfo(&(_PRIV_ACCESS(wrap_commbank)[commid].sendinfo),
			    reg,numens,ens,elemsize,_SEND);

  /* if both are NULL, we can't tell if it's local or global, so can't
     keep Ironman in synch.  So we now have the WRAP_LOCAL field */

  if (_WRAP_LOCAL(reg)) {
    if (recvinfo) { /* if I'm actually participating */
      _LocalComm(recvinfo,sendinfo);
    }
    _PRIV_ACCESS(wrap_commbank)[commid].commhandle = NULL;
  } else {
    commpair.recvinfo = recvinfo;
    commpair.sendinfo = sendinfo;
    _PRIV_ACCESS(wrap_commbank)[commid].commhandle = IM_New(grid,1,&commpair);
    if (_PRIV_ACCESS(wrap_commbank)[commid].commhandle != NULL) {
      IM_DR(grid, _PRIV_ACCESS(wrap_commbank)[commid].commhandle);
      IM_SR(grid, _PRIV_ACCESS(wrap_commbank)[commid].commhandle);
    }
  }
}


void _WrapII(_region reg, int commid) {
  IM_info *commhandle;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  commhandle = _PRIV_ACCESS(wrap_commbank)[commid].commhandle;
  if (commhandle) {
    if (commhandle != NULL) {
      IM_DN(grid, commhandle);
      IM_SV(grid, commhandle);
      IM_Old(commhandle);
    }
  }
}
