/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"


_bound _invisibleBounds[_MAXRANK];

_DIST_DECL( , _distribution_info, _distribution_fnc, _DefaultDistribution);

void _SetupDistribution(_distribution_fnc dist, _grid_fnc grid, char* label) {
  int i;

  _DIST_GRID(dist) = grid;
  _DIST_REGLIST(dist) = NULL;
  _GRID_REGISTER(grid, dist);
  _DIST_LABEL(dist) = label;
  for (i = 0; i < _MAXRANK; i++) {
    _DIST_DIMDIST(dist, i) = NULL;
  }
  _DIST_CLEAR_FLAG(dist);
  _DIST_SET_SETUP_ON(dist);
}


void _InitDistribution(_distribution_fnc dist, int process_arrays) {
  _grid grid = _DIST_GRID(dist);

  _InitDistributionCore(grid, dist, process_arrays, 1);
}

void _InitDistributionCore(_grid grid, _distribution_fnc dist, int process_arrays, int deep) {
  _registry* tmp;
  int i;

  if (!_GRID_GET_INIT(grid) || !_DIST_GET_SETUP(dist)) {
    _DIST_SET_INIT_OFF(dist);
    return;
  }

  _DIST_SET_INIT_ON(dist);

  if (_QueryVerboseHierarchy() && _INDEX == 0) {
    printf("Initializing Distribution %s\n", _DIST_LABEL(dist));
  }

  for (i = 0; i < _MAXRANK; i++) {
    if (_DIST_DIMDIST(dist, i)) {
      _DIMDIST_INIT(*_DIST_DIMDIST(dist, i))(dist, i);
    }
  }

  if (deep) {
    for (tmp = _DIST_REGLIST(dist); tmp != NULL; tmp = _DIST_REGLIST_NEXT(tmp)) {
      if (!_SPS_REGION(_DIST_REGLIST_REG(tmp))) {
	_InitRegion(_DIST_REGLIST_REG(tmp), process_arrays);
      }
    }
  }
}


void _print_distribution_hierarchy(_distribution dist) {
  _registry* tmp = _DIST_REGLIST(dist);

  _SCALAR_FPRINTF1(zout, "Distribution %s\n", _DIST_LABEL(dist));
  while (tmp) {
    _print_region_hierarchy(_DIST_REGLIST_REG(tmp));
    tmp = _DIST_REGLIST_NEXT(tmp);
  }
}


void _UpdateDistribution(_distribution D1, _distribution_nc D2, int preserve) {
  _registry* dtmp;
  _registry* rtmp;
  _array_action_list atmp;
  int found;

  for (dtmp = _DIST_REGLIST(D1); dtmp != NULL; dtmp = _DIST_REGLIST_NEXT(dtmp)) {
    for (rtmp = _REG_ARRLIST(_DIST_REGLIST_REG(dtmp)); rtmp != NULL; rtmp = _REG_ARRLIST_NEXT(rtmp)) {
      found = 0;
      for (atmp = _ACTION_LIST; atmp != NULL && !found; atmp = _ARR_ACTION_NEXT(atmp)) {
	if (_REG_ARRLIST_ARR(rtmp) == _ARR_ACTION_ARR(atmp)) {
	  found = 1;
	  _ARR_ACTION_DIST(atmp) = D2;
	  _ARR_ACTION_PRESERVE(atmp) = _ARR_ACTION_PRESERVE(atmp) & preserve;
	}
      }
      if (!found) {
	_array_action_list new = (_array_action_list)_zmalloc(sizeof(_array_action_list_info), "array action");
	_ARR_ACTION_GRID(new) = NULL;
	_ARR_ACTION_DIST(new) = D2;
	_ARR_ACTION_REG(new) = NULL;
	_ARR_ACTION_ARR(new) = _REG_ARRLIST_ARR(rtmp);
	_ARR_ACTION_PRESERVE(new) = preserve;
	_ARR_ACTION_NEXT(new) = _ACTION_LIST;
	_ACTION_LIST = new;
      }
    }
  }
}


void _CopyDistribution(_distribution_fnc D1, _distribution D2, int process_arrays) {
  int i;

  for (i = 0; i < _MAXRANK; i++) {
    if (_DIST_DIMDIST(D2, i)) {
      if (_DIST_DIMDIST(D1, i)) {
	_zfree(_DIST_DIMDIST(D1, i), "distribution function");
      }
      _DIST_DIMDIST(D1, i) = _DIMDIST_COPY(*_DIST_DIMDIST(D2, i))(_DIST_DIMDIST(D2, i));
    }
  }
  _InitDistribution(D1, process_arrays);
}


void _blkSetup(_distribution_fnc dist, int dim, int lo, int hi) {
  _DIST_LO(dist, dim) = lo;
  _DIST_HI(dist, dim) = hi;
  _DIMDIST_TO_PROC(*_DIST_DIMDIST(dist, dim)) = _blkToProc;
  _DIMDIST_INIT(*_DIST_DIMDIST(dist, dim)) = _blkInit;
  _DIMDIST_COPY(*_DIST_DIMDIST(dist, dim)) = _blkCopy;
}

