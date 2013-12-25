/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* Rewritten by Brad to use E's new interprocedural framework 11/13/98 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/datatype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/interproc.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/statement.h"
#include "../include/symmac.h"
#include "../include/symboltable.h"
#include "../include/symtab.h"
#include "../include/traverse.h"
#include "../include/treemac.h"


static int made_mod;


/* Here starts the local rank determination stuff */


static void rankres_stmt(statement_t *stmt) {
  expr_t *reg;
  int rank=0;
  function_t *fn;
  expr_t* curreg;
  int covrank;
  
  if (T_TYPE(stmt) == S_REGION_SCOPE) {
    reg = T_REGION_SYM(T_REGION(stmt));
    if (datatype_reg_inherits(T_TYPEINFO(reg))) {
      rank = D_REG_NUM(T_TYPEINFO(reg));
    }
  } else {
    rank = stmt_rank(stmt);
  }
  
  if (rank != 0) {
    curreg = RMSCurrentRegion();
    covrank = expr_is_qreg(curreg);
    if (covrank == -1 || covrank == rank) {
      fn = T_PARFCN(stmt);
      if (T_UNC_STMT(fn,rank) == NULL) {
	T_UNC_STMT(fn,rank) = stmt;
      }
    } else {
      covrank = D_REG_NUM(T_TYPEINFO(curreg));
      if (rank != covrank) {
	if (T_TYPE(stmt) == S_REGION_SCOPE) {
	  /* list region scopes that don't match their cover later */
	  fn = T_PARFCN(stmt);
	  if (T_UNC_STMT(fn,rank) == NULL) {
	    T_UNC_STMT(fn,rank) = stmt;
	  }
	} else {
	  /* list other statements now (could do later, but I'm being
	     lazy at architecting postponement of two different errors */
	  USR_FATAL_CONT(stmt, "Statement/region rank mismatch");
	}
      }
    }
  }
}


static void rankres_local(module_t *mod) {
  function_t *fn;
  int i;
  int uncovered;

  fn = T_FCNLS(mod);
  while (fn != NULL) {
    for (i=0;i<=MAXRANK;i++) {
      T_UNC_STMT(fn,i) = NULL;
    }
    traverse_stmtls_g(T_STLS(fn),rankres_stmt,NULL,NULL,NULL);
    uncovered = 0;
    for (i=0;i<MAXRANK+1;i++) {
      if (T_UNC_STMT(fn,i)) {
	uncovered = 1;
	DBS2(2,"  Locally-uncovered statement of rank %d: %d\n",i,
	     T_LINENO(T_UNC_STMT(fn,i)));
      }
    }
    if (uncovered) {
      add_callees(fn);
    }
    fn = T_NEXT(fn);
  }
}


static void rankres_func(expr_t *expr) {
  function_t *caller;
  function_t *callee;
  int i;

  if (T_TYPE(expr) == FUNCTION) {
    caller = T_PARFCN(T_STMT(expr));
    callee = S_FUN_BODY(T_IDENT(T_OPLS(expr)));
    for (i=1;i<=MAXRANK;i++) {
      if ((T_UNC_STMT(callee,i) != NULL || T_UNC_FUNC(callee,i) != NULL) &&
	  (T_UNC_FUNC(caller,i) == NULL) && 
	  expr_is_qreg(RMSCurrentRegion())) {
	T_UNC_FUNC(caller,i) = expr;
	made_mod = 1;
      }
    }
  }
}


static void rankres_expr(expr_t *expr) {
  traverse_expr(expr,FALSE,rankres_func);
}


static int rankres_prop(function_t *fn,glist junk) {
  DBS1(2,"rankres considering %s...\n",S_IDENT(T_FCN(fn)));
  made_mod = 0;
  traverse_stmtls_g(T_STLS(fn),NULL,NULL,rankres_expr,NULL);
  if (made_mod) {
    add_callees(fn);
  }
  return 0;
}


static void rankres_global(module_t *mod) {
  RMSInit();
  ip_analyze(mod,IPDN,rankres_local,rankres_prop,NULL);
  RMSFinalize();
}


static void rankres_report_errors(function_t *fn,char *callchain) {
  int i;
  int recurse;
  char *newcallchain;
  char *fnname;
  expr_t *func_cache[MAXRANK+1];  /* used to prevent infinite recursion */

  for (i=1;i<=MAXRANK;i++) {
    if (T_UNC_STMT(fn,i) != NULL) {
      USR_FATAL_CONT(T_UNC_STMT(fn,i),
		     "Statement not covered by a %d-dimensional region%s%s",i,
		     callchain ? " when called from\n       " : "",
		     callchain ? callchain : "");
    }
  }
  recurse = 0;
  for (i=1;i<=MAXRANK;i++) {
    if (T_UNC_FUNC(fn,i) != NULL) {
      recurse = 1;
    }
  }
  if (recurse) {
    for (i=1;i<=MAXRANK;i++) {
      func_cache[i] = T_UNC_FUNC(fn,i);
      T_UNC_FUNC(fn,i) = NULL;
    }
    fnname = S_IDENT(T_FCN(fn));
    if (callchain == NULL) {
      newcallchain = malloc(strlen(fnname)+14);
      sprintf(newcallchain,"%s",fnname);
    } else {
      newcallchain = malloc(strlen(callchain)+strlen(fnname)+16);
      sprintf(newcallchain,"%s->%s",callchain,fnname);
    }
    for (i=1;i<=MAXRANK;i++) {
      if (func_cache[i] != NULL) {
	sprintf(newcallchain,"%s:%d",newcallchain,
		T_LINENO(T_STMT(func_cache[i])));
	rankres_report_errors(S_FUN_BODY(T_IDENT(T_OPLS(func_cache[i]))),
			      newcallchain);
      }
    }
    free(newcallchain);
    for (i=1;i<=MAXRANK;i++) {
      T_UNC_FUNC(fn,i) = func_cache[i];
    }
  }
}


int call_rank_res(module_t *mod, char *s) {
/*  traverse_modules(mod,TRUE,rankres_local,NULL);*/
  FATALCONTINIT();
  traverse_modules(mod,TRUE,rankres_global,NULL);
  rankres_report_errors(S_FUN_BODY(pstMAIN),NULL);
  FATALCONTDONE();

  return 0;
}
