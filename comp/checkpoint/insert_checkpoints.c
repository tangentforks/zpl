/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** FILE: insert_checkpoints.c - insert checkpoint(s) into user code
 *** DATE: 4 september 2001
 *** CREATOR: Sung-Eun Choi (sungeun@lanl.gov)
 ***
 *** SUMMARY
 *** first cut heuristic, as suggested by Steve Deitz
 ***  - single checkpoint call in every loop nest
 ***  - location determined by the minimum NUMBER of live variables
 ***  - NOT in a conditional
 ***
 *** Note that this pass must run BEFORE insert_mloops,
 ***   but BEFORE insert_comm
 **
 ***
 ***/

#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "../include/db.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/buildstmt.h"
#include "../include/buildsymutil.h"
#include "../include/stmtutil.h"
#include "../include/glist.h"
#include "../include/glistmac.h"
#include "../include/macros.h"
#include "../include/passes.h"
#include "../include/statement.h"
#include "../include/traverse.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/live.h"

static int num_inserted = 0;

typedef struct ckpt_location {
  statement_t *loop;     /*** the outermost loop ***/
  int live_arrays;       /*** number of live array variables ***/
  int live_scalars;      /*** number of live scalar variable ***/
  statement_t *location; /*** insert ckpt BEFORE this statement ***/

  int max_live_arrays;   /*** for accounting purposes ***/
  int max_live_scalars;  /***  keep track of the worst spot ***/
  statement_t *max_loc;  /***  to place a checkpoint ***/
} ckpt_location_t;

static glist checkpoints = NULL;

static int
loop_equal(void *s, void *n) {
  if (((ckpt_location_t *) n)->loop == (statement_t *) s) {
    return 1;
  } else {
    return 0;
  }
}

#ifdef _DA_OLD_WAY_

static void
insert_checkpoints(statement_t *s) {
  ckpt_location_t *ckpt;
  statement_t *ts;

  if (s == NULL) {
    return;
  }

  DBS1(10, "statement at line %d...\n", T_LINENO(s));

  if ((T_TYPE(s) == S_LOOP) &&
      ((T_PARENT(s) == NULL) ||
       ((T_PARENT(s) != NULL) && (T_TYPE(T_PARENT(s)) != S_LOOP)))) {
    /***
     *** this is an outermost loop
     *** add it to list of checkpoint locations
     ***/
    DBS0(10, "\t*** found outermost loop...\n");
    INT_COND_FATAL((glist_find_equal(checkpoints, (void *) s, loop_equal) == NULL),
		   s, "Should not travese statements more than once!\n");
    ckpt = (ckpt_location_t *) malloc(sizeof(ckpt_location_t));
    ckpt->loop = s;
    ckpt->live_arrays = INT_MAX;
    ckpt->live_scalars = 0;
    ckpt->location = NULL;
    ckpt->max_live_arrays = 0;
    ckpt->max_live_scalars = 0;
    ckpt->max_loc = NULL;
    checkpoints = glist_prepend(checkpoints, (void *) ckpt, GLIST_NODE_SIZE);
  } else {
    /***
     *** check if we're in a loop
     ***/
    ckpt = NULL;
    for (ts = T_PARENT(s); ts != NULL; ts = T_PARENT(ts)) {
      if ((T_TYPE(ts) == S_IF) || (T_TYPE(ts) == S_MSCAN)) {
	/*** ignore conditionals and mscans within loops ***/
	break;
      } else if (T_TYPE(ts) == S_LOOP) {
	gnode_t *node = glist_find_equal(checkpoints,
					 (void *) ts, loop_equal);
	if (node != NULL) {
	  ckpt = (ckpt_location_t *) GLIST_DATA(node);
	  break;
	}
      }
    }

    if (ckpt != NULL) {
      /*** consider this statement for checkpoint location ***/
      int live_arrays = num_live_array_vars(s);
      int live_scalars = num_live_vars(s)-live_arrays;
      DBS2(10, "\t*** %d LIVE ARRAY VARIABLES, %d SCALARS\n",
	   live_arrays, live_scalars);
      /*** NOTE: FIRST PART OF CONDITIONAL IS A HACK TO TAKE CARE
       ***       OF STATEMENTS WITHOUT EXPRESSIONS TO TRAVERSE, E.G.,
       ***       S_MLOOP, S_NLOOP, ETC.  THE IDFA PASS DOESN'T ENABLE
       ***       A WAY TO ASSIGN TO A FIELD OF THE STATEMENT.
       ***
       ***       IF num_live_vars(s) == 0 THEN s IS SUCH A STATEMENT.
       ***/
      if ((live_arrays+live_scalars != 0) &&
	  (live_arrays <= ckpt->live_arrays)) {
	if ((live_arrays < ckpt->live_arrays) ||
	    ((live_arrays == ckpt->live_arrays) &&
	     (live_scalars < ckpt->live_scalars))) {
	  DBS0(10, "\t*** NEW MIN ***\n")
	  ckpt->live_arrays = live_arrays;
	  ckpt->live_scalars = live_scalars;
	  ckpt->location = s;
	}
	IFDB(5) {
	  if ((live_arrays > ckpt->max_live_arrays) ||
	      ((live_arrays == ckpt->max_live_arrays) &&
	       (live_scalars > ckpt->max_live_scalars))) {
	    DBS0(10, "\t*** NEW MAX ***\n")
	      ckpt->max_live_arrays = live_arrays;
	    ckpt->max_live_scalars = live_scalars;
	    ckpt->max_loc = s;
	  }
	}
      }
    }
  }
}
#else

