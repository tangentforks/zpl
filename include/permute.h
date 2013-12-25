/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _PERMUTE_H_
#define _PERMUTE_H_

#define _PERM_SRCDIRECT 1
#define _PERM_DSTDIRECT 2
#define _PERM_SRCBUCKET 4
#define _PERM_DSTBUCKET 8
#define _PERM_UNODIRECT 16

#define _assignop_arbitrary =
#define _assignop_plus +=

extern _permmap** _pm;
extern int _numpermmaps;

/*** Permute Macros ***/

#define _PERM_LCNT(pm, i) (((pm)->lind[i]) ? (pm)->lind[i][1] : 0)
#define _PERM_RCNT(pm, i) (((pm)->rind[i]) ? (pm)->rind[i][1] : 0)

#define _PERM_LINDSIZE(pm, proc) (pm)->lind[proc][0]
#define _PERM_RINDSIZE(pm, proc) (pm)->rind[proc][0]

#define _LOOP0(counter, stop, stmt) \
  { \
    int counter; \
    for (counter = 0; counter < stop; counter++) { \
      stmt; \
    } \
  }

void _MI_PERM_Init(const int numpermmaps);
void _PERM_DeleteMaps(void);
int _PERM_DeleteLRUMap(void);
void _FastComputeRemapMap(_permmap* const restrict pm, int scatter, _region reg, _distribution dist, int* _index_maps);
int _PERM_StraightIStream(const int * const restrict, const int, const int, const int * const restrict);
void _PERM_CleanIndices(int * const restrict, int * const restrict,
			int * restrict * const restrict,
			int * restrict * const restrict,
			int **, int **);
void _PERM_INFinish(_permmap* const restrict, const int, const int, _array, _array, _region,
		    const int, const int, const int, const int, const int, const int);
void _PERM_SetupDirect(const _permmap* const, _permdata* const, const int, const int,
		       _array, _array, _region, const int, const int, const int,
		       const int, const int, const int);

#define _CALL_PERM_MS(pm, setup, reg, dims, num, filename, lineno) \
  if (_QueryVerboseRemap() && _INDEX == 0) { \
    printf("REMAP (%s:%d)\n", filename, lineno); \
  } \
  setup = 0; \
  if (!(*(pm))) { \
    setup = 1; \
    *(pm) = _PERM_MS(reg, dims); \
  } \
  else { \
    if (_QueryVerboseRemap() && _INDEX == 0) { \
      printf("REMAP OPT= MAP SAVE\n"); \
    } \
  }


#define _CALL_PERM_IR(pm, setup) \
  if (setup) _PERM_IR(pm)

#define _CALL_PERM_IN(pm, setup, scatter, eltsize, dst, src, reg, prilevels, dstsrcconflict, dstdead, srcdead, dstdirect, srcdirect) \
  if (setup) _PERM_IN(pm, scatter, eltsize, dst, src, reg, prilevels, dstsrcconflict, dstdead, srcdead, dstdirect, srcdirect)

#define _CALL_PERM_DS(num, pm, pd, scatter, eltsize, dst, src, reg, prilevels, dstsrcconflict, dstdead, srcdead, dstdirect, srcdirect) \
  _PERM_DS(pm, pd, scatter, eltsize, dst, src, reg, prilevels, dstsrcconflict, dstdead, srcdead, dstdirect, srcdirect)

#define _CALL_PERM_DR(pm, pd, scatter, dst, src) \
  _PERM_DR(pm, pd, scatter, dst, src)

#define _CALL_PERM_DN(pm, pd, scatter)

#define _CALL_PERM_DD(num, pm, pd, scatter) \
  _PERM_DD(pm, pd, scatter)

#define _CALL_PERM_MD(pm) \
  if (pm) _PERM_MD(pm); \
  pm = NULL

/***
 *** Permute Get Counts & Indices
 ***/
#define _PERM_FAST_GCI_COMP_1D(pm, setup, scatter, reg, dist, map1) \
  if (setup) { \
    int _index_maps[1] = {map1}; \
    _FastComputeRemapMap(pm, scatter, reg, dist, _index_maps); \
  }

#define _PERM_FAST_GCI_COMP_2D(pm, setup, scatter, reg, dist, map1, map2) \
  if (setup) { \
    int _index_maps[2] = {map1, map2}; \
    _FastComputeRemapMap(pm, scatter, reg, dist, _index_maps); \
  }

#define _PERM_FAST_GCI_COMP_3D(pm, setup, scatter, reg, dist, map1, map2, map3) \
  if (setup) { \
    int _index_maps[3] = {map1, map2, map3}; \
    _FastComputeRemapMap(pm, scatter, reg, dist, _index_maps); \
  }

