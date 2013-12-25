/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "stand_inc.h"

/*#define _MPI_PERMUTE_DEBUG*/

static MPI_Status __status_ignore;
static MPI_Status * const restrict _status_ignore = &__status_ignore;

void _PERM_Init(const int numpermmaps) {
  _MI_PERM_Init(numpermmaps);
}

_permmap* _PERM_MS(_region reg, const int dims) {
  _permmap* const restrict pm = (_permmap*)_zmalloc(sizeof(_permmap), "perm ms pm");
  pm->dims = dims;
  pm->lind = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "perm ms lind table");
  pm->rind = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "perm ms rind table");
  pm->lf = (int*)_zmalloc(_PROCESSORS*dims*2*sizeof(int), "perm ms first lasts");
  pm->procmap = NULL;
  pm->collective = 0;
  return pm;
}

void _PERM_IR(_permmap* const pm) {
  int use_collectives;
  int * rindbase;
  int * lindbase;
  int * const restrict lsize = (int *)_zmalloc(_PROCESSORS * sizeof(int), "perm ir lcnt");
  int * const restrict rsize = (int *)_zmalloc(_PROCESSORS * sizeof(int), "perm ir rcnt");
  int * restrict * const restrict lind = pm->lind;
  int * restrict * const restrict rind = pm->rind;
  int i, j;
  int commcount;
  int use_collectives_intent;

  commcount = 0;
  for (i = 0; i < _PROCESSORS; i++) {
    lsize[i] = lind[i] ? lind[i][0] : 0;
    if (lsize[i] > 0) {
      commcount++;
    }
  }
  use_collectives_intent = (commcount >= (_PROCESSORS > 4 ? 4 : _PROCESSORS));
  MPI_Allreduce(&use_collectives_intent, &use_collectives, 1, MPI_INT, MPI_SUM, _GRID_WORLD(_DefaultGrid));
  if (use_collectives >= (_PROCESSORS > 4 ? 4 : _PROCESSORS)) {
    use_collectives = 1;
  }
  else {
    use_collectives = 0;
  }
  use_collectives_intent = _QueryMPICollective();
  if (use_collectives_intent != -1) {
    use_collectives = use_collectives_intent;
  }
  pm->collective = use_collectives;
  MPI_Alltoall(lsize, 1, MPI_INT, rsize, 1, MPI_INT, _GRID_WORLD(_DefaultGrid));

  _PERM_CleanIndices(lsize, rsize, lind, rind, &lindbase, &rindbase);

#ifdef _MPI_PERMUTE_DEBUG
  sleep(_INDEX);
  printf("FROM PROCESSOR %d\n", _INDEX);
  /*
  printf("  PROCMAP: size = %d, # elts = %d, encoded = %d :: ", pm->procmap[0], pm->procmap[1], pm->procmap[2]);
   for (j = 3; j < pm->procmap[0]; j++) {
    printf("%d ", pm->procmap[j]);
  }
  printf("\n");
  */
  for (i = 0; i < _PROCESSORS; i++) {
    if (lind[i] != 0) {
      printf("  TO PROCESSOR %d: ", i);
      printf("size = %d, # elts = %d, encoded = %d :: ", lind[i][0], lind[i][1], lind[i][2]);
      for (j = 3; j < lind[i][0]; j++) {
	printf("%d ", lind[i][j]);
      }
      printf("\n");
    }
  }
  printf("\n");
  sleep(100);
#endif

  if (use_collectives) {
    int* const loff = (int*)_zmalloc(_PROCESSORS*sizeof(int), "perm ir loff");
    int* const roff = (int*)_zmalloc(_PROCESSORS*sizeof(int), "perm ir roff");

    loff[0] = 0;
    roff[0] = 0;
    for (i=1; i<_PROCESSORS; i++) {
      loff[i] = loff[i - 1] + lsize[i - 1];
      roff[i] = roff[i - 1] + rsize[i - 1];
    }
    MPI_Alltoallv(lindbase, lsize, loff, MPI_INT, rindbase, rsize, roff, MPI_INT, _GRID_WORLD(_DefaultGrid));
    _zfree(loff, "perm ir loff");
    _zfree(roff, "perm ir roff");
  }
  else {
    MPI_Request* const restrict arrreq = (MPI_Request*)_zmalloc(_PROCESSORS * sizeof(MPI_Request), "perm ir mpi request");
    int dyncommid;

    dyncommid = _GetDynamicCommID();
    for (i = 0; i < _PROCESSORS; i++) {
      if (i != _INDEX && rind[i]) {
	MPI_Irecv(rind[i], rind[i][0], MPI_INT, i, dyncommid, _GRID_WORLD(_DefaultGrid), &(arrreq[i]));
      }
    }
    for (i = 0; i < _PROCESSORS; i++) {
      if (i != _INDEX && lind[i]) {
	MPI_Send(lind[i], lind[i][0], MPI_INT, i, dyncommid, _GRID_WORLD(_DefaultGrid));
      }
    }
    pm->arrreq = arrreq;
  }
  if (lsize[_INDEX] > 0) {
    memcpy(rind[_INDEX], lind[_INDEX], lsize[_INDEX]*sizeof(int));
  }
  pm->lindbase = lindbase;
  pm->rindbase = rindbase;
  _zfree(lsize, "perm ir lcnt");
  _zfree(rsize, "perm ir rcnt");
}

