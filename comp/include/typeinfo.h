/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _TYPEINFO_H_
#define _TYPEINFO_H_

/* Header for both type/typeinfo.c and type/typecheck.c */


/* typecheck.c exports */

int typecheck_compat(datatype_t*,datatype_t*,int,int,int);


/* typeinfo.c exports */

datatype_t* typeinfo_expr(expr_t*);
void typeinfo_stmt(statement_t *);



#endif

