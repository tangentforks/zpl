/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"

_array_action_list _aal = NULL;

_rmsframe _RMStack;

char* _ComputeEnsembleOrigin(_array arr) {
  char* origin;
  switch (_ARR_NUMDIMS(arr)) {
  case 1:
    origin = _ARR_COMP_ORIG(arr, 1);
    break;
  case 2:
    origin = _ARR_COMP_ORIG(arr, 2);
    break;
  case 3:
    origin = _ARR_COMP_ORIG(arr, 3);
    break;
  case 4:
    origin = _ARR_COMP_ORIG(arr, 4);
    break;
  case 5:
    origin = _ARR_COMP_ORIG(arr, 5);
    break;
  case 6:
    origin = _ARR_COMP_ORIG(arr, 6);
    break;
  default:
    origin = _ARR_COMP_ORIG(arr, 1);
    break;
  }
  return origin;
}

void _SetupEnsemble(_array_fnc arr,
		    _region_fnc reg, 
		    int elemsize,
		    char*(*fn)(_array, _vector),
		    void(*copier)(_region,_array,_array),
		    char* label,
		    int exists,  /* use realloc instead of alloc */
		    int dnr) { /* do not register */

  _ARR_DECL_REG(arr) = reg;
  _ARR_LABEL(arr) = label;
  _ARR_ELEMSIZE(arr) = elemsize;
  _ARR_NUMDIMS(arr) = _NUMDIMS(reg);
  if (!dnr) _REG_REGISTER(reg, arr);
  if (!exists) {
    _ARR_DATA(arr) = NULL;
  }
  _ARR_SET_ACCESSOR(arr, fn);
  _ARR_SET_COPY(arr, copier);
}


static void _InitEnsembleSparse(_region reg, _array_fnc arr) {
  const int rank = _ARR_NUMDIMS(arr);
  int lo[_MAXRANK], hi[_MAXRANK];
  int i;

  for (i = 0; i < rank; i++) {
    lo[i] = _REG_MYLO(reg, i);
    hi[i] = _REG_MYHI(reg, i);
  }
  _ARR_NUMDIMS(arr) = rank;
  _ARR_BLK(arr, rank-1) = _ARR_ELEMSIZE(arr);
  _ARR_DATA_SIZE(arr) = _ARR_BLK(arr, rank-1) * _SPS_NUMVALS(reg);
  for (i = 0; i < rank; i++) {
    if (_FLOODED_DIM(reg, i) || (lo[i] == hi[i])) {
      _ARR_BLK(arr, i) = 0;
    }
  }
  _ARR_DATA(arr) = (char *)_zmalloc(_ARR_DATA_SIZE(arr),"sparse array data");
  _ARR_ORIGIN(arr) = _ARR_DATA(arr);
  _ARR_SPS_ZERO_IDENT(arr);
  _ARR_TYPE(arr)="whichever";
  _ARR_W(arr)=NULL;
  _ARR_R(arr)=NULL;
}


void _InitEnsemble(_array_fnc arr) {
  _region reg = _ARR_DECL_REG(arr);
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  _InitEnsembleCore(grid, dist, reg, arr);
}


