/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** permcomm.c: stuff statements between communication for permutations
 ***
 *** Sung-Eun Choi (sungeun@lanl.gov)
 *** 17 September 2002
 ***
 *** NOTES:
 ***
 *** The algorithm is pretty naive and conservative.
 ***
 *** - pull statement s up between DR and DN, iff
 ***   for all dep=T_INDEP(s), INDEP_TAIL_STMT(dep) dominates DN
 *** - push statement s down between IR and IN, iff
 ***   for all dep=T_OUTDEP(s), IR dominates OUTDEP_HEAD_STMT(dep)
 ***
 ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/comm.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"
#include "../include/traverse.h"
#include "../include/error.h"
#include "../include/stmtutil.h"
#include "../include/depgraph.h"

/*** for debugging ***/
#include "../include/db.h"
#include "../include/dbg_code_gen.h"


int call_permcomm(module_t *, char *);

static statement_t **stuff_after = NULL;
static int stuff_after_size = 0;
static int stuff_after_loc = 0;
static statement_t **stuff_before = NULL;
static int stuff_before_size = 0;
static int stuff_before_loc = 0;

#define T_MOVED(s) (s)->user1_p

static const int not_moved = 0;
static const int have_moved = 1;
static const int do_not_move = 2;

static void
init_stmt(statement_t *s) {
  /*** do not move the following statement types because... ***/
  if ((T_TYPE(s) == S_IO) ||    /*** might have comm ***/
      (T_TYPE(s) == S_COMM) ||  /*** COMM is comm ***/
      (T_TYPE(s) == S_WRAP) ||  /*** WRAP is comm ***/
      (T_TYPE(s) == S_REFLECT)) /*** REFLECT is comm ***/ {
    T_MOVED(s) = (int *) &do_not_move;
  } else {
    T_MOVED(s) = (int *) &not_moved;
  }
}

static void
find_perms(expr_t *expr)
{
  if (expr == NULL || T_TYPE(expr) != FUNCTION) {
    return;
  }

  /*** DR/DN separation *** stuff statements before DN ***/
  if (strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_CALL_PERM_DN") == 0) {
    INT_COND_FATAL((T_EXPR(T_STMT(expr)) == expr), T_STMT(expr),
		   "_CALL_PERM_DN improperly inserted");
    if (stuff_before_loc >= stuff_before_size) {
      statement_t **tsl = (statement_t **) PMALLOC(2*stuff_before_size*
						   sizeof(statement_t *));
      memcpy(tsl, stuff_before, stuff_before_size*sizeof(statement_t *));
      PFREE(stuff_before, stuff_before_size*sizeof(statement_t *));
      stuff_before = tsl;
      stuff_before_size *= 2;
    }
    stuff_before[stuff_before_loc++] = T_STMT(expr);
  }

  /*** IR/IN separation *** stuff statements after IR ***/
  if (strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_CALL_PERM_IR") == 0) {
    INT_COND_FATAL((T_EXPR(T_STMT(expr)) == expr), T_STMT(expr),
		   "_CALL_PERM_IR improperly inserted");
    if (stuff_after_loc >= stuff_after_size) {
      statement_t **tsl = (statement_t **) PMALLOC(2*stuff_after_size*
						   sizeof(statement_t *));
      memcpy(tsl, stuff_after, stuff_after_size*sizeof(statement_t *));
      PFREE(stuff_after, stuff_after_size*sizeof(statement_t *));
      stuff_after = tsl;
      stuff_after_size *= 2;
    }
    stuff_after[stuff_after_loc++] = T_STMT(expr);
  }

}

static int
legal_indeps(statement_t* s, statement_t *insertion_point) {
  indep_t *dep;

  for (dep = T_INDEP(s); dep != NULL; dep = INDEP_NEXT(dep)) {
    /*** don't muck with back arcs *** too confusing ***/
    if (INDEP_BACK(dep)) {
      return 0;
    }
    if (INDEP_TYPE(dep) != XINPUT) {
      if ((T_TYPE(INDEP_EXPR(dep)) != VARIABLE) ||
	  ((T_TYPE(INDEP_EXPR(dep)) == VARIABLE) &&
	   (T_IDENT(INDEP_EXPR(dep)) != ExternalDep))) {
	/*** don't look any more ***/
	if (!(INDEP_TAIL_STMT(dep) != insertion_point &&
	      bb_dominates(INDEP_TAIL_STMT(dep), insertion_point))) {
	  if ((s->mloop_p != NULL) &&
	      (s->mloop_p == INDEP_TAIL_STMT(dep)->mloop_p)) {
	    /*** this is okay because statements are in the same mloop ***/
	    /***  and only entire mloops are moved ***/
	  } else {
	    return 0;
	  }
	}
      }
    }
  }

  return 1;
}

static int
legal_outdeps(statement_t* s, statement_t *insertion_point) {
  outdep_t *dep;

  for (dep = T_OUTDEP(s); dep != NULL; dep = OUTDEP_NEXT(dep)) {
    /*** don't muck with back arcs *** too confusing ***/
    if (OUTDEP_BACK(dep)) {
      return 0;
    }
    if (OUTDEP_TYPE(dep) != XINPUT) {
      if ((T_TYPE(OUTDEP_EXPR(dep)) != VARIABLE) ||
	  ((T_TYPE(OUTDEP_EXPR(dep)) == VARIABLE) &&
	   (T_IDENT(OUTDEP_EXPR(dep)) != ExternalDep))) {
	/*** don't look any more ***/
	if (!(insertion_point != OUTDEP_HEAD_STMT(dep) &&
	      bb_dominates(insertion_point, OUTDEP_HEAD_STMT(dep)))) {
	  return 0;
	}
      }
    }
  }

  return 1;

}

