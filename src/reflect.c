/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <memory.h>
#include <string.h>
#include "stand_inc.h"

void _ReflectI(_region R,_array * E,unsigned int * elemsize,int commid) {
  int numdims;
  _array * Eptr;
  unsigned int * elemptr;
  int i;
  int is[_MAXRANK];
  int id[_MAXRANK];

  _SetupWrapReflect(R, _REG_PREP_BASEREG(R), _REG_PREP_DIR(R));
  
  if (_I_REFLECT(R) != _NO_PROCESSOR) {
    numdims = _NUMDIMS(R);

    Eptr = E;
    elemptr = elemsize;
    while (*Eptr != NULL) {
      for (i=0;i<numdims;i++) {
	is[i] = _REFLECT_SRC_LO(R,i);
	id[i] = _REFLECT_DST_LO(R,i);
      }
      
      while (1) {
	
	memcpy(_GenAccess(*Eptr,id),_GenAccess(*Eptr,is),*elemptr);

	is[numdims-1] += _REFLECT_SRC_STEP(R,numdims-1);
	id[numdims-1] += _REFLECT_DST_STEP(R,numdims-1);
	for (i=numdims-1;i>0;i--) {
	  if (id[i]>_REFLECT_DST_HI(R,i)) {
	    is[i-1] += _REFLECT_SRC_STEP(R,i-1);
	    id[i-1] += _REFLECT_DST_STEP(R,i-1);
	    is[i] = _REFLECT_SRC_LO(R,i);
	    id[i] = _REFLECT_DST_LO(R,i);
	  } else {
	    break;
	  }
	}
	if (id[0] > _REFLECT_DST_HI(R,0)) {
	  break;
	}
      }
      Eptr++;
      elemptr++;
    }
  }
}