void _blkInit(_distribution_fnc dist, int dim) {
  _grid grid = _DIST_GRID(dist);
  int lo = _DIST_LO(dist, dim);
  int hi = _DIST_HI(dist, dim);
  double bigval;

  /**
   ** what's all this grid stuff for?  if the grid is changed, it
   ** won't be reflected since lo and hi change; this is old code
   ** though, maybe it is only important for the implicit distribution
   ** in which case it's okay since the grid won't be changed.
   **/
  /** Actually, the thing that is now disconcerting is that the old values
   ** of lo and hi are lost so if the grid changes back, we may have a problem
   **/
  if (lo == INT_MAX && hi == INT_MIN) {
    lo = 1;
    hi = _GRID_SIZE(grid, dim);
  }
  if (_GRID_SIZE(grid, dim) > hi - lo + 1) {
    hi = lo + _GRID_SIZE(grid, dim) - 1;
  }
  _DIST_LO(dist, dim) = lo;
  _DIST_HI(dist, dim) = hi;
  bigval = (double)(hi - lo + 1) * _GRID_LOC(grid, dim);
  _DIST_MYLO(dist, dim) = _PROC_TO_DATA((bigval) / _GRID_SIZE(grid, dim)) + lo;
  bigval = (double)(hi - lo + 1) * (_GRID_LOC(grid, dim) + 1);
  _DIST_MYHI(dist, dim) = _PROC_TO_DATA((bigval) / _GRID_SIZE(grid, dim)) - 1 + lo;
}

_dimdist_info* _blkCopy(_dimdist_info* dimdist) {
  _dimdist_info* new = _zmalloc(sizeof(_blk_info), "distribution function");
  new->localBound = dimdist->localBound;
  new->globalBound = dimdist->globalBound;
  new->toProc = dimdist->toProc;
  new->init = dimdist->init;
  new->copy = dimdist->copy;
  return new;
}

int _blkToProc(_distribution dist, int dim, int i) {
  return ((int)((((double)(i) - _DIST_LO(dist, dim)) * _DIST_GRIDSIZE(dist, dim)) / _DIST_SIZE(dist, dim)));
}

void _cutSetup(_distribution_fnc dist, int dim, const int * const restrict a) {
  int i;
  _grid grid = _DIST_GRID(dist);
  const int numCuts = _GRID_SIZE(grid, dim) - 1;

  _DIST_LO(dist, dim) = INT_MIN>>1;
  _DIST_HI(dist, dim) = INT_MAX>>1;
  ((_cut)_DIST_DIMDIST(dist, dim))->numCuts = numCuts;
  if (numCuts) {
    ((_cut)_DIST_DIMDIST(dist, dim))->cuts = _zmalloc(sizeof(int) * numCuts, "cut distribution cuts array");
    for (i = 0; i < numCuts; i++) {
      ((_cut)_DIST_DIMDIST(dist, dim))->cuts[i] = a[i];
    }
  }
  _DIMDIST_TO_PROC(*_DIST_DIMDIST(dist, dim)) = _cutToProc;
  _DIMDIST_INIT(*_DIST_DIMDIST(dist, dim)) = _cutInit;
  _DIMDIST_COPY(*_DIST_DIMDIST(dist, dim)) = _cutCopy;
}

void _cutInit(_distribution_fnc dist, int dim) {
  _grid grid = _DIST_GRID(dist);
  const int * const restrict cuts = ((_cut)_DIST_DIMDIST(dist, dim))->cuts;

  if (_GRID_SIZE(grid, dim) == 1) {
    _DIST_MYLO(dist, dim) = INT_MIN>>1;
    _DIST_MYHI(dist, dim) = INT_MAX>>1;
  }
  else if (_GRID_LOC(grid, dim) == 0) {
    _DIST_MYLO(dist, dim) = INT_MIN>>1;
    _DIST_MYHI(dist, dim) = cuts[_GRID_LOC(grid, dim)];
  }
  else if (_GRID_LOC(grid, dim) == _GRID_SIZE(grid, dim) - 1) {
    _DIST_MYLO(dist, dim) = cuts[_GRID_LOC(grid, dim)-1]+1;
    _DIST_MYHI(dist, dim) = INT_MAX>>1;
  }
  else {
    _DIST_MYLO(dist, dim) = cuts[_GRID_LOC(grid, dim)-1]+1;
    _DIST_MYHI(dist, dim) = cuts[_GRID_LOC(grid, dim)];
  }
}

