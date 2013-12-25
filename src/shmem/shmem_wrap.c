/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "ironman.h"
#include "stand_inc.h"
#include "shmem_ironman.h"

#define _ISendWrap(R) (_I_SEND_WRAP((R)))
#define _IRecvWrap(R) (_I_RECV_WRAP((R)))


static int _FindWrapSendRect(_region R,_vector lorect,_vector hirect) {
  int numdims;
  int i;

  numdims = _NUMDIMS(R);
  for (i=0;i<numdims;i++) {
    lorect[i] = _WRAP_SEND_LO(R,i);
    hirect[i] = _WRAP_SEND_HI(R,i);
  }

  return numdims;
} 


static int _FindWrapRecvRect(_region R,_vector lorect,_vector hirect) {
  int numdims;
  int i;

  numdims = _NUMDIMS(R);
  for (i=0;i<numdims;i++) {
    lorect[i] = _WRAP_RECV_LO(R,i);
    hirect[i] = _WRAP_RECV_HI(R,i);
  }

  return numdims;
}
 

void _SetupWrapMemInfo(_region R,_ensemble * E,unsigned int * elemsize,
		       int (* findrect)(_region,_vector,_vector),
		       fullmeminfo * mem,int commid) {
  _ensemble * ensptr;
  int numens = 0;
  int i;
  meminfo * memptr;
  int w;
  int r;
  int numensdims;
  _vector lorect;
  _vector hirect;
  _vector temp;
  unsigned int origelemsize;

  ensptr = E;
  while (*ensptr != NULL) {
    numens++;
    ensptr++;
  }

  if (numens+1 > mem->numalloced) {
    mem->membank = (meminfo *)_zrealloc(mem->membank,(numens+1)*sizeof(meminfo), "");
    mem->numalloced = numens;
  }
  mem->num = numens;

  for (i=0;i<numens;i++) {
    memptr = &(mem->membank[i]);
    numensdims = findrect(R,lorect,hirect);

    memptr->ptr = _GenAccess(E[i],lorect);
    origelemsize=elemsize[i];
    memptr->elemsize=origelemsize;

    for (w=0;w<numensdims;w++) {
      temp[w]=lorect[w];
    }

    for (r=numensdims-1,w=0;r>=0;r--,w++) {
      memptr->numelems[w] = (hirect[r]-lorect[r])/_REG_STRIDE(R,r)+1;
      if (memptr->numelems[w] == 1) {
	if (r==0 && w==0) {
	} else {
	  w--;
	}
      } else  {
	lorect[r]+= _REG_STRIDE(R,r);
	
	memptr->stride[w] = _GenAccessDistance(E[i],lorect,temp);

	if (w <= 1) {
	  if (memptr->stride[w] == origelemsize) {
	    memptr->elemsize *= memptr->numelems[w];
	    memptr->stride[w] *= memptr->numelems[w];
	    if (w == 0) {
	      memptr->numelems[w]=1;
	    } else {
	      w--;
	    }
	    hirect[r]=lorect[r]-_REG_STRIDE(R,r);
	  }
	} else {
	  if (memptr->stride[w] == memptr->stride[w-1]) {
	    memptr->numelems[w-1] += memptr->numelems[w];
	    w--;
	  }
	}
	lorect[r]-= _REG_STRIDE(R,r);
      }
      temp[r] = hirect[r];
    }
    memptr->numdims=w;
  }
}


void _SetupWrapInfo(_region R,_ensemble * E,unsigned int * elemsize,
		    int commid,int dyn_commid) {
  commbank[commid].isend = _ISendWrap(R);
  commbank[commid].irecv = _IRecvWrap(R);
  commbank[commid].dyn_commid = dyn_commid;

  if (commbank[commid].irecv >= 0) {
    _SetupWrapMemInfo(R,E,elemsize,_FindWrapRecvRect,
		      &(commbank[commid].recvinfo),commid);
    _CapMemInfo(&(commbank[commid].recvinfo));
    _PutMemInfo(commid);
  }
  if (commbank[commid].isend >= 0) {
    _SetupWrapMemInfo(R,E,elemsize,_FindWrapSendRect,
		      &(commbank[commid].sendinfo),commid);
  }
}


void _IronWrapI(_region R,_ensemble * E,unsigned int * elemsize,int commid) {
  int dyn_commid;

  dyn_commid = _GetDynamicCommID();
  if (_ISendWrap(R)>=0 && _IRecvWrap(R)>=0) {
    _LocalWrap(R,E,elemsize);
  } else {
    _SetupWrapInfo(R,E,elemsize,commid,dyn_commid);
    if (commbank[commid].isend>=0) {
      _CommII(commid);
    }
  }
}


void _IronWrapII(_region R,_ensemble * E,unsigned int * elemsize,int commid) {
  if (_ISendWrap(R)>=0 && _IRecvWrap(R)>=0) {
  } else {
    if (commbank[commid].irecv>=0) {
      _CommIII(commid);
    }
  }
}
