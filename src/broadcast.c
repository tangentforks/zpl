/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md_zinc.h"
#include "grid.h"
#include "ironman.h"
#include "ironman_utils.h"
#include "md_zlib.h"
#include "zlib.h"


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


void _Broadcast_dflt(_grid grid, IM_comminfo *dst,IM_comminfo *src,int rootproc,
		     int commid) {
  IM_memblock *mem;
  int msgsize;
  char *buffer;
  
  if (src != NULL) {
    if (_PROCESSORS != 1) {
      mem = &(src->mbvect[0]);
      if (src->numblocks == 1 && mem->numdims == 1 && 
	  mem->diminfo[0].numelems == 1) {
	_BroadcastSimple(grid, mem->baseptr,mem->elemsize,rootproc,commid);
      } else {
	buffer = _IMU_AllocBuffer(src,&msgsize);
	_IMU_Marshall(buffer,src);
	_BroadcastSimple(grid, buffer,msgsize,rootproc,commid);
	_IMU_FreeBuffer(buffer);
      }
    }
    if (dst != src) {
      GenericCopy(&(dst->mbvect[0]),mem);
    }
  } else if (dst != NULL) {
    mem = &(dst->mbvect[0]);
    if (dst->numblocks == 1 && mem->numdims == 1 && 
	mem->diminfo[0].numelems == 1) {
      _BroadcastSimple(grid, mem->baseptr,mem->elemsize,rootproc,commid);
    } else {
      buffer = _IMU_AllocBuffer(dst,&msgsize);
      _BroadcastSimple(grid, buffer,msgsize,rootproc,commid);
      _IMU_Unmarshall(dst,buffer);
      _IMU_FreeBuffer(buffer);
    }
  } else { /* uninvolved need to keep in synch w/ commids */
    if (_PROCESSORS != 1) {
      _BroadcastSimple(grid, NULL,0,rootproc,commid);
    }
  }
}


void _BroadcastToSlice_dflt(_grid grid, IM_comminfo* dst, IM_comminfo* src,
			    int rootproc, int involved, int commid, 
			    int slicenum) {
  IM_memblock *mem;
  int msgsize;
  char *buffer;
  
  if (src != NULL) {
    if (_PROCESSORS != 1) {
      mem = &(src->mbvect[0]);
      if (src->numblocks == 1 && mem->numdims == 1 && 
	  mem->diminfo[0].numelems == 1) {
	_BroadcastSimpleToSlice(grid, mem->baseptr, mem->elemsize, 1, rootproc,
				commid, slicenum);
      } else {
	buffer = _IMU_AllocBuffer(src,&msgsize);
	_IMU_Marshall(buffer,src);
	_BroadcastSimpleToSlice(grid, buffer, msgsize, 1,rootproc, commid, 
				slicenum);
	_IMU_FreeBuffer(buffer);
      }
    }
    if (dst != src) {
      GenericCopy(&(dst->mbvect[0]),mem);
    }
  } else if (dst != NULL) {
    mem = &(dst->mbvect[0]);
    if (dst->numblocks == 1 && mem->numdims == 1 && 
	mem->diminfo[0].numelems == 1) {
      _BroadcastSimpleToSlice(grid, mem->baseptr, mem->elemsize, 1, rootproc, 
			      commid, slicenum);
    } else {
      buffer = _IMU_AllocBuffer(dst,&msgsize);
      _BroadcastSimpleToSlice(grid, buffer,msgsize,1,rootproc,commid,slicenum);
      _IMU_Unmarshall(dst,buffer);
      _IMU_FreeBuffer(buffer);
    }
  } else { /* uninvolved need to keep in synch w/ commids */
    if (_PROCESSORS != 1) {
      _BroadcastSimpleToSlice(grid, NULL,0,1,rootproc,commid,slicenum);
    }
  }
}
