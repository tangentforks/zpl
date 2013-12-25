/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/*** FILE: live.c - live variable analysis
 *** DATE: 4 september 2001
 *** CREATOR: Sung-Eun Choi (sungeun@lanl.gov)
 ***
 *** PRE-REQS: depend_array
 ***
 *** SUMMARY
 *** This pass fills in T_LIVE for each statement (just a glist for now)
 *** via IDFA.   It uses in/out sets to perform the analysis.
 ***
 ***/

#include <stdio.h>
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/glist.h"
#include "../include/glistmac.h"
#include "../include/idfa_traverse.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/macros.h"
#include "../include/passes.h"
#include "../include/statement.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/live.h"

static void
print_live_var(void *var) {
  printf("\t%s\n", S_IDENT((symboltable_t *) var));
}

static int
equal_live_vars(void *v1, void *v2) {
  if (v1 == v2) {
    return(1);
  } else {
    return(0);
  }
}

void
print_live_vars(statement_t *s) {
  glist_apply(T_LIVE(s), print_live_var);
}

int
num_live_vars(statement_t *s) {
  return(glist_length(T_LIVE(s)));
}

static int total_arrays;
static void
is_array_var(void *var) {
  IFDB(20) {
    printf("%s is of type ", S_IDENT((symboltable_t *) var));
    switch(S_CLASS(S_DTYPE((symboltable_t *) var))) {
    case DT_INTEGER:
      printf("DT_INTEGER");
      break;
    case DT_REAL:
      printf("DT_REAL");
      break;
    case DT_CHAR:
      printf("DT_CHAR");
      break;
    case DT_ARRAY:
      printf("DT_ARRAY");
      break;
    case DT_ENSEMBLE:
      printf("DT_ENSEMBLE");
      break;
    case DT_FILE:
      printf("DT_FILE");
      break;
    case DT_ENUM:
      printf("DT_ENUM");
      break;
    case DT_STRING:
      printf("DT_STRING");
      break;
    case DT_STRUCTURE:
      printf("DT_STRUCTURE");
      break;
    case DT_VOID:
      printf("DT_VOID");
      break;
    case DT_SUBPROGRAM:
      printf("DT_SUBPROGRAM");
      break;
    case DT_REGION:
      printf("DT_REGION");
      break;
    case DT_SHORT:
      printf("DT_SHORT");
      break;
    case DT_LONG:
      printf("DT_LONG");
      break;
    case DT_DOUBLE:
      printf("DT_DOUBLE");
      break;
    case DT_QUAD:
      printf("DT_QUAD");
      break;
    case DT_UNSIGNED_INT:
      printf("DT_UNSIGNED_INT");
      break;
    case DT_UNSIGNED_SHORT:
      printf("DT_UNSIGNED_SHORT");
      break;
    case DT_UNSIGNED_LONG:
      printf("DT_UNSIGNED_LONG");
      break;
    case DT_SIGNED_BYTE:
      printf("DT_SIGNED_BYTE");
      break;
    case DT_UNSIGNED_BYTE:
      printf("DT_UNSIGNED_BYTE");
      break;
    case DT_GENERIC:
      printf("DT_GENERIC");
      break;
    case DT_GENERIC_ENSEMBLE:
      printf("DT_GENERIC_ENSEMBLE");
      break;
    case DT_PROCEDURE:
      printf("DT_PROCEDURE");
      break;
    case DT_BOOLEAN:
      printf("DT_BOOLEAN");
      break;
    case DT_OPAQUE:
      printf("DT_OPAQUE");
      break;
    case DT_COMPLEX:
      printf("DT_COMPLEX");
      break;
    case DT_DCOMPLEX:
      printf("DT_DCOMPLEX");
      break;
    case DT_QCOMPLEX:
      printf("DT_QCOMPLEX");
      break;
    default:
      INT_FATAL(NULL, "Invalid data type (%d)\n",
		S_CLASS(S_DTYPE((symboltable_t *) var)));
    }
    printf(" (%d)\n", S_CLASS(S_DTYPE((symboltable_t *) var)));
  }

  if (D_IS_ANY_ENSEMBLE(S_DTYPE((symboltable_t *) var))) {
    total_arrays++;
  }
}

int
num_live_array_vars(statement_t *s) {
  total_arrays = 0;
  glist_apply(T_LIVE(s), is_array_var);
  return(total_arrays);
}

int
is_live(statement_t *s, symboltable_t *var) {
  if (glist_find_equal(T_LIVE(s), (void *) var, equal_live_vars)) {
    return(1);
  } else {
    return(0);
  }
}


