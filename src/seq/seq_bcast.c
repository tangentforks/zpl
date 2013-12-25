/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "stand_inc.h"

static void GenericCopy(IM_memblock *dst,IM_memblock *src) {
  int j;
  _vector k = {0,0,0,0,0,0};  /* dependent on MAXRANK */
  int done;
  int done2;
  char *dstptr;
  char *srcptr;
  int elemsize;

  dstptr = (char *)dst->baseptr;
  srcptr = (char *)src->baseptr;
  elemsize = dst->elemsize;
  done = 0;
  while (!done) {
    memcpy(dstptr,srcptr,elemsize);

    j=0;
    done2=0;
    while (!done2) {
      k[j]++;
      if (k[j] == dst->diminfo[j].numelems) {
	k[j] = 0;
	if (j==dst->numdims-1) {
	  done=1;
	  done2=1;
	} else {
	  j++;
	}
      } else {
	done2=1;
      }
    }

    dstptr += dst->diminfo[j].stride;
    srcptr += src->diminfo[j].stride;
  }
}


void _Broadcast(_grid grid, IM_comminfo *dst,IM_comminfo *src,int rootproc,int commid) {
  if (dst != src) {
    GenericCopy(&(dst->mbvect[0]),&(src->mbvect[0]));
  }
}


void _BroadcastToSlice(_grid grid, IM_comminfo *dst,IM_comminfo *src,int rootproc,
		       int involved,int commid,int slicenum) {
  if (dst != src) {
    GenericCopy(&(dst->mbvect[0]),&(src->mbvect[0]));
  }
}