static int
stuff_stmts(statement_t *pst, int before) {
  statement_t *s, *ts, *insertion_point = pst;
  int legal, stop = 0;
  int num_stuffed = 0;

  if (pst == NULL) {
    return 0;
  }

  if (before) {
    /*** for each statement AFTER pst ***/
    s = T_NEXT(pst);
  } else {
    /*** for each statement BEFORE pst ***/
    s = T_PREV(pst);
  }

  while (s != NULL) {
    if (T_MOVED(s) == &not_moved) {
      IFDB(10) {
	fprintf(stdout, "\t*** considering moving statement on line %d:\n\t",
		T_LINENO(s));
	dbg_gen_stmt(stdout, s);
      }
      switch (T_TYPE(s)) {
      case S_MLOOP:
	/*** must look inside mloops ***/
	/*** see Brad's comments in degraph.c ***/

	ts = T_MLOOP_BODY(T_MLOOP(s));
	if (!before) {
	  if (ts != NULL) {
	    while (T_NEXT(ts) != NULL) {
	      ts = T_NEXT(ts);
	    }
	  }
	}
	while (ts != NULL) {
	  if (before) {
	    if ((legal = legal_indeps(ts, insertion_point)) == 0) {
	      break;
	    }
	    ts = T_NEXT(ts);
	  } else {
	    if ((legal = legal_outdeps(ts, insertion_point)) == 0) {
	      break;
	    }
	    ts = T_PREV(ts);
	  }
	}
	break;
      default:
	if (before) {
	  legal = legal_indeps(s, insertion_point);
	} else {
	  legal = legal_outdeps(s, insertion_point);
	}
	break;
      case S_EXIT:
      case S_END:
      case S_HALT:
      case S_CONTINUE:
      case S_RETURN:
      case S_NLOOP:
      case S_REGION_SCOPE:
      case S_COMPOUND:
      case S_IF:
      case S_LOOP:
	/*** these statement types break basic blocks -- or at least as ***/
	/***  defined in bb_traverse.c, so stop looking now ***/
	legal = 0;
	stop = 1; /*** break out of main loop ***/
	break;
      }
      if (legal) {
	IFDB(5) {
	  fprintf(stdout, "\t*** moved statement on line %d:\n\t", T_LINENO(s));
	  dbg_gen_stmt(stdout, s);
	}
	if (before) {
	  /*** stuff this statement BEFORE the insertion point ***/
	  ts = T_NEXT(s);
	  remove_stmt(s);
	  insertbefore_stmt(s, insertion_point);
	} else {
	  /*** stuff this statement AFTER the insertion point ***/
	  ts = T_PREV(s);
	  remove_stmt(s);
	  insertafter_stmt(s, insertion_point);
	}
	T_MOVED(s) = (int *) &have_moved;
	insertion_point = s;
	s = ts;
	num_stuffed++;
      } else {
	if (stop == 1) {
	  /*** end of basic block ***/
	  break; /*** out of main loop ***/
	}
	if (before) {
	  s = T_NEXT(s);
	} else {
	  s = T_PREV(s);
	}
      }
    } else {
      if (before) {
	s = T_NEXT(s);
      } else {
	s = T_PREV(s);
      }
    }
  }

  return(num_stuffed);
}


int
call_permcomm(module_t *modls, char *s) {
  int i;
  module_t* mod;
  symboltable_t* pst;
  statement_t* stmt;
  int num_stuffed;

  stuff_before_size = 1;
  stuff_before = (statement_t **) PMALLOC(stuff_before_size*sizeof(statement_t *));
  stuff_after_size = 1;
  stuff_after = (statement_t **) PMALLOC(stuff_after_size*sizeof(statement_t *));
  for (mod = modls; mod != NULL; mod = T_NEXT(mod)) {
    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
      if (S_CLASS(pst) == S_SUBPROGRAM) {
	if ((stmt = T_STLS(S_FUN_BODY(pst))) != NULL) {
	  traverse_stmtls(T_STLS(T_CMPD(stmt)), 0, init_stmt, find_perms, 0);
	}
      }
    }
  }

  for (i = 0; i < stuff_before_loc; i++) {
    IFDB(2) {
      fprintf(stdout, "[DR/DN] Considering statement on line %d:\n\t",
	      T_LINENO(stuff_before[i]));
      dbg_gen_stmt(stdout, stuff_before[i]);
    }
    num_stuffed = stuff_stmts(stuff_before[i], 1);
    IFDB(2) {
      fprintf(stdout, "%d statements stuffed before statement.\n",
	      num_stuffed);
    }
  }
  for (i = 0; i < stuff_after_loc; i++) {
    IFDB(2) {
      fprintf(stdout, "[IR/IN] Considering statement on line %d:\n\t",
	      T_LINENO(stuff_after[i]));
      dbg_gen_stmt(stdout, stuff_after[i]);
    }
    num_stuffed = stuff_stmts(stuff_after[i], 0);
    IFDB(2) {
      fprintf(stdout, "%d statements stuffed after statement.\n",
	      num_stuffed);
    }
  }

  PFREE(stuff_before, stuff_before_size*sizeof(statement_t *));
  PFREE(stuff_after, stuff_after_size*sizeof(statement_t *));

  return 0;
}
