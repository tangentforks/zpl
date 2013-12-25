/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "md_zinc.h"
#include "ironman_utils.h"
#include "zerror.h"
#include "zlib.h"


void _IMU_PrintMemBlock(const IM_memblock* const memblock) {
  int numdims;
  int i;

  numdims = memblock->numdims;
  
  printf("----------------------\n");
  printf("%dD memblock:\n",numdims);
  printf("    base = 0x%x\n",(_ZPL_PTR_AS_INT)memblock->baseptr);
  printf("elemsize = %d  \n",memblock->elemsize);
  for (i=0; i<numdims; i++) {
    printf("dim %d: %d elements strided by %d\n",i,
	   memblock->diminfo[i].numelems,memblock->diminfo[i].stride);
  }
  printf("----------------------\n");
}


int _IMU_PackMemBlock(IM_memblock *given,IM_memblock *save) {
  int elemsize;
  int numdims;
  int r;
  int packed;
  int stride;
  int numelems;
  int w;
  int newnumelems;
  int newstride;
  int loc;
  int step;

  /*
  _IMU_PrintMemBlock(given);
  */

  save->baseptr = given->baseptr;
  elemsize = given->elemsize;
  numdims = given->numdims;

  /* as long as memory is packed tightly, count it up... */
  r = 0;
  packed = 1;
  while (packed && (r < numdims)) {
    numelems = given->diminfo[r].numelems;
    stride = given->diminfo[r].stride;
    if (numelems > 1) {
      if (stride == elemsize) {
	elemsize *= numelems;
	numelems = 1;
      } else {
	packed = 0;
      }
    }
    r++;
  }

  save->elemsize = elemsize;
  /* if it was all packed tightly, save info and return */
  if (packed) {
    save->numdims = 1;
    save->diminfo[0].numelems = 1;
    save->diminfo[0].stride = 0;
    return 1;
  }

  /* otherwise, finish up */
  w = 0;
  loc = (numelems-1)*stride;
  while (r < numdims) {
    newnumelems = given->diminfo[r].numelems;
    newstride = given->diminfo[r].stride;
    step = newstride-loc;
    loc += (newnumelems-1) * newstride;
    if (stride == step) {
      numelems *= newnumelems;
    } else {
      save->diminfo[w].numelems = numelems;
      save->diminfo[w].stride = stride;
      w++;
      numelems = newnumelems;
      stride = step;
    }
    r++;
  }

  save->diminfo[w].numelems = numelems;
  save->diminfo[w].stride = stride;
  w++;

  save->numdims = w;

  /*
  _IMU_PrintMemBlock(given);
  */

  return 0;
}


static void IMU_ConstrainedPackMemBlock(IM_memblock *given,IM_memblock *save,
					IM_memblock *constraint) {
  int i;
  int elemsize;
  int numdims;
  int r;
  int packed;
  int stride;
  int constride;
  int numelems;
  int w;
  int newnumelems;
  int newstride;
  int newconstride;
  int loc;
  int conloc;
  int step;
  int constep;
  _PRIV_TID_DECL;

  {
    int elem1;
    int elem2;

    elem1 = given->elemsize;
    elem2 = constraint->elemsize;

    if (elem1 != elem2) {
      _RT_ANY_FATAL3("[%d] Can't yet handle different element sizes (%d!=%d) "
		     "(ironman_utils.c)",_INDEX,elem1,elem2);
    }
  }

  save->baseptr = given->baseptr;
  elemsize = given->elemsize;
  numdims = given->numdims;

  for (i=0;i<numdims;i++) {
    if (given->diminfo[i].numelems != constraint->diminfo[i].numelems) {
      _RT_ANY_FATAL0("Can't yet handle different layouts (ironman_utils.c)");
    }
  }

  /* as long as memory is packed tightly, count it up... */
  r = 0;
  packed = 1;
  conloc = 0;
  while (packed && (r < numdims)) {
    numelems = given->diminfo[r].numelems;
    stride = given->diminfo[r].stride;
    constride = constraint->diminfo[r].stride;
    if (numelems > 1) {
      if (stride==elemsize && constride==elemsize) {
	elemsize *= numelems;
	numelems = 1;
      } else {
	packed = 0;
      }
    }
    r++;
  }

  save->elemsize = elemsize;
  constraint->elemsize = elemsize;
  /* if it was all packed tightly, save info and return */
  if (packed) {
    save->numdims = 1;
    save->diminfo[0].numelems = 1;
    save->diminfo[0].stride = 0;
    constraint->numdims = 1;
    constraint->diminfo[0].numelems = 1;
    constraint->diminfo[0].stride = 0;
    return;
  }

  /* otherwise, finish up */
  w = 0;
  loc = (numelems-1)*stride;
  conloc = (numelems-1)*constride;
  while (r < numdims) {
    newnumelems = given->diminfo[r].numelems;
    newstride = given->diminfo[r].stride;
    newconstride = constraint->diminfo[r].stride;
    step = newstride-loc;
    constep = newconstride-conloc;
    if (stride == step && constride == constep) {
      numelems *= newnumelems;
    } else {
      save->diminfo[w].numelems = numelems;
      constraint->diminfo[w].numelems = numelems;
      save->diminfo[w].stride = stride;
      constraint->diminfo[w].stride = constride;
      w++;
      numelems = newnumelems;
      stride = step;
      constride = constep;
    }
    r++;
  }
  save->diminfo[w].numelems = numelems;
  constraint->diminfo[w].numelems = numelems;
  save->diminfo[w].stride = stride;
  constraint->diminfo[w].stride = constride;
  w++;
	
  save->numdims = w;
  constraint->numdims = w;
}


