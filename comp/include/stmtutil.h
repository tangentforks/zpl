/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***************************************************************************
	Center for Supercomputing Research and Development
	at the University of Illinois at Urbana-Champaign
		305 Talbot Laboratory
		104 South Wright Street
		Urbana, Illinois  61801-2932 USA

Write to the above address for information on the CSRD Software Catalog,
or send e-mail to:  softcat@csrd.uiuc.edu

Copyright (c) 1991 by the Board of Trustees of the University of Illinois
*unpublished*.  All rights reserved.  This proprietary created material is
provided under a license agreement which restricts its use and protects its
confidentiality.  No publication or waiver of confidentiality is implied by
this transfer under the license.
***************************************************************************/

#ifndef __STMTUTIL_H_
#define __STMTUTIL_H_

extern statement_t *first_stmt_in_stmtls(statement_t *s);
extern statement_t *last_stmt_in_stmtls(statement_t *s);
extern statement_t *cat_stmt_ls(statement_t *,statement_t *);
statement_t* reverse_stmtls (statement_t*);
extern expr_t *cat_expr_ls(expr_t *,expr_t *);
extern expr_t *cat_io_expr(expr_t *,expr_t *);
extern function_t *cat_fcn_ls(function_t *,function_t *);
extern symboltable_t *cat_symtab_ls(symboltable_t *,symboltable_t *);
extern genlist_t *cat_genlist_ls(genlist_t *,genlist_t *);

extern int numoperands(expr_t *);
extern expr_t *nthoperand(expr_t *,int);
extern expr_t *left_expr(expr_t *);
extern expr_t *right_expr(expr_t *);

extern void insertbefore_stmt(statement_t*,statement_t *);
extern void insertafter_stmt(statement_t*,statement_t *);
extern void insert_cmpd_stmt(statement_t*, statement_t*);
extern statement_t *remove_stmt(statement_t *);
extern statement_t *clear_stmt(statement_t *);

extern void             delete_stmt(statement_t *);
extern symboltable_t    *copy_decls(symboltable_t *, int);
extern genlist_t 	*copy_genlist(genlist_t *);
extern statement_t	*copy_stmtls(statement_t *);
extern statement_t	*copy_stmt(statement_t *);
extern expr_t	        *copy_exprls(expr_t *, expr_t*);
extern expr_t	        *copy_expr(expr_t *);
extern statement_t      *copy_comm_stmt(statement_t *);

/* replaces the former expr_t with the latter expr_t */
expr_t* replace_expr (expr_t*, expr_t*);

/* replaces all the occurrences of the second expr_t in the first with
   a copy of the third, returns the possibly new first */
expr_t* replaceall_expr (expr_t*, expr_t*, expr_t*);
expr_t* replaceall_atom_exprs(expr_t*, expr_t*, expr_t*);

/* replaces all the occurrences of the second expr_t in the list of
   expressions given in the first argument with a copy of the third */
void replaceall_exprls(expr_t*, expr_t*, expr_t*);

/* applies replaceall_exprls to all expression lists in the statement list */
void replaceall_stmtls(statement_t*, expr_t*, expr_t*);

/* changes return statements to assignments to the expression */
statement_t* returns2assigns(statement_t*, expr_t*);

/* replaces the expr_t with a zero expr_t */
void zeroreplace_expr (expr_t*);

/*
 * minimizes the number of multiplications by replacing
 *
 *        x * y + ... + z * y  with  (x + z) * y + ... + 0 * y
 */
void factor_expr (expr_t*);

/*
 * replace 1 * x with x, 0 * x with 0, 0 + x with x recursively
 * throughout the expr_t
 */
expr_t* clean_expr (expr_t*);

expr_t* plainify_expr (expr_t* expr);

void compose_expr (expr_t* expr);

expr_t* groupmult_expr (expr_t* expr);

expr_t* constfold_expr (expr_t* expr);

int varequals_expr(expr_t* expr, symboltable_t* var);

int mloopinvariant_expr(expr_t* expr);

int mloopassigned_expr(expr_t* expr);

/* returns 1 if expr or stmtls contains a BIAT of subtype attype_t */
int expr_contains_uat(expr_t* expr, attype_t);
int stmtls_contains_uat(statement_t* stmt, attype_t);

int stmtls_contains_zmacro(statement_t* stmt);


expr_t* stmtls_find_ens_expr(statement_t*);




#endif
