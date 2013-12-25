/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <mpp/shmem.h>
#include "stand_inc.h"
#include "ironman_utils.h"
#include "shmem_scanred.h"
#include "zerror.h"

static int SYMbuffsize = 0;
static char *SYMbuffer = NULL;


static int _SYM_RequiredSize(IM_comminfo *comminfo) {
  int size;
  int maxsize;
  long* pSync;
  char* inp;
  char* outp;
  void* workspace;

  if (comminfo == NULL) {
    size = 0;
  } else {
    size = _IMU_RequiredSize(comminfo);
  }
  _GetRedBuffs(1,sizeof(int),&pSync,&inp,&outp,&workspace);
  shmem_int_max_to_all(&maxsize,&size,1,0,0,_PROCESSORS,workspace,pSync);

  return maxsize;
}


static char *_SYM_AllocBuffer(IM_comminfo *comminfo,int *buffsize) {
  *buffsize = _SYM_RequiredSize(comminfo);
  if (*buffsize > SYMbuffsize) {
    SYMbuffsize = *buffsize;
    SYMbuffer = shrealloc(SYMbuffer,*buffsize);
  }
  return SYMbuffer;
}


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


static void shmem_bcast_wrapper(void *ptr,unsigned int size,int root,int start,
				int stride,int procs,int involved) {
  long *pSync;

  pSync = _GetBcastSync();
  if (involved) {
    if (size%8 == 0) {
      size /= 8;
/*
      printf("[%d] shmem_broadcast64(0x%x,0x%x,%d,%d,%d,%d,%d,0x%x);\n",
	     _INDEX,ptr,ptr,size,root,start,stride,procs,pSync);
*/
      shmem_broadcast64(ptr,ptr,size,root,start,stride,procs,pSync);
    } else if (size%4 == 0) {
      size /= 4;
/*
   printf("[%d] shmem_broadcast32(0x%x,0x%x,%d,%d,%d,%d,%d,0x%x);\n",
	     _INDEX,ptr,ptr,size,root,start,stride,procs,pSync);
*/
      shmem_broadcast32(ptr,ptr,size,root,start,stride,procs,pSync);
    } else {
      _RT_ALL_FATAL0("Problem in broadcasts -- "
		     "contact zpl-bugs@cs.washington.edu\n");
    }
  }
}


/* All the "Simple" routines assume that ptr is aligned on all processors */
void _BroadcastSimple(_grid grid, void *ptr,unsigned int size,int rootproc,int commid) {
  if (_PROCESSORS != 1) {
    
    shmem_bcast_wrapper(ptr,size,rootproc,0,0,_PROCESSORS,1);
  }
}


void _BroadcastSimpleToSlice(_grid grid, void* ptr,unsigned int size,int involved,
			     int rootproc,int commid,int slicenum) {
  if (slicenum == 0) {
    _BroadcastSimple(grid,ptr,size,rootproc,commid);
  } else {
    _RT_ALL_FATAL0("_BroadcastSimpleToSlice() not yet implemented");
  }
}


void _Broadcast(_grid grid, IM_comminfo *dst,IM_comminfo *src,int rootproc,int commid) {
  IM_memblock *mem;
  int msgsize;
  char *buffer;
  
  if (src != NULL) {
    if (_PROCESSORS != 1) {
      mem = &(src->mbvect[0]);
      buffer = _SYM_AllocBuffer(src,&msgsize);
      _IMU_Marshall(buffer,src);
      _BroadcastSimple(grid,buffer,msgsize,rootproc,commid);
      _IMU_FreeBuffer(buffer);
    }
    if (dst != src) {
      GenericCopy(&(dst->mbvect[0]),mem);
    }
  } else if (dst != NULL) {
    if (_PROCESSORS != 1) {
      mem = &(dst->mbvect[0]);
      buffer = _SYM_AllocBuffer(dst,&msgsize);
      _BroadcastSimple(grid,buffer,msgsize,rootproc,commid);
      _IMU_Unmarshall(dst,buffer);
      _IMU_FreeBuffer(buffer);
    }
  } else { /* uninvolved need to keep in synch w/ commids */
    if (_PROCESSORS != 1) {
      _SYM_AllocBuffer(NULL,&msgsize);
      _BroadcastSimple(grid,NULL,0,rootproc,commid);
    }
  }
}
