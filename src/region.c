/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"


int _REG_TOTELMS(_region reg) {
  int i, numelms;

  for (numelms = 1, i = 0; i < _NUMDIMS(reg); i++) {
    numelms *= _REG_NUMELMS(reg, i);
  }
  return numelms;
}


static void _CalcRegionBounds(_region_fnc reg, int dim) {
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  if (dim >= _NUMDIMS(reg)) {
    _MIN_PROC(reg, dim) = 0;
    _MAX_PROC(reg, dim) = 0;
  } else {
    _MIN_PROC(reg, dim) = _DIST_TO_PROC(unknown, dist, _REG_GLOB_LO(reg, dim), dim);
    if (_MIN_PROC(reg, dim) >= _GRID_SIZE(grid, dim)) {
      _MIN_PROC(reg, dim) = _GRID_SIZE(grid, dim)-1;
      _MAX_PROC(reg, dim) = _GRID_SIZE(grid, dim)-1;
      return;
    } else if (_MIN_PROC(reg, dim) < 0) {
      _MIN_PROC(reg, dim) = 0;
    }
    _MAX_PROC(reg, dim) = _DIST_TO_PROC(unknown, dist, _REG_GLOB_HI(reg, dim), dim);
    if (_MAX_PROC(reg, dim) < 0) {
      _MAX_PROC(reg, dim)=0;
    } else if (_MAX_PROC(reg, dim) >= _GRID_SIZE(grid, dim)) {
      _MAX_PROC(reg, dim)=_GRID_SIZE(grid, dim) - 1;
    }
  }
}


void _SetMyBlock(int dim, _region_fnc reg, int distlo, int disthi, _grid grid) {
  const int lo = _REG_GLOB_LO(reg, dim);
  const int hi = _REG_GLOB_HI(reg, dim);
  int mylo;
  int myhi;

  if ((lo > disthi && !_GRID_END(grid, dim)) ||
      ((hi < distlo) && !_GRID_BEG(grid, dim))) {
    mylo = 0;
    myhi = -1;
  } else {
    if (lo < distlo && !_GRID_BEG(grid, dim)) {
      mylo = _SNAP_UP(distlo, reg, dim);
    } else {
      mylo = lo;
    }

    if (hi > disthi && !_GRID_END(grid, dim)) {
      myhi = _SNAP_DN(disthi, reg, dim);
    } else {
      myhi=hi;
    }

    if (mylo > myhi) {
      mylo = 0;
      myhi = -1;
    }
  }
  _REG_MYLO(reg, dim) = mylo;
  _REG_MYHI(reg, dim) = myhi;
}


static void _GrabDimension(_region_fnc reg,int dim) {
  _PRIV_TID_DECL;

  _REG_MYLO(reg, dim) = _REG_GLOB_LO(reg, dim);
  _REG_MYHI(reg, dim) = _REG_GLOB_HI(reg, dim);
}


static void _DistribDimension(_region_fnc reg,int dim) {
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  _PRIV_TID_DECL;

  _SetMyBlock(dim, reg, _DIST_MYLO(dist, dim), _DIST_MYHI(dist, dim), grid);
}


void _InitRegion(_region_fnc reg, int process_arrays) {
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  _InitRegionCore(grid, dist, reg, process_arrays, 1);
}

