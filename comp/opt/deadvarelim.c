/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: uve.c - unused variable elimination
 * DATE: 29 Aug 2000
 * CREATOR: echris
 *
 * NOTES
 * 
 */

#include <stdio.h>
#include <limits.h> 
#include <string.h>
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/main.h"
#include "../include/passes.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/symboltable.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/buildzplstmt.h"
#include "../include/dtype.h"
#include "../include/dimension.h"

static void uve_expr(expr_t *expr);
static void uve_fn(function_t *f);
static void uve_st(symboltable_t *s);

/* FN: uve_st - mark symboltable entry s as referenced
 * In order to eliminate globals, this routine must recursively look into
 * the datatype of s.  For example, if local array X is derlared in terms
 * of region R=[1..n], both R and n must also be marked as referenced.
 */

static void uve_st(symboltable_t *s) {
  S_IS_USED(s) = 1; 
}

/* FN: uve_expr - mark all variables in expr as referenced
 */

static void uve_expr(expr_t *expr) {
  if (expr) {
    uve_expr(T_NEXT(expr));
    uve_expr(T_OPLS(expr));
    if (T_TYPE(expr) == VARIABLE) {
      uve_st(T_IDENT(expr));
    }
  }
}

/* FN: uve_fn - perform uve on function f
 */

static void uve_fn(function_t *f) {
  statement_t *stmtls = T_STLS(f);
  symboltable_t *locals;

  /* clear all local symboltable tags */
  locals = T_CMPD_DECL(T_STLS(f));
  while (locals) {
    if (!symtab_is_param(locals) && S_IS_ENSEMBLE(locals)) {
      S_IS_USED(locals) = 0;
    }
    locals = S_SIBLING(locals);
  }

  /* look at all stmts */
  traverse_stmtls_g(stmtls, NULL, NULL, uve_expr, NULL);
}


/* FN: call_uve - entry-point to the uve pass
 */

int call_uve(module_t *m, char *d)
{
  symboltable_t *globals;

  INT_COND_FATAL((NULL==NULL), NULL, "");

  /* first, clear all global symboltable tags */
  /* currently, we only do arrays */
  globals = maplvl[0];
  while(globals) {
    if (S_IS_ENSEMBLE(globals) && !symtab_is_qmask(globals) && 
	!symtab_is_indexi(globals)) {
      S_IS_USED(globals) = 0;
    }
    globals = S_SIBPREV(globals);
  }

  /* next, set symboltable tags of used vars */
  traverse_modules(m, FALSE, NULL, uve_fn);

  return (0);
}
