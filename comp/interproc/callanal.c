/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/callsite.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/function.h"
#include "../include/interproc.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"


static int made_mod;


static int add_callsite(function_t *callee,cover_t cover,expr_t **actuals) {
  int numactuals;
  callsite_t *listptr;
  callsite_t *new_call;
  callsite_t *last;
  symboltable_t *formals;
  int i;

  DBS1(3,"   call to %s...\n",S_IDENT(T_FCN(callee)));
  numactuals = T_NUM_PARAMS(callee);
  listptr = T_CALLINFO(callee);
  if (listptr == (callsite_t *)-1) {
    listptr = NULL;
  }
  last = NULL;
  while (listptr != NULL) {
    if (match_callsite(listptr,cover,actuals,numactuals)) {
      return 0;
    }
    last = listptr;
    listptr = CALL_NEXT(listptr);
  }
  new_call = new_callsite(cover,actuals,numactuals);
  if (last == NULL) {
    T_CALLINFO(callee) = new_call;
    formals = T_DECL(callee);
    for (i=0;i<numactuals;i++) {
      S_ACTUALS(formals) = &(CALL_ACTUALS(new_call)[i]);
      do {
	formals = S_SIBLING(formals);
      } while (formals && !S_FG_PARAM(formals));
    }
  } else {
    CALL_NEXT(last) = new_call;
    for (i=0;i<numactuals;i++) {
      G_NEXT(&(CALL_ACTUALS(last)[i])) = &(CALL_ACTUALS(new_call)[i]);
    }
  }
  return 1;
}