static void IMU_CopyMemBlock(IM_memblock *dst,IM_memblock *src) {
  *(dst) = *(src);
}


int _IMU_RequiredSize(const IM_comminfo* const comminfo) {
  int size;
  int i;
  int elemsize;
  int j;

  size = 0;
  for (i=0;i<comminfo->numblocks;i++) {
    elemsize = comminfo->mbvect[i].elemsize;
    for (j=0;j<comminfo->mbvect[i].numdims;j++) {
      elemsize *= comminfo->mbvect[i].diminfo[j].numelems;
    }
    size += elemsize;
  }
  return size;
}


int _IMU_Pack(IM_comminfo *given,IM_comminfo *save) {
  int numblocks;
  int i;
  
  if (given == NULL) {
    save->procnum = -1;
  } else {
    save->procnum = given->procnum;
    numblocks = given->numblocks;
    save->numblocks = numblocks;

    if (numblocks == 1) {
      return _IMU_PackMemBlock(&(given->mbvect[0]),&(save->mbvect[0]));
    } else {
      for (i=0;i<numblocks;i++) {
	_IMU_PackMemBlock(&(given->mbvect[i]),&(save->mbvect[i]));
      }
    }
  }
  return 0;
}


void _IMU_ConstrainedPack(IM_memblock *given,IM_comminfo *save,
			  IM_comminfo *constraint) {
  int numblocks;
  int i;

  if (given == NULL) {
    save->procnum = -1;
  } else {
    numblocks = constraint->numblocks;
    save->numblocks = numblocks;

    for (i=0;i<numblocks;i++) {
      IMU_ConstrainedPackMemBlock(&(given[i]),&(save->mbvect[i]),
				  &(constraint->mbvect[i]));
    }
  }
}


void _IMU_Copy(IM_comminfo *given,IM_comminfo *save) {
  int numblocks;
  int i;

  if (given == NULL) {
    save->procnum = -1;
  } else {
    save->procnum = given->procnum;
    numblocks = given->numblocks;
    save->numblocks = numblocks;
    
    for (i=0;i<numblocks;i++) {
      IMU_CopyMemBlock(&(save->mbvect[i]),&(given->mbvect[i]));
    }
  }
}


char *_IMU_AllocBuffer(const IM_comminfo* const comminfo,int* const buffsize) {
  *buffsize = _IMU_RequiredSize(comminfo);
  return (char *)_zmalloc(*buffsize,"imu buffer");
}


void _IMU_FreeBuffer(char *buffer) {
  _zfree(buffer,"imu buffer");
}


void _IMU_Marshall(char* const buffer,const IM_comminfo* const comminfo) {
  char *buffptr;
  int i;
  char *dataptr;
  int numdims;
  int j;
  int k[IM_MAXRANK];
  int elemsize;

  buffptr = buffer;
  for (i=0;i<comminfo->numblocks;i++) {
    dataptr = (char *)comminfo->mbvect[i].baseptr;
    numdims = comminfo->mbvect[i].numdims;
    for (j=0;j<numdims;j++) {
      k[j]=0;
    }
    elemsize = comminfo->mbvect[i].elemsize;
    while (k[numdims-1] < comminfo->mbvect[i].diminfo[numdims-1].numelems) {
      memcpy(buffptr,dataptr,elemsize);
      buffptr += elemsize;
      
      k[0]++;
      for (j=0;j<numdims-1;j++) {
	if (k[j] == comminfo->mbvect[i].diminfo[j].numelems) {
	  k[j] = 0;
	  k[j+1]++;
	} else {
	  break;
	}
      }
      dataptr += comminfo->mbvect[i].diminfo[j].stride;
    }
  }
}


void _IMU_Unmarshall(const IM_comminfo* const comminfo,
		     const char* const buffer) {
  const char* buffptr;
  int i;
  char *dataptr;
  int numdims;
  int j;
  int k[IM_MAXRANK];
  int elemsize;

  buffptr = buffer;
  for (i=0;i<comminfo->numblocks;i++) {
    dataptr = (char *)comminfo->mbvect[i].baseptr;
    numdims = comminfo->mbvect[i].numdims;
    for (j=0;j<numdims;j++) {
      k[j]=0;
    }
    elemsize = comminfo->mbvect[i].elemsize;
    while (k[numdims-1] < comminfo->mbvect[i].diminfo[numdims-1].numelems) {
      memcpy(dataptr,buffptr,elemsize);
      buffptr += elemsize;
      
      k[0]++;
      for (j=0;j<numdims-1;j++) {
	if (k[j] == comminfo->mbvect[i].diminfo[j].numelems) {
	  k[j] = 0;
	  k[j+1]++;
	} else {
	  break;
	}
      }
      dataptr += comminfo->mbvect[i].diminfo[j].stride;
    }
  }
}
