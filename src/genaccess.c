/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "md_zinc.h"
#include "access.h"
#include "ensemble.h"
#include "zerror.h"
#include "zlib.h"

void *_GenAccess(_array e,int *i) {
  int numdims;
  _PRIV_TID_DECL;

  numdims=_NUMDIMS(e);

  switch (numdims) {
  case 1:
    return _ACCESS_1D(e,i[0]);
  case 2:
    return _ACCESS_2D(e,i[0],i[1]);
  case 3:
    return _ACCESS_3D(e,i[0],i[1],i[2]);
  case 4:
    return _ACCESS_4D(e,i[0],i[1],i[2],i[3]);
  case 5:
    return _ACCESS_5D(e,i[0],i[1],i[2],i[3],i[4]);
  case 6:
    return _ACCESS_6D(e,i[0],i[1],i[2],i[3],i[4],i[5]);
  default:
    _RT_ANY_FATAL1("Can't handle %d-dimensional arrays in GenAccess",numdims);
    return NULL;
  }
}


void *_GenFastAccess(_array e,int *i) {
  int numdims;
  _PRIV_TID_DECL;

  numdims=_NUMDIMS(e);

  switch (numdims) {
  case 1:
    return _F_ACCESS_1D(e,i[0]);
  case 2:
    return _F_ACCESS_2D(e,i[0],i[1]);
  case 3:
    return _F_ACCESS_3D(e,i[0],i[1],i[2]);
  case 4:
    return _F_ACCESS_4D(e,i[0],i[1],i[2],i[3]);
  case 5:
    return _F_ACCESS_5D(e,i[0],i[1],i[2],i[3],i[4]);
  case 6:
    return _F_ACCESS_6D(e,i[0],i[1],i[2],i[3],i[4],i[5]);
  default:
    _RT_ANY_FATAL1("Can't handle %d-dimensional arrays in GenFastAccess",
		   numdims);
    return NULL;
  }
}


int _GenAccessDistance(_array e,int *lo,int *hi) {
  int numdims;
  _PRIV_TID_DECL;

  numdims = _NUMDIMS(e);
  switch (numdims) {
  case 1:
    return _ACCESS_1D_DISTANCE(e,lo[0],hi[0]);
  case 2:
    return _ACCESS_2D_DISTANCE(e,lo[0],lo[1],hi[0],hi[1]);
  case 3:
    return _ACCESS_3D_DISTANCE(e,lo[0],lo[1],lo[2],hi[0],hi[1],hi[2]);
  case 4:
    return _ACCESS_4D_DISTANCE(e,lo[0],lo[1],lo[2],lo[3],
              hi[0],hi[1],hi[2],hi[3]);
  case 5:
    return _ACCESS_5D_DISTANCE(e,lo[0],lo[1],lo[2],lo[3],lo[4],
              hi[0],hi[1],hi[2],hi[3],hi[4]);
  case 6:
    return _ACCESS_6D_DISTANCE(e,lo[0],lo[1],lo[2],lo[3],lo[4],lo[5],
              hi[0],hi[1],hi[2],hi[3],hi[4],hi[5]);
  default:
    _RT_ANY_FATAL1("Can't handle %d-dimensional arrays in GenAccessDistance",
		   numdims);
    return 0;
  }
}

