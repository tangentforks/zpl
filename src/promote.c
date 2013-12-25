/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <string.h>
#include <limits.h>
#include "md_zinc.h"
#include "zplglobal.h"
#include "region.h"
#include "ensemble.h"
#include "zlib.h"

static void *_AccessPromotedScalar(_array arr,_vector i) {
  return _ARR_DATA(arr);
}

_array_nc _PromoteScalarToEnsemble(void *scalarptr,_array_fnc arr,
				   unsigned long scalar_size,_region_fnc reg) {
  int i;

  _ARR_NUMDIMS(arr) = _NUMDIMS(reg);
  _ARR_DECL_REG(arr) = reg;
  for (i=0;i<_MAXRANK;i++) {
    _ARR_BLK(arr,i)=0;
    _ARR_STR(arr,i)=1;
    _ARR_OFF(arr,i)=0;
  }
  _ARR_DATA_SIZE(arr)=scalar_size;
  _ARR_DATA(arr)=scalarptr;
  _ARR_DATA(arr)=_ARR_DATA(arr);
  _ARR_SET_ACCESSOR(arr,_AccessPromotedScalar);
  _ARR_W(arr) = NULL;
  _ARR_R(arr) = NULL;
  
  return arr;
}
