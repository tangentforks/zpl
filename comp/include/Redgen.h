/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __REDGEN_H_
#define __REDGEN_H_

#include <stdio.h>
#include "parsetree.h"

#define RED_LOCAL    (-1)
#define RED_UNKNOWN  (-2)
#define RED_ILLEGAL  (-3)


void gen_reduce(FILE *,expr_t *);
void gen_ironCC(FILE *,expr_t *);
void gen_decomp_reduce(FILE *,expr_t *);
void gen_decomp_bcast(FILE *,expr_t *);
void gen_decomp_flood(FILE *,expr_t *);

void gen_mloop_cc(FILE*,statement_t*);

#endif
