/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "md_zinc.h"
#include "ironman_utils.h"
#include "mpi.h"
#include "mpi_zlib.h"
#include "stand_inc.h"


void _BroadcastSimpleToSlice(_grid grid, void* ptr,unsigned int size,int involved,
			     int rootproc,int commid,int slicenum) {
  int color;
  int test;

  if (_GRID_SLICE(grid, slicenum) == MPI_COMM_NULL) {
    color = _GetSliceColor(grid, slicenum);
    MPI_Comm_split(_GRID_WORLD(grid), color, _MyLocInSlice(grid, slicenum), &(_GRID_SLICE(grid, slicenum)));
    MPI_Comm_rank(_GRID_SLICE(grid, slicenum), &test);
    if (test != _MyLocInSlice(grid, slicenum)) {
      fprintf(stderr,"[%d/%d] pos didn't match in slice (%d != %d)\n",
	      _INDEX,_PROCESSORS,test,_MyLocInSlice(grid, slicenum));
    }
  }
  if (_GRID_DISTRIBUTED(grid, slicenum) && involved) {
    MPI_Bcast(ptr,size,MPI_BYTE,rootproc,_GRID_SLICE(grid, slicenum));
  }
}


void _BroadcastSimple(_grid grid, void * ptr,unsigned int size,int rootproc,int commid) {
  _BroadcastSimpleToSlice(grid, ptr,size,1,rootproc,commid,0);
}

