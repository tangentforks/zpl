/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MD_IRONMAN_H_
#define __MD_IRONMAN_H_

#define _perminfo void

#define _InitCommID()

typedef struct {
  IM_comminfo* infoptr;
  IM_comminfo remmeminfo;
} IM_persend;

typedef struct {
  int commid;
  int numrecvs;
  int* recvinfo;
  int numsends;
  IM_persend* sendinfo;
  int firstsend;
} IM_info;

#define IM_SV(g,x)

#define _MD_GRID_INFO

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
  char** rptr;
  int* rflag;
} _permmap;

typedef struct {
  int optsvector;
  int eltsize;
  char** ldata;
  char** rdata;
  char* lbase;
  char* rbase;
  int count;
} _permdata;

#endif
