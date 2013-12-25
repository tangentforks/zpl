/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MD_IRONMAN_H_
#define __MD_IRONMAN_H_

#include "pvm_commid.h"
#include "zplglobal.h"

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
  int numsends;
  IM_percomm* sendinfo;
} IM_info;

#define IM_DR(g,x)
#define IM_SV(g,x)

#define _MD_GRID_INFO \
  int gridid;

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
  int dyncommid;
} _permmap;

typedef struct {
  int optsvector;
  int eltsize;
  char** ldata;
  char** rdata;
  char* lbase;
  char* rbase;
  int dyncommid;
  int count;
} _permdata;

#endif
