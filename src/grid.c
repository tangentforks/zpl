/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "md_zinc.h"
#include "grid.h"
#include "distribution.h"
#include "md_zlib.h"
#include "zlib.h"
#include "zplglobal.h"
#include "file_io.h"
#include "zerror.h"

_GRID_DECL( , _grid_info, _grid_fnc, _DefaultGrid);

int _PROCESSORS;
int _INDEX;

int _GRID_TO_PROC(_grid grid, const int nd, const int loc[]) {
  int index;
  int i;

  index = 0;
  for (i=0; i<nd; i++) {
    index += loc[i]*_GRID_BLK(grid, i);
  }
  return index;
}


int _GetSliceColor(_grid grid, int slicenum) {
  int i;
  int dimbit;
  int loc[_MAXRANK];

  if (slicenum == 0) {
    return 0;
  }

  dimbit = 1;
  for (i=0; i<_MAXRANK; i++) {
    if (dimbit & slicenum) {
      loc[i] = _GRID_LOC(grid, i);
    } else {
      loc[i] = 0;
    }
    dimbit = dimbit << 1;
  }

  return _GRID_TO_PROC(grid, _MAXRANK,loc);
}


int _BegOfSlice(_grid grid, int slicenum) {
  int dimbit;
  int i;
  int first = 1;

  dimbit = 1;
  for (i=0; i<_MAXDIM_GRID; i++) {
    if (dimbit & slicenum) {
    } else {
      if (!_GRID_BEG(grid, i)) {
	first = 0;
      }
    }
    dimbit = dimbit << 1;
  }
  return first;
}


int _EndOfSlice(_grid grid, int slicenum) {
  int dimbit;
  int i;
  int last = 1;

  dimbit = 1;
  for (i=0; i<_MAXDIM_GRID; i++) {
    if (dimbit & slicenum) {
    } else {
      if (!_GRID_END(grid, i)) {
	last = 0;
      }
    }
    dimbit = dimbit << 1;
  }
  return last;
}


int _NextProcSameSlice(_grid grid, int slicenum) {
  int dimbit;
  int i;
  int loc[_MAXRANK];
  int carry = 1;

  dimbit = 1 << (_MAXDIM_GRID-1);
  for (i=_MAXDIM_GRID-1; i>=0; i--) {
    if (dimbit & slicenum) {
      loc[i] = _GRID_LOC(grid, i);
    } else {
      if (carry) {
	if (_GRID_END(grid, i)) {
	  loc[i] = 0;
	} else {
	  loc[i] = _GRID_LOC(grid, i)+1;
	  carry = 0;
	}
      } else {
	loc[i] = _GRID_LOC(grid, i);
      }
    }

    dimbit = dimbit >> 1;
  }

  return _GRID_TO_PROC(grid, _MAXDIM_GRID,loc);
}


int _PosToProcInSlice(_grid grid, const int numdims,const int loc[],const int slicenum) {
  int dimbit;
  int i;
  int ind;
  int procs;

  if (slicenum == 0) {
    return _GRID_TO_PROC(grid, numdims,loc);
  } else {
    ind = 0;
    procs = 1;
    dimbit = 1 << (_MAXDIM_GRID-1);
    for (i=_MAXDIM_GRID-1; i>=0; i--) {
      if (dimbit & slicenum) {
      } else {
	ind += loc[i]*procs;
	procs *= _GRID_SIZE(grid, i);
      }
      dimbit = dimbit >> 1;
    }
   
    return ind;
  }
}


int _MaxProcInSlice(_grid grid, const int slicenum) {
  int dimbit;
  int i;
  int ind;

  if (slicenum == 0) {
    return _PROCESSORS-1;
  } else {
    ind = 1;
    dimbit = 1 << (_MAXDIM_GRID-1);
    for (i=_MAXDIM_GRID-1; i>=0; i--) {
      if (dimbit & slicenum) {
      } else {
	ind *= _GRID_SIZE(grid, i);
      }

      dimbit = dimbit >> 1;
    }
    return ind-1;
  }
}


int _MyLocInSlice(_grid grid, int slicenum) {
  return _PosToProcInSlice(grid, _MAXDIM_GRID, _GRID_LOC_V(grid), slicenum);
}


static int _GetNumSlices(void) {
  int i;
  int numslices;

  numslices = 1;
  for (i=0; i<_MAXDIM_GRID; i++) {
    numslices = numslices << 1;
  }
  numslices--;

  return numslices;
}


