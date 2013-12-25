/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _DISTRIBUTION_H
#define _DISTRIBUTION_H

#include "struct.h"
#include "zplglobal.h"
#include "type.h"
#include "grid.h"
#include "ironman.h"
#include "registry.h"
#include "priv_access.h"

#define _PROC_TO_DATA(x)   ((int)((x)+(((double)(x))!=((double)((int)(x))))))
#define _DATA_TO_PROC(x)   ((int)(x))

#define _BND_LO(bnd) ((bnd).lo)
#define _BND_HI(bnd) ((bnd).hi)

#define _DIST_GRID(dist) (dist->grid)
#define _DIST_GRIDSIZE_V(dist) (_GRID_SIZE_V(_DIST_GRID(dist)))
#define _DIST_GRIDSIZE(dist, dim) (_GRID_SIZE(_DIST_GRID(dist), dim))
#define _DIST_GRIDLOC_V(dist) (_GRID_LOC_V(_DIST_GRID(dist)))
#define _DIST_GRIDLOC(dist, dim) (_GRID_LOC(_DIST_GRID(dist), dim))
#define _DIST_GRIDBLK_V(dist) (_GRID_BLK_V(_DIST_GRID(dist)))
#define _DIST_GRIDBLK(dist, dim) (_GRID_BLK(_DIST_GRID(dist), dim))
#define _DIST_DIMDIST(dist, dim) (dist->dimdist[dim])
#define _DIMDIST_LOCAL_LO(dimdist) (_BND_LO((dimdist).localBound))
#define _DIMDIST_LOCAL_HI(dimdist) (_BND_HI((dimdist).localBound))
#define _DIMDIST_TO_PROC(dimdist) ((dimdist).toProc)
#define _DIMDIST_INIT(dimdist) ((dimdist).init)
#define _DIMDIST_COPY(dimdist) ((dimdist).copy)
#define _DIMDIST_GLOBAL_LO(dimdist) (_BND_LO((dimdist).globalBound))
#define _DIMDIST_GLOBAL_HI(dimdist) (_BND_HI((dimdist).globalBound))
#define _DIMDIST_GLOBAL_SIZE(dimdist) (_DIMDIST_GLOBAL_HI(dimdist) - _DIMDIST_GLOBAL_LO(dimdist) + 1)

#define _DIST_MYLO(dist, dim) (_DIMDIST_LOCAL_LO(*_DIST_DIMDIST(dist, dim)))
#define _DIST_MYHI(dist, dim) (_DIMDIST_LOCAL_HI(*_DIST_DIMDIST(dist, dim)))
#define _DIST_LO(dist, dim) (_DIMDIST_GLOBAL_LO(*_DIST_DIMDIST(dist, dim)))
#define _DIST_HI(dist, dim) (_DIMDIST_GLOBAL_HI(*_DIST_DIMDIST(dist, dim)))
#define _DIST_SIZE(dist, dim) (_DIMDIST_GLOBAL_SIZE(*_DIST_DIMDIST(dist, dim)))

#define _DIST_LABEL(dist) (dist->distribution_label)

#define _DIST_INIT_BIT 01
#define _DIST_SETUP_BIT 02

#define _DIST_CLEAR_FLAG(dist)      (dist->distflag = 0)

#define _DIST_GET_INIT(dist)        (dist->distflag & _DIST_INIT_BIT)
#define _DIST_SET_INIT_ON(dist)     (dist->distflag |= _DIST_INIT_BIT)
#define _DIST_SET_INIT_OFF(dist)    (dist->distflag &= ~_DIST_INIT_BIT)

#define _DIST_GET_SETUP(dist)        (dist->distflag & _DIST_SETUP_BIT)
#define _DIST_SET_SETUP_ON(dist)     (dist->distflag |= _DIST_SETUP_BIT)
#define _DIST_SET_SETUP_OFF(dist)    (dist->distflag &= ~_DIST_SETUP_BIT)

#define _DIST_REGISTER(dist, reg) _DIST_REGLIST(dist) = _REGISTER(_DIST_REGLIST(dist), (void*)(reg))
#define _DIST_UNREGISTER(dist, reg) _DIST_REGLIST(dist) = _UNREGISTER(_DIST_REGLIST(dist), (void*)(reg))
#define _DIST_REGLIST(dist) ((dist)->regions)
#define _DIST_REGLIST_REG(drlist) ((_region_fnc)_REGISTRY_PTR(drlist))
#define _DIST_REGLIST_NEXT(drlist) (_REGISTRY_NEXT(drlist))

/** name is blk, cut, myblk, ... unknown if unknown **/
#define _DIST_TO_PROC(name, dist, i, dim) (_fast_##name##ToProc(dist, dim, i))
#define _DIST_TO_PROC_BLK(name, dist, i, dim) (_DIST_TO_PROC(name, dist, i, dim) * _DIST_GRIDBLK(dist, dim))

