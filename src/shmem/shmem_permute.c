/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <mpp/shmem.h>
#include "stand_inc.h"

/*#define _SHMEM_PERMUTE_DEBUG*/

void _PERM_Init(const int numpermmaps) {
  _MI_PERM_Init(numpermmaps);
}

_permmap* _PERM_MS(_region reg, const int dims) {
  int i;

  _permmap* const restrict pm = (_permmap*)_zmalloc(sizeof(_permmap), "perm ms pm");
  pm->dims = dims;
  pm->lind = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "perm ms lind table");
  pm->rind = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "perm ms rind table");
  pm->lf = (int*)_zmalloc(_PROCESSORS*dims*2*sizeof(int), "perm ms first lasts");
  pm->rptr = (char**)shmalloc(_PROCESSORS*sizeof(char*));
  for (i=0; i<_PROCESSORS; i++) {
    pm->rptr[i] = 0;
  }
  pm->rflag = (int*)shmalloc(_PROCESSORS*sizeof(int));
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("%d reporting\n", _INDEX);
  fflush(stdout);
      sleep(10);
#endif
  return pm;
}

void _PERM_IR(_permmap* const pm) {
  const int one = 1;
  int * rindbase;
  int * lindbase;
  int * const restrict lsize = (int *)shmalloc(_PROCESSORS * sizeof(int));
  int * const restrict rsize = (int *)shmalloc(_PROCESSORS * sizeof(int));
  int * restrict * const restrict lind = pm->lind;
  int * restrict * const restrict rind = pm->rind;
  char * restrict * const restrict rptr = pm->rptr;
  int * const restrict rflag = pm->rflag;
  int* addr;
  int i, j;

  for (i = 0; i < _PROCESSORS; i++) {
    lsize[i] = lind[i] ? lind[i][0] : 0;
    rsize[i] = 0;
  }
  shmem_barrier_all();
  for (i = (_INDEX == _PROCESSORS - 1) ? 0 : _INDEX+1;
       i != _INDEX;
       i = (i == _PROCESSORS - 1) ? 0 : i++) {
    if (lsize[i] > 0) {
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("%d sending count to %d\n", _INDEX, i);
  fflush(stdout);
#endif
      shmem_int_put(&(rsize[_INDEX]), &(lsize[i]), 1, i);
    }
  }
  rsize[_INDEX] = lsize[_INDEX];
  shmem_barrier_all();
#ifdef _SHMEM_PERMUTE_DEBUG
  sleep(_PROCESSORS);
#endif
  _PERM_CleanIndices(lsize, rsize, lind, rind, &lindbase, &rindbase);

#ifdef _SHMEM_PERMUTE_DEBUG
  sleep(_INDEX);
  printf("FROM PROCESSOR %d\n", _INDEX);
  printf("  LSIZE = ");
  for (i = 0; i < _PROCESSORS; i++) {
    printf("%d ", lsize[i]);
  }
  printf("\n");
  printf("  RSIZE = ");
  for (i = 0; i < _PROCESSORS; i++) {
    printf("%d ", rsize[i]);
  }
  printf("\n");

  printf("  PROCMAP: size = %d, # elts = %d, encoded = %d :: ", pm->procmap[0], pm->procmap[1], pm->procmap[2]);
   for (j = 3; j < pm->procmap[0]; j++) {
    printf("%d ", pm->procmap[j]);
  }
  printf("\n");
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
  fflush(stdout);
  sleep(_PROCESSORS-_INDEX);
#endif

  for (i = (_INDEX == _PROCESSORS - 1) ? 0 : _INDEX+1;
       i != _INDEX;
       i = (i == _PROCESSORS - 1) ? 0 : i++) {
    if (rsize[i] > 0) {
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("%d sending rind address to %d\n", _INDEX, i);
  fflush(stdout);
#endif
      rflag[_INDEX] = 0;
      shmem_put((void*)&(rptr[_INDEX]), (void*)&(rind[i]), 1, i);
    }
  }
#ifdef _SHMEM_PERMUTE_DEBUG
  sleep(_PROCESSORS);
#endif
  for (i = (_INDEX == 0) ? _PROCESSORS-1 : _INDEX-1;
       i != _INDEX;
       i = (i == 0) ? _PROCESSORS-1 : i--) {
    if (lsize[i] > 0) {
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("%d waiting for rind address from %d, sending lind\n", _INDEX, i);
  fflush(stdout);
#endif
      shmem_wait((long*)&(rptr[i]), 0);
      addr = (int*)rptr[i];
      rptr[i] = 0;
      shmem_int_put(addr, lind[i], lsize[i], i);
    }
  }
#ifdef _SHMEM_PERMUTE_DEBUG
  sleep(_PROCESSORS);
#endif
  shmem_fence();
  for (i = (_INDEX == 0) ? _PROCESSORS-1 : _INDEX-1;
       i != _INDEX;
       i = (i == 0) ? _PROCESSORS-1 : i--) {
    if (lsize[i] > 0) {
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("IR %d sending one to %d\n", _INDEX, i);
  fflush(stdout);
#endif
      shmem_int_put(&(rflag[_INDEX]), &one, 1, i);
    }
  }
  if (lsize[_INDEX] > 0) {
    memcpy(rind[_INDEX], lind[_INDEX], lsize[_INDEX]*sizeof(int));
  }
  pm->lindbase = lindbase;
  pm->rindbase = rindbase;
  shfree(lsize);
  shfree(rsize);
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
  int * const restrict rflag = pm->rflag;
  const int * restrict const * restrict const rind = pm->rind;
  int i, j;
#ifdef _SHMEM_PERMUTE_DEBUG
  sleep(_PROCESSORS);
#endif
  for (i = 0; i < _PROCESSORS; i++) {
    if (i != _INDEX && rind[i]) {
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("IN %d waiting one from %d\n", _INDEX, i);
  fflush(stdout);
#endif
      shmem_int_wait(&(rflag[i]), 0);
    }
  }
#ifdef _SHMEM_PERMUTE_DEBUG
  sleep(_INDEX);
  printf("PROCESSOR %d\n", _INDEX);
  for (i = 0; i < _PROCESSORS; i++) {
    if (pm->rind[i]) {
      printf("  FROM PROCESSOR %d: ", i);
      printf("size = %d, # elts = %d, encoded = %d :: ", pm->rind[i][0], pm->rind[i][1], pm->rind[i][2]);
      for (j = 3; j < pm->rind[i][0]; j++) {
        printf("%d ", pm->rind[i][j]);
      }
      printf("\n");
    }
  }
  printf("\n");
  fflush(stdout);
  sleep(_PROCESSORS-_INDEX);
#endif
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
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("DS %d\n", _INDEX);
  fflush(stdout);
#endif
  pd->ldata = (char**)_zmalloc(_PROCESSORS*sizeof(char*), "perm ds ldata table");
  pd->rdata = (char**)_zmalloc(_PROCESSORS*sizeof(char*), "perm ds rdata table");
  _PERM_SetupDirect(pm, pd, scatter, eltsize, dst, src, reg, prilevels, dstsrcconflict, dstdead, srcdead, dstdirect, srcdirect);
}

