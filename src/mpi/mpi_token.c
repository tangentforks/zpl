/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "mpi.h"
#include "md_zinc.h"
#include "md_zlib.h"

#define _TOKENID 0


void _ResetToken() {
  int flag;
  MPI_Status status;
  long value;

  _BarrierSynch();

  MPI_Iprobe(MPI_ANY_SOURCE, _TOKENID, _GRID_WORLD(_DefaultGrid), &flag, &status);
  while (flag == TRUE) {
    MPI_Recv((void *)&value,sizeof(long),MPI_BYTE,MPI_ANY_SOURCE,_TOKENID,
	     _GRID_WORLD(_DefaultGrid),&status);
    MPI_Iprobe(MPI_ANY_SOURCE, _TOKENID, _GRID_WORLD(_DefaultGrid), &flag, &status);
  }

  _BarrierSynch();
}


void _SendTokenWithValue(int next, long *value) {
  MPI_Send((void *)value,sizeof(long),MPI_BYTE,next,_TOKENID,_GRID_WORLD(_DefaultGrid));
}


void _RecvTokenWithValue(long *value) {
  MPI_Status status;

  MPI_Recv((void *)value,sizeof(long),MPI_BYTE,MPI_ANY_SOURCE,_TOKENID,
	   _GRID_WORLD(_DefaultGrid),&status);
}


