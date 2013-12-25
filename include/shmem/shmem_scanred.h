/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __SHMEM_SCANRED_H_
#define __SHMEM_SCANRED_H_

#ifndef DEFINE_SHMEM_SR
#define DEFINE_SHMEM_SR extern
#endif

#include <malloc.h>
#include "zplglobal.h"

DEFINE_SHMEM_SR int colstride;

void _InitScanRed(void);
void _GetRedBuffs(int,size_t,long**,char**,char**,void**);
long *_GetBcastSync(void);

void shmem_dcomplex_sum_to_all(dcomplex*, dcomplex*, int, int, int, int, dcomplex *, long *);

#endif