void _PERM_DR(const _permmap* const pm, _permdata* const pd, const int scatter, _array_fnc dst, _array_fnc src) {
  const int one = 1;
  const int eltsize = pd->eltsize;
  int i;
  int * const restrict ldecnt = (int*)_zmalloc(_PROCESSORS*sizeof(int), "perm dr lcnt");
  int * const restrict rdecnt = (int*)_zmalloc(_PROCESSORS*sizeof(int), "perm dr rcnt");
  char * const restrict * const restrict ldata = pd->ldata;
  char * const restrict * const restrict rdata = pd->rdata;
  char * restrict * const restrict rptr = pm->rptr;
  int * restrict rflag = pm->rflag;
  char* addr;

#ifdef _SHMEM_PERMUTE_DEBUG
  printf("DR start %d\n", _INDEX);
  fflush(stdout);
  sleep(5);
#endif
  for (i=0; i<_PROCESSORS; i++) {
    ldecnt[i] = (scatter ? _PERM_LCNT(pm, i) : _PERM_RCNT(pm, i)) * eltsize;
    rdecnt[i] = (scatter ? _PERM_RCNT(pm, i) : _PERM_LCNT(pm, i)) * eltsize;
  }
  for (i = (_INDEX == _PROCESSORS - 1) ? 0 : _INDEX+1;
       i != _INDEX;
       i = (i == _PROCESSORS - 1) ? 0 : i++) {
    if (rdecnt[i] > 0) {
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("DR %d sending addr, %d, to %d\n", _INDEX, (int)&(rdata[i]), i);
  fflush(stdout);
  sleep(5);
#endif
      rflag[i] = 0;
      shmem_put((void*)&(rptr[_INDEX]), (void*)&(rdata[i]), 1, i);
    }
  }
  for (i = (_INDEX == 0) ? _PROCESSORS-1 : _INDEX-1;
       i != _INDEX;
       i = (i == 0) ? _PROCESSORS-1 : i--) {
    if (ldecnt[i] > 0) {
      shmem_wait((long*)&(rptr[i]), 0);
      addr = rptr[i];
      rptr[i] = 0;
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("DR %d waiting addr, %d, from %d\n", _INDEX, (int)addr, i);
  fflush(stdout);
  sleep(5);
#endif
      shmem_putmem(addr, ldata[i], ldecnt[i], i);
    }
  }
  shmem_fence();
  for (i = (_INDEX == 0) ? _PROCESSORS-1 : _INDEX-1;
       i != _INDEX;
       i = (i == 0) ? _PROCESSORS-1 : i--) {
    if (ldecnt[i] > 0) {
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("DR %d sending flag to %d\n", _INDEX, i);
  fflush(stdout);
  sleep(5);
#endif
      shmem_int_put(&(rflag[_INDEX]), &one, 1, i);
    }
  }
  if (ldecnt[_INDEX] > 0) {
    memcpy(rdata[_INDEX], ldata[_INDEX], ldecnt[_INDEX]);
  }
  _zfree(ldecnt, "perm dr lcnt");
  _zfree(rdecnt, "perm dr rcnt");
  pd->count = -1;
}

int _PERM_DN(const _permmap* const pm, _permdata* const pd, const int scatter) {
  int cnt = pd->count;
  int* restrict rflag = pm->rflag;

  while (++cnt < _PROCESSORS) {
    if (cnt != _INDEX) {
      const int size = pd->eltsize * (scatter ? _PERM_RCNT(pm, cnt) : _PERM_LCNT(pm, cnt));
      if (size) {
#ifdef _SHMEM_PERMUTE_DEBUG
      printf("DR %d waiting flag from %d\n", _INDEX, cnt);
#endif
        shmem_int_wait(&(rflag[cnt]), 0);
        pd->count = cnt;
        return cnt;
      }
    }
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
  shfree(pm->rptr);
  shfree(pm->rflag);
  _zfree(pm->procmap, "perm md procmap");
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
