/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <string.h>
#include "stand_inc.h"

void _PERM_Init(const int numpermmaps) {
  _MI_PERM_Init(numpermmaps);
}

_permmap* _PERM_MS(_region reg, const int dims) {
  _permmap* const restrict pm = (_permmap*)_zmalloc(sizeof(_permmap), "perm ms pm");
  pm->dims = dims;
  pm->lf = (int*)_zmalloc(_PROCESSORS*dims*2*sizeof(int), "perm ms first lasts");
  pm->rind = pm->lind = (int**)_zmalloc(sizeof(int*), "perm ms ind table");
  pm->procmap = NULL;
  return pm;
}

void _PERM_IR(_permmap* const pm) { }

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
	      const int srcdirect) { }

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
  pd->eltsize = eltsize;
  pd->rdata = pd->ldata = (char**)_zmalloc(sizeof(char*), "perm ds data table");
  pd->ldata[0] = (char*)_zmalloc(_PERM_RCNT(pm, 0)*eltsize, "perm ds data");
  pd->optsvector = 0;
}

void _PERM_DR(const _permmap* const pm, _permdata* const pd, const int scatter, _array_fnc dst, _array_fnc src) { }

int _PERM_DN(const _permmap* const pm, _permdata* const pd, const int scatter) {
  return -1;
}

void _PERM_DD(const _permmap* const pm, _permdata* const pd, const int scatter) {
  _zfree(pd->ldata[0], "perm dd data");
  _zfree(pd->ldata, "perm dd data table");
}

void _PERM_MD(_permmap* pm) {
  _zfree(pm->lf, "perm md first lasts");
  if (pm->procmap) {
    _zfree(pm->procmap, "perm md procmap");
  }
  _zfree(pm->lind, "perm md ind table");
  _zfree(pm, "perm md pm");
}

void _PERM_Done() {
  _PERM_DeleteMaps();
}