static void *
copy_live_var(void *v) {
  return(v);
}

glist
copy_live_var_list(glist l) {
  return glist_copy(l, GLIST_NODE_SIZE, copy_live_var);
}

static void *
live_stmt_fn(statement_t *stmt, void *in, int final) {
  set_t *ts;
  glist currlive;

  DBS1(10,"in live_stmt_fn for stmt %d...\n",T_LINENO(stmt));
  IFDB(20) {
    DBS0(20, "T_LIVE(stmt) =\n");
    glist_apply(T_LIVE(stmt), print_live_var);
    DBS0(20, "LIVE in =\n")
    glist_apply((glist) in, print_live_var);
  }

  currlive = glist_join((glist) in, T_LIVE(stmt),
			GLIST_NODE_SIZE, equal_live_vars);
  /*** remove vars in the outset from the list of live variables ***/
  for (ts = T_OUT(stmt); ts != NULL; ts = SET_NEXT(ts)) {
    if ((SET_VAR(ts) != ExternalDep) &&
	((S_CLASS(SET_VAR(ts)) == S_VARIABLE) || 
	 (S_CLASS(SET_VAR(ts)) == S_PARAMETER))) {
      DBS2(20, "*** removing %s from currlive=%p\n",
	   S_IDENT(SET_VAR(ts)), (void *) currlive);
      currlive = glist_remove_equal(currlive, (void *) SET_VAR(ts),
				    equal_live_vars);
    }
  }

  /*** add vars in the inset to the list of live variables ***/
  for (ts = T_IN(stmt); ts != NULL; ts = SET_NEXT(ts)) {
    if ((SET_VAR(ts) != ExternalDep) &&
	((S_CLASS(SET_VAR(ts)) == S_VARIABLE) ||
	 (S_CLASS(SET_VAR(ts)) == S_PARAMETER))) {
      DBS2(20, "*** adding %s to currlive=%p\n",
	   S_IDENT(SET_VAR(ts)), (void *) currlive);
      currlive = glist_prepend_new(currlive, (void *) SET_VAR(ts),
				   GLIST_NODE_SIZE, equal_live_vars);
    }
  }

  T_LIVE(stmt) = currlive;
  /*** sungeun *** hack for MLOOPS ***/
  if (T_IN_MLOOP(stmt) != NULL) {
    T_LIVE(T_IN_MLOOP(stmt)) = glist_join(T_LIVE(T_IN_MLOOP(stmt)),
					  glist_copy(currlive,
						     GLIST_NODE_SIZE,
						     copy_live_var),
					  GLIST_NODE_SIZE, equal_live_vars);
  }

  IFDB(15) {
    DBS0(15, "currlive =\n");
    glist_apply(currlive, print_live_var);
  }

  return(currlive);
}


static void *
live_merge_fn(void *one, void *two) {
  return(glist_join((glist) two,
		    glist_copy((glist) one, GLIST_NODE_SIZE, copy_live_var),
		    GLIST_NODE_SIZE, equal_live_vars));
}

static int
live_equal_fn(void *one, void *two) {
  return(glist_equal((glist) one, (glist) two, equal_live_vars));
}


static void *
live_copy_fn(void *blah) {
  return(glist_copy((glist) blah, GLIST_NODE_SIZE, copy_live_var));
}


static void
live_fun(function_t *fcn) {
  statement_t *stmtls;
  idfa_t idfa_dat;

  stmtls = T_STLS(fcn);

  INT_COND_FATAL((stmtls!=NULL), NULL, "NULL stmtls in live_fun()");
  IDFA_STMT_FN(&idfa_dat) = live_stmt_fn;
  IDFA_MERGE_FN(&idfa_dat) = live_merge_fn;
  IDFA_EQUAL_FN(&idfa_dat) = live_equal_fn;
  IDFA_COPY_FN(&idfa_dat) = live_copy_fn;
  IDFA_DIR(&idfa_dat) = REVERSE;

  no_iterate = FALSE;     /* don't iterate to fixed point */

  idfa_stmtls(stmtls, (void *)NULL, &idfa_dat, 1);

}

static int
live(module_t *mod, char *s) {
  function_t *ftp;

  while (mod) {
    ftp = T_FCNLS(mod);
    INT_COND_FATAL((ftp!=NULL), NULL, "No functions in mod in dead()");
    while (ftp) {
      live_fun(ftp);
      ftp = T_NEXT(ftp);
    }
    mod = T_NEXT(mod);
  }

  return (0);
}


int
call_live(module_t *mod,char *s) {
  return (live(mod, s));
}
