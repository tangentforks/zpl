/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#include "../include/buildstmt.h"
#include "../include/buildsymutil.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/main.h"
#include "../include/Repgen.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"

int call_copts(module_t*, char*);

/*
 * in which we factor loop-invariant expressions into the
 * pre-statements of an mloop.
 */
static int minvnum = 0;

static void mloop_factor_expr(mloop_t* mloop, expr_t* expr) {
  char* name;
  expr_t* new;
  symboltable_t* pst;
  int minv;
  int i;
  statement_t* stmt;

  /* don't process below the top parallel expression; when we had
   UATs, this would happen automatically, as things wouldn't recurse
   below that point... (out of laziness or correctness, I'm not
   sure... */
  if (expr_at_ensemble_root(expr)) {
    return;
  }

  if (T_TYPE(expr) == NULLEXPR ||
      T_TYPE(expr) == CONSTANT ||
      T_TYPE(expr) == SIZE ||
      T_TYPE(expr) == VARIABLE ||
      T_TYPE(expr) == BIDOT) {
    return;
  }
  if (T_TYPE(expr) == FUNCTION && 
      !(strncmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_PERM", 5) == 0))
    return;
  /*
  if (S_CLASS(T_TYPEINFO(expr)) == DT_VOID) {
    return;
  }
  */
  for (i = 0; i < T_MLOOP_RANK(mloop); i++) {
    if (T_MLOOP_TILE(mloop, i)) {
      return;
    }
    if (T_MLOOP_UNROLL(mloop, i) != 1) {
      return;
    }
  }
  minv = mloopinvariant_expr(expr);
  if (minv > 0) {
    if (S_CLASS(T_TYPEINFO(expr)) == DT_DIRECTION) {
      return;
    }
    if (S_CLASS(T_TYPEINFO(expr)) == DT_COMPLEX ||
	S_CLASS(T_TYPEINFO(expr)) == DT_DCOMPLEX ||
	S_CLASS(T_TYPEINFO(expr)) == DT_QCOMPLEX) {
      return;
    }
    if (S_CLASS(T_TYPEINFO(expr)) == DT_VOID) {
      stmt = build_expr_statement(copy_expr(expr), T_LINENO(T_STMT(expr)),
				  T_FILENAME(T_STMT(expr)));
      T_ADD_MLOOP_PRE(mloop, stmt, (minv == T_MLOOP_RANK(mloop)) ? -1 : T_MLOOP_ORDER(mloop, T_MLOOP_RANK(mloop) - minv));
      delete_stmt(T_STMT(expr));
    }
    else {
      name = malloc(128);
      sprintf(name, "_minvtmp%d", ++minvnum);
      pst = alloc_loc_st (S_VARIABLE, name);
      S_DTYPE(pst) = T_TYPEINFO(expr);
      S_VAR_INIT(pst) = NULL;
      S_INIT(pst) = build_init(expr, NULL);
      new = build_0ary_op(VARIABLE, pst);
      T_ADD_MLOOP_VAR(mloop, pst, (minv == T_MLOOP_RANK(mloop)) ? -1 : T_MLOOP_ORDER(mloop, T_MLOOP_RANK(mloop) - minv)); /* T_MLOOP_RANK(mloop) - minv - 1); */
      replaceall_stmtls(T_BODY(mloop), expr, new);
    }
  }
  else {
    if (T_IS_BINARY(T_TYPE(expr))) {
      mloop_factor_expr(mloop, T_OPLS(expr));
      mloop_factor_expr(mloop, T_NEXT(T_OPLS(expr)));
    }
    else if (T_TYPE(expr) == ARRAY_REF || T_TYPE(expr) == FUNCTION) {
      for (expr = T_OPLS(expr); T_NEXT(expr) != NULL; expr = T_NEXT(expr)) {
	mloop_factor_expr(mloop, T_NEXT(expr));
      }
    }
  }
}

static void mloop_factor(statement_t* stmt) {
  mloop_t* mloop;
  statement_t* next;

  if (stmt == NULL || T_TYPE(stmt) != S_MLOOP) {
    return;
  }
  mloop = T_MLOOP(stmt);
  for (stmt = T_BODY(mloop); stmt != NULL; stmt = next) {
    next = T_NEXT(stmt);
    if (T_TYPE(stmt) == S_EXPR) {
      mloop_factor_expr(mloop, T_EXPR(stmt));
    }
  }
}

/*
 * in which we replace the stop and step with constants so as to avoid
 * excessive recomputations during a loop, see loop invariant code
 * motion but note we don't check for loop invariance (by definition)
 */
static void ldo_optimize(statement_t* stmt)
{
  static int unum = 1;
  loop_t* loop;
  char* name;
  symboltable_t* pst;
  statement_t* pre;
  expr_t* te;
  expr_t* pe;
  statement_t* cstmt;
  statement_t* dummy;

  if (stmt == NULL ||
      T_TYPE(stmt) != S_LOOP ||
      T_TYPE(T_LOOP(stmt)) != L_DO) {
    return;
  }
  loop = T_LOOP(stmt);
  if (!expr_const(T_STOP(loop))) {
    name = malloc(128);
    sprintf(name, "_stop%d", unum++);
    pst = alloc_loc_st(S_VARIABLE, name);
    if (T_TYPEINFO(T_STOP(loop)) == pdtGENERIC_ENSEMBLE) {
      S_DTYPE(pst) = pdtINT;
    }
    else {
      S_DTYPE(pst) = T_TYPEINFO(T_STOP(loop));
    }
    pe = build_typed_0ary_op(VARIABLE, pst);
    te = build_binary_op(BIASSIGNMENT, T_STOP(loop), copy_expr(pe));
    pre = build_expr_statement(te, T_LINENO(stmt), T_FILENAME(stmt));
    T_STOP(loop) = pe;
    dummy = build_expr_statement(NULL, T_LINENO(stmt), T_FILENAME(stmt));
    insertbefore_stmt(dummy, stmt);
    remove_stmt(stmt);
    cstmt = build_compound_statement(pst, stmt, T_LINENO(stmt), T_FILENAME(stmt));
    T_PARENT(stmt) = cstmt;
    insertbefore_stmt(pre, stmt);
    insertbefore_stmt(cstmt, dummy);
    remove_stmt(dummy);
  }
  if (!expr_const(T_STEP(loop))) {
    name = malloc(128);
    sprintf(name, "_step%d", unum++);
    pst = alloc_loc_st(S_VARIABLE, name);
    S_DTYPE(pst) = T_TYPEINFO(T_STEP(loop));
    pe = build_typed_0ary_op(VARIABLE, pst);
    te = build_binary_op(BIASSIGNMENT, T_STEP(loop), copy_expr(pe));
    pre = build_expr_statement(te, T_LINENO(stmt), T_FILENAME(stmt));
    T_STEP(loop) = pe;
    dummy = build_expr_statement(NULL, T_LINENO(stmt), T_FILENAME(stmt));
    insertbefore_stmt(dummy, stmt);
    remove_stmt(stmt);
    cstmt =
      build_compound_statement(pst, stmt, T_LINENO(stmt), T_FILENAME(stmt));
    T_PARENT(stmt) = cstmt;
    insertbefore_stmt(pre, stmt);
    insertbefore_stmt(cstmt, dummy);
    remove_stmt(dummy);
  }
}

/*
 * in which we convert all region scope statements to compound
 * statements within which the region is saved
 */
static void rscope2compound(statement_t* stmt)
{
  static int unum = 1;
  statement_t* cstmt;
  char *name;
  symboltable_t* pst;
  expr_t* save;
  expr_t* te;
  statement_t* pre;
  statement_t* dummy;
  statement_t* post;

  if (stmt == NULL || T_TYPE(stmt) != S_REGION_SCOPE) {
    return;
  }
  name = malloc(128);
  sprintf(name, "_Save%d", unum++);
  pst = alloc_loc_st(S_VARIABLE, name);
  S_DTYPE(pst) = rmsframe;
  save = build_typed_0ary_op(VARIABLE, pst);
  te = build_typed_0ary_op(VARIABLE, rmstack);
  te = build_binary_op(BIASSIGNMENT, te, copy_expr(save));
  pre = build_expr_statement(te, T_LINENO(stmt), T_FILENAME(stmt));
  te = build_typed_0ary_op(VARIABLE, rmstack);
  te = build_binary_op(BIASSIGNMENT, copy_expr(save), te);
  post = build_expr_statement(te, T_LINENO(stmt), T_FILENAME(stmt));
  dummy = build_expr_statement(NULL, T_LINENO(stmt), T_FILENAME(stmt));
  insertbefore_stmt(dummy, stmt);
  remove_stmt(stmt);
  cstmt = build_compound_statement(pst, stmt, T_LINENO(stmt), T_FILENAME(stmt));
  T_PARENT(stmt) = cstmt;
  insertbefore_stmt(cstmt, dummy);
  remove_stmt(dummy);
  insertbefore_stmt(pre, stmt);
  insertbefore_stmt(post, stmt);
  remove_stmt(stmt);
  insertbefore_stmt(stmt, post);
}

expr_t* rgrid2free_ls = NULL;

static void eliminate_rgrid_optimize_expr(expr_t* expr) {
  if (expr == NULL) {
    return;
  }

  if (T_TYPE(expr) == SCAN) {
    symboltable_t* pst = expr_find_root_pst(T_OPLS(expr));
    expr_t* tmp;

    for (tmp = rgrid2free_ls; tmp != NULL; tmp = T_NEXT(tmp)) {
      if (T_TYPE(tmp) == VARIABLE && T_IDENT(tmp) == pst) {
	T_TYPE(tmp) = NULLEXPR;
      }
    }
  }
}

static void eliminate_rgrid_optimize_stmt(statement_t* stmt) {
  if (stmt != NULL) {
    if (T_TYPE(stmt) == S_EXPR) {
      traverse_exprls(T_EXPR(stmt), 0, eliminate_rgrid_optimize_expr);
    }
    else if (T_TYPE(stmt) == S_IO) {
      if (IO_EXPR1(T_IO(stmt))) {
	symboltable_t* pst = expr_find_root_pst(IO_EXPR1(T_IO(stmt)));
	expr_t* tmp;

	for (tmp = rgrid2free_ls; tmp != NULL; tmp = T_NEXT(tmp)) {
	  if (T_TYPE(tmp) == VARIABLE && T_IDENT(tmp) == pst) {
	    T_TYPE(tmp) = NULLEXPR;
	  }
	}
      }
    }
  }
}

static void optimize_rgrid(symboltable_t* pst) {
  int i;
  datatype_t* reg;

  if (D_CLASS(S_DTYPE(pst)) == DT_ENSEMBLE) {
    reg = T_TYPEINFO(D_ENS_REG(S_DTYPE(pst)));
    for (i = 0; i < D_REG_NUM(reg); i++) {
      if (D_REG_DIM_TYPE(reg, i) != DIM_GRID) {
	return;
      }
    }
    /** put symboltables to convert in a list we can't just do it now
	since we can't do it with things that are written or scanned.
	we should be able to but then these would have to be taken
	care of before codegen as far as their region application
	goes.
    **/
    rgrid2free_ls = cat_expr_ls(rgrid2free_ls, build_0ary_op(VARIABLE, pst));
  }
}

int call_copts(module_t* modls, char* s)
{
  module_t* mod;
  symboltable_t* pst;
  symboltable_t* tmp;
  statement_t* stmt;
  expr_t* expr;

  RMSInit();
  for (mod = modls; mod != NULL; mod = T_NEXT(mod)) {
    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
      if (S_CLASS(pst) == S_SUBPROGRAM) {
	if ((stmt = T_STLS(S_FUN_BODY(pst))) != NULL) {
	  traverse_stmtls(T_STLS(T_CMPD(stmt)), 0, ldo_optimize, 0, 0);
	}
      }
    }
  }
  RMSFinalize();
  RMSInit();
  for (mod = modls; mod != NULL; mod = T_NEXT(mod)) {
    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
      if (S_CLASS(pst) == S_SUBPROGRAM) {
	if ((stmt = T_STLS(S_FUN_BODY(pst))) != NULL) {
	  traverse_stmtls(T_STLS(T_CMPD(stmt)), 0, rscope2compound, 0, 0);
	}
      }
    }
  }
  RMSFinalize();

  /** find symboltable entries to convert :: to free **/
  for (mod = modls; mod != NULL; mod = T_NEXT(mod)) {
    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
      if (S_STD_CONTEXT(pst) == 0) {
	if (S_CLASS(pst) == S_SUBPROGRAM) {
	  if (T_STLS(S_FUN_BODY(pst)) != NULL) {
	    for (tmp = T_CMPD_DECL(T_STLS(S_FUN_BODY(pst))); tmp != NULL; tmp = S_SIBLING(tmp)) {
	      if (S_CLASS(tmp) != S_PARAMETER) {
		optimize_rgrid(tmp);
	      }
	    }
	  }
	}
	else if (S_CLASS(pst) == S_VARIABLE) {
	  optimize_rgrid(pst);
	}
      }
    }
  }

  /** eliminate candidates for :: to free conversion if they are written or scanned **/
  RMSInit();
  for (mod = modls; mod != NULL; mod = T_NEXT(mod)) {
    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
      if (S_CLASS(pst) == S_SUBPROGRAM) {
	if ((stmt = T_STLS(S_FUN_BODY(pst))) != NULL) {
	  traverse_stmtls(T_STLS(T_CMPD(stmt)), 0, eliminate_rgrid_optimize_stmt, 0, 0);
	}
      }
    }
  }
  RMSFinalize();

  /** actually convert :: to free **/
  for (expr = rgrid2free_ls; expr != NULL; expr = T_NEXT(expr)) {
    if (T_TYPE(expr) == VARIABLE) {
      S_DTYPE(T_IDENT(expr)) = D_ENS_TYPE(S_DTYPE(T_IDENT(expr)));
      S_STYPE(T_IDENT(expr)) |= SC_FREE;
    }
  }

  if (!c_opt) {
    return 0;
  }
  RMSInit();
  for (mod = modls; mod != NULL; mod = T_NEXT(mod)) {
    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
      if (S_CLASS(pst) == S_SUBPROGRAM) {
	if ((stmt = T_STLS(S_FUN_BODY(pst))) != NULL) {
	  traverse_stmtls(T_STLS(T_CMPD(stmt)), 0, mloop_factor, 0, 0);
	}
      }
    }
  }
  RMSFinalize();

  return 0;
}
