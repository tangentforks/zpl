/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* to get definition of NULL */

#include "md_zinc.h"
#include "priv_access.h"
#include "region.h"
#include "zc_proto.h"
#include "zerror.h"
#include "zlib.h"


void _InitDynamicRegion(_region_fnc DR) {
  int numdims;
  int i;
  int flooddim = 0;
  _distribution dist = _REG_DIST(DR);

  numdims = _NUMDIMS(DR);
  for (i=0;i<numdims;i++) {
    if (_FLOODED_DIM(DR,i)) {
      _REG_LO(DR,i) = _DIST_LO(dist, i);
      _REG_HI(DR,i) = _DIST_HI(dist, i);
      _REG_ALIGN(DR,i) = 0;
      _REG_STRIDE(DR,i) = 1;
      flooddim = 1;
    }
  }

  _SPS_REGION(DR) = 0;
  _REG_SET_SETUP_ON(DR);
  _InitRegion(DR, 0); /* no arrays to process */
  
  if (flooddim) {
    for (i=0;i<numdims;i++) {
      if (_FLOODED_DIM(DR,i)) {
	_REG_MYLO(DR,i) = _DIST_MYLO(dist, i);
	_REG_MYHI(DR,i) = _DIST_MYLO(dist, i);
      }
    }
  }
}


void _AdjustRegionWithOf(_region_fnc DR,_region prevR,_direction dir) {
  int numdims;
  int i;
  _PRIV_TID_DECL;

  _REG_DIST(DR) = _REG_DIST(prevR);
  numdims = _NUMDIMS(prevR);
  _NUMDIMS(DR) = numdims;
  _REG_PREP(DR) = _OF;
  _REG_PREP_DIR(DR) = dir;
  _REG_PREP_BASEREG(DR) = prevR;

  for (i=0;i<numdims;i++) {
    if (dir[i] == 0) {
      _REG_LO(DR,i) = _REG_LO(prevR,i);
      _REG_HI(DR,i) = _REG_HI(prevR,i);
      _FLOODED_DIM(DR,i) = _FLOODED_DIM(prevR,i);
    } else {
      if (dir[i] < 0) {
	if (_SPS_REGION(prevR)) {
	  /* of regions for sparse regions are degenerate */
	  _REG_LO(DR,i) = _REG_LO(prevR,i);
	  _REG_HI(DR,i) = _REG_LO(prevR,i)-1;
	} else {
	  _REG_LO(DR,i) = _REG_LO(prevR,i)+dir[i];
	  _REG_HI(DR,i) = _REG_LO(prevR,i)-1;
	}
      } else if (dir[i] > 0) {
	if (_SPS_REGION(prevR)) {
	  /* of regions for sparse regions are degenerate */
	  _REG_LO(DR,i) = _REG_HI(prevR,i)+1;
	  _REG_HI(DR,i) = _REG_HI(prevR,i);
	} else {
	  _REG_LO(DR,i) = _REG_HI(prevR,i)+1;
	  _REG_HI(DR,i) = _REG_HI(prevR,i)+dir[i];
	}
      }
      if (_FLOODED_DIM(prevR,i)) {
	_RT_ALL_FATAL0("Attempt to make an of region from a floodable region\n"
		       "in a floodable dimension.");
      }
      _FLOODED_DIM(DR,i) = 0;
    }
    _REG_ALIGN(DR,i) = _REG_ALIGN(prevR,i);
    _REG_STRIDE(DR,i) = _REG_STRIDE(prevR,i);
    _REG_GLOB_LO(DR,i) = _SNAP_UP(_REG_LO(DR,i),DR,i);
    _REG_GLOB_HI(DR,i) = _SNAP_DN(_REG_HI(DR,i),DR,i);
  }
}

void _AdjustRegionWithAt(_region_fnc DR,_region prevR,_direction dir) {
  int i;
  
  _REG_DIST(DR) = _REG_DIST(prevR);
  _NUMDIMS(DR) = _NUMDIMS(prevR);
  _REG_PREP(DR) = _AT;
  _REG_PREP_DIR(DR) = dir;
  _REG_PREP_BASEREG(DR) = prevR;
  
  for (i=0;i<_NUMDIMS(DR);i++) {
    _REG_LO(DR,i) = _REG_LO(prevR,i) + dir[i];
    _REG_HI(DR,i) = _REG_HI(prevR,i) + dir[i];
    _REG_STRIDE(DR,i) = _REG_STRIDE(prevR,i);
    _REG_ALIGN(DR,i) = _SNAP_POSNEG(_REG_ALIGN(prevR,i)+dir[i],_REG_STRIDE(DR,i));
    _FLOODED_DIM(DR,i) = _FLOODED_DIM(prevR,i);
    _REG_GLOB_LO(DR,i) = _SNAP_UP(_REG_LO(DR,i),DR,i);
    _REG_GLOB_HI(DR,i) = _SNAP_DN(_REG_HI(DR,i),DR,i);
  }
}


