/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "mpi.h"
#include "md_zlib.h"


void _ReadPort(int src, int port, int commid, void *data , int size) {
  MPI_Status status;

  MPI_Recv(data, size, MPI_BYTE, src, commid, _GRID_WORLD(_DefaultGrid), &status);
}


void _WritePort(int dest, int port, int commid, void *data , int size) {
  MPI_Request request;
  MPI_Status status;

  MPI_Isend(data, size, MPI_BYTE, dest, commid, _GRID_WORLD(_DefaultGrid), &request);
  MPI_Wait(&request, &status);
}
