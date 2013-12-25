/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __DBG_CODE_GEN_H_
#define __DBG_CODE_GEN_H_

#include <stdio.h>
#include "struct.h"

#if (!RELEASE_VER)
void dbg_gen_stmt(FILE *,statement_t *);
void dbg_gen_stmtls(FILE *,statement_t *);
void dbg_gen_expr(FILE *,expr_t *);
void print_symtab_entry(symboltable_t *);
#else
#define dbg_gen_stmt(x,y)
#define dbg_gen_stmtls(x,y)
#define dbg_gen_expr(x,y)
#define print_symtab_entry(x)
#endif

/* from bdebug.c */


#endif
