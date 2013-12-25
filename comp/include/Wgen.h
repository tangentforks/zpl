/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __WGEN_H_
#define __WGEN_H_

#include <stdio.h>
#include "glistmac.h"
#include "parsetree.h"

void WgenSetInOldMloop(void);
void WgenSetOutOldMloop(void);

void gen_walker_name(FILE*,elist);
void gen_walker_name_from_expr(FILE*,expr_t*);

int walker_in_use(expr_t*);
int walker_sparse(expr_t*);

int gen_walker_with_offset(FILE *,expr_t *);
void bump_walker(FILE *,elist,int,int,int *);
void bump_tiled_walker(FILE *,elist,int,int,int,int,int *);
glist search_downfor_ensemble_exprs(expr_t *expr);
elist search_for_ensembles_in_stmt_ls(statement_t *stmt_ls);
void gen_rec_walker_decl_inits(FILE*,mloop_t*);
void gen_walker_decl_inits(FILE*,expr_t*,int,int [],int [],int [],elist,
			   rlist*,int[]);
void gen_tile_g_init(FILE *outfile, int dims, int depth, int *order, 
		     int *torder, int *direction, int *tcase, elist wb);
void number_elist(elist);

#endif
