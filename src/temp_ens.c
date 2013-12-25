/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <string.h>
#include "md_zlib.h"
#include "region.h"
#include "ensemble.h"
#include "zlib.h"

void _ConstructTempEnsemble(_region_fnc reg,_array_fnc arr,
			    unsigned int elementsize) {
  int i;
  int offset;
  int numdims;

  numdims = _NUMDIMS(reg);

  _ARR_NUMDIMS(arr) = numdims;
  _ARR_ELEMSIZE(arr)=elementsize;
  _ARR_BLK(arr,numdims-1)=_ARR_ELEMSIZE(arr);
  _ARR_STR(arr,numdims-1)=_REG_STRIDE(reg,numdims-1);
  _ARR_OFF(arr,numdims-1)=_REG_MYLO(reg,numdims-1);
  for (i=numdims-2;i>=0;i--) {
    _ARR_BLK(arr,i)=_ARR_BLK(arr,i+1)*_REG_NUMELMS(reg,i+1);
    _ARR_STR(arr,i)=_REG_STRIDE(reg,i);
    _ARR_OFF(arr,i)=_REG_MYLO(reg,i);
  }
  _ARR_DECL_REG(arr)=reg;
  _ARR_DATA_SIZE(arr)=_ARR_BLK(arr,0)*_REG_NUMELMS(reg,0);
  _ARR_DATA(arr)=(void *)_zmalloc(_ARR_DATA_SIZE(arr),"temp ensemble data");

  offset = 0;
  for (i=0;i<numdims;i++) {
    offset += -_REG_MYLO(reg,i)*_ARR_BLK(arr,i);
  }
  _ARR_ORIGIN(arr)=(((char *)(_ARR_DATA(arr)))+offset);
  _ARR_W(arr) = NULL;
  _ARR_R(arr) = NULL;
}


void _DestroyTempEnsemble(_array arr) {
  _zfree(_ARR_DATA(arr),"temp ensemble data");
}
