/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __AGEN_H_
#define __AGEN_H_

#include "parsetree.h"
#include "struct.h"

void gen_access_begin(FILE *,expr_t *,int);
void gen_access_end(FILE *,expr_t *,expr_t *);
void gen_randaccess_end(FILE *,expr_t *);
void gen_access_begin_distance(FILE *,expr_t *);
void gen_access_end_distance(FILE *,expr_t *);

void gen_cached_access_stuff(FILE*, mloop_t*);

#endif
