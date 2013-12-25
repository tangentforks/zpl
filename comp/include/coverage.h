/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _COVERAGE_H_
#define _COVERAGE_H_

#include "struct.h"

int RankOfCover(function_t*);
expr_t *BestCoveringMask(function_t *);
int NoStridedRegCover(function_t *);
int NoSparseRegCover(function_t *);

#endif
