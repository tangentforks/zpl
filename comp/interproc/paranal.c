/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/interproc.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"


static void paranal_expr(expr_t *e) {
  statement_t *stmt;
  function_t *fn;
  int isparallel = 0;

  stmt = T_STMT(e);
  fn = T_PARFCN(stmt);
  if (T_PARALLEL(fn) == TRUE) {
    return; /* short-circuit */
  }

  switch (T_TYPE(e)) {
  case VARIABLE:
    if ((S_DTYPE(T_IDENT(e)) != NULL) && 
	(D_CLASS(S_DTYPE(T_IDENT(e))) == DT_ENSEMBLE)) {
      isparallel = 1;
    }
    break;
  case FUNCTION:
    if (T_PARALLEL(S_FUN_BODY(T_IDENT(T_OPLS(e)))) == TRUE) {
      isparallel = 1;
    }
    break;
    
  case BIAT:         /* parallel expressions */
  case REDUCE:
  case SCAN:
  case FLOOD:
  case PERMUTE:
    isparallel = 1;
    break;

  case BIASSIGNMENT:  /* assignments to globals are parallel */
  case BIOP_GETS:
    if (!expr_is_free(T_OPLS(e)) &&
	(expr_is_global(T_OPLS(e)) ||
	 expr_is_static(T_OPLS(e)))) { /* check if T_OPLS(e) is global or static, but not free */
      isparallel = 1;
    }
    break;

  default:
    break;
  }

  if (isparallel) {
    T_PARALLEL(fn) = TRUE;
    if (paranal_help) {
      printf("   %s is parallel because of line %d\n",S_IDENT(T_FCN(fn)),
	     T_LINENO(stmt));
    }
  }
}


static void paranal_stmt(statement_t *stmt) {
  function_t *fn;

  fn = T_PARFCN(stmt);
  if (T_PARALLEL(fn) == TRUE) {
    return;    /* short-circuit */
  }

  switch (T_TYPE(stmt)) {
  case S_EXPR:
    traverse_exprls_g(T_EXPR(stmt),paranal_expr,NULL);
    break;
  case S_IF:
    traverse_exprls_g(T_IFCOND(T_IF(stmt)),paranal_expr,NULL);
    break;
  case S_LOOP:
    switch (T_TYPE(T_LOOP(stmt))) {
    case L_DO:
      traverse_exprls_g(T_IVAR(T_LOOP(stmt)),paranal_expr,NULL);
      traverse_exprls_g(T_START(T_LOOP(stmt)),paranal_expr,NULL);
      traverse_exprls_g(T_STOP(T_LOOP(stmt)),paranal_expr,NULL);
      traverse_exprls_g(T_STEP(T_LOOP(stmt)),paranal_expr,NULL);
      break;
    case L_WHILE_DO:
    case L_REPEAT_UNTIL:
      traverse_exprls_g(T_LOOPCOND(T_LOOP(stmt)),paranal_expr,NULL);
      break;
    default:
      INT_FATAL(stmt, "Invalid loop type.");
      break;
    }
    break;
  case S_RETURN:
    traverse_exprls_g(T_RETURN(stmt),paranal_expr,NULL);
    break;
  case S_WRAP:  /* parallel statement types */
  case S_REFLECT:
  case S_REGION_SCOPE:
  case S_MLOOP:
  case S_IO:
  case S_MSCAN:
    T_PARALLEL(fn) = TRUE;
    if (paranal_help) {
      printf("   %s is parallel because of line %d\n",S_IDENT(T_FCN(fn)),
	     T_LINENO(stmt));
    }
    break;
  case S_NLOOP:
  case S_NULL:
  case S_EXIT:
  case S_COMPOUND:
  case S_HALT:
  case S_CONTINUE:
  case S_END:
    break;
  default:
    INT_FATAL(stmt, "Unrecognized statement type %d", T_TYPE(stmt));
    break;
  }
}


static int paranal_fn(function_t *fn,glist junk) {
  DBS1(2,"paranal considering %s...\n",S_IDENT(T_FCN(fn)));
  if (T_PARALLEL(fn) == TRUE) {
    return 0;
  }
  traverse_stmtls_g(T_STLS(fn),paranal_stmt,NULL,NULL,NULL);
  if (T_PARALLEL(fn) == -1) {
    T_PARALLEL(fn) = 0;
    return 1;
  } else if (T_PARALLEL(fn) == TRUE) {
    return 1;
  } else {
    return 0;
  }
}


static void reset_paranal(module_t *mod) {
  function_t *fn;

  for (fn = T_FCNLS(mod); fn != NULL; fn = T_NEXT(fn)) {
    T_PARALLEL(fn) = -1;
  }
}


static void print_paranal(module_t *mod) {
  function_t *fn;

  printf("\n");
  printf("Serial Functions\n");
  printf("----------------\n");
  for (fn = T_FCNLS(mod); fn != NULL; fn = T_NEXT(fn)) {
    if (T_PARALLEL(fn) == FALSE) {
      printf("%s\n",S_IDENT(T_FCN(fn)));
    }
  }
  printf("\n");
  printf("Parallel Functions\n");
  printf("------------------\n");
  for (fn = T_FCNLS(mod); fn != NULL; fn = T_NEXT(fn)) {
    if (T_PARALLEL(fn) == TRUE) {
      printf("%s\n",S_IDENT(T_FCN(fn)));
    }
  }
  printf("\n");
}


static void paranal_do_module(module_t *mod) {
  reset_paranal(mod);
  ip_analyze(mod,IPUP,NULL,paranal_fn,NULL);
  IFDB(1) {
    print_paranal(mod);
  }
}


int call_paranal(module_t *mod,char *s) {
  traverse_modules(mod, TRUE, paranal_do_module, NULL);
  paranal_help = 0;
 
  return 0;
}
