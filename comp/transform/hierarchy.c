/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/const.h"
#include "../include/datatype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symbol.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"

static int any_preserving = 0; /* only insert copy functions if <=# is used */

static symboltable_t* insert_array_copy_function(datatype_t* pdt) {
  static int id = 1;
  char name[128];
  symboltable_t* pst;
  expr_t* arr1;
  expr_t* arr2;
  expr_t* maps;
  expr_t* reg;
  int i;
  expr_t* remap;
  statement_t* stmt;

  sprintf(name, "_z_arr_copy_%d", id++);
  pst = insert_function(name, pdtVOID, 3, 
			T_TYPEINFO(D_ENS_REG(pdt)), SC_CONST,
			pdt, SC_INOUT,
			pdt, SC_INOUT);
  S_STD_CONTEXT(pst) = FALSE;
  reg = build_typed_0ary_op(VARIABLE, T_DECL(S_FUN_BODY(pst)));
  arr1 = build_typed_0ary_op(VARIABLE, S_SIBLING(T_DECL(S_FUN_BODY(pst))));
  arr2 = build_typed_0ary_op(VARIABLE, S_SIBLING(S_SIBLING(T_DECL(S_FUN_BODY(pst)))));
  maps = NULL;
  for (i = 0; i < D_ENS_NUM(pdt); i++) {
      maps = cat_expr_ls(maps, build_0ary_op(VARIABLE, NULL));
  }
  remap = build_permute_op(P_PERMUTE, maps, arr2);
  stmt = build_expr_statement(build_binary_op(BIASSIGNMENT, remap, arr1), 0, "");

  T_STLS(S_FUN_BODY(pst)) = build_compound_statement(NULL, build_reg_mask_scope(reg, NULL, 0, stmt, 0, ""), 0, "");
  add_module(zpl_module, pst);
  return pst;
}

static void hierarchy_temps_stmt(statement_t* stmt) {
  statement_t* cmpd = NULL;
  symboltable_t* pst;

  if (stmt && T_TYPE(stmt) == S_EXPR && T_TYPE(T_EXPR(stmt)) == FUNCTION &&
      (T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_DESTROY") ||
       T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_PRESERVE"))) {
    if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_GRID)) {
      pst = build_grid_decl_list(T_NEXT(T_NEXT(T_OPLS(T_EXPR(stmt)))), T_LINENO(stmt), T_FILENAME(stmt));
    }
    else if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_DISTRIBUTION)) {
      pst = build_distribution_decl_list(T_NEXT(T_NEXT(T_OPLS(T_EXPR(stmt)))), T_LINENO(stmt), T_FILENAME(stmt));
    }
    else if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_REGION)) {
      pst = build_region_decl_list(T_NEXT(T_NEXT(T_OPLS(T_EXPR(stmt)))), T_LINENO(stmt), T_FILENAME(stmt));
    }
    else {
      INT_FATAL(NULL, "Illegal LHS with <== or <=#");
    }
    if (pst) {
      T_NEXT(T_NEXT(T_OPLS(T_EXPR(stmt)))) = build_0ary_op(CONSTANT, pst);
      T_PREV(T_NEXT(T_NEXT(T_OPLS(T_EXPR(stmt))))) = T_NEXT(T_OPLS(T_EXPR(stmt)));
    }
    cmpd = build_compound_statement(pst, copy_stmt(stmt), T_LINENO(stmt), T_FILENAME(stmt));
    insertbefore_stmt(cmpd, stmt);
    remove_stmt(stmt);
  }
}

static void hierarchy_temps_mod(module_t* mod) {
  function_t* fn;

  for (fn = T_FCNLS(mod); fn != NULL; fn = T_NEXT(fn)) {
    traverse_stmtls(T_STLS(fn), 0, hierarchy_temps_stmt, NULL, NULL);
  }
}

