/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __CGEN_H_
#define __CGEN_H_

#include <stdio.h>
#include "symtab.h"
#include "glistmac.h"

void gen_arrayref(FILE*, expr_t*,datatype_t*);
void gen_expr(FILE*, expr_t*);
void gen_stmt(FILE*, statement_t*);
void gen_stmtls(FILE*, statement_t*);

void find_element_size(datatype_t*,FILE*);

void gen_complex_temp_name(FILE *,datatype_t *,int);
void gen_setup_complex_temp(FILE *,expr_t *,datatype_t *,int);

void gen_real_expr(FILE*,expr_t *);
void gen_imag_expr(FILE*,expr_t *);
void gen_noncomplex_expr(FILE*,expr_t *);

#ifndef LINEINFO
#define LINEINFO(stmt) if (debug && !GLOBAL_MLOOP && T_LINENO(stmt)) fprintf(outfile, "/* ZLINE: %d %s */\n", T_LINENO(stmt), T_FILENAME(stmt))
#endif

void fgenf(FILE*, const char*, ...);

void gen_dim_lo_bound(FILE* outfile, dimension_t* dimptr);
void gen_dim_hi_bound(FILE* outfile, dimension_t* dimptr);

void gen_updn(FILE* outfile,int up);

expr_t* build_array_size_multiplier(dimension_t*);
int get_array_size_multiplier(dimension_t*);

#endif
