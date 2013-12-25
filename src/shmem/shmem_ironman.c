/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <mpp/shmem.h>
#include "ironman.h"
#include "ironman_utils.h"
#include "zerror.h"
#include "zlib.h"

static int maxcomms;
static int maxdirs;
static int maxblocks;
static int numprocs;

static IM_info *commbank;
static IM_info **freelist;
static int numfree;

static IM_memblock* remmem;
static volatile int* remmemready;
static volatile int* remmemwritable;
static volatile int* dstready;
static volatile int* putdone;
static int* sendstoproc;
static int* recvsfmproc;
static int* counter;

static const int trueval=1;

#define REMMEM(p,c,d,b)     (remmem[(c)*maxdirs*numprocs*maxblocks+\
                                    (d)*numprocs*maxblocks+\
                                    (p)*maxblocks+(b)])

#define REMMEMREADY(c,p)    (remmemready[(c)*numprocs+(p)])
#define REMMEMWRITABLE(c,p) (remmemwritable[(c)*numprocs+(p)])
#define DSTREADY(c,p,d)     (dstready[(c)*numprocs*maxdirs+(p)*maxdirs+(d)])
#define PUTDONE(c,p)        (putdone[(c)*numprocs+(p)])

#define SENDSTOPROC(c,p)    (sendstoproc[(c)*numprocs+(p)])
#define RECVSFMPROC(c,p)    (recvsfmproc[(c)*numprocs+(p)])
#define COUNTER(c,p)        (counter[(c)*numprocs+(p)])


void IM_Initialize(int procnum,int procs,int comms,int maxdirspercomm,
		   int maxblockspercomm) {
  int i;
  int j;
  int k;

  shmem_set_cache_inv();

  maxcomms = comms;
  maxdirs = maxdirspercomm;
  maxblocks = maxblockspercomm;
  numprocs = procs;

  commbank = (IM_info *)_zmalloc(maxcomms*sizeof(IM_info),
				   "shmem ironman commbank");
  freelist = (IM_info **)_zmalloc(maxcomms*sizeof(IM_info *),
				    "shmem ironman freelist");
  for (i=0;i<maxcomms;i++) {
    commbank[maxcomms-(i+1)].commid = i;
    commbank[maxcomms-(i+1)].numrecvs = 0;
    commbank[maxcomms-(i+1)].recvinfo = _zmalloc(maxdirs*sizeof(int),
						   "shmem ironman recvinfo");
    commbank[maxcomms-(i+1)].numsends = 0;
    commbank[maxcomms-(i+1)].sendinfo = _zmalloc(maxdirs*sizeof(IM_persend),
						   "shmem ironman sendinfo");
    commbank[maxcomms-(i+1)].firstsend = 0;
    for (j=0;j<maxdirs;j++) {
      commbank[maxcomms-(i+1)].sendinfo[j].remmeminfo.mbvect =
	_zmalloc(maxblocks*sizeof(IM_memblock),"shmem ironman mbvect"); 
   }
    freelist[i] = &(commbank[maxcomms-(i+1)]);
  }
  numfree = maxcomms-1;

  /* BLC -- maxdirs is a very loose bound in the following? */
  remmem = (IM_memblock *)shmalloc(procs*maxcomms*maxdirs*maxblocks*
				   sizeof(IM_memblock));

  remmemready = (int *)shmalloc(procs*maxcomms*sizeof(int));
  remmemwritable = (int *)shmalloc(procs*maxcomms*sizeof(int));
  dstready = (int *)shmalloc(procs*maxcomms*maxdirs*sizeof(int));
  putdone = (int *)shmalloc(procs*maxcomms*sizeof(int));

  sendstoproc = (int*)_zmalloc(procs*maxcomms*sizeof(int),
				 "shmem ironman sendstoproc");
  recvsfmproc = (int*)_zmalloc(procs*maxcomms*sizeof(int),
				 "shmem ironman recvsfmproc");
  counter = (int*)_zmalloc(procs*maxcomms*sizeof(int),
			     "shmem counter");

  for (i=0; i<maxcomms; i++) {
    for (j=0; j<procs; j++) {
      REMMEMREADY(i,j) = 0;
      REMMEMWRITABLE(i,j) = trueval;
      for (k=0; k<maxdirs; k++) {
	DSTREADY(i,j,k) = 0;
      }
      PUTDONE(i,j) = 0;

      SENDSTOPROC(i,j) = 0;
      RECVSFMPROC(i,j) = 0;
      COUNTER(i,j) = 0;
    }
  }

  if (sizeof(IM_memblock)%8 != 0) {
    _RT_ALL_FATAL0("Mis-assumption about IM_memblock size (shmem_ironman.c)\n");
  }
  shmem_barrier_all();
}


