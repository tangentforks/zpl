/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: scope.c - scope resolution code
 * DATE: 9 September 1996
 * CREATOR: echris
 *
 * SUMMARY
 *  This pass does two things: (1) assigns a rank to each stmt 
 *  (in T_RANK), and (2) assigns a "scope" to each stmt (in T_SCOPE).
 *  The only non-obvious detail about RANK ASSIGNMENT is when a stmt
 *  involves arrays of different rank.  AFTER NORMALIZATION, this
 *  case only arises in procedure arguments of different rank.  In
 *  this case, the assigned rank is the rank of the stmt that contains
 *  the procedure call.
 *
 *  SCOPE ASSIGNMENT is only relevant for stmts with nonzero rank.
 *  The T_SCOPE field either points to the region scope where this
 *  stmt gets its region, or it is NULL.  A NULL scope indicates that
 *  this stmt gets its region off the stack outside of this procedure.
 *  
 *  Here are two SIMPLE USES of the T_SCOPE information.  (1) To
 *  derive the applying region if it is available via intra-procedural
 *  analysis.  (2) To decide if two statements of the same rank have
 *  the same applying region; one simply tests that their T_SCOPEs
 *  are the same.
 */

#include <stdio.h>
#include "../include/const.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/generic_stack.h"
#include "../include/passes.h"
#include "../include/parsetree.h"
#include "../include/statement.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"

/* global array variable used in traversal routines */
static stack *scope_stack[MAXRANK+2];

/***
 *** scope assignment code
 ***/

/* scope_fun - fills T_SCOPE field for each procedure in fcn
 *             pushes NULL onto scope_stack,
 *             calls traversal routine to traverse statements,
 *             and pops stack
 * 9/9/96 - echris
 */

/* scope_pre - function executed before each statement is traversed
 * 9/9/96 - echris
 */

static void scope_pre(statement_t *s)
{
  statement_t *current_scope;
  int push_rank, s_rank;

  /* if region scope, push it on the scope_stack */
  if (T_TYPE(s) == S_REGION_SCOPE) {
    push_rank = D_REG_NUM(T_TYPEINFO(T_REGION_SYM(T_REGION(s))));
    if (push_rank > 0) {
      IFDB(10) { dbg_gen_stmt(zstdout, s); }
      g_push(scope_stack[push_rank-1], (void *)s);
    }
  }

  /* set scope pointer to top of scope_stack based on statement's rank */
  if (T_TYPE(s) == S_MLOOP) {
    s_rank = T_MLOOP_RANK(T_MLOOP(s));
  } else {
    s_rank = stmt_rank(s);
  }
  if (s_rank > 0) {
    current_scope = g_top(scope_stack[s_rank-1]);
    T_SCOPE(s) = current_scope;
  } else {
    T_SCOPE(s) = NULL;
  }

  IFDB(10) { dbg_gen_stmt(zstdout, s); }
}

/* scope_post - function executed after each function is traversed
 * 9/9/96 - echris
 */

static void scope_post(statement_t *s)
{
  int push_rank;

  /* if region scope, pop data that was pushed in scope_pre() */
  if (T_TYPE(s) == S_REGION_SCOPE) {
    push_rank = D_REG_NUM(T_TYPEINFO(T_REGION_SYM(T_REGION(s))));
    if (push_rank > 0) {
      g_pop(scope_stack[push_rank-1]);
    }
  }
}


static void scope_fun(function_t *fcn)
{
  int i;

  /* push NULL onto top of scope_stack */
  for (i=0; i<MAXRANK; i++) {  
    g_push(scope_stack[i], NULL);
  }

  /* call scope_pre() and scope_post() on each statement */
  traverse_stmtls_g(T_STLS(fcn), scope_pre, scope_post, NULL, NULL);

  /* pop off the NULLs that were pushed above */
  for (i=0; i<MAXRANK; i++) {
    g_pop(scope_stack[i]);
  }
}


/* scope - entry function for scope pass
 *         calls scope_fun() for each function in module mod
 * 9/9/96 - echris
 */

static int scope(module_t *mod, char *s)
{
  function_t *ftp;
  int i;

  /* intialize scope_stack */
  for (i=0; i<MAXRANK; i++) {
    scope_stack[i] = g_new_stack();
  }

  /* call scope_fun() for each function in mod */
  while (mod) {
    ftp = T_FCNLS(mod);
    INT_COND_FATAL((ftp!=NULL), NULL, "No functions in mod in scope()");
    while (ftp) {
      scope_fun(ftp);
      ftp = T_NEXT(ftp);
    }
    mod = T_NEXT(mod);
  }

  /* destroy scope_stack */
  for (i=0; i<MAXRANK; i++) {
    g_free_stack(scope_stack[i]);
  }

  return (0);                  /* this pass always "succeeds" */
}


/* function: call_scope - scope resolution pass
 * date: 8 September 1996
 * creator: echris
 */

int call_scope(module_t *mod,char *s) {
  return (scope(mod, s));
}