static void _InitSlices(_grid_fnc grid) {
  int i;
  int dim;
  int dimbit;
  int distributed;

  if (_GRID_DISTRIBUTED_V(grid)) {
    _zfree(_GRID_DISTRIBUTED_V(grid), "slice distributed vector");
  }
  _GRID_NUMSLICES(grid) = _GetNumSlices();
  _GRID_DISTRIBUTED_V(grid) = (int*)_zmalloc(_GRID_NUMSLICES(grid)*sizeof(int),
					      "slice distributed vector");
  for (i=0; i<_GRID_NUMSLICES(grid); i++) {
    dimbit = 1;
    distributed = 0;
    for (dim=0; dim<_MAXDIM_GRID; dim++) {
      if (i & dimbit) {
      } else {
	if (_GRID_SIZE(grid, dim) > 1) {
	  distributed = 1;
	}
      }
      dimbit = dimbit << 1;
    }
    _GRID_DISTRIBUTED(grid, i) = distributed;
  }
}

#define _MAXFACTORS 32

/*
 * factors remaining processors amongst unallocated dimensions
 */
static void factor(int val, int factor[], int numslots) {
  int initval;
  int numfactors;
  int i;
  int newfactor;
  int foundslot;

  if (val <= 1) {
    numfactors = 1;
    factor[0] = 1;
  } else {
    initval = val;
    numfactors = 0;
    for (i=2; i<=initval; i++) {
      while (val%i == 0) {
	factor[numfactors] = i;
	numfactors++;
	val/=i;
      }
    }
  }

  if (numfactors > _MAXFACTORS) {
    fprintf(stderr,"ERROR: Brad knows nothing about number theory\n");
    exit(1);
  }

  if (numfactors > numslots) {
    while (numfactors > numslots) {
      numfactors--;
      newfactor = factor[0] * factor[1];

      foundslot = 0;
      for (i=0; !foundslot && i<(numfactors-1); i++) {
	if (newfactor < factor[i+2]) {
	  factor[i] = newfactor;
	  foundslot = 1;
	} else {
	  factor[i] = factor[i+2];
	}
      }
      if (foundslot) {
	for ( ; i<numfactors; i++) {
	  factor[i] = factor[i+1];
	}
      } else {
	factor[numfactors-1] = newfactor;
      }
    }
  }

  if (numfactors < numslots) {
    while (numfactors < numslots) {
      numfactors++;
      factor[numfactors-1] = 1;
    }
  }
}


void _SetupGrid(_grid_fnc grid, char* label, int init[], int rank) {
  int i;

  _GRID_LABEL(grid) = label;
  _GRID_DISTLIST(grid) = NULL;
  _GRID_CLEAR_FLAG(grid);
  _GRID_SET_SETUP_ON(grid);
  _GRID_DIRPTR(grid) = NULL;
  _GRID_DISTRIBUTED_V(grid) = NULL;
  if (init) {
    for (i = 0; i < rank; i++) {
      _GRID_SIZE(grid, i) = init[i];
    }
    for (i = rank; i < _MAXRANK; i++) {
      _GRID_SIZE(grid, i) = 1;
    }
  }
  _MDSetupGrid(grid);
}


void _InitGrid(_grid_fnc grid, int process_arrays) {
  _InitGridCore(grid, process_arrays, 1);
}

void _InitGridCore(_grid_fnc grid, int process_arrays, int deep) {
  int i;
  int allocated;
  int factors[_MAXFACTORS];
  int unallocated;
  int procs;
  int index;
  _registry* tmp;

  allocated = 1;
  unallocated = 0;
  for (i = 0; i < _MAXRANK; i++) {
    if (_GRID_SIZE(grid, i) == 0) {
      unallocated++;
    }
    else {
      allocated *= _GRID_SIZE(grid, i);
    }
  }

  if (allocated > _PROCESSORS) {
    fprintf(stderr, "Allocated too many processors to grid\n");
    exit(1);
  }

  if (unallocated > 0) {
    factor(_PROCESSORS / allocated, factors, unallocated);
    for (i = 0; i < _MAXRANK; i++) {
      if (_GRID_SIZE(grid, i) == 0) {
	_GRID_SIZE(grid, i) = factors[unallocated - 1];
	unallocated--;
      }
    }
  }

  if (unallocated != 0) {
    fprintf(stderr, "Did not allocate processors to grid\n");
    exit(1);
  }

  procs = _PROCESSORS;
  index = _INDEX;
  for (i = 0; i < _MAXRANK; i++) {
    procs /= _GRID_SIZE(grid, i);
    _GRID_BLK(grid, i) = procs;
    _GRID_LOC(grid, i) = index / procs;
    index = index % procs;
  }
  /*
    for ( i = 0; i < _MAXRANK; i++) {
    printf("%d ", _GRID_SIZE(grid, i));
    }
    printf("\n");
  */
  _InitSlices(grid);

  _MDInitGrid(grid);
  _InitAtCommPerGrid(grid);

  _GRID_SET_INIT_ON(grid);

  if (_QueryVerboseHierarchy() && _INDEX == 0) {
    printf("Initializing Grid %s\n", _GRID_LABEL(grid));
  }

  if (deep) {
    for (tmp = _GRID_DISTLIST(grid); tmp != NULL; tmp = _GRID_DISTLIST_NEXT(tmp)) {
      _InitDistribution(_GRID_DISTLIST_DIST(tmp), process_arrays);
    }
  }
}


