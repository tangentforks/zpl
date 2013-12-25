/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _REPGEN_H_
#define _REPGEN_H_

#include "parsetree.h"

/* USER FUNCTIONS */

/* add_replacement(mloop,origexpr,newexpr): Add a replacement to the
   given MLOOP in which the "origexpr" will be replaced by "newexpr"
   whenever it is generated within the MLOOP EXCEPTION: statement of
   the form "newexpr = origexpr" will be left unchanged */

void add_replacement(mloop_t*,expr_t*,expr_t*);



/* CODEGEN FUNCTIONS */

/* assignment_replace(mloop,assignexpr): returns 1 if replacements
   should be made on this assignment; 0 otherwise */

int assignment_replace(mloop_t*,expr_t*);


/* find_replacement(mloop,expr): returns the expression that should be
   generated for the expression passed in.  If there is no replacement
   for the expression, it will be returned itself. */

expr_t* find_replacement(mloop_t*,expr_t*);


/* test_replacement(mloop): when called within Mgen.c for jacobi.c,
   this can be used to create all sorts of hilarious and incorrect results */

void test_replacements(mloop_t*);

#endif