void _PERM_IN(_permmap* const restrict pm,
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
  const int use_collectives = pm->collective;
  int i;

  if (!use_collectives) {
    MPI_Request* const restrict arrreq = pm->arrreq;
    int * const restrict * const restrict rind = pm->rind;
    int i;

    for (i = 0; i < _PROCESSORS; i++) {
      if (i != _INDEX && rind[i]) {
        MPI_Wait(&(arrreq[i]), _status_ignore);
      }
    }
    _zfree(arrreq, "perm in mpi request");
  }
  _PERM_INFinish(pm, scatter, eltsize, dst, src, reg, prilevels, dstsrcconflict, dstdead,
		 srcdead, dstdirect, srcdirect);
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
  pd->ldata = (char**)_zmalloc(_PROCESSORS*sizeof(char*), "perm ds ldata table");
  pd->rdata = (char**)_zmalloc(_PROCESSORS*sizeof(char*), "perm ds rdata table");
  _PERM_SetupDirect(pm, pd, scatter, eltsize, dst, src, reg, prilevels, dstsrcconflict, dstdead, srcdead, dstdirect, srcdirect);
}

void _PERM_DR(const _permmap* const restrict pm, _permdata* const restrict pd, const int scatter, _array_fnc dst, _array_fnc src) {
  const int use_collectives = pm->collective;
  const int eltsize = pd->eltsize;
  int i;
  int * const restrict ldecnt = (int*)_zmalloc(_PROCESSORS*sizeof(int), "perm dr lcnt");
  int * const restrict rdecnt = (int*)_zmalloc(_PROCESSORS*sizeof(int), "perm dr rcnt");
  char * const restrict * const restrict ldata = pd->ldata;
  char * const restrict * const restrict rdata = pd->rdata;

  if (pd->optsvector & _PERM_UNODIRECT) {
    char* tmp = _ARR_DATA(src);
    _ARR_DATA(src) = _ARR_DATA(dst);
    _ARR_DATA(dst) = tmp;
    _ARR_ORIGIN(src) = _ComputeEnsembleOrigin(src);
    _ARR_ORIGIN(dst) = _ComputeEnsembleOrigin(dst);
  }

  for (i=0; i<_PROCESSORS; i++) {
    ldecnt[i] = (scatter ? _PERM_LCNT(pm, i) : _PERM_RCNT(pm, i)) * eltsize;
    rdecnt[i] = (scatter ? _PERM_RCNT(pm, i) : _PERM_LCNT(pm, i)) * eltsize;
  }
  if (use_collectives) {
    int* const restrict loff = (int*)_zmalloc(_PROCESSORS*sizeof(int), "perm dr loff");
    int* const restrict roff = (int*)_zmalloc(_PROCESSORS*sizeof(int), "perm dr roff");

    loff[0] = 0;
    roff[0] = 0;
    for (i=1; i<_PROCESSORS; i++) {
      loff[i] = loff[i-1] + ldecnt[i-1];
      roff[i] = roff[i-1] + rdecnt[i-1];
    }

    /*
    long stuff;
    char* tmp;

    stuff=0;
    for (i=0; i<PROCESSORS; i++) {
      int j;
      for (j=0; j<ldecnt[i]; j++) {
	stuff += *(pd->lbase+loff[i]+j);
      }
    }
    printf("%d\n", stuff);

    stuff=0;
    tmp = pd->lbase;
    for (i=0; i<PROCESSORS; i++) {
      int j;
      for (j=0; j<ldecnt[i]; j++) {
	stuff += *tmp++;
      }
    }
    printf("%d\n", stuff);
    */

    MPI_Alltoallv(pd->lbase, ldecnt, loff, MPI_BYTE, pd->rbase, rdecnt, roff, MPI_BYTE, _GRID_WORLD(_DefaultGrid));

    /*
    stuff=0;
    for (i=0; i<PROCESSORS; i++) {
      int j;
      for (j=0; j<rdecnt[i]; j++) {
	stuff += *(pd->rbase+roff[i]+j);
      }
    }
    printf("%d\n", stuff);

    stuff=0;
    tmp = pd->rbase;
    for (i=0; i<PROCESSORS; i++) {
      int j;
      for (j=0; j<rdecnt[i]; j++) {
	stuff += *tmp++;
      }
    }
    printf("%d\n", stuff);
    */
    _zfree(loff, "perm dr loff");
    _zfree(roff, "perm dr roff");
  }
  else {
    MPI_Request* const restrict arrreq = (MPI_Request*)_zmalloc(_PROCESSORS*sizeof(MPI_Request), "perm dr mpi request");
    int dyncommid;

    dyncommid = _GetDynamicCommID();
    for (i = 0; i < _PROCESSORS; i++) {
      if (i != _INDEX && rdecnt[i]) {
	MPI_Irecv(rdata[i], rdecnt[i], MPI_BYTE, i, dyncommid, _GRID_WORLD(_DefaultGrid), &(arrreq[i]));
      }
    }
    for (i = 0; i < _PROCESSORS; i++) {
      if (i != _INDEX && ldecnt[i]) {
	MPI_Send(ldata[i], ldecnt[i], MPI_BYTE, i, dyncommid, _GRID_WORLD(_DefaultGrid));
      }
    }
    if (ldecnt[_INDEX] > 0) {
      memcpy(rdata[_INDEX], ldata[_INDEX], ldecnt[_INDEX]);
    }
    pd->arrreq = arrreq;
  }
  _zfree(ldecnt, "perm dr lcnt");
  _zfree(rdecnt, "perm dr rcnt");

  pd->count = -1;
}