static void
insert_checkpoints(statement_t *s) {
  ckpt_location_t *ckpt;
  statement_t *ts;

  if (s == NULL) {
    return;
  }

  DBS1(10, "statement at line %d...\n", T_LINENO(s));

  if ((T_TYPE(s) == S_LOOP) &&
      ((T_PARENT(s) == NULL) ||
       ((T_PARENT(s) != NULL) && (T_TYPE(T_PARENT(s)) != S_LOOP)))) {
    /***
     *** this is an outermost loop
     *** add it to list of checkpoint locations
     ***/
    DBS0(10, "\t*** found outermost loop...\n");
    INT_COND_FATAL((glist_find_equal(checkpoints, (void *) s, loop_equal) == NULL),
		   s, "Should not travese statements more than once!\n");
    ckpt = (ckpt_location_t *) malloc(sizeof(ckpt_location_t));
    ckpt->loop = s;
    ckpt->live_arrays = INT_MAX;
    ckpt->live_scalars = 0;
    ckpt->location = NULL;
    ckpt->max_live_arrays = 0;
    ckpt->max_live_scalars = 0;
    ckpt->max_loc = NULL;
    checkpoints = glist_prepend(checkpoints, (void *) ckpt, GLIST_NODE_SIZE);
  } else {
    /***
     *** only do this for MLOOPS
     ***/
    if (T_TYPE(s) == S_MLOOP) {
      /***
       *** check if we're in a loop
       ***/
      ckpt = NULL;
      for (ts = T_PARENT(s); ts != NULL; ts = T_PARENT(ts)) {
	if ((T_TYPE(ts) == S_IF) || (T_TYPE(ts) == S_MSCAN)) {
	  /*** ignore conditionals and mscans within loops ***/
	  break;
	} else if (T_TYPE(ts) == S_LOOP) {
	  gnode_t *node = glist_find_equal(checkpoints,
					   (void *) ts, loop_equal);
	  if (node != NULL) {
	    ckpt = (ckpt_location_t *) GLIST_DATA(node);
	    break;
	  }
	}
      }

      if (ckpt != NULL) {
	/*** consider this statement for checkpoint location ***/
	int live_arrays = num_live_array_vars(s);
	int live_scalars = num_live_vars(s)-live_arrays;
	DBS2(10, "\t*** %d LIVE ARRAY VARIABLES, %d SCALARS\n",
	     live_arrays, live_scalars);
	/*** NOTE: FIRST PART OF CONDITIONAL IS A HACK TO TAKE CARE
	 ***       OF STATEMENTS WITHOUT EXPRESSIONS TO TRAVERSE, E.G.,
	 ***       S_NLOOP.  THE IDFA PASS DOESN'T ENABLE A WAY TO
	 ***       ASSIGN TO A FIELD OF THE STATEMENT SO THE LIVE
	 ***       VAR LIST IS NULL.
	 ***
	 ***       IF num_live_vars(s) == 0 THEN s IS SUCH A STATEMENT.
	 ***/
	if ((live_arrays+live_scalars != 0) &&
	    (live_arrays <= ckpt->live_arrays)) {
	  if ((live_arrays < ckpt->live_arrays) ||
	      ((live_arrays == ckpt->live_arrays) &&
	       (live_scalars < ckpt->live_scalars))) {
	    DBS0(10, "\t*** NEW MIN ***\n")
	      ckpt->live_arrays = live_arrays;
	    ckpt->live_scalars = live_scalars;
	    ckpt->location = s;
	  }
	  IFDB(5) {
	    if ((live_arrays > ckpt->max_live_arrays) ||
		((live_arrays == ckpt->max_live_arrays) &&
		 (live_scalars > ckpt->max_live_scalars))) {
	      DBS0(10, "\t*** NEW MAX ***\n")
		ckpt->max_live_arrays = live_arrays;
	      ckpt->max_live_scalars = live_scalars;
	      ckpt->max_loc = s;
	    }
	  }
	}
      }
    }
  }
}
#endif