#define _PERM_FAST_GCI_COMP_4D(pm, setup, scatter, reg, dist, map1, map2, map3, map4) \
  if (setup) { \
    int _index_maps[4] = {map1, map2, map3, map4}; \
    _FastComputeRemapMap(pm, scatter, reg, dist, _index_maps); \
  }

#define _PERM_FAST_GCI_COMP_5D(pm, setup, scatter, reg, dist, map1, map2, map3, map4, map5) \
  if (setup) { \
    int _index_maps[5] = {map1, map2, map3, map4, map5}; \
    _FastComputeRemapMap(pm, scatter, reg, dist, _index_maps); \
  }

#define _PERM_FAST_GCI_COMP_6D(pm, setup, scatter, reg, dist, map1, map2, map3, map4, map5, map6) \
  if (setup) { \
    int _index_maps[6] = {map1, map2, map3, map4, map5, map6}; \
    _FastComputeRemapMap(pm, scatter, reg, dist, _index_maps); \
  }

#define _PERM_GCI_BEGIN_1D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_GCI_BEGIN_2D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_GCI_BEGIN_3D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_GCI_BEGIN_4D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_GCI_BEGIN_5D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_GCI_BEGIN_6D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; int _proc5_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_GC_TOPROC(map, dim, uid) \
    _proc##dim##_##uid = _DIST_TO_PROC_BLK(unknown, _dist_##uid, map, dim)

#define _PERM_GC_1D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_GC_2D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_GC_3D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_GC_4D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_GC_5D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid + _proc4_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_GC_6D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid + _proc4_##uid + _proc5_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_GI_1D(direct, pm, prilevels, uid, map1) \
  _ie_nums[0] = map1; \
  _PERM_GI_1D_##direct(pm, prilevels, uid, map1)

#define _PERM_GI_2D(direct, pm, prilevels, uid, map1, map2) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; \
  _PERM_GI_2D_##direct(pm, prilevels, uid, map1, map2)

#define _PERM_GI_3D(direct, pm, prilevels, uid, map1, map2, map3) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; _ie_nums[2] = map3; \
  _PERM_GI_3D_##direct(pm, prilevels, uid, map1, map2, map3)

#define _PERM_GI_4D(direct, pm, prilevels, uid, map1, map2, map3, map4) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; _ie_nums[2] = map3; _ie_nums[3] = map4; \
  _PERM_GI_4D_##direct(pm, prilevels, uid, map1, map2, map3, map4)

#define _PERM_GI_5D(direct, pm, prilevels, uid, map1, map2, map3, map4, map5) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; _ie_nums[2] = map3; _ie_nums[3] = map4; _ie_nums[4] = map5; \
  _PERM_GI_5D_##direct(pm, prilevels, uid, map1, map2, map3, map4, map5)

#define _PERM_GI_6D(direct, pm, prilevels, uid, map1, map2, map3, map4, map5, map6) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; _ie_nums[2] = map3; _ie_nums[3] = map4; _ie_nums[4] = map5; _ie_nums[5] = map6; \
  _PERM_GI_6D_##direct(pm, prilevels, uid, map1, map2, map3, map4, map5, map6)

