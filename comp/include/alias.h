/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _ALIAS_H_
#define _ALIAS_H_

#include "struct.h"

int exprs_alias(expr_t *,expr_t *);     /* do two expressions alias? (0/1) */
int exprs_cda_alias(expr_t *,expr_t *); /* alias analysis for cda (0/1) */
int exprs_really_cda_alias(expr_t*,expr_t*); /* same but don't use primeats */

#endif