#define _fast_unknownToProc(dist, dim, i) (_DIMDIST_TO_PROC(*_DIST_DIMDIST(dist, dim))(dist, dim, i))

#define _MAXMINIMIZE_DIST(dist) \
  { \
    int i; \
    for (i=0; i<_MAXRANK; i++) { \
      _BND_LO(_invisibleBounds[i]) = INT_MAX; \
      _BND_HI(_invisibleBounds[i]) = INT_MIN; \
    } \
  }

typedef struct __bound {
  int lo; /* lower bound */
  int hi; /* upper bound */
} _bound;

struct __dimdist_info {
  _bound localBound;
  _bound globalBound;
  int (*toProc)(_distribution, int, int);
  void (*init)(_distribution_fnc, int);
  _dimdist_info* (*copy)(_dimdist_info*);
};

struct __distribution_info {
  int distflag;
  _grid_nc grid;                     /* distribution's grid          */
  _registry* regions;                /* distribution's regions       */
  _dimdist_info* dimdist[_MAXRANK]; /* distribution's distribution functions */
  char* distribution_label;                /* distribution label           */
  _MD_DISTRIBUTION_INFO                    /* machine dependent info */
};

extern _distribution_fnc _DefaultDistribution;

extern _bound _invisibleBounds[_MAXRANK];

#define _InitDistributionDimdist(dist, dim, dimdist) \
  _DIST_DIMDIST(dist, dim) = _zrealloc(_DIST_DIMDIST(dist, dim), sizeof(_##dimdist##_info), "distribution function")

#define _UPDATE_DISTRIBUTION(D1, D2, preserve) _UpdateDistribution(D1, D2, preserve)
#define _COPY_DISTRIBUTION(D1, D2, process_arrays) _CopyDistribution(D1, D2, process_arrays)

/*** Blk Distribution ***/

typedef struct {
  _bound localBound;
  _bound globalBound;
  int (*toProc)(_distribution, int, int);
  void (*init)(_distribution_fnc, int);
  _dimdist_info* (*copy)(_dimdist_info*);
} _blk_info;
typedef _blk_info* _blk;

void _blkSetup(_distribution_fnc dist, int dim, int lo, int hi);
void _blkInit(_distribution_fnc dist, int dim);
_dimdist_info* _blkCopy(_dimdist_info*);
int _blkToProc(_distribution dist, int dim, int i);

#define _fast_blkToProc(dist, dim, i) \
  (_DATA_TO_PROC((((double)(i) - _DIST_LO(dist, dim)) * _DIST_GRIDSIZE(dist, dim)) / _DIST_SIZE(dist, dim)))

/*** Cut Distribution ***/

typedef struct {
  _bound localBound;
  _bound globalBound;
  int (*toProc)(_distribution, int, int);
  void (*init)(_distribution_fnc, int);
  _dimdist_info* (*copy)(_dimdist_info*);
  int numCuts;
  int *cuts;
} _cut_info;
typedef _cut_info* _cut;

void _cutSetup(_distribution_fnc dist, int dim, const int * const restrict a);
void _cutInit(_distribution_fnc dist, int dim);
_dimdist_info* _cutCopy(_dimdist_info*);
int _cutToProc(_distribution dist, int dim, int i);

#define _fast_cutToProc(dist, dim, i) \
  (_cutToProc(dist, dim, i))

/*** Goofer Distribution ***/

typedef struct {
  _bound localBound;
  _bound globalBound;
  int (*toProc)(_distribution, int, int);
  void (*init)(_distribution_fnc, int);
  _dimdist_info* (*copy)(_dimdist_info*);
} _goofer_info;
typedef _goofer_info* _goofer;

void _gooferSetup(_distribution_fnc dist, int dim);
void _gooferInit(_distribution_fnc dist, int dim);
_dimdist_info* _gooferCopy(_dimdist_info*);
int _gooferToProc(_distribution dist, int dim, int i);

#define _fast_gooferToProc(dist, dim, i) \
  (_gooferToProc(dist, dim, i))

/*** Fourfer Distribution ***/

typedef struct {
  _bound localBound;
  _bound globalBound;
  int (*toProc)(_distribution, int, int);
  void (*init)(_distribution_fnc, int);
  _dimdist_info* (*copy)(_dimdist_info*);
} _fourfer_info;
typedef _fourfer_info* _fourfer;

void _fourferSetup(_distribution_fnc dist, int dim);
void _fourferInit(_distribution_fnc dist, int dim);
_dimdist_info* _fourferCopy(_dimdist_info*);
int _fourferToProc(_distribution dist, int dim, int i);

#define _fast_fourferToProc(dist, dim, i) \
  (_fourferToProc(dist, dim, i))

#endif