void _InitRegionCore(_grid grid, _distribution dist, _region_fnc reg, int process_arrays, int deep) {
   int numdims;
  int i;
  int igetnothing;
  int hi;

  if (!_DIST_GET_INIT(dist) || !_REG_GET_SETUP(reg)) {
    _REG_SET_INIT_OFF(reg);
    return;
  }

  if (_QueryVerboseHierarchy() && _INDEX == 0) {
    if (_REG_LABEL(reg)[0] != '_') {
      printf("Initializing Region %s\n", _REG_LABEL(reg));
    }
  }

  for (i=0;i<_NUMDIMS(reg);i++) {
    if (_FLOODED_DIM(reg,i)) {
      _SET_GLOB_BOUNDS_FLOOD(reg, i, _DIST_LO(dist, i), _DIST_HI(dist, i), _FLOODED_DIM(reg,i));
    }
  }
  for (i = 0; i < _MAXRANK; i++) {
    _CalcRegionBounds(reg,i);
  }
  numdims = _NUMDIMS(reg);
  for (i = 0; i < numdims; i++) {
    if (_FLOODED_DIM(reg, i)) {
      _REG_MYLO(reg, i) = _DIST_MYLO(dist, i);
      _REG_MYHI(reg, i) = _DIST_MYLO(dist, i);
      _REG_STRIDE(reg, i) = 1;
    } else if (_MIN_PROC(reg, i) == _MAX_PROC(reg, i)) {
      if (_MIN_PROC(reg, i) == _GRID_LOC(grid, i)) {
	_GrabDimension(reg, i);
      } else {
	_REG_MYLO(reg, i) = 0;
	_REG_MYHI(reg, i) = -1;
      }
    } else {
      _DistribDimension(reg, i);
    }
  }
  if (numdims < _MAXDIM) {
    igetnothing = 0;
    for (i = numdims; i < _MAXDIM; i++) {
      _REG_MYLO(reg, i) = 0;
      _REG_MYHI(reg, i) = -1;
      if (!_GRID_BEG(grid, i)) {
	igetnothing = 1;
      }
    }
    if (igetnothing) {
      for (i = 0; i < numdims; i++) {
	_REG_MYLO(reg, i) = 0;
	_REG_MYHI(reg, i) = -1;
      }
    }
  }
  _REG_I_OWN(reg) = 1;
  for (i = 0; i < numdims; i++) {
    if (_REG_MYLO(reg, i) > _REG_MYHI(reg, i)) {
      _REG_I_OWN(reg) = 0;
    }
  }

  _REG_SET_INIT_ON(reg);

  if (deep && process_arrays && !_SPS_REGION(reg)) {
    _registry* tmp;
    for (tmp = _REG_ARRLIST(reg); tmp != NULL; tmp = _REG_ARRLIST_NEXT(tmp)) {
      /*      _RT_ANY_WARN1("Initialized Array %s\n", _ARR_LABEL(_REG_ARRLIST_ARR(tmp)));*/
      _InitEnsemble(_REG_ARRLIST_ARR(tmp));
    }
  }
}


void _print_region_hierarchy(_region reg) {
  _registry* tmp = _REG_ARRLIST(reg);

  _SCALAR_FPRINTF1(zout, "Region %s\n", _REG_LABEL(reg));
  while (tmp) {
    _SCALAR_FPRINTF1(zout, "Array %s\n", _ARR_LABEL(_REG_ARRLIST_ARR(tmp)));
    tmp = _REG_ARRLIST_NEXT(tmp);
  }
}


void _UpdateRegion(_region R1, _region_nc R2, int preserve) {
  _registry* dtmp;
  _registry* rtmp;
  _array_action_list atmp;
  int found;

  for (rtmp = _REG_ARRLIST(R1); rtmp != NULL; rtmp = _REG_ARRLIST_NEXT(rtmp)) {
    found = 0;
    for (atmp = _ACTION_LIST; atmp != NULL && !found; atmp = _ARR_ACTION_NEXT(atmp)) {
      if (_REG_ARRLIST_ARR(rtmp) == _ARR_ACTION_ARR(atmp)) {
	found = 1;
	_ARR_ACTION_REG(atmp) = R2;
	_ARR_ACTION_PRESERVE(atmp) = _ARR_ACTION_PRESERVE(atmp) & preserve;
      }
    }
    if (!found) {
      _array_action_list new = (_array_action_list)_zmalloc(sizeof(_array_action_list_info), "array action");
      _ARR_ACTION_GRID(new) = NULL;
      _ARR_ACTION_DIST(new) = NULL;
      _ARR_ACTION_REG(new) = R2;
      _ARR_ACTION_ARR(new) = _REG_ARRLIST_ARR(rtmp);
      _ARR_ACTION_PRESERVE(new) = preserve;
      _ARR_ACTION_NEXT(new) = _ACTION_LIST;
      _ACTION_LIST = new;
    }
  }
}


