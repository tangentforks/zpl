/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __UTIL_STATEMENT_H_
#define __UTIL_STATEMENT_H_

#include "parsetree.h"

statement_t *stmt_in_loop(statement_t *); /* is it in a loop (loop ptr/NULL) */
int stmtls_length(statement_t *stmtls);   /* num of stmts in stmtls */
int stmt_rank(statement_t *);
int stmtls_rank(statement_t *);
expr_t *stmt_find_reg(statement_t *);
int stmt_is_shard(statement_t *);
int stmts_are_ancestors(statement_t*,statement_t*); /* tells if two stmts
						       are in an ancestor 
						       relationship (0/1) */

#endif