static void hierarchy_aggregate_stmt(statement_t* stmt) {
  statement_t* cmpd;
  statement_t* cmpd2;

  if (stmt && T_TYPE(stmt) == S_EXPR && T_TYPE(T_EXPR(stmt)) == FUNCTION &&
      (T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_DESTROY") ||
       T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_PRESERVE"))) {
    cmpd = T_PARENT(stmt);
    if (T_TYPE(cmpd) != S_COMPOUND) {
      INT_FATAL(NULL, "All hierarchy assignment statements should be in compound statements");
    }
    if (T_NEXT(cmpd) &&
	T_TYPE(T_NEXT(cmpd)) == S_COMPOUND &&
	T_CMPD_STLS(T_NEXT(cmpd)) &&
	T_TYPE(T_CMPD_STLS(T_NEXT(cmpd))) == S_EXPR &&
	T_TYPE(T_EXPR(T_CMPD_STLS(T_NEXT(cmpd)))) == FUNCTION &&
	(T_IDENT(T_OPLS(T_EXPR(T_CMPD_STLS(T_NEXT(cmpd))))) == lu_pst("_DESTROY") ||
	 T_IDENT(T_OPLS(T_EXPR(T_CMPD_STLS(T_NEXT(cmpd))))) == lu_pst("_PRESERVE"))) {
      cmpd2 = T_NEXT(cmpd);
      cmpd = remove_stmt(cmpd);
      insert_cmpd_stmt(cmpd, cmpd2);
    }
  }
}

static void hierarchy_aggregate_mod(module_t* mod) {
  function_t* fn;

  for (fn = T_FCNLS(mod); fn != NULL; fn = T_NEXT(fn)) {
    traverse_stmtls(T_STLS(fn), 0, hierarchy_aggregate_stmt, NULL, NULL);
  }
}

static void hierarchy_transform_stmt(statement_t* cmpd) {
  statement_t* last;
  statement_t* stmt;
  statement_t* new = NULL;
  symboltable_t* pst;
  expr_t* expr;
  int preserve;

  if (T_TYPE(cmpd) == S_COMPOUND) {
    last = T_CMPD_STLS(cmpd);
    while (last && T_NEXT(last)) {
      last = T_NEXT(last);
    }
    if (!(last && T_TYPE(last) == S_EXPR && T_TYPE(T_EXPR(last)) == FUNCTION &&
	  (T_IDENT(T_OPLS(T_EXPR(last))) == lu_pst("_DESTROY") ||
	   T_IDENT(T_OPLS(T_EXPR(last))) == lu_pst("_PRESERVE")))) {
      return;
    }
  }
  else {
    return;
  }
  for (stmt = T_CMPD_STLS(cmpd); stmt; stmt = T_NEXT(stmt)) {
    if (last && T_TYPE(last) == S_EXPR && T_TYPE(T_EXPR(last)) == FUNCTION &&
	(T_IDENT(T_OPLS(T_EXPR(last))) == lu_pst("_DESTROY") ||
	 T_IDENT(T_OPLS(T_EXPR(last))) == lu_pst("_PRESERVE"))) {
      preserve = T_IDENT(T_OPLS(T_EXPR(last))) == lu_pst("_PRESERVE");
      any_preserving |= preserve;
      if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_GRID)) {
	pst = lu_pst("_UPDATE_GRID");
      }
      else if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_DISTRIBUTION)) {
	pst = lu_pst("_UPDATE_DISTRIBUTION");
      }
      else if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_REGION)) {
	pst = lu_pst("_UPDATE_REGION");
      }
      else {
	INT_FATAL(NULL, "Unknown case in hierarchy_transform_stmt");
      }
      expr = build_function_call(pst, 3,
				 copy_expr(T_NEXT(T_OPLS(T_EXPR(stmt)))),
				 copy_expr(T_NEXT(T_NEXT(T_OPLS(T_EXPR(stmt))))),
				 new_const_int(preserve));
      new = cat_stmt_ls(new, build_expr_statement(expr,T_LINENO(stmt), T_FILENAME(stmt)));
    }
  }
  expr = build_function_call(lu_pst("_PROCESS_ENSEMBLES"), 0);
  new = cat_stmt_ls(new, build_expr_statement(expr,T_LINENO(cmpd), T_FILENAME(cmpd)));
  for (stmt = T_CMPD_STLS(cmpd); stmt; stmt = T_NEXT(stmt)) {
    if (last && T_TYPE(last) == S_EXPR && T_TYPE(T_EXPR(last)) == FUNCTION &&
	(T_IDENT(T_OPLS(T_EXPR(last))) == lu_pst("_DESTROY") ||
	 T_IDENT(T_OPLS(T_EXPR(last))) == lu_pst("_PRESERVE"))) {
      if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_GRID)) {
	pst = lu_pst("_COPY_GRID");
      }
      else if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_DISTRIBUTION)) {
	pst = lu_pst("_COPY_DISTRIBUTION");
      }
      else if (datatype_find_dtclass(T_TYPEINFO(T_NEXT(T_OPLS(T_EXPR(stmt)))), DT_REGION)) {
	pst = lu_pst("_COPY_REGION");
      }
      else {
	INT_FATAL(NULL, "Unknown case in hierarchy_transform_stmt");
      }
      expr = build_function_call(pst, 3,
				 copy_expr(T_NEXT(T_OPLS(T_EXPR(stmt)))),
				 copy_expr(T_NEXT(T_NEXT(T_OPLS(T_EXPR(stmt))))),
				 new_const_int(0) /* do not change arrays */);
      new = cat_stmt_ls(new, build_expr_statement(expr,T_LINENO(stmt), T_FILENAME(stmt)));
    }
  }
  insertafter_stmt(new, last);
}

