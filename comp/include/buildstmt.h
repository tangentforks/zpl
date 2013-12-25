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

#include "parsetree.h"
#include "symtab.h"
#include "struct.h"

expr_t *build_null_op(void);
expr_t *build_0ary_op(exprtype,symboltable_t *);
expr_t *build_unary_op(exprtype,expr_t *);
expr_t* build_prep_region_expr(regionclass, expr_t*, expr_t*,int lineno, char* filename);
expr_t* build_with_region_expr(regionclass, expr_t*, expr_t*,int lineno, char* filename);
expr_t* build_sparse_region_expr(expr_t*, expr_t*, masktype);
expr_t *build_binary_op(exprtype,expr_t *,expr_t *);
expr_t* build_binary_op_gets(exprtype,binop_t,expr_t*,expr_t*);
expr_t *build_Nary_op(exprtype,expr_t *,expr_t *);

expr_t* build_typed_0ary_op(exprtype,symboltable_t*);
expr_t* build_typed_unary_op(exprtype,expr_t*);
expr_t* build_typed_binary_op(exprtype,expr_t*,expr_t*);
expr_t* build_typed_Nary_op(exprtype,expr_t*,expr_t*);

expr_t *build_struct_op(exprtype,expr_t *,symboltable_t *);
expr_t *build_unary_at_op(expr_t *,expr_t *,attype_t);
statement_t *build_expr_statement(expr_t *,int,char *);
statement_t *build_return_statement(expr_t *,int,char *);
statement_t *build_mscan_statement(symboltable_t *,statement_t *,int,char *);
statement_t *build_compound_statement(symboltable_t *,statement_t *,int,char *);
statement_t *build_if_statement(expr_t *,statement_t *,statement_t *,
				statement_t *,int,char *);
statement_t *build_loop_statement(looptype,expr_t *,statement_t *,int,char *);
statement_t *build_do_statement(looptype,int updown,symboltable_t *,expr_t *,
				expr_t *,expr_t *,expr_t *,statement_t *,int,
				char *);
function_t *build_function(symboltable_t *,symboltable_t *,genlist_t *,
			   statement_t *,function_t *);
module_t *build_module(symboltable_t *);
module_t *add_module(module_t *,symboltable_t *);
module_t *finish_module(module_t *);