void _AdjustRegionWithIn(_region_fnc DR,_region prevR,_direction dir) {
  int i;

  _REG_DIST(DR) = _REG_DIST(prevR);
  _NUMDIMS(DR) = _NUMDIMS(prevR);
  _REG_PREP(DR) = _IN;
  _REG_PREP_DIR(DR) = dir;
  _REG_PREP_BASEREG(DR) = prevR;

  for (i=0;i<_NUMDIMS(DR);i++) {
    if (dir[i] == 0) {
      _REG_LO(DR,i) = _REG_LO(prevR,i);
      _REG_HI(DR,i) = _REG_HI(prevR,i);
    } else if (dir[i] < 0) {
      _REG_LO(DR,i) = _REG_LO(prevR,i);
      _REG_HI(DR,i) = _REG_LO(prevR,i)-(dir[i]+1);
    } else {
      _REG_LO(DR,i) = _REG_HI(prevR,i)-(dir[i]-1);
      _REG_HI(DR,i) = _REG_HI(prevR,i);
    }
    _REG_ALIGN(DR,i) = _REG_ALIGN(prevR,i);
    _REG_STRIDE(DR,i) = _REG_STRIDE(prevR,i);
    _FLOODED_DIM(DR,i) = _FLOODED_DIM(prevR,i);
    _REG_GLOB_LO(DR,i) = _SNAP_UP(_REG_LO(DR,i),DR,i);
    _REG_GLOB_HI(DR,i) = _SNAP_DN(_REG_HI(DR,i),DR,i);
  }
}


void _AdjustRegionWithPlus(_region_fnc DR,_region prevR,_direction dir) {
  int i;

  _REG_DIST(DR) = _REG_DIST(prevR);
  _NUMDIMS(DR) = _NUMDIMS(prevR);
  _REG_PREP(DR) = _PLUS;
  _REG_PREP_DIR(DR) = dir;
  _REG_PREP_BASEREG(DR) = prevR;

  for (i=0;i<_NUMDIMS(DR);i++) {
    if (dir[i] == 0) {
      _REG_LO(DR,i) = _REG_LO(prevR,i);
      _REG_HI(DR,i) = _REG_HI(prevR,i);
    } else if (dir[i] < 0) {
      _REG_LO(DR,i) = _REG_LO(prevR,i);
      _REG_HI(DR,i) = _REG_LO(prevR,i)-(dir[i]+1);
    } else {
      _REG_LO(DR,i) = _REG_HI(prevR,i)-(dir[i]-1);
      _REG_HI(DR,i) = _REG_HI(prevR,i);
    }
    _REG_ALIGN(DR,i) = _REG_ALIGN(prevR,i);
    _REG_STRIDE(DR,i) = _REG_STRIDE(prevR,i);
    _FLOODED_DIM(DR,i) = _FLOODED_DIM(prevR,i);
    _REG_GLOB_LO(DR,i) = _SNAP_UP(_REG_LO(DR,i),DR,i);
    _REG_GLOB_HI(DR,i) = _SNAP_DN(_REG_HI(DR,i),DR,i);
  }
}


void _AdjustRegionWithMinus(_region_fnc DR,_region prevR,_direction dir) {
  int i;

  _REG_DIST(DR) = _REG_DIST(prevR);
  _NUMDIMS(DR) = _NUMDIMS(prevR);
  _REG_PREP(DR) = _MINUS;
  _REG_PREP_DIR(DR) = dir;
  _REG_PREP_BASEREG(DR) = prevR;

  for (i=0;i<_NUMDIMS(DR);i++) {
    if (dir[i] == 0) {
      _REG_LO(DR,i) = _REG_LO(prevR,i);
      _REG_HI(DR,i) = _REG_HI(prevR,i);
    } else if (dir[i] < 0) {
      _REG_LO(DR,i) = _REG_LO(prevR,i);
      _REG_HI(DR,i) = _REG_LO(prevR,i)-(dir[i]+1);
    } else {
      _REG_LO(DR,i) = _REG_HI(prevR,i)-(dir[i]-1);
      _REG_HI(DR,i) = _REG_HI(prevR,i);
    }
    _REG_ALIGN(DR,i) = _REG_ALIGN(prevR,i);
    _REG_STRIDE(DR,i) = _REG_STRIDE(prevR,i);
    _FLOODED_DIM(DR,i) = _FLOODED_DIM(prevR,i);
    _REG_GLOB_LO(DR,i) = _SNAP_UP(_REG_LO(DR,i),DR,i);
    _REG_GLOB_HI(DR,i) = _SNAP_DN(_REG_HI(DR,i),DR,i);
  }
}


void _AdjustRegionWithBy(_region_fnc DR,_region prevR,_direction dir) {
  int i;
  int alignfrom;

  _REG_DIST(DR) = _REG_DIST(prevR);
  _NUMDIMS(DR) = _NUMDIMS(prevR);
  _REG_PREP(DR) = _BY;
  _REG_PREP_DIR(DR) = dir;
  _REG_PREP_BASEREG(DR) = prevR;

  for (i=0;i<_NUMDIMS(DR);i++) {
    _REG_LO(DR,i) = _REG_LO(prevR,i);
    _REG_HI(DR,i) = _REG_HI(prevR,i);
    if (dir[i] < 0) {
      _REG_STRIDE(DR,i) = -_REG_STRIDE(prevR,i)*dir[i];
      alignfrom = _REG_GLOB_HI(prevR,i);
    } else {  /* maybe 0 should cause a flood dimension? */
      _REG_STRIDE(DR,i) = _REG_STRIDE(prevR,i)*dir[i];
      alignfrom = _REG_GLOB_LO(prevR,i);
    }
    _FLOODED_DIM(DR,i) = _FLOODED_DIM(prevR,i);
    if (_FLOODED_DIM(DR,i)) {
      _REG_STRIDE(DR,i) = 1;
    }
    _REG_ALIGN(DR,i) = _SNAP_LO_TO_ALIGN(alignfrom,_REG_STRIDE(DR,i));
    _REG_GLOB_LO(DR,i) = _SNAP_UP(_REG_LO(DR,i),DR,i);
    _REG_GLOB_HI(DR,i) = _SNAP_DN(_REG_HI(DR,i),DR,i);
  }
}
