/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: floodopt.c - tiling optimization
 * DATE: 30 March 2000
 * CREATOR: maria
 *
 * SUMMARY: This file examines the structure of data in an mloop.  If the
 * innermost iterated dimension of an array is flooded, references to the
 * walker are hoisted, and innermost bumps removed.  */


#include <stdlib.h>
#include "../include/buildstmt.h"
#include "../include/buildsymutil.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/Repgen.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/traverse.h"
#include "../include/treemac.h"


static void floodopt(statement_t*);
int call_floodopt(module_t*,char*);
static int idnum;


/* this function determines whether the expr is assigned to in the stmtls,
 * returning 1 if yes or unsure, 0 if no */
static int is_written(expr_t *expr, statement_t *stmtls) {
  statement_t *temp;
  expr_t *stmt_expr;

  for (temp=stmtls; temp; temp=T_NEXT(temp)) {
    switch(T_TYPE(temp)) {
    case S_EXPR:
      stmt_expr = T_EXPR(temp);
      if (T_TYPE(stmt_expr)==FUNCTION) {
	expr_t *args;
	for (args=nthoperand(stmt_expr,2); args; args=T_NEXT(args)) {
	  if (expr_equal(args,expr)) {
	    return(1);
	  }
	}
      }
      if (expr_equal(left_expr(stmt_expr),expr)) {
	return(1);
      }
      break;
    case S_IF:
      if (is_written(expr,T_THEN(T_IF(temp))))
        return(1);
      if (is_written(expr,T_ELSE(T_IF(temp))))
        return(1);
      break;
    case S_LOOP:
      if (is_written(expr,T_BODY(T_LOOP(temp))))
	return(1);
      break;
    case S_NLOOP:
      if (is_written(expr,T_NLOOP_BODY(T_NLOOP(temp))))
	return(1);
      break;
    case S_CONTINUE:
      break;
    default:
      printf("**** %d ****/n", T_TYPE(temp));
      exit(1);
	     
      return(1);
    }
  }
  return(0);
} /* is_written(expr,stmtls) */


/* this function does the transformation involved in replacing an
 * expression */
static void replace_expression(expr_t* expr,mloop_t* mloop) {
  symboltable_t *new_temp;
  expr_t *new_expr;
  char *new_id = (char *)malloc(10);
  statement_t *body=T_MLOOP_BODY(mloop);

  /* drop out if replacement expr already exists */
  if (find_replacement(mloop, expr)) return;

  /* create a new temporary variable */
  sprintf(new_id,"__temp%d",idnum);
  new_temp = alloc_loc_st (S_VARIABLE,new_id);
  S_DTYPE(new_temp) = T_TYPEINFO(expr);
  S_VAR_INIT(new_temp) = NULL;
  S_INIT(new_temp) = build_init(expr,NULL);
  S_PROHIBITIONS(new_temp) |= TILED_OUTER_MLOOP_FLAG;
  new_expr = build_0ary_op(VARIABLE,new_temp);
  idnum++;

  /* replace the variable */
  T_ADD_MLOOP_VAR (mloop, new_temp, T_MLOOP_ORDER(mloop, T_MLOOP_RANK(mloop)-1));
  replace_expr(expr, copy_expr(new_expr));
  /*add_replacement(mloop,copy_expr(expr),copy_expr(new_expr));*/

  /* if necessary, reassign the variable */
  if (is_written(new_expr,body)) {
    statement_t* post;
    expr_t* reassign;
    reassign=build_binary_op(BIASSIGNMENT,copy_expr(new_expr),copy_expr(expr));
    post=build_expr_statement(reassign,T_LINENO(body),T_FILENAME(body));
    T_PROHIBITIONS(post) |= TILED_OUTER_MLOOP_FLAG;
    T_ADD_MLOOP_POST(mloop,post,T_MLOOP_ORDER(mloop, T_MLOOP_RANK(mloop)-1));
  }
} /* replace_expression(expr,mloop) */

/* this function searches an expression to see if it includes a loop invariant
 * atom */