IM_info *IM_New(_grid grid,const int numcomms,const IM_commpair* const commvect) {
  IM_info* const newinfo = freelist[numfree--];
  const int commid = newinfo->commid;
  int i;
  int numrecvs=0;
  int numsends=0;

  for (i=0; i<numcomms; i++) {
    const IM_comminfo* const in_recvinfo = commvect[i].recvinfo;
    IM_comminfo* const in_sendinfo = commvect[i].sendinfo;
    if (in_recvinfo != NULL) {
      const int irecv = in_recvinfo->procnum;

      newinfo->recvinfo[numrecvs] = irecv;

      if (numrecvs == 0) {
	while (REMMEMWRITABLE(commid,irecv) != trueval) {
	}
	REMMEMWRITABLE(commid,irecv) = 0;
      }

      shmem_put64(&(REMMEM(_INDEX,commid,RECVSFMPROC(commid,irecv),0)),
		  in_recvinfo->mbvect,
		  (in_recvinfo->numblocks*sizeof(IM_memblock))/8,irecv);
      
      numrecvs++;
      RECVSFMPROC(commid,irecv)++;
    }
    if (in_sendinfo != NULL) {
      const int isend = in_sendinfo->procnum;

      newinfo->sendinfo[numsends].infoptr = in_sendinfo;

      numsends++;
      SENDSTOPROC(commid,isend)++;
    }
  }

  newinfo->numrecvs = numrecvs;
  newinfo->numsends = numsends;
  newinfo->firstsend = 1;

  if (numrecvs > 0) {
    shmem_fence();
    for (i=0;i<numrecvs;i++) {
      const int irecv = newinfo->recvinfo[i];

      COUNTER(commid,irecv)++;
      if (COUNTER(commid,irecv) == RECVSFMPROC(commid,irecv)) {
	COUNTER(commid,irecv) = 0;
	shmem_int_put((int*)&(REMMEMREADY(commid,_INDEX)),&trueval,1,irecv);
      }
    }
  }

  return newinfo;
}


void IM_DR(_grid grid, IM_info* const info) {
  const int numrecvs = info->numrecvs;

  if (numrecvs > 0) {
    const int commid = info->commid;
    int i;

    for (i=0; i<numrecvs; i++) {
      const int irecv = info->recvinfo[i];

      PUTDONE(commid,irecv) = 0;

      shmem_int_put((int*)&(DSTREADY(commid,_INDEX,COUNTER(commid,irecv))),
		&trueval,1,irecv);

      COUNTER(commid,irecv)++;
      if (COUNTER(commid,irecv) == RECVSFMPROC(commid,irecv)) {
	COUNTER(commid,irecv) = 0;
      }
    }
  }
}


