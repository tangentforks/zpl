/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"


#define _GSR(t)           _Scan##t
#define GSR(t)            _GSR(t)
#define GLB_SCAN_ROUT     GSR(TYPE)

void GLB_SCAN_ROUT(TYPE *input,TYPE *output,_region R,int dim,int numelms,
		   int dir,int op,TYPE ident) {
  int i;

  for (i=0; i<numelms; i++) {
    output[i] = ident;
  }
}