static void callanal_fncall(expr_t *expr) {
  /* general locals */
  function_t *caller;
  function_t *callee;

  /* region locals */
  expr_t *reg;
  expr_t *mask;
  int with;
  cover_t cover;
  int coverused;
  int propagate_reg;
  int prop_reg[MAXRANK+1];
  int propagate_mask;
  int prop_mask[MAXRANK+1];
  callsite_t *callptr;

  expr_t *actuallist;
  expr_t *actual;
  int numactuals;
  expr_t **actualinfo;
  symboltable_t *actual_pst;
  int propagate_params;
  genlist_t **alias_list;
  expr_t **plug_in;
  expr_t *expr_to_copy;
  int i;

  if (T_TYPE(expr) == FUNCTION) {
    callee = S_FUN_BODY(T_IDENT(T_OPLS(expr)));
    if (!function_internal(callee)) {
      return;
    }
    caller = T_PARFCN(T_STMT(expr));
    actuallist = nthoperand(expr,2);

    numactuals = T_NUM_PARAMS(callee);
    if (numactuals < 0) {
      INT_FATAL(NULL,"BLC -- Didn't count parameters!");
    }

    coverused = 0;
    propagate_reg = 0;
    propagate_mask = 0;
    COV_REG(cover) = NULL;
    COV_MSK(cover) = NULL;
    for (i=1; i<=MAXRANK; i++) {
      prop_reg[i] = 0;
      prop_mask[i] = 0;
      if (T_UNC_STMT(callee,i) || T_UNC_FUNC(callee,i) || /* fn uncovered */
	  (T_PROMOTED_FUNC(expr) && T_TYPEINFO_REG(expr) &&
	   D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr))) == i)) { /* fn promoted */
	coverused = 1;
	reg = RMSCurrentRegion();
	if (!expr_is_qreg(reg)) {
	  COV_REG(cover) = reg;
	} else {
	  propagate_reg = 1;
	  prop_reg[i] = 1;
	}
	mask = RMSCurrentMask(&with);
	if (!expr_is_qmask(mask)) {
	  COV_MSK(cover) = mask;
	  COV_WTH(cover) = with;
	} else {
	  propagate_mask = 1;
	  prop_mask[i] = 1;
	}
      }
    }

    /* BLC!! Need to set made_mod the first time we reach any function */
    
    propagate_params = 0;
    if (numactuals != 0) {
      actualinfo = malloc(numactuals*sizeof(expr_t *));
      alias_list = malloc(numactuals*sizeof(genlist_t *));

      for (i=0;i<numactuals;i++) {
	actualinfo[i] = NULL;
	alias_list[i] = NULL;
      }

      actual = actuallist;
      for (i=0;i<numactuals;i++) {
	if (expr_is_lvalue(actual)) {
	  actualinfo[i] = actual;
	  actual_pst = expr_find_root_pst(actual);
	  if (S_SUBCLASS(actual_pst) == SC_INOUT) { /** IN AND OUT ?? **/
	    propagate_params = 1;
	    alias_list[i] = S_ACTUALS(actual_pst);
	  }
	}
	actual = T_NEXT(actual);
      }
    } else {
      actualinfo = NULL;
      alias_list = NULL;
    }

    if (coverused || numactuals != 0) {
      if (propagate_reg || propagate_mask || propagate_params) {
	callptr = T_CALLINFO(caller);
	while (callptr != NULL) {
	  for (i=1;i<=MAXRANK;i++) {
	    if (prop_reg[i]) {
	      COV_REG(cover) = COV_REG(CALL_COVER(callptr));
	    }
	    if (prop_mask[i]) {
	      COV_MSK(cover) = COV_MSK(CALL_COVER(callptr));
	      COV_WTH(cover) = COV_WTH(CALL_COVER(callptr));
	    }
	  }
	  actual = actuallist;
	  IFDB(4) {
	    dbg_gen_expr(stdout, actual);
	    printf(":\n");
	  }
	  for (i=0;i<numactuals;i++) {
	    if (alias_list[i]) {
	      /* need to make a private copy of the original actual */
	      actualinfo[i] = copy_expr(actual);
	      plug_in = expr_find_root_exprptr(actualinfo[i]);
	      IFDB(4) {
		printf("  >");
		if (plug_in != NULL && *plug_in != NULL) {
		  dbg_gen_expr(stdout, *plug_in);
		} else {
		  dbg_gen_expr(stdout, actualinfo[i]);
		}
		printf("\n");
	      }
	      if (plug_in != NULL) {
		/* without this conditional, NULL @'s accumulate.  Cleaner
		   way of doing this? Is this deep enough? */
		/* BLC: Null-@'s are gone.  What to do here? */
		expr_to_copy = G_EXPR(alias_list[i]);
		IFDB(4) {
		  dbg_gen_expr(stdout, expr_to_copy);
		  printf("\n");
		}
		if (*plug_in != NULL) {
		  *plug_in = copy_expr(expr_to_copy);
		} else {
		  actualinfo[i] = copy_expr(expr_to_copy);
		}
		/* need to update types so that region info is up-to-date
		   simple.ek.z is suboptimal otherwise -- parameters assumed
		   to be rank-defined, therefore strided. */
		typeinfo_expr(actualinfo[i]);
		alias_list[i] = G_NEXT(alias_list[i]);
	      }
	    }
	    actual = T_NEXT(actual);
	  }
	  made_mod |= add_callsite(callee,cover,actualinfo);
	  callptr = CALL_NEXT(callptr);
	}
      } else {
	made_mod |= add_callsite(callee,cover,actualinfo);
      }
    }
    if (actualinfo != NULL) {
      free(alias_list);
      free(actualinfo);
    }
  }
}

	
static void callanal_expr(expr_t *expr) {
  traverse_expr(expr,FALSE,callanal_fncall);
}


static int callanal_fn(function_t *fn,glist junk) {
  DBS1(2,"callanal considering %s\n",S_IDENT(T_FCN(fn)));
  made_mod = 0;
  if (T_CALLINFO(fn) == (callsite_t *)-1) {
    made_mod = 1;  /* consider every function once at least */
    T_CALLINFO(fn) = NULL;
  }
  traverse_stmtls_g(T_STLS(fn),NULL,NULL,callanal_expr,NULL);
  return made_mod;
}


#if (!RELEASE_VER)
static void print_callanal(module_t *mod) {
  function_t *fn;
  callsite_t *callsite;

  printf("\n");
  fn = T_FCNLS(mod);
  while (fn != NULL) {
    callsite = T_CALLINFO(fn);
    while (callsite != NULL) {
      print_callsite(fn,callsite);
      callsite = CALL_NEXT(callsite);
    }
    fn = T_NEXT(fn);
  }
}
#endif


static void callanal_module(module_t *mod) {
  RMSInit();
  ip_analyze(mod,IPDN,NULL,callanal_fn,NULL);
  RMSFinalize();
#if (!RELEASE_VER)
  IFDB(1) {
    print_callanal(mod);
  }
#endif
}

int call_callanal(module_t *mod,char *s) {
  traverse_modules(mod,TRUE,callanal_module,NULL);

  return 0;
}
