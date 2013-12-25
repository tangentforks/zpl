/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MAIN_H_
#define __MAIN_H_

#include <stdio.h>

void fix_stmt(statement_t*);
void fix_stmtls(statement_t*, statement_t *, function_t *);

/* runpass.c routines */
int get_flag_arg(char*,char);
int runpasses(module_t*,FILE*);

#endif
