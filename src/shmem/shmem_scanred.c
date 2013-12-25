/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <mpp/shmem.h>
#define DEFINE_SHMEM_SR
#include "stand_inc.h"
#include "shmem_scanred.h"

#define _SYNCSIZE (2*_MAXRANK)

typedef struct _wsrec {
  int size;
  void* ptr;
} wsrec;

typedef struct _iorec {
  int size;
  char* input;
  char* output;
} iorec;

static int currentRSync=0;
static int currentBSync=0;
static long* RSync[_SYNCSIZE];
static long* BSync[_SYNCSIZE];
static wsrec workspace[_SYNCSIZE];
static iorec iospace[_SYNCSIZE];

void shmem_dcomplex_sum_to_all(dcomplex* target, dcomplex* source, int nreduce, int PE_start, int logPE_stride, int PE_size, dcomplex *pWrk, long *pSync) {
  shmem_double_sum_to_all((double*)target, (double*)source, nreduce*2, PE_start, logPE_stride, PE_size, (double*)pWrk, pSync);
}

static int CalcColStride(void) {
  int stride;
  int cols;

  stride=0;
  cols=_GRID_SIZE(_DefaultGrid, 1);
  cols >>= 1;
  while (cols) {
    stride++;
    cols >>= 1;
  }
  return stride;
}


void _InitScanRed() {
  int i,j;

  colstride = CalcColStride();
  
  for (j=0;j<_SYNCSIZE;j++) {
    RSync[j] = (long *)shmalloc(_SHMEM_REDUCE_SYNC_SIZE*sizeof(long));
    for (i=0;i<_SHMEM_REDUCE_SYNC_SIZE;i++) {
      RSync[j][i] = _SHMEM_SYNC_VALUE;
    }
    BSync[j] = (long *)shmalloc(_SHMEM_BCAST_SYNC_SIZE*sizeof(long));
    for (i=0;i<_SHMEM_BCAST_SYNC_SIZE;i++) {
      BSync[j][i] = _SHMEM_SYNC_VALUE;
    }
    workspace[j].size = 0;
    workspace[j].ptr = NULL;
    iospace[j].size = 0;
    iospace[j].input = NULL;
    iospace[j].output = NULL;
  }

  shmem_barrier_all();
}

/*
void _GrowWorkspace(int numelems,size_t elemsize) {
  size_t reqsize;

  reqsize = elemsize * max(numelems/2+1,_SHMEM_REDUCE_MIN_WRKDATA_SIZE);
  if (reqsize > ws_size) {
    ws_size = reqsize;
    workspace = (void *)shrealloc(workspace,ws_size);
  }
  reqsize = elemsize*numelems;
  if (reqsize > io_size) {
    io_size = reqsize;
    sym_input = (char *)shrealloc(sym_input,io_size);
    sym_output = (char *)shrealloc(sym_output,io_size);
  }
}
*/


void _GetRedBuffs(int numelems,size_t elemsize,long** synch,char** in,
		  char** out,void** workspc) {
  size_t reqsize;

  *synch = RSync[currentRSync];
  
  reqsize = elemsize * max(numelems/2+1,_SHMEM_REDUCE_MIN_WRKDATA_SIZE);
  if (reqsize > workspace[currentRSync].size) {
    workspace[currentRSync].size = reqsize;
    workspace[currentRSync].ptr = (void *)shrealloc(workspace[currentRSync].ptr,
						    reqsize);
  }
  *workspc = workspace[currentRSync].ptr;

  reqsize = elemsize*numelems;
  if (reqsize > iospace[currentRSync].size) {
    iospace[currentRSync].size = reqsize;
    iospace[currentRSync].input = (char*)shrealloc(iospace[currentRSync].input,
						   reqsize);
    iospace[currentRSync].output = (char*)shrealloc(iospace[currentRSync].output,
						    reqsize);
  }
  *in = iospace[currentRSync].input;
  *out = iospace[currentRSync].output;

  currentRSync = (currentRSync+1)%(_SYNCSIZE);
}


long *_GetBcastSync() {
  long *retval;

  retval = BSync[currentBSync];
  currentBSync = (currentBSync+1)%(_SYNCSIZE);
  shmem_barrier_all();  
              /* This is necessary due to the requirement that *all* PE's
		 must not be using the target when *any* PE calls broadcast.
		 In File I/O this is a problem, for example.  The other way
		 to go would be to have a data array associated with this
		 bsync stuff as with reductions, but that requires extra
		 stuff, and a barrier is not usually overly restraining.
		 */

  return retval;
}


void _Reduceuser(_grid grid, void* data, int numelems, int redtype,
		 int destproc, int bcast, void (*userfn)(void*,void*),
		 size_t size) {
  _RT_ALL_FATAL0("User-defined reductions not implemented for shmem yet");
}