_dimdist_info* _cutCopy(_dimdist_info* dimdist) {
  int i;
  _cut_info* new = _zmalloc(sizeof(_cut_info), "distribution function");
  new->localBound = dimdist->localBound;
  new->globalBound = dimdist->globalBound;
  new->numCuts = ((_cut)dimdist)->numCuts;
  new->cuts = _zmalloc(sizeof(int) * new->numCuts, "cut distribution cuts array");
  for (i = 0; i < new->numCuts; i++) {
    new->cuts[i] = ((_cut)dimdist)->cuts[i];
  }
  new->toProc = dimdist->toProc;
  new->init = dimdist->init;
  new->copy = dimdist->copy;
  return (_dimdist_nc)new;
}

int _cutToProc(_distribution dist, int dim, int i) {
  const int numCuts = ((_cut)_DIST_DIMDIST(dist, dim))->numCuts;
  const int * const restrict cuts = ((_cut)_DIST_DIMDIST(dist, dim))->cuts;
  int j;

  if (numCuts == 0) {
    return 0;
  }
  for (j = 0; j < numCuts; j++) {
    if (i <= cuts[j]) {
      return j;
    }
  }
  return j;
}

void _gooferSetup(_distribution_fnc dist, int dim) {
  _DIST_LO(dist, dim) = INT_MIN>>1;
  _DIST_HI(dist, dim) = INT_MAX>>1;
  _DIMDIST_TO_PROC(*_DIST_DIMDIST(dist, dim)) = _gooferToProc;
  _DIMDIST_INIT(*_DIST_DIMDIST(dist, dim)) = _gooferInit;
  _DIMDIST_COPY(*_DIST_DIMDIST(dist, dim)) = _gooferCopy;
}

void _gooferInit(_distribution_fnc dist, int dim) {
  _grid grid = _DIST_GRID(dist);

  if (_GRID_SIZE(grid, dim) == 1) {
    _DIST_MYLO(dist, dim) = INT_MIN>>1;
    _DIST_MYHI(dist, dim) = INT_MAX>>1;
  }
  else if (_GRID_LOC(grid, dim) == 0) {
    _DIST_MYLO(dist, dim) = INT_MIN>>1;
    _DIST_MYHI(dist, dim) = 0;
  }
  else if (_GRID_LOC(grid, dim) == _GRID_SIZE(grid, dim) - 1) {
    _DIST_MYLO(dist, dim) = _GRID_LOC(grid, dim);
    _DIST_MYHI(dist, dim) = INT_MAX>>1;
  }
  else {
    _DIST_MYLO(dist, dim) = _GRID_LOC(grid, dim);
    _DIST_MYHI(dist, dim) = _GRID_LOC(grid, dim);
  }
}

_dimdist_info* _gooferCopy(_dimdist_info* dimdist) {
  return _blkCopy(dimdist);
}

int _gooferToProc(_distribution dist, int dim, int i) {
  _grid grid = _DIST_GRID(dist);

  if (i <= 0) {
    return 0;
  }
  else if (i >= _GRID_SIZE(grid, dim)-1) {
    return _GRID_SIZE(grid, dim)-1;
  }
  else {
    return i;
  }
}

void _fourferSetup(_distribution_fnc dist, int dim) {
  _DIST_LO(dist, dim) = INT_MIN/2;
  _DIST_HI(dist, dim) = INT_MAX/2;
  _DIMDIST_TO_PROC(*_DIST_DIMDIST(dist, dim)) = _fourferToProc;
  _DIMDIST_INIT(*_DIST_DIMDIST(dist, dim)) = _fourferInit;
  _DIMDIST_COPY(*_DIST_DIMDIST(dist, dim)) = _fourferCopy;
}

void _fourferInit(_distribution_fnc dist, int dim) {
  _grid grid = _DIST_GRID(dist);

  if (_GRID_SIZE(grid, dim) == 1) {
    _DIST_MYLO(dist, dim) = INT_MIN/2;
    _DIST_MYHI(dist, dim) = INT_MAX/2;
  }
  else if (_GRID_LOC(grid, dim) == 0) {
    _DIST_MYLO(dist, dim) = INT_MIN/2;
    _DIST_MYHI(dist, dim) = 3;
  }
  else if (_GRID_LOC(grid, dim) == _GRID_SIZE(grid, dim) - 1) {
    _DIST_MYLO(dist, dim) = _GRID_LOC(grid, dim) * 4;
    _DIST_MYHI(dist, dim) = INT_MAX/2;
  }
  else {
    _DIST_MYLO(dist, dim) = _GRID_LOC(grid, dim) * 4;
    _DIST_MYHI(dist, dim) = _GRID_LOC(grid, dim) * 4 + 3;
  }
}

_dimdist_info* _fourferCopy(_dimdist_info* dimdist) {
  return _blkCopy(dimdist);
}

int _fourferToProc(_distribution dist, int dim, int i) {
  _grid grid = _DIST_GRID(dist);

  if (i <= 3) {
    return 0;
  }
  else if (i >= (_GRID_SIZE(grid, dim) - 1) * 4) {
    return _GRID_SIZE(grid, dim) - 1;
  }
  else {
    return i / 4;
  }
}

