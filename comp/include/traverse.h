/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _TRAVERSE_H_
#define _TRAVERSE_H_

void traverse_expr (expr_t *, int, void (*) (expr_t *));
void traverse_expr_g (expr_t *, void (*) (expr_t *), void (*) (expr_t *));

void traverse_exprls(expr_t *,int,void (*) (expr_t *));
void traverse_exprls_g(expr_t *,void (*) (expr_t *),void (*) (expr_t *));

void traverse_stmt (statement_t *, int, void (*) (statement_t *),
		    void (*) (expr_t *), void (*) (symboltable_t *));
void traverse_stmt_g (statement_t *, void (*) (statement_t *), 
		      void (*) (statement_t *), void (*) (expr_t *), 
		      void (*) (symboltable_t *));

void traverse_stmtls(statement_t*,int,void (*) (statement_t *),
		     void (*) (expr_t *),void (*) (symboltable_t *));
void traverse_stmtls_g(statement_t *,void (*) (statement_t *),
		       void (*) (statement_t *),void (*) (expr_t *),
		       void (*) (symboltable_t *));

int traverse_functions(int (*)(function_t *));

int traverse_modules(module_t *,int,void (*)(module_t*),void (*)(function_t *));

#endif