static void hierarchy_transform_mod(module_t* mod) {
  function_t* fn;

  for (fn = T_FCNLS(mod); fn != NULL; fn = T_NEXT(fn)) {
    traverse_stmtls(T_STLS(fn), 0, hierarchy_transform_stmt, NULL, NULL);
  }
}

static void hierarchy_clean_stmt(statement_t* stmt) {
  if (stmt && T_TYPE(stmt) == S_EXPR && T_TYPE(T_EXPR(stmt)) == FUNCTION &&
      (T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_DESTROY") ||
       T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_PRESERVE"))) {
    remove_stmt(stmt);
  }
}

static void hierarchy_clean_mod(module_t* mod) {
  function_t* fn;

  for (fn = T_FCNLS(mod); fn != NULL; fn = T_NEXT(fn)) {
    traverse_stmtls(T_STLS(fn), 0, hierarchy_clean_stmt, NULL, NULL);
  }
}

static void hierarchy_copies_pst(symboltable_t* pst) {
    datatype_t* pdt;

    pdt = S_DTYPE(pst);
    if (pdt && S_CLASS(pdt) == DT_ENSEMBLE && S_CLASS(pst) == S_VARIABLE) {
	if (!S_IS_CONSTANT(expr_find_root_pst(D_ENS_REG(pdt)))) {
	    D_ENS_COPY(pdt) = insert_array_copy_function(pdt);
	}
    }
}

static void hierarchy_copies_mod(module_t* mod) {
    function_t* fn;
    symboltable_t* pst;

    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
	hierarchy_copies_pst(pst);
    }
    for (fn = T_FCNLS(mod); fn != NULL; fn = T_NEXT(fn)) {
	for (pst = T_DECL(fn); pst != NULL; pst = S_SIBLING(pst)) {
	    hierarchy_copies_pst(pst);
	}
    }
    }

int call_hierarchy_aggregate(module_t* mod, char* s) {

  /*
   * insert temps for anonymous hierarchy assignments
   */
  RMSInit();
  traverse_modules(mod, TRUE, hierarchy_temps_mod, NULL);
  RMSFinalize();

  /*
   * aggregate hierarchy assignments into compound statements
   */
  RMSInit();
  traverse_modules(mod, TRUE, hierarchy_aggregate_mod, NULL);
  RMSFinalize();

  /*
   * insert _update, _process, and _copy statements
   */
  RMSInit();
  traverse_modules(mod, TRUE, hierarchy_transform_mod, NULL);
  RMSFinalize();

  /*
   * remove _preserve and _destroy statements
   */
  RMSInit();
  traverse_modules(mod, TRUE, hierarchy_clean_mod, NULL);
  RMSFinalize();

  /*
   * insert array copy functions if necessary
   */
  if (any_preserving) {
    RMSInit();
    traverse_modules(mod, TRUE, hierarchy_copies_mod, NULL);
    RMSFinalize();
  }

  return 0;
}
