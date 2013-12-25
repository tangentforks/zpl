/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** FILE:      commstats.c
 *** PURPOSE:   Emit code to measure time spent performing communication
 ***             or the dual (time spent performing computation).
 *** AUTHOR:    Sung-Eun Choi (sungeun@cs.washington.edu)
 *** DATE:      17 April 1998
 ***
 *** NOTES:     This pass inserts START_COMM and END_COMM macros around
 ***            Ironman communication routines.  It also inserts
 ***            START_COMM_TIMING and END_COMM_TIMING macros where the
 ***            user specified ResetTimer() and CheckTimer().  The
 ***            PRINT_COMM_TIMING macro is inserted after the call to
 ***            CheckTimer();
 ***
 ***            For now, these macros are defined at the top of the
 ***            first file generated.
 ***
 ***/

#include <stdio.h>
#include <string.h>
#include "../include/error.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"
#include "../include/symmac.h"
#include "../include/global.h"
#include "../include/buildzplstmt.h"
#include "../include/depgraph.h"
#include "../include/callgraph.h"
#include "../include/bb_traverse.h"
#include "../include/commstats.h"
#include "../include/traverse.h"
#include "../include/stmtutil.h"

static void commstats_per_module(module_t *);
static statement_t *insert_timing(statement_t *);
static int insert_init(function_t *);
static int rt_found = FALSE;
static int ct_found = FALSE;

int
call_commstats(module_t *mod, char *options)
{
  if (commstats || compstats) {

    /*** insert timing calls ***/
    traverse_modules(mod, TRUE, commstats_per_module, NULL);

    /*** insert initialization ***/
    traverse_functions(insert_init);

    if (rt_found == FALSE) {
      USR_WARN(NULL, "ResetTimer() never called.  "
	       "Communication statistics may be inaccurate.");
    }
    if (ct_found == FALSE) {
      USR_WARN(NULL, "CheckTimer() never called.  "
	       "Communication statistics may be inaccurate.");
    }
  }

  return 0;

}

static void
commstats_per_module(module_t *mod)
{
  /*** for each basic block ***/
  bb_traversal(mod, insert_timing, BBTR_NONE);
}

/*** insert macros for timing ***/
static statement_t *
insert_timing(statement_t *stmtls)
{
  statement_t *stmt, *new;
  int previous = FALSE;

  for (stmt = stmtls; stmt != NULL; stmt = T_NEXT(stmt)) {
    switch (T_TYPE(stmt)) {
    case S_COMM:
      /*** insert START_COMM macro ***/
      if (!previous) {
	new = build_commstat_statement(START_COMM, T_LINENO(stmt),
				       T_FILENAME(stmt));
	insertbefore_stmt(new, stmt);
	previous = TRUE;
      }
      if ((T_NEXT(stmt) == NULL) || (T_TYPE(T_NEXT(stmt)) != S_COMM)) {
	/*** insert END_COMM macro ***/
	new = build_commstat_statement(END_COMM, T_LINENO(stmt),
				       T_FILENAME(stmt));
	insertafter_stmt(new, stmt);
	stmt = new;
      }
      break;
    default:
      previous = FALSE;
      break;
    }
  }

  for (new = stmtls; T_PREV(new) != NULL; new = T_PREV(new)) ;

  return new;

}

/*** insert macros for initialization ***/
static int
insert_init(function_t *f)
{
  calledge_t *cgedge;
  statement_t *stmt, *new;

  for (cgedge = CG_EDGE(T_CGNODE(f));
       cgedge != NULL; cgedge = CG_NEXT(cgedge)) {
    if (!strcmp(S_IDENT(CG_SINK(cgedge)), "ResetTimer")) {
      /*** call to ResetTimer() ***/
      if (rt_found == TRUE) {
	USR_WARN(stmt, "ResetTimer() called more than once.  ",
		 "Communication statistics may be inaccurate.");
      }
      else {
	rt_found = TRUE;
      }
      stmt = T_STMT(CG_CALL(cgedge));
      new = build_commstat_statement(START_COMM_TIMING,
				     T_LINENO(stmt), T_FILENAME(stmt));
      insertafter_stmt(new, stmt);
    }
    else if (!strcmp(S_IDENT(CG_SINK(cgedge)), "CheckTimer")) {
      /*** call to CheckTimer() ***/
      if (ct_found == TRUE) {
	USR_WARN(stmt, "CheckTimer() called more than once.  ",
		 "Communication statistics may be inaccurate.");
      }
      else {
	ct_found = TRUE;
      }
      stmt = T_STMT(CG_CALL(cgedge));
      new = build_commstat_statement(END_COMM_TIMING,
				     T_LINENO(stmt), T_FILENAME(stmt));
      insertbefore_stmt(new, stmt);
      new = build_commstat_statement(PRINT_COMM_TIMING,
				     T_LINENO(stmt), T_FILENAME(stmt));
      insertbefore_stmt(new, stmt);
    }
  }

  return 0;

}