void _InitEnsembleCore(_grid grid, _distribution dist, _region reg, _array_fnc arr) {
  const int rank = _ARR_NUMDIMS(arr);
  int alo, ahi;
  int lo[_MAXRANK], hi[_MAXRANK];
  int i;
  int degenerate;
  int fluffinteresting;

  if (!_REG_GET_INIT(reg)) {
    return;
  }

  if (_QueryVerboseHierarchy() && _INDEX == 0) {
    if (_ARR_LABEL(arr)[0] != '_') {
      printf("Initializing Ensemble %s\n", _ARR_LABEL(arr));
    }
  }

  if (_SPS_REGION(reg)) {
    _InitEnsembleSparse(reg, arr);
    return;
  }

  fluffinteresting = 0;
  for (i=0; i<rank; i++) {
    if (_ARR_FLUFF_UP(arr, i) != 0 || _ARR_FLUFF_DOWN(arr, i) != 0) {
      fluffinteresting = 1;
    }
  }

  if (fluffinteresting == 0) {
    /* if there is no fluff, we simply use the region bounds */
    for (i=0; i<rank; i++) {
      lo[i] = _REG_MYLO(reg, i);
      hi[i] = _REG_MYHI(reg, i);
    }
  } else {
    /* if there is fluff, then we want to expand our distribution bounds by
       the fluff amounts (making sure not to wrap past INT_MIN/MAX)
       and intersect it with the array's declared region to see what
       bounds we should allocate.  It should be noted that even if a
       processor owns no indices of a region, it may need to allocate
       fluff for that region.

       Wrap-@'s present another weird case.  In this case, we spoof
       the region's bounds to make it appear larger than it truly is.
    */

    _reg_info fakereginfo;
    _region_fnc fakereg = &fakereginfo;
    int distlo;
    int disthi;

    for (i=0; i<rank; i++) {
      _REG_LO(fakereg, i) = _REG_LO(reg, i);
      _REG_HI(fakereg, i) = _REG_HI(reg, i);
      _REG_STRIDE(fakereg, i) = _REG_STRIDE(reg, i);
      _REG_ALIGN(fakereg, i) = _REG_ALIGN(reg, i);

      if (_ARR_FLUFF_WRAP(arr)) {
	_REG_LO(fakereg, i) -= _ARR_FLUFF_DOWN(arr, i);
	_REG_HI(fakereg, i) += _ARR_FLUFF_UP(arr, i);
      }
      _REG_GLOB_LO(fakereg, i) = _SNAP_UP(_REG_LO(fakereg, i), reg, i);
      _REG_GLOB_HI(fakereg, i) = _SNAP_DN(_REG_HI(fakereg, i), reg, i);

      distlo = _DIST_MYLO(dist, i);
      disthi = _DIST_MYHI(dist, i);
      if (distlo - _ARR_FLUFF_DOWN(arr, i) > distlo) {
	distlo = INT_MIN;
      } else {
	distlo = distlo - _ARR_FLUFF_DOWN(arr, i);
      }
      if (disthi + _ARR_FLUFF_UP(arr, i) < disthi) {
	disthi = INT_MAX;
      } else {
	disthi = disthi + _ARR_FLUFF_UP(arr, i);
      }

#ifdef DEBUG_FLUFF_DISTRIB
      printf("[%d] sending in %d..%d and %d..%d\n", _INDEX,
	     _REG_GLOB_LO(fakereg, i), _REG_GLOB_HI(fakereg, i),
	     distlo, disthi);
#endif
      
      _SetMyBlock(i, fakereg, distlo, disthi, grid);
      lo[i] = _REG_MYLO(fakereg, i);
      hi[i] = _REG_MYHI(fakereg, i);
#ifdef DEBUG_FLUFF_DISTRIB
      printf("[%d] I own %d..%d by %d of %d\n", _INDEX, lo[i], hi[i], 
	     _REG_STRIDE(reg, i), i);
#endif
    }
  }

  degenerate = 0;
  for (i = 0; i < rank; i++) {
    if (lo[i] > hi[i]) {
      degenerate = 1;
    }
  }
  if (!degenerate) {
    for (i = 0; i < rank; i++) {
      _ARR_STR(arr, i) = _REG_STRIDE(reg, i);
      _ARR_OFF(arr, i) = lo[i];
    }
    _ARR_BLK(arr, rank-1) = _ARR_ELEMSIZE(arr);
    for (i = rank - 1; i > 0; i--) {
      _ARR_BLK(arr, i-1) = _ARR_BLK(arr, i) *
	                   (((hi[i] - lo[i])/_ARR_STR(arr, i))+1);
    }
  } else {
    for (i = 0; i < rank; i++) {
      _ARR_STR(arr, i) = 1;
      _ARR_OFF(arr, i) = 0;
      _ARR_BLK(arr, i) = 0;
    }
  }
  _ARR_DATA_SIZE(arr) = _ARR_BLK(arr, 0) * ((hi[0] - lo[0])/_ARR_STR(arr, 0) + 1);
  for (i = 0; i < rank; i++) {
    if (_FLOODED_DIM(reg, i) || (lo[i] == hi[i])) {
      _ARR_BLK(arr, i) = 0;
    }
  }
  if (_ARR_DATA(arr) == NULL) {
    _ARR_DATA(arr) = (char *)_zmalloc(_ARR_DATA_SIZE(arr),"array data");
  }
  else {
    _zfree(_ARR_DATA(arr), "array data");
    _ARR_DATA(arr) = (char *)_zmalloc(_ARR_DATA_SIZE(arr),"array data");
  }
  _ARR_ORIGIN(arr) = _ComputeEnsembleOrigin(arr);
  _ARR_TYPE(arr)="whichever";
  _ARR_W(arr)=NULL;
  _ARR_R(arr)=NULL;
}


void _DestroyEnsemble(_array_fnc arr, int dnr) {
  if (!dnr) _REG_UNREGISTER(_ARR_REG(arr), arr);
  _zfree(_ARR_DATA(arr), "array data");
}