void IM_SR(_grid grid, IM_info* const info) {
  const int numsends = info->numsends;

  if (numsends > 0) {
    const int commid = info->commid;
    const int firstsend = info->firstsend;
    char* srcptr;
    char* dstptr;
    int i;
    int j;
    int k[_MAXRANK];
    int sendnum;
    int put32;
    IM_memblock *sendblock;
    IM_memblock *recvblock;

    for (sendnum=0; sendnum<numsends; sendnum++) {
      IM_persend* const sendinfo = &(info->sendinfo[sendnum]);
      IM_comminfo* const sendinfoptr = sendinfo->infoptr;
      const int isend = sendinfoptr->procnum;
      const int numblocks = sendinfoptr->numblocks;
      IM_comminfo* const remmeminfo = &(sendinfo->remmeminfo);

      if (firstsend) {
	if (COUNTER(commid,isend) == 0) {
	  while (REMMEMREADY(commid,isend) != trueval) {
	  }
	  REMMEMREADY(commid,isend) = 0;
	}
	_IMU_ConstrainedPack(&(REMMEM(isend,commid,COUNTER(commid,isend),0)),
			     remmeminfo,sendinfoptr);
      }

      while (DSTREADY(commid,isend,COUNTER(commid,isend)) != trueval) {
      }
      DSTREADY(commid,isend,COUNTER(commid,isend))=0;
      
      sendblock = sendinfoptr->mbvect;
      recvblock = remmeminfo->mbvect;
      for (i=0; i<numblocks; i++) {
	const int numdims = sendblock->numdims;
	int elemsize = sendblock->elemsize;

	srcptr = sendblock->baseptr;
	dstptr = recvblock->baseptr;
	
	if (elemsize%8 == 0) {
	  if (((long)dstptr)%8 == 0 && ((long)srcptr)%8 == 0) {
	    put32 = 0;
	    for (j=0;j<numdims;j++) {
	      if (recvblock->diminfo[j].stride%8 != 0 ||
		  sendblock->diminfo[j].stride%8 != 0) {
		put32 = 1;
		break;
	      }
	    }
	  } else {
	    put32 = 1;
	  }
	} else {
	  put32 = 1;
	}
	if (put32) {
	  elemsize /= 4;
	} else {
	  elemsize /= 8;
	}

	for (j=0;j<numdims;j++) {
	  k[j] = 0;
	}
#ifdef STUPID_PTR_CHECK_NECESSARY
	{
	  char *topptr;
	  int numbumps;
	  
	  topptr = dstptr - recvblock->diminfo[numdims-1].stride;
	  numbumps = 1;
	  for (j=numdims-1;j>=0;j--) {
	    numbumps *= recvblock->diminfo[j].numelems;
	    topptr += numbumps*recvblock->diminfo[j].stride;
	  }
	  malloc_brk(topptr);
	}
#endif
	if (put32) {
	  while (1) {
	    shmem_put32(dstptr,srcptr,elemsize,isend);
	    
	    for (j=0;j<numdims;j++) {
	      k[j]++;
	      if (k[j] == sendblock->diminfo[j].numelems) {
		k[j] = 0;
	      } else {
		break;
	      }
	    }
	    if (j == numdims && k[j-1] == 0) {
	      break;
	    } else {
	      srcptr += sendblock->diminfo[j].stride;
	      dstptr += recvblock->diminfo[j].stride;
	    }
	  }
	} else if (elemsize != 1) {
	  while (1) {
	    shmem_put64(dstptr,srcptr,elemsize,isend);
	    
	    for (j=0;j<numdims;j++) {
	      k[j]++;
	      if (k[j] == sendblock->diminfo[j].numelems) {
		k[j] = 0;
	      } else {
		break;
	      }
	    }
	    if (j == numdims && k[j-1] == 0) {
	      break;
	    } else {
	      srcptr += sendblock->diminfo[j].stride;
	      dstptr += recvblock->diminfo[j].stride;
	    }
	  }
	} else {
	  const int tstr = recvblock->diminfo[0].stride/8;
	  const int sstr = sendblock->diminfo[0].stride/8;
	  const int numelems = sendblock->diminfo[0].numelems;
	  const int srcbump = (numelems-1)*sendblock->diminfo[0].stride;
	  const int dstbump = (numelems-1)*recvblock->diminfo[0].stride;
	  
	  while (1) {
	    shmem_iput64(dstptr,srcptr,tstr,sstr,numelems,isend);
	    srcptr += srcbump;
	    dstptr += dstbump;
	    
	    for (j=1;j<numdims;j++) {
	      k[j]++;
	      if (k[j] == sendblock->diminfo[j].numelems) {
		k[j] = 0;
	      } else {
		break;
	      }
	    }
	    if (j == numdims && k[j-1] == 0) {
	      break;
	    } else {
	      srcptr += sendblock->diminfo[j].stride;
	      dstptr += recvblock->diminfo[j].stride;
	    }
	  }
	}
	sendblock++;
	recvblock++;
      }

      COUNTER(commid,isend)++;
      if (COUNTER(commid,isend) == SENDSTOPROC(commid,isend)) {
	COUNTER(commid,isend) = 0;
      }
    }

    shmem_fence();

    for (sendnum=0; sendnum<numsends; sendnum++) {
      const int isend = info->sendinfo[sendnum].infoptr->procnum;

      COUNTER(commid,isend)++;
      if (COUNTER(commid,isend) == SENDSTOPROC(commid,isend)) {

	shmem_int_put((int*)&(PUTDONE(commid,_INDEX)),&trueval,1,isend);

	COUNTER(commid,isend) = 0;
      }
    }
    info->firstsend = 0;
  }
}


void IM_DN(_grid grid, IM_info* const info) {
  const int numrecvs = info->numrecvs;

  if (numrecvs > 0) {
    const int commid = info->commid;
    int i;

    for (i=0; i<numrecvs; i++) {
      const int irecv = info->recvinfo[i];

      while (PUTDONE(commid,irecv) != trueval) {
      }
    }
  }
}


void IM_Old(IM_info* const oldinfo) {
  const int numrecvs = oldinfo->numrecvs;
  const int numsends = oldinfo->numsends;
  const int commid = oldinfo->commid;
  int i;

  for (i=0; i<numsends; i++) {
    const int isend = oldinfo->sendinfo[i].infoptr->procnum;
    
    shmem_int_put((int*)&(REMMEMWRITABLE(commid,_INDEX)),&trueval,1,isend);
    SENDSTOPROC(commid,isend) = 0;
  }

  for (i=0; i<numrecvs; i++) {
    const int irecv = oldinfo->recvinfo[i];

    RECVSFMPROC(commid,irecv) = 0;
  }
  
  freelist[++numfree] = oldinfo;
}

