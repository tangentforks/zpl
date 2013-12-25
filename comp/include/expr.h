/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __UTIL_EXPR_H_
#define __UTIL_EXPR_H_

#include "parsetree.h"

int expr_const(expr_t *);              /* tells if it's a constant (0/1) */
int expr_computable_const(expr_t *);   /* tells if it's computable at c-t */
int expr_rt_const(expr_t *);           /* tells if it's a runtime constant */

symboltable_t *expr_is_var(expr_t *);  /* tells if it's a variable (NULL/pst) */

int expr_is_free(expr_t*);             /* tells if expr is free */

symboltable_t *expr_find_ens_pst(expr_t *); /* expr's ens pst (NULL/pst) */
expr_t *expr_find_root_expr(expr_t*);   /* expr's root expr (NULL/expr) */
symboltable_t *expr_find_root_pst(expr_t *); /* expr's root pst (NULL/pst) */
expr_t **expr_find_root_exprptr(expr_t *); /* expr's root exprptr (NULL/ptr) */

long expr_intval(expr_t *);             /* tells int val of expr (0/val) */
unsigned long expr_uintval(expr_t *);   /* tells uns int val of expr (0/val) */
double expr_doubleval(expr_t *);        /* tells double val of expr (0/val) */

int expr_is_global(expr_t *);          /* tells if it's a global var (0/1) */
int expr_is_static(expr_t *);          /* tells if it's a static var (0/1) */

int expr_is_qmask(expr_t *);           /* tells if it's quote mask (0/i) */

int expr_is_indexi(expr_t *);          /* tells if it's Indexi (0/i) */
int expr_contains_indexi(expr_t *);    /* tells if it contains Indexi (0/i) */
int expr_is_ia_index(expr_t *);        /* tells if it's [] index (0/1) */
int expr_contains_ia_indexi(expr_t*);  /* tells if contains [] index (0/1) */

int expr_equal(expr_t *,expr_t *);     /* tells if two exprs are == (0/1) */

expr_t *expr_find_at_up(expr_t *);     /* same as below, but a parent expr */
expr_t *expr_find_at(expr_t *);        /* finds a BIAT subexpr (NULL/pexpr) */

int expr_rank(expr_t *);               /* returns rank of statement (0/rank) */
int expr_parallel(expr_t *);           /* same as above but includes indexi */
int parallelindim_expr(expr_t*, int);  /* returns 1 if expr is parallel in given dimension */
int expr_contains_par_fns(expr_t *);   /* returns (0/1) */

expr_t *expr_find_reg(expr_t *);   /* finds a non-NULL T_ENSEMBLE in expr */

int expr_null_arrayref(expr_t *);      /* is expr a null arrayref? (0/1) */

int expr_c_legal_init(expr_t *);       /* is expr a legal C initializer? (0/1) */
int expr_is_lvalue(expr_t *);          /* does expr have l-value? (0/1) */
int expr_is_atom(expr_t *);            /* const, var, bidot, array */

int expr_is_ever_strided_ens(expr_t *); /* is expr ever strided? */

int expr_is_scan_reduce(expr_t *);     /* is expr in a scan or reduce? (0/1) */



expr_t* expr_sibling(expr_t*);

int expr_requires_parens(expr_t*);     /* parentheses required around expr? */

symboltable_t* expr_bidot_find_field(expr_t*,datatype_t*);
datatype_t* expr_bidot_find_field_dt(expr_t*,datatype_t*);

expr_t* new_const_int(int);
expr_t* new_const_string(char*);

expr_t* build_function_call(symboltable_t*, int, ...);

int expr_is_char(expr_t* expr, int unknown);

int expr_at_ensemble_root(expr_t*); /* is this where NULL-@ would've been? */
expr_t* expr_find_ensemble_root(expr_t*); /* find old NULL-@ level... */
expr_t* expr_find_an_ensemble_root(expr_t*); /* find any NULL-@ in expr... */
int expr_is_qreg(expr_t*);       /* is expr areg? */
int expr_contains_qreg(expr_t*); /* is expr reg and contains qreg? */
int expr_is_grid_scalar(expr_t*);
int expr_is_dense_reg(expr_t*);
int expr_is_strided_reg(expr_t*,int);
int expr_dyn_reg(expr_t*);

int expr_regs_share_sparsity(expr_t*, expr_t*);
int expr_is_clean_sparse_reg(expr_t*);
int expr_prep_reg(expr_t*);
int expr_is_sparse_reg(expr_t*);
int expr_is_ofreg(expr_t*);
int expr_is_promoted_indarr(expr_t*);

expr_t* expr_direction_value(expr_t*, int);

#endif
