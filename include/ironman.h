/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __IRONMAN_H_
#define __IRONMAN_H_

#include "struct.h"

/* Ironman constants and types */

#define IM_MAXRANK 8

typedef struct {
  void *baseptr;
  int elemsize;
  int numdims;
  struct {
    int numelems;
    int stride;
  } diminfo[IM_MAXRANK];
} IM_memblock;

typedef struct {
  int procnum;
  int numblocks;
  IM_memblock *mbvect;
} IM_comminfo;

typedef struct {
  IM_comminfo* recvinfo;
  IM_comminfo* sendinfo;
} IM_commpair;


/* get the machine-specific IM_info definition */

#include "md_ironman.h"

/* Ironman prototypes */

#ifndef IM_Initialize
void IM_Initialize(const int,const int,const int,const int,const int);
#endif

#ifndef IM_New
IM_info* IM_New(_grid, const int,const IM_commpair* const);
#endif

#ifndef IM_DR
void IM_DR(_grid, IM_info* const);
#endif

#ifndef IM_SR
void IM_SR(_grid, IM_info* const);
#endif

#ifndef IM_DN
void IM_DN(_grid, IM_info* const);
#endif

#ifndef IM_SV
void IM_SV(_grid, IM_info* const);
#endif

#ifndef IM_Old
void IM_Old(IM_info* const);
#endif

#endif