int
call_insert_checkpoints(module_t *modls, char *s) {
  module_t* mod;
  symboltable_t* pst;
  statement_t* stmt;
  expr_t *e;
  ckpt_location_t *ckpt;

  if (do_insert_checkpoints) {
    for (mod = modls; mod != NULL; mod = T_NEXT(mod)) {
      for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
	if (S_CLASS(pst) == S_SUBPROGRAM) {
	  if ((T_PARALLEL(S_FUN_BODY(pst))) &&
	      ((stmt = T_STLS(S_FUN_BODY(pst))) != NULL)) {
	    traverse_stmtls(T_STLS(T_CMPD(stmt)), 0, insert_checkpoints, 0, 0);
	  }
	}
      }
    }

    if ((num_inserted = glist_length(checkpoints)) > 0) {
      for (checkpoints = glist_pop_head(checkpoints, (void **) &ckpt);
	   ckpt != NULL;
	   checkpoints = glist_pop_head(checkpoints, (void **) &ckpt)) {
	e = build_function_call(lu_pst("checkpoint"), 0);
	stmt = build_expr_statement(e, T_LINENO(ckpt->location),
				    T_FILENAME(ckpt->location));
	insertbefore_stmt(stmt, ckpt->location);
	T_LIVE(stmt) = copy_live_var_list(T_LIVE(ckpt->location));
	if (quiet == FALSE) {
	  printf("***\n");
	  printf("*** INSERTED CHECKPOINT at for outermost loop at line %d\n",
		 T_LINENO(ckpt->loop));
	  printf("***  at line %d (%d live array vars, %d scalars)\n",
		 T_LINENO(ckpt->location), ckpt->live_arrays, ckpt->live_scalars);
	  IFDB(5) {
	    printf("***  worst location at line %d (%d live array vars, %d scalars)\n",
		   T_LINENO(ckpt->max_loc),
		   ckpt->max_live_arrays, ckpt->max_live_scalars);
	  }
	  IFDB(10) {
	    print_live_vars(ckpt->location);
	  }
	  printf("***\n");
	}
	free(ckpt);
      }

      /*** do codegen for checkpointing ***/
      do_checkpoint = TRUE;

    }

    if (quiet == FALSE) {
      printf("***\n*** %d TOTAL CHECKPOINTS INSERTED\n***\n", num_inserted);
    }
  }

  return (0);
}