void _InitNodeStateVector(void) {
  int i;
  int procs;
  int index;

  procs = _PROCESSORS;
  index = _INDEX;
  for (i = 0; i < _MAXRANK; i++) {
    procs /= _GRID_SIZE(_DefaultGrid, i);

    _GRID_BLK(_DefaultGrid, i) = procs;
    _GRID_LOC(_DefaultGrid, i) = index / procs;

    index = index%procs;
  }
  _GRID_DIRPTR(_DefaultGrid) = NULL;
  _GRID_DISTRIBUTED_V(_DefaultGrid) = NULL;
  _GRID_LABEL(_DefaultGrid) = "Implicit Grid";

  _InitSlices(_DefaultGrid);
  _MDInitGrid(_DefaultGrid);
  _InitAtCommPerGrid(_DefaultGrid);
  _GRID_SET_SETUP_ON(_DefaultGrid);
  _GRID_SET_INIT_ON(_DefaultGrid);

}


void _PrintGridConfiguration(_grid grid) {
  int i;

  if (_INDEX == 0) {
    printf("Using %d processors\n",_PROCESSORS);
    if (_MAXDIM == 0) {
      printf("Computation is completely sequential\n");
    } else {
      if (_MAXDIM_GRID > 1) {
	printf("Using a %d",_GRID_SIZE(grid, 0));
	for (i=1; i<_MAXDIM; i++) {
	  printf(" x %d",_GRID_SIZE(grid, i));
	}
	if (i<_MAXDIM_GRID) {
	  printf(" (");
	  for ( ; i<_MAXDIM_GRID; i++) {
	    printf(" x %d",_GRID_SIZE(grid, i));
	  }
	  printf(" wasted)");
	}
	printf(" processor grid\n");
      }
    }
    fflush(stdout);
  }
}

void _print_grid_hierarchy(_grid grid) {
  _registry* tmp = _GRID_DISTLIST(grid);

  _SCALAR_FPRINTF1(zout, "Grid %s\n", _GRID_LABEL(grid));
  while (tmp) {
    _print_distribution_hierarchy(_GRID_DISTLIST_DIST(tmp));
    tmp = _GRID_DISTLIST_NEXT(tmp);
  }
}

void _UpdateGrid(_grid G1, _grid_nc G2, int preserve) {
  _registry* gtmp;
  _registry* dtmp;
  _registry* rtmp;
  _array_action_list atmp;
  int found;

  for (gtmp = _GRID_DISTLIST(G1); gtmp != NULL; gtmp = _GRID_DISTLIST_NEXT(gtmp)) {
    for (dtmp = _DIST_REGLIST(_GRID_DISTLIST_DIST(gtmp)); dtmp != NULL; dtmp = _DIST_REGLIST_NEXT(dtmp)) {
      for (rtmp = _REG_ARRLIST(_DIST_REGLIST_REG(dtmp)); rtmp != NULL; rtmp = _REG_ARRLIST_NEXT(rtmp)) {
	found = 0;
	for (atmp = _ACTION_LIST; atmp != NULL && !found; atmp = _ARR_ACTION_NEXT(atmp)) {
	  if (_REG_ARRLIST_ARR(rtmp) == _ARR_ACTION_ARR(atmp)) {
	    found = 1;
	    _ARR_ACTION_GRID(atmp) = G2;
	    _ARR_ACTION_PRESERVE(atmp) = _ARR_ACTION_PRESERVE(atmp) & preserve;
	  }
	}
	if (!found) {
	  _array_action_list new = (_array_action_list)_zmalloc(sizeof(_array_action_list_info), "array action");
	  _ARR_ACTION_GRID(new) = G2;
	  _ARR_ACTION_DIST(new) = NULL;
	  _ARR_ACTION_REG(new) = NULL;
	  _ARR_ACTION_ARR(new) = _REG_ARRLIST_ARR(rtmp);
	  _ARR_ACTION_PRESERVE(new) = preserve;
	  _ARR_ACTION_NEXT(new) = _ACTION_LIST;
	  _ACTION_LIST = new;
	}
      }
    }
  }
}

void _CopyGrid(_grid_fnc G1, _grid G2, int process_arrays) {
  int i;

  for (i = 0; i < _MAXRANK; i++) {
    _GRID_SIZE(G1, i) = _GRID_SIZE(G2, i);
  }
  _InitGrid(G1, process_arrays);
}
