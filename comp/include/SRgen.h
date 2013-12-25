/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __SRGEN_H_
#define __SRGEN_H_

#include <stdio.h>
#include "Dgen.h"
#include "parsetree.h"
#include "symtab.h"

void gen_sr_pdt(FILE *,datatype_t *,int);
void gen_op_op(FILE *,exprtype);
void gen_op_stmt(FILE *,exprtype,datatype_t *);
void gen_ident_elem(FILE *,exprtype,datatype_t *);

void gen_flat_bloop_access(FILE *);

void gen_sr_tot_array_size(FILE *,datatype_t *);
void gen_sr_nloop_index(FILE *,int,int);
void gen_sr_nloop_decls(FILE *,datatype_t *);
void gen_sr_nloop_start(FILE *,datatype_t *);
void gen_sr_nloop_access_1d(FILE*,dimension_t*,int);
void gen_sr_nloop_access(FILE *,datatype_t *);
void gen_sr_nloop_stop(FILE *,datatype_t *);

void gen_sr_setup_complex_temp(FILE *,expr_t *,datatype_t *);
void gen_sr_noncomplex_expr(FILE *,expr_t *,datatype_t *);

#endif
