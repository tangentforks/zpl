/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "direction.h"
#include "ensemble.h"
#include "zlib.h"

void _CreateOffsetEnsemble(_array_fnc copy,_array orig,int offset,
			   _direction dir,int copyIOfuncs) {
  int numdims;
  int i;
  char *origin;

  *copy = *orig;
  _ARR_DATA(copy) = ((char *)_ARR_DATA(copy)) + offset;
  if (dir) {
    numdims = _ARR_NUMDIMS(copy);
    origin = _ARR_DATA(copy);
    for (i=0;i<numdims;i++) {
      _ARR_OFF(copy,i) -= _DIRCOMP(dir,i);
      origin -= (_ARR_OFF(copy,i)*_ARR_BLK(copy,i));
    }
    _ARR_ORIGIN(copy) = origin;
  } else {
    _ARR_ORIGIN(copy) = ((char *)_ARR_ORIGIN(copy)) + offset;
  }
  if (!copyIOfuncs) {
    copy->Readfn = NULL;
    copy->Writefn = NULL;
  }
}