void _BOUND_CHECK(_region reg, _array arr, int lineno) {
  _region areg = _ARR_DECL_REG(arr);
  int i;

  for (i = 0; i < _NUMDIMS(reg); i++) {
    if (_REG_GLOB_LO(reg, i) < _REG_GLOB_LO(areg, i) ||
	_REG_GLOB_HI(reg, i) > _REG_GLOB_HI(areg, i)) {
      _SCALAR_FPRINTF1(zout, "Access out of bounds at line %d", lineno);
      _SCALAR_LINEFEED(zout);
      _ZPL_halt(lineno);
    }
  }
}


void _ProcessEnsembles() {
  _array_action_list tmp;
  int i;
  int compute_intersect;
  int copy;
  static char* ptr = NULL;

  if (_QueryVerboseHierarchy() && _INDEX == 0) {
    printf("PROCESSING ENSEMBLES\n");
  }
  tmp = _ACTION_LIST;
  while (tmp) {
    _array_action_list action = tmp;
    _grid_fnc grid = _ARR_ACTION_GRID(action)
      ? _ARR_ACTION_GRID(action)
      : _DIST_GRID(_REG_DIST(_ARR_DECL_REG(_ARR_ACTION_ARR(action))));
    _DIST_DECL( , _distribution_info, _distribution_fnc, dist);
    _REG_DECL( , _reg_info, _region_fnc, reg);
    _ENS_DECL( , _arr_info, _array_nc, arr);
    _array_nc arrold;

    _SetupDistribution(dist, grid, "Copy in ProcessEnsembles");
    if (_ARR_ACTION_DIST(action)) {
      _CopyDistribution(dist, _ARR_ACTION_DIST(action), 0);
    }
    else {
      _CopyDistribution(dist, _REG_DIST(_ARR_DECL_REG(_ARR_ACTION_ARR(action))), 0);
    }
    _REG_DIST(reg) = dist;
    _REG_LABEL(reg) = "Copy in ProcessEnsembles";
    _REG_ARRLIST(reg) = NULL;
    _REG_SET_SETUP_ON(reg);
    if (_ARR_ACTION_REG(action)) {
      _CopyRegion(reg, _ARR_ACTION_REG(action), 0);
      compute_intersect = 1;
    }
    else {
      _CopyRegion(reg, _ARR_DECL_REG(_ARR_ACTION_ARR(action)), 0);
      compute_intersect = 0;
    }
    if (!_ARR_ACTION_PRESERVE(action)) {
      arr = _ARR_ACTION_ARR(action);
      arrold = NULL;
    }
    else {
      arrold = _ARR_ACTION_ARR(action);
      for (i = 0; i < _ARR_NUMDIMS(arrold); i++) {
	_ARR_FLUFF_DOWN(arr, i) = _ARR_FLUFF_DOWN(arrold, i);
	_ARR_FLUFF_UP(arr, i) = _ARR_FLUFF_UP(arrold, i);
      }
      _ARR_FLUFF_WRAP(arr) = _ARR_FLUFF_WRAP(arrold);
      _SetupEnsemble(arr, reg, _ARR_ELEMSIZE(arrold), _ARR_ACCESSOR(arrold),
		     _ARR_COPIER(arrold), "Copy in ProcessEnsembles", 0, 1);
      if (ptr) /* delete -- fragment fix, this zfree should be later, no ptr, yuk! */
        _zfree(ptr, "array data");
    }
    _InitEnsembleCore(grid, dist, reg, arr);
    if (arrold) {
      _REG_DECL( , _reg_info, _region_nc, intersect);

      if (compute_intersect) {
	_REG_DIST(intersect) = dist;
	_REG_LABEL(intersect) = "Copy in ProcessEnsembles";
	_REG_ARRLIST(intersect) = NULL;
	_REG_SET_SETUP_ON(intersect);
	copy = _IntersectRegion(intersect, reg, _ARR_DECL_REG(_ARR_ACTION_ARR(action)));
      }
      else {
	intersect = reg;
	copy = 1;
      }
      if (copy) {
	_ARR_COPY(intersect, arr, arrold);
      }
      _InitEnsembleCore(grid, dist, reg, arrold);
      /* _zfree(_ARR_DATA(arrold), "array data"); save for next time, fragment fix */
      ptr = _ARR_DATA(arrold);
      _ARR_DATA(arrold) = _ARR_DATA(arr);
      _ARR_ORIGIN(arrold) = _ARR_ORIGIN(arr);
    }
    _GRID_UNREGISTER(grid, dist);
    for (i = 0; i < _MAXRANK; i++) {
      if (_DIST_DIMDIST(dist, i)) {
	_zfree(_DIST_DIMDIST(dist, i), "distribution function");
      }
    }
    tmp = _ARR_ACTION_NEXT(tmp);
    _zfree(action, "array action");
  }
  _ACTION_LIST = NULL;
}
