/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _SPSGEN_H_
#define _SPSGEN_H_

#include "glistmac.h"

#define _DIR_DNS 0
#define _DIR_SPS 1

void mark_region_needs(expr_t* reg,int dim,int inner_loop, int up);

void gen_sps_reg_walkers(FILE*,rlist,expr_t*,int,int[]);
rlist add_sps_expr_to_reglist(rlist,elist,expr_t*,expr_t*,int*);

void gen_sps_arr_access(FILE*,elist,expr_t*);
void gen_sps_base_n_walker_decl_inits(FILE*,elist);

void gen_sps_mloop_prefix(FILE*,expr_t*,int,int,int);
void gen_sps_mloop_postfix(FILE*,int,int,int);

void gen_sps_prepre_stmt(FILE*,expr_t*,rlist,int [],int [],int [],int,
			 int);
void gen_sps_pre_stmt(FILE*,expr_t*,rlist,int,int [],int,int);

void gen_sps_structure_use(void);

#endif

