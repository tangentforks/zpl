/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __BGEN_H_
#define __BGEN_H_

#include <stdio.h>
#include "struct.h"

#define BUMP_NEXT         1
#define BUMP_NEXT_IN_THIN 2
#define BUMP_TILE         3
#define BUMP_THIN_TILE    4

void gen_access_distance(FILE*,expr_t*,int,int,int [],int [],int[]);
void gen_bump_distance(FILE*,expr_t*,int,int,int [],int [],int [],expr_t *[],
		       int,int[]);
void gen_bump_tile_distance(FILE*,expr_t*,int,int,int [],int [],int [],
			    expr_t *[],int [],int [],int,int[]);

#endif
