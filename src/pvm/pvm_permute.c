/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "pvm3.h"
#include "stand_inc.h"

extern int *_tids;

void _PERM_Init(const int numpermmaps) {
  _MI_PERM_Init(numpermmaps);
}

_permmap* _PERM_MS(_region reg, const int dims) {
  _permmap* pm;

  pm = (_permmap*)_zmalloc(sizeof(_permmap), "permute map");
  pm->dims = dims;
  pm->lind = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "local indices");
  pm->procmap = NULL;
  pm->rind = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "");
  pm->lf = (int*)_zmalloc(_PROCESSORS*dims*2*sizeof(int), "perm ms first lasts");
  return pm;
}

void _PERM_IR(_permmap* const pm) {
  const int dims = pm->dims;
  int* const rcnt = (int*)_zmalloc(_PROCESSORS * sizeof(int), "");
  int** const rind = pm->rind;
  int** const lind = pm->lind;
  int dyncommid;
  int i;
  const int zero = 0;

  dyncommid = _GetDynamicCommID();
  for (i = 0; i < _PROCESSORS; i++) {
    if (i != _INDEX) {
      pvm_initsend(PvmDataInPlace);
      if (lind[i]) {
	pvm_pkbyte((char *)&(lind[i][0]), sizeof(int), 1);
      }
      else {
	pvm_pkbyte((char *)&zero, sizeof(int), 1);
      }
      pvm_send(_tids[i], dyncommid);
    }
  }
  for (i = 0; i < _PROCESSORS; i++) {
    if (i != _INDEX) {
      pvm_recv(_tids[i], dyncommid);
      pvm_upkbyte((char *)&(rcnt[i]), sizeof(int), 1);
    }
  }
  for (i = 0; i < _PROCESSORS; i++) {
    if (i != _INDEX) {
      if (rcnt[i]) {
	rind[i] = (int*)_zmalloc(rcnt[i] * sizeof(int), "");
	rind[i][0] = rcnt[i];
      }
      else {
	rind[i] = 0;
      }
    }
    else {
      rind[i] = lind[i];
    }
  }
  _zfree(rcnt, "");
  for (i = 0; i < _PROCESSORS; i++) {
    if (i != _INDEX) {
      if (lind[i]) {
	pvm_initsend(PvmDataInPlace);
	pvm_pkbyte((char*)(lind[i]), lind[i][0]*sizeof(int), 1);
	pvm_send(_tids[i], dyncommid);
      }
    }
  }
  pm->rind = rind;
  pm->dyncommid = dyncommid;
}

void _PERM_IN(_permmap* const pm,
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
  int** const rind = pm->rind;
  int dyncommid = pm->dyncommid;
  int i;

  for (i = 0; i < _PROCESSORS; i++) {
    if (i != _INDEX) {
      if (pm->rind[i]) {
	pvm_recv(_tids[i], dyncommid);
	pvm_upkbyte((char *)(rind[i]), rind[i][0]*sizeof(int), 1);
      }
    }
  }
}

void _PERM_DS(const _permmap* const pm,
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
  int tlcnt, tldcnt;
  int trcnt, trdcnt;
  char ** const ldata = (char**)_zmalloc(_PROCESSORS*sizeof(char*), "");
  char ** const rdata = (char**)_zmalloc(_PROCESSORS*sizeof(char*), "");
  int i;

  tlcnt = 0;
  trcnt = 0;
  for (i=0; i<_PROCESSORS; i++) {
    tlcnt += _PERM_LCNT(pm, i);
    trcnt += _PERM_RCNT(pm, i);
  }
  tldcnt = scatter ? tlcnt : trcnt;
  trdcnt = scatter ? trcnt : tlcnt;
  pd->lbase = ldata[0] = (char*)_zmalloc(tldcnt*eltsize, "");
  for (i=1;i<_PROCESSORS;i++) {
    ldata[i] = ldata[i-1] + (scatter ? _PERM_LCNT(pm, i-1) : _PERM_RCNT(pm, i-1)) * eltsize;
  }
  pd->rbase = rdata[0] = (char*)_zmalloc(trdcnt*eltsize, "");
  for (i=1;i<_PROCESSORS;i++) {
    rdata[i] = rdata[i-1] + (scatter ? _PERM_RCNT(pm, i-1) : _PERM_LCNT(pm, i-1)) * eltsize;
  }
  pd->ldata = ldata;
  pd->rdata = rdata;
  pd->eltsize = eltsize;
  pd->optsvector = 0;
}

void _PERM_DR(const _permmap* const pm, _permdata* const pd, const int scatter, _array_fnc dst, _array_fnc src) {
  const int eltsize = pd->eltsize;
  char** const ldata = pd->ldata;
  char** const rdata = pd->rdata;
  int dyncommid;
  int i;

  dyncommid = _GetDynamicCommID();
  for (i = 0; i < _PROCESSORS; i++) {
    if (i != _INDEX) {
      if ((scatter ? _PERM_LCNT(pm, i) : _PERM_RCNT(pm, i)) > 0) {
	pvm_initsend(PvmDataInPlace);
	pvm_pkbyte(ldata[i], (scatter ? _PERM_LCNT(pm, i) : _PERM_RCNT(pm, i))*eltsize, 1);
	pvm_send(_tids[i], dyncommid);
      }
    }
  }
  rdata[_INDEX] = ldata[_INDEX];
  pd->dyncommid = dyncommid;
  pd->count = (_INDEX + 1 == _PROCESSORS) ? 0 : _INDEX + 1;
}

int _PERM_DN(const _permmap* const pm, _permdata* const pd, const int scatter) {
  const int eltsize = pd->eltsize;
  char** const rdata = pd->rdata;
  int dyncommid = pd->dyncommid;
  int count = pd->count;

  while (count != _INDEX) {
    if ((scatter ? _PERM_RCNT(pm, count) : _PERM_LCNT(pm, count)) > 0) {
      pvm_recv(_tids[count], dyncommid);
      pvm_upkbyte(rdata[count], (scatter ? _PERM_RCNT(pm, count) : _PERM_LCNT(pm, count))*eltsize, 1);
      pd->count = (count + 1 == _PROCESSORS) ? 0 : count + 1;
      return count;
    }
    count = (count + 1 == _PROCESSORS) ? 0 : count + 1;
  }
  return -1;
}

void _PERM_DD(const _permmap* const pm,
	      _permdata* const pd,
	      const int scatter) {
  _zfree(pd->rbase, "");
  _zfree(pd->lbase, "");
  _zfree(pd->rdata, "");
  _zfree(pd->ldata, "");
}

void _PERM_MD(_permmap* pm) {
  int i, j;

  _zfree(pm->lf, "perm md first lasts");
  if (pm->procmap) {
    _zfree(pm->procmap, "");
  }
  for (i = 0; i < _PROCESSORS; i++) {
    if (pm->lind[i]) {
      _zfree(pm->lind[i], "");
    }
    if (i != _INDEX) {
      if (pm->rind[i]) {
	_zfree(pm->rind[i], "");
      }
    }
  }
  _zfree(pm, "");
}

void _PERM_Done() {
  _PERM_DeleteMaps();
}