static void search_expr_for_unchanged_atom(expr_t *expr, mloop_t *mloop) {
  int array=0;
  int numdims;
  int innerdim;

  if (expr  && T_TYPEINFO(expr) &&
      S_CLASS(T_TYPEINFO(expr))==DT_ARRAY) {
    array=1;
  }
  if (expr) {
    if (expr_is_atom(expr) && !array &&
	(T_TYPE(expr)!=ARRAY_REF || (expr_const(nthoperand(expr,2)) &&  
				     !expr_contains_indexi(nthoperand(expr,2))
				     && !expr_contains_ia_indexi(nthoperand(expr,2))))) {
      expr_t *typeinfo;
      datatype_t *type = NULL;
      expr_t* at_expr;

      at_expr = expr_find_at(expr);
      if (at_expr && T_SUBTYPE(at_expr) == AT_RANDACC) {
        return;
      }
      typeinfo = T_TYPEINFO_REG(expr);
      if (typeinfo) {
        type = T_TYPEINFO(typeinfo);
      }
      if (type && (D_CLASS(type) == DT_REGION)) {
	numdims = T_MLOOP_RANK(mloop);
	innerdim = T_MLOOP_ORDER(mloop,numdims);
	if (D_REG_DIM_TYPE(type, innerdim) == DIM_FLOOD) {
	  replace_expression(expr,mloop);
	}
      }
    } else if (T_TYPE(expr)!=ARRAY_REF) {
      search_expr_for_unchanged_atom(T_OPLS(expr),mloop);
    }
    search_expr_for_unchanged_atom(T_NEXT(expr),mloop);
  } /* if expr */

}


/* this function searches a statement list to find atoms that are loop
 * invariant */
static void search_for_unchanged_atom(statement_t *stmt_ls, mloop_t *mloop) {
  expr_t *expr;
  statement_t *stmt_ls2;
  while (stmt_ls) {
    switch(T_TYPE(stmt_ls)) {
    case S_EXPR:
      expr = T_EXPR(stmt_ls);
      if(BIASSIGNMENT==T_TYPE(expr))
        expr=T_NEXT(T_OPLS(expr));
      search_expr_for_unchanged_atom(expr, mloop);
      break;
    case S_NLOOP:
      stmt_ls2 = T_NLOOP_BODY(T_NLOOP(stmt_ls));
      search_for_unchanged_atom(stmt_ls2,mloop);
      break;
    case S_IF:
      stmt_ls2 = T_THEN(T_IF(stmt_ls));
      search_for_unchanged_atom(stmt_ls2, mloop);
      stmt_ls2 = T_ELSE(T_IF(stmt_ls));
      search_for_unchanged_atom(stmt_ls2, mloop);

      expr = T_IFCOND(T_IF(stmt_ls));
      search_expr_for_unchanged_atom(expr, mloop);

      break;
    case S_LOOP:
      {
        stmt_ls2 = T_BODY(T_LOOP(stmt_ls));
        search_for_unchanged_atom(stmt_ls2, mloop);

        switch (T_TYPE(T_LOOP(stmt_ls))) {
        case L_DO:
          expr = T_IVAR(T_LOOP(stmt_ls));
          search_expr_for_unchanged_atom(expr, mloop);

          expr = T_START(T_LOOP(stmt_ls));
          search_expr_for_unchanged_atom(expr, mloop);

          expr = T_STOP(T_LOOP(stmt_ls));
          search_expr_for_unchanged_atom(expr, mloop);

          expr = T_STEP(T_LOOP(stmt_ls));
          search_expr_for_unchanged_atom(expr, mloop);

          break;

        case L_WHILE_DO:
        case L_REPEAT_UNTIL:
          expr = T_LOOPCOND(T_LOOP(stmt_ls));
          search_expr_for_unchanged_atom(expr, mloop);

          break;
        default:
          INT_FATAL(stmt_ls, "bad loop type (%d) in "
            "search_for_unchanged_atom",T_TYPE(T_LOOP(stmt_ls)));
        }
      } /* case loop */
      break;

    default:
      break;
    }
    stmt_ls = T_NEXT(stmt_ls);
  }

}


static void floodopt (statement_t* stmt) {
  mloop_t* mloop;
  int numdims;
  if (S_MLOOP==T_TYPE(stmt)) {
    mloop = T_MLOOP(stmt);
    numdims = T_MLOOP_RANK(mloop);
    if (1<numdims) {
      idnum=0;
      search_for_unchanged_atom(T_MLOOP_BODY(mloop),mloop);
    }
  } /* if mloop */
} /* floodopt(stmt) */


int call_floodopt (module_t* mod, char* s) {
  module_t* modls;
  symboltable_t* pst;
  statement_t *stmt;
  if (flood_opt) {
    RMSInit();
    for (modls = mod; modls != NULL; modls = T_NEXT (modls)) {
      for (pst = T_DECL (modls); pst != NULL; pst = S_SIBLING (pst)) {
        if (S_CLASS (pst) == S_SUBPROGRAM) {
          stmt = T_STLS (S_FUN_BODY (pst));
          if (stmt != NULL)
            traverse_stmtls (T_STLS (T_CMPD (stmt)), 0, floodopt, NULL, NULL);
        }
      } /* for pst */
    } /* for modls */
    RMSFinalize();
  } /* if flood_opt */
  return 0;
}