int _PERM_DN(const _permmap * const restrict pm, _permdata * const restrict pd, const int scatter) {
  int cnt = pd->count;

  const int use_collectives = pm->collective;
  while (++cnt < _PROCESSORS) {
    if (cnt != _INDEX && (scatter ? _PERM_RCNT(pm, cnt) : _PERM_LCNT(pm, cnt)) > 0) {
      if (!use_collectives) {
	MPI_Wait(&(pd->arrreq[cnt]), _status_ignore);
      }
      pd->count = cnt;
      return cnt;
    }
  }
  if (!use_collectives) {
    _zfree(pd->arrreq, "perm dn mpi request");
  }
  return -1;
}

void _PERM_DD(const _permmap* const pm,
	      _permdata* const pd,
	      const int scatter) {
  const int optvec = pd->optsvector;

  if (optvec & _PERM_SRCBUCKET) {
    _zfree(pd->lbase, "perm dd ldata");
  }
  if (optvec & _PERM_DSTBUCKET) {
    _zfree(pd->rbase, "perm dd rdata");
  }
  _zfree(pd->rdata, "perm dd rdata table");
  _zfree(pd->ldata, "perm dd ldata table");
}

void _PERM_MD(_permmap* pm) {
  const int use_collectives = pm->collective;
  int i, j;

  if (pm->procmap) {
    _zfree(pm->procmap, "perm md procmap");
  }
  _zfree(pm->lindbase, "perm md lind");
  _zfree(pm->rindbase, "perm md rind");
  _zfree(pm->lind, "perm md lind table");
  _zfree(pm->rind, "perm md rind table");
  _zfree(pm->lf, "perm md first lasts");
  _zfree(pm, "perm md pm");
}

void _PERM_Done() {
  _PERM_DeleteMaps();
}
