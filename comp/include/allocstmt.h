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

#ifndef __ALLOCSTMT_H_
#define __ALLOCSTMT_H_

#include "../include/struct.h"
#include "../include/parsetree.h"

int call_mem_usage(module_t *,char *);
int net_mem_used(void);
int mem_used(void);
char *our_malloc(int);
void our_free(char *,int);
expr_t *alloc_expr(exprtype);
loop_t *alloc_loop(looptype);
if_t *alloc_if(void);
stmtlist_t *alloc_stmtlist(statement_t*,stmtlist_t*);
compound_t *alloc_compound(void);
statement_t *alloc_statement(stmnttype,int,char *);
function_t *alloc_function(void);
module_t *alloc_module(void);
io_t *alloc_io(iotype);
genlist_t *alloc_gen(void);
void free_gen(genlist_t *);
void free_expr(expr_t *);
void free_stmt(statement_t *);
void free_compound(compound_t *);
void free_if(if_t *);
void free_loop(loop_t *);
void free_stmtlist(stmtlist_t *);
void destroy_exprls(expr_t *);
void destroy_expr(expr_t *);
void destroy_stmtls(statement_t *);
void destroy_stmt(statement_t *);

#endif
