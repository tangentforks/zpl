/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

changequote({{, }})dnl
changecom({{/*}}, {{*/}})dnl
define({{m4fordo}}, {{ifelse(eval({{$2}} <= {{$3}}), {{1}}, {{pushdef({{$1}}, {{$2}})_m4fordo({{$1}}, {{$2}}, {{$3}}, {{$4}})popdef({{$1}})}})}})dnl
define({{_m4fordo}}, {{$4{{}}ifelse($1, {{$3}}, , {{define({{$1}}, incr($1))_m4fordo({{$1}}, {{$2}}, {{$3}}, {{$4}})}})}})dnl
define({{m4fordotween}}, {{ifelse(eval({{$2}} <= {{$3}}), {{1}}, {{pushdef({{$1}}, {{$2}})_m4fordotween({{$1}}, {{$2}}, {{$3}}, {{$4}}, {{$5}})popdef({{$1}})}})}})dnl
define({{_m4fordotween}}, {{$4{{}}ifelse($1, {{$3}}, , {{$5{{}}define({{$1}}, incr($1))_m4fordotween({{$1}}, {{$2}}, {{$3}}, {{$4}}, {{$5}})}})}})dnl
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
m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_FAST_GCI_COMP_{{}}dims{{}}D(pm, setup, scatter, reg, dist, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }})) \
  if (setup) { \
    int _index_maps[dims] = {m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }})}; \
    _FastComputeRemapMap(pm, scatter, reg, dist, _index_maps); \
  }
}}, {{
}})dnl

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_GCI_BEGIN_{{}}dims{{}}D(pm, setup, uid, dist) \
  if (setup) { \
    m4fordotween({{dim}}, 0, eval(dims - 1), {{int _proc{{}}dim{{}}_##uid}}, {{; }}); \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)
}}, {{
}})dnl

#define _PERM_GC_TOPROC(map, dim, uid) \
    _proc##dim##_##uid = _DIST_TO_PROC_BLK(unknown, _dist_##uid, map, dim)

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_GC_{{}}dims{{}}D(pm, prplevels, uid) \
    _proc_##uid = m4fordotween({{dim}}, 0, eval(dims - 1), {{_proc{{}}dim{{}}_##uid}}, {{ + }}); \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)
}}, {{
}})dnl

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_GI_{{}}dims{{}}D(direct, pm, prilevels, uid, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }})) \
  m4fordotween({{dim}}, 0, eval(dims-1), {{_ie_nums[dim] = map{{}}eval(dim+1)}}, {{; }}); \
  _PERM_GI_{{}}dims{{}}D_##direct(pm, prilevels, uid, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }}))
}}, {{
}})dnl

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_GI_{{}}dims{{}}D_1(pm, prilevels, uid, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }})) \
    if (!_tind_##uid[_proc_##uid]) { \
      m4fordotween({{dim}}, 0, eval(dims-1), {{(*pm)->lf[_proc_##uid*dims*2+dim] = _i{{}}dim}}, {{; }}); \
    } \
    m4fordotween({{dim}}, 0, eval(dims-1), {{(*pm)->lf[_proc_##uid*dims*2+dims+dim] = _i{{}}dim}}, {{; }}); \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, dims)

#define _PERM_GI_{{}}dims{{}}D_0(pm, prilevels, uid, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }})) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, dims)
}}, {{
}})dnl

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
m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_NP_GCI_BEGIN_{{}}dims{{}}D(pm, setup, uid, dist) \
  if (setup) { \
    m4fordotween({{dim}}, 0, eval(dims - 1), {{int _proc{{}}dim{{}}_##uid}}, {{; }}); \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    int** _tbuf_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices buffers"); \
    int _ie_nums[_MAXRANK]; \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = NULL)
}}, {{
}})dnl

#define _PERM_NP_GC_TOPROC(map, dim, uid) \
    _proc##dim##_##uid = _DIST_TO_PROC_BLK(unknown, _dist_##uid, map, dim)

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_NP_GC_{{}}dims{{}}D(pm, prplevels, uid) \
    _proc_##uid = m4fordotween({{dim}}, 0, eval(dims - 1), {{_proc{{}}dim{{}}_##uid}}, {{ + }});
}}, {{
}})dnl

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_NP_GI_{{}}dims{{}}D(direct, pm, prilevels, uid, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }})) \
  m4fordotween({{dim}}, 0, eval(dims-1), {{_ie_nums[dim] = map{{}}eval(dim+1)}}, {{; }}); \
  _PERM_GI_{{}}dims{{}}D_##direct(pm, prilevels, uid, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }}))
}}, {{
}})dnl

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_NP_GI_{{}}dims{{}}D_1(pm, prilevels, uid, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }})) \
    if (!_tind_##uid[_proc_##uid]) { \
      m4fordotween({{dim}}, 0, eval(dims-1), {{(*pm)->lf[_proc_##uid*dims*2+dim] = _i{{}}dim}}, {{; }}); \
    } \
    m4fordotween({{dim}}, 0, eval(dims-1), {{(*pm)->lf[_proc_##uid*dims*2+dims+dim] = _i{{}}dim}}, {{; }}); \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, dims)

#define _PERM_NP_GI_{{}}dims{{}}D_0(pm, prilevels, uid, m4fordotween({{dim}}, 1, dims, {{map{{}}dim}}, {{, }})) \
    _IE_WRITE(_ie_nums, _tind_##uid[_proc_##uid], _tbuf_##uid[_proc_##uid], prilevels, dims)
}}, {{
}})dnl

#define _PERM_NP_GCI_END(pm, prplevels, prilevels, dims, uid) \
    _LOOP0(i, _PROCESSORS, _IE_STOP(_tind_##uid[i], _tbuf_##uid[i], prilevels, dims)); \
    _LOOP0(i, _PROCESSORS, (*pm)->lind[i] = _tind_##uid[i]); \
    _zfree(_tind_##uid, "temp indices pointers"); \
  }

/***
 *** Permute Get Counts & Indices (Squelch Indices)
 ***/
m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_NI_GCI_BEGIN_{{}}dims{{}}D(pm, setup, uid, dist) \
  if (setup) { \
    m4fordotween({{dim}}, 0, eval(dims - 1), {{int _proc{{}}dim{{}}_##uid}}, {{; }}); \
    int* _procmap_##uid = 0; \
    int* _procbuf_##uid = 0; \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)
}}, {{
}})dnl

#define _PERM_NI_GC_TOPROC(map, dim, uid) \
    _proc##dim##_##uid = _dist_##uid ? map * _DIST_GRIDBLK(_dist_##uid, dim) : map

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_NI_GC_{{}}dims{{}}D(pm, prplevels, uid) \
    _proc_##uid = m4fordotween({{dim}}, 0, eval(dims - 1), {{_proc{{}}dim{{}}_##uid}}, {{ + }}); \
    _IE_WRITE(&_proc_##uid, _procmap_##uid, _procbuf_##uid, prplevels, 1)
}}, {{
}})dnl

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
m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_NP_NI_GCI_BEGIN_{{}}dims{{}}D(pm, setup, uid, dist) \
  if (setup) { \
    m4fordotween({{dim}}, 0, eval(dims - 1), {{int _proc{{}}dim{{}}_##uid}}, {{; }}); \
    _distribution _dist_##uid = dist; \
    int _proc_##uid; \
    int** _tind_##uid = (int**)_zmalloc(_PROCESSORS*sizeof(int*), "temp indices pointers"); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i] = _zmalloc(2*sizeof(int), "counts")); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][0] = 2); \
    _LOOP0(i, _PROCESSORS, _tind_##uid[i][1] = 0)
}}, {{
}})dnl

#define _PERM_NP_NI_GC_TOPROC(map, dim, uid) \
    _proc##dim##_##uid = _dist_##uid ? map * _DIST_GRIDBLK(_dist_##uid, dim) : map

m4fordotween({{dims}}, 1, 6, {{dnl
#define _PERM_NP_NI_GC_{{}}dims{{}}D(pm, prplevels, uid) \
    _proc_##uid = m4fordotween({{dim}}, 0, eval(dims - 1), {{_proc{{}}dim{{}}_##uid}}, {{ + }});
}}, {{
}})dnl

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
    m4fordotween({{dim}}, 0, 5, {{int _proc{{}}dim{{}}_##uid}}, {{; }}); \
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
    m4fordotween({{dim}}, 0, 5, {{int _proc{{}}dim{{}}_##uid}}, {{; }}); \
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