#define _PERM_GI_1D_1(pm, prilevels, uid, map1) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*1*2+0] = _i0; \
    } \
    (*pm)->lf[_proc_##uid*1*2+1+0] = _i0; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 1)

#define _PERM_GI_1D_0(pm, prilevels, uid, map1) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 1)

#define _PERM_GI_2D_1(pm, prilevels, uid, map1, map2) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*2*2+0] = _i0; (*pm)->lf[_proc_##uid*2*2+1] = _i1; \
    } \
    (*pm)->lf[_proc_##uid*2*2+2+0] = _i0; (*pm)->lf[_proc_##uid*2*2+2+1] = _i1; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 2)

#define _PERM_GI_2D_0(pm, prilevels, uid, map1, map2) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 2)

#define _PERM_GI_3D_1(pm, prilevels, uid, map1, map2, map3) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*3*2+0] = _i0; (*pm)->lf[_proc_##uid*3*2+1] = _i1; (*pm)->lf[_proc_##uid*3*2+2] = _i2; \
    } \
    (*pm)->lf[_proc_##uid*3*2+3+0] = _i0; (*pm)->lf[_proc_##uid*3*2+3+1] = _i1; (*pm)->lf[_proc_##uid*3*2+3+2] = _i2; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 3)

#define _PERM_GI_3D_0(pm, prilevels, uid, map1, map2, map3) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 3)

#define _PERM_GI_4D_1(pm, prilevels, uid, map1, map2, map3, map4) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*4*2+0] = _i0; (*pm)->lf[_proc_##uid*4*2+1] = _i1; (*pm)->lf[_proc_##uid*4*2+2] = _i2; (*pm)->lf[_proc_##uid*4*2+3] = _i3; \
    } \
    (*pm)->lf[_proc_##uid*4*2+4+0] = _i0; (*pm)->lf[_proc_##uid*4*2+4+1] = _i1; (*pm)->lf[_proc_##uid*4*2+4+2] = _i2; (*pm)->lf[_proc_##uid*4*2+4+3] = _i3; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 4)

#define _PERM_GI_4D_0(pm, prilevels, uid, map1, map2, map3, map4) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 4)

#define _PERM_GI_5D_1(pm, prilevels, uid, map1, map2, map3, map4, map5) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*5*2+0] = _i0; (*pm)->lf[_proc_##uid*5*2+1] = _i1; (*pm)->lf[_proc_##uid*5*2+2] = _i2; (*pm)->lf[_proc_##uid*5*2+3] = _i3; (*pm)->lf[_proc_##uid*5*2+4] = _i4; \
    } \
    (*pm)->lf[_proc_##uid*5*2+5+0] = _i0; (*pm)->lf[_proc_##uid*5*2+5+1] = _i1; (*pm)->lf[_proc_##uid*5*2+5+2] = _i2; (*pm)->lf[_proc_##uid*5*2+5+3] = _i3; (*pm)->lf[_proc_##uid*5*2+5+4] = _i4; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 5)

#define _PERM_GI_5D_0(pm, prilevels, uid, map1, map2, map3, map4, map5) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 5)

#define _PERM_GI_6D_1(pm, prilevels, uid, map1, map2, map3, map4, map5, map6) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*6*2+0] = _i0; (*pm)->lf[_proc_##uid*6*2+1] = _i1; (*pm)->lf[_proc_##uid*6*2+2] = _i2; (*pm)->lf[_proc_##uid*6*2+3] = _i3; (*pm)->lf[_proc_##uid*6*2+4] = _i4; (*pm)->lf[_proc_##uid*6*2+5] = _i5; \
    } \
    (*pm)->lf[_proc_##uid*6*2+6+0] = _i0; (*pm)->lf[_proc_##uid*6*2+6+1] = _i1; (*pm)->lf[_proc_##uid*6*2+6+2] = _i2; (*pm)->lf[_proc_##uid*6*2+6+3] = _i3; (*pm)->lf[_proc_##uid*6*2+6+4] = _i4; (*pm)->lf[_proc_##uid*6*2+6+5] = _i5; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 6)

#define _PERM_GI_6D_0(pm, prilevels, uid, map1, map2, map3, map4, map5, map6) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 6)

#define _PERM_GCI_END(pm, prplevels, prilevels, dims, uid) \
    _LOOP0(i, _PROCESSORS, _IE_STOP(_tind_##uid[i], _tbuf_##uid[i], prilevels, dims)); \
    _LOOP0(i, _PROCESSORS, (*pm)->lind[i] = _tind_##uid[i]); \
    _zfree(_tind_##uid, "temp indices pointers"); \
    _IE_STOP(_procmap_##uid, _procbuf_##uid, prplevels, 1); \
    (*pm)->procmap = _procmap_##uid; \
  }

/***
 *** Permute Get Counts & Indices (No ProcMap)
 ***/
#define _PERM_NP_GCI_BEGIN_1D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_NP_GCI_BEGIN_2D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_NP_GCI_BEGIN_3D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_NP_GCI_BEGIN_4D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_NP_GCI_BEGIN_5D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_NP_GCI_BEGIN_6D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; int _proc5_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)

#define _PERM_NP_GC_TOPROC(map, dim, uid) \
    _proc##dim##_##uid = _DIST_TO_PROC_BLK(unknown, _dist_##uid, map, dim)

#define _PERM_NP_GC_1D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid;

#define _PERM_NP_GC_2D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid;

#define _PERM_NP_GC_3D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid;

#define _PERM_NP_GC_4D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid;

#define _PERM_NP_GC_5D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid + _proc4_##uid;

#define _PERM_NP_GC_6D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid + _proc4_##uid + _proc5_##uid;

#define _PERM_NP_GI_1D(direct, pm, prilevels, uid, map1) \
  _ie_nums[0] = map1; \
  _PERM_GI_1D_##direct(pm, prilevels, uid, map1)

#define _PERM_NP_GI_2D(direct, pm, prilevels, uid, map1, map2) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; \
  _PERM_GI_2D_##direct(pm, prilevels, uid, map1, map2)

#define _PERM_NP_GI_3D(direct, pm, prilevels, uid, map1, map2, map3) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; _ie_nums[2] = map3; \
  _PERM_GI_3D_##direct(pm, prilevels, uid, map1, map2, map3)

#define _PERM_NP_GI_4D(direct, pm, prilevels, uid, map1, map2, map3, map4) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; _ie_nums[2] = map3; _ie_nums[3] = map4; \
  _PERM_GI_4D_##direct(pm, prilevels, uid, map1, map2, map3, map4)

#define _PERM_NP_GI_5D(direct, pm, prilevels, uid, map1, map2, map3, map4, map5) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; _ie_nums[2] = map3; _ie_nums[3] = map4; _ie_nums[4] = map5; \
  _PERM_GI_5D_##direct(pm, prilevels, uid, map1, map2, map3, map4, map5)

#define _PERM_NP_GI_6D(direct, pm, prilevels, uid, map1, map2, map3, map4, map5, map6) \
  _ie_nums[0] = map1; _ie_nums[1] = map2; _ie_nums[2] = map3; _ie_nums[3] = map4; _ie_nums[4] = map5; _ie_nums[5] = map6; \
  _PERM_GI_6D_##direct(pm, prilevels, uid, map1, map2, map3, map4, map5, map6)

#define _PERM_NP_GI_1D_1(pm, prilevels, uid, map1) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*1*2+0] = _i0; \
    } \
    (*pm)->lf[_proc_##uid*1*2+1+0] = _i0; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 1)

#define _PERM_NP_GI_1D_0(pm, prilevels, uid, map1) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 1)

#define _PERM_NP_GI_2D_1(pm, prilevels, uid, map1, map2) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*2*2+0] = _i0; (*pm)->lf[_proc_##uid*2*2+1] = _i1; \
    } \
    (*pm)->lf[_proc_##uid*2*2+2+0] = _i0; (*pm)->lf[_proc_##uid*2*2+2+1] = _i1; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 2)

#define _PERM_NP_GI_2D_0(pm, prilevels, uid, map1, map2) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 2)

#define _PERM_NP_GI_3D_1(pm, prilevels, uid, map1, map2, map3) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*3*2+0] = _i0; (*pm)->lf[_proc_##uid*3*2+1] = _i1; (*pm)->lf[_proc_##uid*3*2+2] = _i2; \
    } \
    (*pm)->lf[_proc_##uid*3*2+3+0] = _i0; (*pm)->lf[_proc_##uid*3*2+3+1] = _i1; (*pm)->lf[_proc_##uid*3*2+3+2] = _i2; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 3)

#define _PERM_NP_GI_3D_0(pm, prilevels, uid, map1, map2, map3) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 3)

#define _PERM_NP_GI_4D_1(pm, prilevels, uid, map1, map2, map3, map4) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*4*2+0] = _i0; (*pm)->lf[_proc_##uid*4*2+1] = _i1; (*pm)->lf[_proc_##uid*4*2+2] = _i2; (*pm)->lf[_proc_##uid*4*2+3] = _i3; \
    } \
    (*pm)->lf[_proc_##uid*4*2+4+0] = _i0; (*pm)->lf[_proc_##uid*4*2+4+1] = _i1; (*pm)->lf[_proc_##uid*4*2+4+2] = _i2; (*pm)->lf[_proc_##uid*4*2+4+3] = _i3; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 4)

#define _PERM_NP_GI_4D_0(pm, prilevels, uid, map1, map2, map3, map4) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 4)

#define _PERM_NP_GI_5D_1(pm, prilevels, uid, map1, map2, map3, map4, map5) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*5*2+0] = _i0; (*pm)->lf[_proc_##uid*5*2+1] = _i1; (*pm)->lf[_proc_##uid*5*2+2] = _i2; (*pm)->lf[_proc_##uid*5*2+3] = _i3; (*pm)->lf[_proc_##uid*5*2+4] = _i4; \
    } \
    (*pm)->lf[_proc_##uid*5*2+5+0] = _i0; (*pm)->lf[_proc_##uid*5*2+5+1] = _i1; (*pm)->lf[_proc_##uid*5*2+5+2] = _i2; (*pm)->lf[_proc_##uid*5*2+5+3] = _i3; (*pm)->lf[_proc_##uid*5*2+5+4] = _i4; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 5)

#define _PERM_NP_GI_5D_0(pm, prilevels, uid, map1, map2, map3, map4, map5) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 5)

#define _PERM_NP_GI_6D_1(pm, prilevels, uid, map1, map2, map3, map4, map5, map6) \
    if (!_tind_##uid[_proc_##uid]) { \
      (*pm)->lf[_proc_##uid*6*2+0] = _i0; (*pm)->lf[_proc_##uid*6*2+1] = _i1; (*pm)->lf[_proc_##uid*6*2+2] = _i2; (*pm)->lf[_proc_##uid*6*2+3] = _i3; (*pm)->lf[_proc_##uid*6*2+4] = _i4; (*pm)->lf[_proc_##uid*6*2+5] = _i5; \
    } \
    (*pm)->lf[_proc_##uid*6*2+6+0] = _i0; (*pm)->lf[_proc_##uid*6*2+6+1] = _i1; (*pm)->lf[_proc_##uid*6*2+6+2] = _i2; (*pm)->lf[_proc_##uid*6*2+6+3] = _i3; (*pm)->lf[_proc_##uid*6*2+6+4] = _i4; (*pm)->lf[_proc_##uid*6*2+6+5] = _i5; \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 6)

#define _PERM_NP_GI_6D_0(pm, prilevels, uid, map1, map2, map3, map4, map5, map6) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, 6)

#define _PERM_NP_GCI_END(pm, prplevels, prilevels, dims, uid) \
    _LOOP0(i, _PROCESSORS, _IE_STOP(_tind_##uid[i], _tbuf_##uid[i], prilevels, dims)); \
    _LOOP0(i, _PROCESSORS, (*pm)->lind[i] = _tind_##uid[i]); \
    _zfree(_tind_##uid, "temp indices pointers"); \
  }

/***
 *** Permute Get Counts & Indices (Squelch Indices)
 ***/
#define _PERM_NI_GCI_BEGIN_1D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NI_GCI_BEGIN_2D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NI_GCI_BEGIN_3D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NI_GCI_BEGIN_4D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NI_GCI_BEGIN_5D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NI_GCI_BEGIN_6D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; int _proc5_##uid; \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NI_GC_TOPROC(map, dim, uid) \
    _proc##dim##_##uid = _dist_##uid ? map * _DIST_GRIDBLK(_dist_##uid, dim) : map

#define _PERM_NI_GC_1D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_NI_GC_2D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_NI_GC_3D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_NI_GC_4D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_NI_GC_5D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid + _proc4_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_NI_GC_6D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid + _proc4_##uid + _proc5_##uid; \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)

#define _PERM_NI_GI(pm, uid) \
  _tind_##uid[_proc_##uid][1]++;

#define _PERM_NI_GCI_END(pm, prplevels, prilevels, dims, uid) \
    _LOOP0(i, _PROCESSORS, (*pm)->lind[i] = _tind_##uid[i]); \
    _zfree(_tind_##uid, "temp indices pointers"); \
    _IE_STOP(_procmap_##uid, _procbuf_##uid, prplevels, 1); \
    (*pm)->procmap = _procmap_##uid; \
  }

/***
 *** Permute Get Counts & Indices (No ProcMap && Squelch Indices)
 ***/
#define _PERM_NP_NI_GCI_BEGIN_1D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NP_NI_GCI_BEGIN_2D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NP_NI_GCI_BEGIN_3D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NP_NI_GCI_BEGIN_4D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NP_NI_GCI_BEGIN_5D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NP_NI_GCI_BEGIN_6D(pm, setup, uid, dist) \
  if (setup) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; int _proc5_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)

#define _PERM_NP_NI_GC_TOPROC(map, dim, uid) \
    _proc##dim##_##uid = _dist_##uid ? map * _DIST_GRIDBLK(_dist_##uid, dim) : map

#define _PERM_NP_NI_GC_1D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid;

#define _PERM_NP_NI_GC_2D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid;

#define _PERM_NP_NI_GC_3D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid;

#define _PERM_NP_NI_GC_4D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid;

#define _PERM_NP_NI_GC_5D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid + _proc4_##uid;

#define _PERM_NP_NI_GC_6D(pm, prplevels, uid) \
    _proc_##uid = _proc0_##uid + _proc1_##uid + _proc2_##uid + _proc3_##uid + _proc4_##uid + _proc5_##uid;

#define _PERM_NP_NI_GI(pm, uid) \
  _tind_##uid[_proc_##uid][1]++;

#define _PERM_NP_NI_GCI_END(pm, prplevels, prilevels, dims, uid) \
    _LOOP0(i, _PROCESSORS, (*pm)->lind[i] = _tind_##uid[i]); \
    _zfree(_tind_##uid, "temp indices pointers"); \
  }

/***
 *** Scatter Get Data / Gather Put Data
 ***
 ***  {_SCATTER_GD,_GATHER_PD}_PRE_MLOOP
 ***                                       compiler inserts mloop begin
 ***  {_SCATTER_GD,_GATHER_PD}_PRE_NLOOP
 ***                                       compiler inserts nloop begin if any
 ***  {_SCATTER_GD,_GATHER_PD}
 ***                                       compiler inserts nloop end if any
 ***  {_SCATTER_GD,_GATHER_PD}_POST_NLOOP
 ***                                       compiler inserts mloop end
 ***  {_SCATTER_GD,_GATHER_PD}_POST_MLOOP
 ***/
#define _SCATTER_GD_PRE_MLOOP(pm, pd, type, prplevels, uid) \
  if (!((*pd).optsvector & _PERM_SRCDIRECT)) { \
    type** _tmpptr = (type**)_zmalloc(sizeof(type*)*_PROCESSORS, "scatter pointers"); \
    const int * restrict const _procmap = (*pm)->procmap; \
    _LOOP0(i, _PROCESSORS, _tmpptr[i] = ((type**)(*pd).ldata)[i]); \
    _ID_START(_proc, _procmap, prplevels, 1, uid)

#define _SCATTER_GD_PRE_NLOOP(pm, pd, type, prplevels, uid) \
    _ID_READ(_proc, _procmap, prplevels, 1, uid)

#define _SCATTER_GD(src, pm, pd, prplevels, uid) \
    *_tmpptr[_proc1]++ = src

#define _SCATTER_GD_POST_NLOOP(pm, pd, type, prplevels, uid) \

#define _SCATTER_GD_POST_MLOOP(pm, pd, type, prplevels, uid) \
    _ID_STOP(_proc, _procmap, prplevels, 1, uid); \
    _zfree(_tmpptr, "scatter pointers"); \
  }

#define _GATHER_PD_PRE_MLOOP(pm, pd, type, prplevels, uid) \
  if (!((*pd).optsvector & _PERM_DSTDIRECT)) { \
    type** _tmpptr = (type**)_zmalloc(sizeof(type*)*_PROCESSORS, "gather pointers"); \
    const int * restrict const _procmap = (*pm)->procmap; \
    while (-1 != _PERM_DN(*pm, pd, 0)); \
    _LOOP0(i, _PROCESSORS, _tmpptr[i] = ((type**)(*pd).rdata)[i]); \
    _ID_START(_proc, _procmap, prplevels, 1, uid)

#define _GATHER_PD_PRE_NLOOP(pm, pd, type, prplevels, uid) \
    _ID_READ(_proc, _procmap, prplevels, 1, uid)

#define _GATHER_PD(dst, pm, pd, prplevels, uid) \
    dst = *_tmpptr[_proc1]++

#define _GATHER_PD_POST_NLOOP(pm, pd, type, prplevels, uid) \

#define _GATHER_PD_POST_MLOOP(pm, pd, type, prplevels, uid) \
    _ID_STOP(_proc, _procmap, prplevels, 1, uid); \
    _zfree(_tmpptr, "gather pointers"); \
  } \
  else { \
    while (-1 != _PERM_DN(*pm, pd, 0)); \
  }

/***
 *** Scatter Get Data / Gather Put Data (No ProcMap)
 ***
 ***  {_SCATTER_GD,_GATHER_PD}_PRE_MLOOP
 ***                                       compiler inserts mloop begin
 ***  {_SCATTER_GD,_GATHER_PD}_PRE_NLOOP
 ***                                       compiler inserts nloop begin if any
 ***  {_SCATTER_GD,_GATHER_PD}
 ***                                       compiler inserts nloop end if any
 ***  {_SCATTER_GD,_GATHER_PD}_POST_NLOOP
 ***                                       compiler inserts mloop end
 ***  {_SCATTER_GD,_GATHER_PD}_POST_MLOOP
 ***/
#define _SCATTER_NP_GD_PRE_MLOOP(pm, pd, type, prplevels, uid, dist) \
  if (!((*pd).optsvector & _PERM_SRCDIRECT)) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; int _proc5_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    type** _tmpptr = (type**)_zmalloc(sizeof(type*)*_PROCESSORS, "scatter pointers"); \
    _LOOP0(i, _PROCESSORS, _tmpptr[i] = ((type**)(*pd).ldata)[i]);

#define _SCATTER_NP_GD_PRE_NLOOP(pm, pd, type, prplevels, uid) \

#define _SCATTER_NP_GD(src, pm, pd, prplevels, uid) \
    *_tmpptr[_proc_##uid]++ = src

#define _SCATTER_NP_GD_POST_NLOOP(pm, pd, type, prplevels, uid) \

#define _SCATTER_NP_GD_POST_MLOOP(pm, pd, type, prplevels, uid) \
    _zfree(_tmpptr, "scatter pointers"); \
  }

#define _GATHER_NP_PD_PRE_MLOOP(pm, pd, type, prplevels, uid, dist) \
  if (!((*pd).optsvector & _PERM_DSTDIRECT)) { \
    int _proc0_##uid; int _proc1_##uid; int _proc2_##uid; int _proc3_##uid; int _proc4_##uid; int _proc5_##uid; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    type** _tmpptr = (type**)_zmalloc(sizeof(type*)*_PROCESSORS, "gather pointers"); \
    while (-1 != _PERM_DN(*pm, pd, 0)); \
    _LOOP0(i, _PROCESSORS, _tmpptr[i] = ((type**)(*pd).rdata)[i]);

#define _GATHER_NP_PD_PRE_NLOOP(pm, pd, type, prplevels, uid) \

#define _GATHER_NP_PD(dst, pm, pd, prplevels, uid) \
    dst = *_tmpptr[_proc_##uid]++

#define _GATHER_NP_PD_POST_NLOOP(pm, pd, type, prplevels, uid) \

#define _GATHER_NP_PD_POST_MLOOP(pm, pd, type, prplevels, uid) \
    _zfree(_tmpptr, "gather pointers"); \
  } \
  else { \
    while (-1 != _PERM_DN(*pm, pd, 0)); \
  }

/***
 *** Scatter Put Data / Gather Get Data
 ***
 ***  {_SCATTER_PD,_GATHER_GD}_PRE_NLOOP
 ***                                          compiler inserts nloop begin if any
 ***  {_SCATTER_PD,_GATHER_GD}
 ***                                          compiler inserts nloop end if any
 ***  {_SCATTER_PD,_GATHER_GD}_POST_NLOOP
 ***
 *** Loop optimization
 ***
 ***  {_SCATTER_PD,_GATHER_GD}_LOOP_PRE_NLOOP
 ***                                               compiler inserts nloop begin if any
 ***  {_SCATTER_PD,_GATHER_GD}_LOOP
 ***                                               compiler inserts nloop end if any
 ***  {_SCATTER_PD,_GATHER_GD}_LOOP_ELSE_NLOOP
 ***                                               compiler inserts nloop begin if any
 ***  {_SCATTER_PD,_GATHER_GD}_LOOP_BAIL
 ***                                               compiler inserts nloop end if any
 ***  {_SCATTER_PD,_GATHER_GD}_LOOP_POST_NLOOP
 ***/
#define _SCATTER_PD_PRE_NLOOP(dims, prilevels, type, pm, pd, uid) \
  if (!((*pd).optsvector & _PERM_DSTDIRECT)) { \
    int _proc; \
    _proc = _INDEX; \
    while (-1 != _proc) { \
      if ((*pm)->rind[_proc]) { \
        type* restrict _tmpdptr = ((type**)(*pd).rdata)[_proc]; \
        const int * restrict const _tindptr = (*pm)->rind[_proc]; \
        int _cnt; \
        const int _done = _tindptr[1]; \
        _ID_START(_tind, _tindptr, prilevels, dims, uid); \
        for (_cnt = 0; _cnt < _done; _cnt++) { \
          _ID_READ(_tind, _tindptr, prilevels, dims, uid)

#define _SCATTER_PD(dst, assignop, dims, prilevels, type, pm, pd, uid) \
            dst assignop *_tmpdptr++

#define _SCATTER_PD_POST_NLOOP(dims, prilevels, type, pm, pd, uid) \
        } \
        _ID_STOP(_tind, _tindptr, prilevels, dims, uid); \
      } \
      _proc = _PERM_DN(*pm, pd, 1); \
    } \
  } \
  else { \
    while (-1 != _PERM_DN(*pm, pd, 0)); \
  }

#define _GATHER_GD_PRE_NLOOP(dims, prilevels, type, pm, pd, uid) \
  if (!((*pd).optsvector & _PERM_SRCDIRECT)) { \
    int _proc; \
    for (_proc = 0; _proc < _PROCESSORS; _proc++) { \
      if ((*pm)->rind[_proc]) { \
        type* restrict _tmpdptr = ((type**)(*pd).ldata)[_proc]; \
        const int * restrict const _tindptr = (*pm)->rind[_proc]; \
        int _cnt; \
        const int _done = _tindptr[1]; \
        _ID_START(_tind, _tindptr, prilevels, dims, uid); \
        for (_cnt = 0; _cnt < _done; _cnt++) { \
          _ID_READ(_tind, _tindptr, prilevels, dims, uid)

#define _GATHER_GD(src, dims, prilevels, type, pm, pd, uid) \
          *_tmpdptr++ = src

#define _GATHER_GD_POST_NLOOP(dims, prilevels, type, pm, pd, uid) \
        } \
        _ID_STOP(_tind, _tindptr, prilevels, dims, uid); \
      } \
    } \
  }

#define _SCATTER_PD_LOOP_PRE_NLOOP(dims, prilevels, type, pm, pd, uid) \
  if (!((*pd).optsvector & _PERM_DSTDIRECT)) { \
    int _proc; \
    _proc = _INDEX; \
    while (-1 != _proc) { \
      if ((*pm)->rind[_proc]) { \
        type* restrict _tmpdptr = ((type**)(*pd).rdata)[_proc]; \
        const int * restrict const _tindptr = (*pm)->rind[_proc]; \
        _ID_LOOPSTART(_tind, _tindptr, prilevels, dims, uid); \
        _ID_LOOPREAD(_tind, _tindptr, prilevels, dims, uid)

#define _SCATTER_PD_LOOP(dst, assignop, dims, prilevels, type, pm, pd, uid) \
        dst assignop *_tmpdptr++

#define _SCATTER_PD_LOOP_ELSE_NLOOP(dims, prilevels, type, pm, pd, uid) \
        _ID_BAILEDLOOPELSE(_tind, _tindptr, prilevels, dims, uid); \
        _ID_BAILEDLOOPREAD(_tind, _tindptr, prilevels, dims, uid)

#define _SCATTER_PD_LOOP_BAIL(dst, assignop, dims, prilevels, type, pm, pd, uid) \
        dst assignop *_tmpdptr++

#define _SCATTER_PD_LOOP_POST_NLOOP(dims, prilevels, type, pm, pd, uid) \
        _ID_LOOPSTOP(_tind, _tindptr, prilevels, dims, uid); \
      } \
      _proc = _PERM_DN(*pm, pd, 1); \
    } \
  } \
  else { \
    while (-1 != _PERM_DN(*pm, pd, 0)); \
  }

#define _GATHER_GD_LOOP_PRE_NLOOP(dims, prilevels, type, pm, pd, uid) \
  if (!((*pd).optsvector & _PERM_SRCDIRECT)) { \
    int _proc; \
    for (_proc = 0; _proc < _PROCESSORS; _proc++) { \
      if ((*pm)->rind[_proc]) { \
        type* restrict _tmpdptr = ((type**)(*pd).ldata)[_proc]; \
        const int * restrict const _tindptr = (*pm)->rind[_proc]; \
        _ID_LOOPSTART(_tind, _tindptr, prilevels, dims, uid); \
        _ID_LOOPREAD(_tind, _tindptr, prilevels, dims, uid)

#define _GATHER_GD_LOOP(src, dims, prilevels, type, pm, pd, uid) \
        *_tmpdptr++ = src

#define _GATHER_GD_LOOP_ELSE_NLOOP(dims, prilevels, type, pm, pd, uid) \
        _ID_BAILEDLOOPELSE(_tind, _tindptr, prilevels, dims, uid); \
        _ID_BAILEDLOOPREAD(_tind, _tindptr, prilevels, dims, uid)

#define _GATHER_GD_LOOP_BAIL(src, dims, prilevels, type, pm, pd, uid) \
        *_tmpdptr++ = src

#define _GATHER_GD_LOOP_POST_NLOOP(dims, prilevels, type, pm, pd, uid) \
        _ID_LOOPSTOP(_tind, _tindptr, prilevels, dims, uid); \
      } \
    } \
  }

/***
 *** Scatter Put Data / Gather Get Data (Squelch Indices)
 ***
 ***  {_SCATTER_PD,_GATHER_GD}_PRE_NLOOP
 ***                                          compiler inserts nloop begin if any
 ***  {_SCATTER_PD,_GATHER_GD}
 ***                                          compiler inserts nloop end if any
 ***  {_SCATTER_PD,_GATHER_GD}_POST_NLOOP
 ***
 ***/
#define _SCATTER_NI_PD_PRE_NLOOP(dims, prilevels, type, pm, pd, uid) \
  if (!((*pd).optsvector & _PERM_DSTDIRECT)) { \
    int _proc; \
    _proc = _INDEX; \
    while (-1 != _proc) { \
      if ((*pm)->rind[_proc]) { \
        type* restrict _tmpdptr = ((type**)(*pd).rdata)[_proc]; \
        const int * restrict const _tindptr = (*pm)->rind[_proc]; \
        int _cnt; \
        const int _done = _tindptr[1]; \
        for (_cnt = 0; _cnt < _done; _cnt++) { \

#define _SCATTER_NI_PD(dst, assignop, dims, prilevels, type, pm, pd, uid) \
            dst assignop *_tmpdptr++

#define _SCATTER_NI_PD_POST_NLOOP(dims, prilevels, type, pm, pd, uid) \
        } \
      } \
      _proc = _PERM_DN(*pm, pd, 1); \
    } \
  } \
  else { \
    while (-1 != _PERM_DN(*pm, pd, 0)); \
  }

#define _GATHER_NI_GD_PRE_NLOOP(dims, prilevels, type, pm, pd, uid) \
  if (!((*pd).optsvector & _PERM_SRCDIRECT)) { \
    int _proc; \
    for (_proc = 0; _proc < _PROCESSORS; _proc++) { \
      if ((*pm)->rind[_proc]) { \
        type* restrict _tmpdptr = ((type**)(*pd).ldata)[_proc]; \
        const int * restrict const _tindptr = (*pm)->rind[_proc]; \
        int _cnt; \
        const int _done = _tindptr[1]; \
        for (_cnt = 0; _cnt < _done; _cnt++) { \

#define _GATHER_NI_GD(src, dims, prilevels, type, pm, pd, uid) \
          *_tmpdptr++ = src

#define _GATHER_NI_GD_POST_NLOOP(dims, prilevels, type, pm, pd, uid) \
        } \
      } \
    } \
  }

#endif
