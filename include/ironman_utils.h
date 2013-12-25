/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __IRONMAN_UTILS_H_
#define __IRONMAN_UTILS_H_

#include "ironman.h"

void _IMU_PrintMemBlock(const IM_memblock* const);

/* this is extern simply for regression testing purposes */
int _IMU_PackMemBlock(IM_memblock *given,IM_memblock *save);


int _IMU_Pack(IM_comminfo *given,IM_comminfo *save);
void _IMU_ConstrainedPack(IM_memblock*,IM_comminfo*,IM_comminfo*);
void _IMU_Copy(IM_comminfo*,IM_comminfo*);

int _IMU_RequiredSize(const IM_comminfo* const comminfo);

char* _IMU_AllocBuffer(const IM_comminfo* const comminfo,int* const buffsize);
void _IMU_FreeBuffer(char *buffer);

void _IMU_Marshall(char* const buffer,const IM_comminfo* const comminfo);
void _IMU_Unmarshall(const IM_comminfo* const comminfo,const char* const buffer);

#endif
