/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/mloop.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"
#include "../include/buildsym.h"
#include "../include/buildstmt.h"
#include "../include/Repgen.h"
#include "../include/rmstack.h"
#include "../include/stencil.h"
#include "../include/ntablet.h"
#include "../include/nudge.h"
#include "../include/zutil.h"
#include "../include/main.h"

int call_runtime_checks(module_t*, char*);

static void runtime_checks_stmt(statement_t* stmt) {
  set_t* set;
  int inout;
  expr_t* expr;
  statement_t* check;
  expr_t* region;
  int lineno;
  char* filename;

  if (stmt != NULL) {
    if (T_TYPE(stmt) == S_MLOOP) {
      inout = 2;
      while (inout--) {
	for (set = (inout) ? T_IN(stmt) : T_OUT(stmt); set != NULL; set = SET_NEXT(set)) {
	  if (S_IS_ENSEMBLE(SET_VAR(set))) {
	    region = RMSCurrentRegion();
	    if (!expr_is_qreg(region)) {
	      lineno = T_LINENO(stmt);
	      filename = T_FILENAME(stmt);
	      expr = build_function_call(lu_pst("_BOUND_CHECK"), 3, region, build_typed_0ary_op(VARIABLE, SET_VAR(set)), new_const_int(lineno));
	      check = build_expr_statement(expr, lineno, filename);
	      insertbefore_stmt(check, stmt);
	    }
	  }
	}
      }
    }
  }
}

static void runtime_checks_mod(module_t* mod) {
  function_t* fn;

  fn = T_FCNLS(mod);
  while (fn != NULL) {
    traverse_stmtls(T_STLS(fn), 0, runtime_checks_stmt, NULL, NULL);
    fn = T_NEXT(fn);
  }
}

int call_runtime_checks(module_t* modls, char* s) {
  if (do_bounds_checking) {
    RMSInit();
    traverse_modules(modls, TRUE, runtime_checks_mod, NULL);
    RMSFinalize();
  }
  return 0;
}