void _CopyRegion(_region_fnc R1, _region R2, int process_arrays) {
  int i;

  _NUMDIMS(R1) = _NUMDIMS(R2);
  for (i = 0; i < _NUMDIMS(R2); i++) {
    _INHERIT_GLOB_BOUNDS(R1, R2, i);
  }
  _REG_PREP(R1) = _REG_PREP(R2);
  _REG_PREP_DIR(R1) = _REG_PREP_DIR(R2);
  _REG_PREP_BASEREG(R1) = _REG_PREP_BASEREG(R2);
  _REG_SET_SETUP_ON(R1);
  if (_SPS_REGION(R2)) {
    _RT_ALL_FATAL0("Sparse Region Cannot Be Changed");
  }
  else {
    _SPS_REGION(R1) = 0;
  }
  _InitRegion(R1, process_arrays);
}


/* Extended-Euclid
 * Source: Knuth Volume 2 --- Section 4.5.2
 *
 * Given two non-negative integers u and v
 * Returns gcd(u, v)
 * And sets x such that u*x + v*y = gcd(u, v)
 *
 */
static int _extended_euclid(int u, int v, int* x) {
  int u1, u2, u3;
  int v1, v2, v3;
  int t1, t2, t3;
  int q;

  u1 = 1;
  u2 = 0;
  u3 = u;
  v1 = 0;
  v2 = 1;
  v3 = v;
  while (v3 != 0) {
    q = u3 / v3;
    t1 = u1 - v1 * q;
    t2 = u2 - v2 * q;
    t3 = u3 - v3 * q;
    u1 = v1;
    u2 = v2;
    u3 = v3;
    v1 = t1;
    v2 = t2;
    v3 = t3;
  }
  *x = u1;
  return u3;
}


/* returns 1 if they intersect, R is the intersection of R1 and R2 */
int _IntersectRegion(_region_fnc R, _region R1, _region R2) {
  int i;

  if (_SPS_REGION(R1) || _SPS_REGION(R2)) {
    _RT_ALL_FATAL0("Sparse Region Cannot Be Intersected");
  }
  _NUMDIMS(R) = _NUMDIMS(R1);
  _REG_PREP(R) = _NONE;
  _REG_PREP_DIR(R) = NULL;
  _REG_PREP_BASEREG(R) = NULL;
  _REG_SET_SETUP_ON(R);
  _SPS_REGION(R) = 0;
  for (i = 0; i < _NUMDIMS(R); i++) {
    if (_FLOODED_DIM(R1, i)) {
      _INHERIT_GLOB_BOUNDS(R, R2, i);
    }
    else if (_FLOODED_DIM(R2, i)) {
      _INHERIT_GLOB_BOUNDS(R, R1, i);
    }
    else {
      int g, x;

      g = _extended_euclid(_REG_STRIDE(R1, i), _REG_STRIDE(R2, i), &x);
      if (abs(_REG_ALIGN(R1, i) - _REG_ALIGN(R2, i)) % g != 0) {
	return 0;
      }
      _REG_LO(R, i) = max(_REG_LO(R1, i), _REG_LO(R2, i));
      _REG_HI(R, i) = min(_REG_HI(R1, i), _REG_HI(R2, i));
      _REG_GLOB_LO(R, i) = max(_REG_GLOB_LO(R1, i), _REG_GLOB_LO(R2, i));
      _REG_GLOB_HI(R, i) = min(_REG_GLOB_HI(R1, i), _REG_GLOB_HI(R2, i));
      _REG_STRIDE(R, i) = _REG_STRIDE(R1, i) * _REG_STRIDE(R2, i) / g;
      _REG_ALIGN(R, i) = _REG_ALIGN(R1, i) + (_REG_ALIGN(R2, i) - _REG_ALIGN(R1, i)) * x * _REG_STRIDE(R1, i) / g;
      _FLOODED_DIM(R, i) = _FLOODED_DIM(R1, i); /** flood ??? **/
    }
  }

  _InitRegion(R, 0);
  return 1;
}
