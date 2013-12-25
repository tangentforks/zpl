/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MD_IRONMAN_H_
#define __MD_IRONMAN_H_

#include "mpi.h"
#include "mpi_commid.h"

typedef struct {
  int consecutive;
  int buffsize;
  char* buff;
  int dyn_commid;
  IM_comminfo* comminfo;
} IM_percomm;

typedef struct {
  int numrecvs;
  IM_percomm* recvinfo;
  MPI_Request* recvreq;
  int numsends;
  IM_percomm* sendinfo;
} IM_info;

#define IM_SV(g,x)

#define _GRID_WORLD(grid) (grid->procs)
#define _GRID_SLICE_V_SIZE(grid) (grid->slicesize)
#define _GRID_SLICE_V(grid) (grid->slice)
#define _GRID_SLICE(grid, dim) (grid->slice[dim])

#define _MD_GRID_INFO \
  MPI_Comm procs;     \
  int slicesize;      \
  MPI_Comm* slice;

#define _MD_DISTRIBUTION_INFO

#define _MD_REGION_INFO

#define _MD_ENSEMBLE_INFO

typedef struct {
  int optsvector;
  int tlcnt;
  int trcnt;
  int dims;
  int* lf;
  int** lind;
  int** rind;
  int* procmap;
  int* lindbase;
  int* rindbase;
  int collective;
  MPI_Request* arrreq;
} _permmap;

typedef struct {
  int optsvector;
  int eltsize;
  char** ldata;
  char** rdata;
  char* lbase;
  char* rbase;
  int count;
  MPI_Request* arrreq;
} _permdata;

#endif
