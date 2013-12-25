/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: floodtile.c - tiling optimization
 * DATE: 23 March 2000
 * CREATOR: maria
 *
 * SUMMARY: This file examines the structure of data in an mloop to decided if
 * it is worthwhile to tile the loop.  At initial pass, if there exists data
 * flooded in the second innermost dimension, the inner two loops are tiled by
 * 4.
 */

#include <stdlib.h>
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"

#define TILESIZE 4

static void floodtile(statement_t*);
int call_floodtile(module_t*,char*);

/* this function examines an expression in an mloop to see if tiling would be
 * beneficial */
static int search_expr_for_tiling_opp(expr_t *expr) {
  int numdims;

  if (expr) {
    if (expr_is_atom(expr) && (T_TYPE(expr)!=ARRAY_REF ||
			       expr_const(nthoperand(expr,2)))) {
      expr_t *typeinfo;
      datatype_t *type;
      typeinfo = T_TYPEINFO_REG(expr);
      if (typeinfo) {
        type = T_TYPEINFO(typeinfo);
	numdims = D_REG_NUM(type);
	if (numdims>1 &&
	    (D_REG_DIM_TYPE(type, numdims-2) == DIM_FLOOD ||
	     D_REG_DIM_TYPE(type, numdims-2) == DIM_GRID)) {
	  return 1;
	}
      }
    } /* if expr_is_atom */
    else if (search_expr_for_tiling_opp(T_OPLS(expr)))
      return 1;
    if (search_expr_for_tiling_opp(T_NEXT(expr)))
      return 1;
  } /* if expr */
  
  return 0;
}


/* this function searches a statements list to see if it includes an
 * expression that would benefit from tiling */
static int search_for_tiling_opportunity(statement_t *stmt_ls) {
  expr_t *expr;
  statement_t *stmt_ls2;

  while (stmt_ls) {
    switch(T_TYPE(stmt_ls)) {
    case S_EXPR:
      expr = T_EXPR(stmt_ls);
      if (search_expr_for_tiling_opp(expr))
        return(1);
      break;
    case S_NLOOP:
      stmt_ls2 = T_NLOOP_BODY(T_NLOOP(stmt_ls));
      if(search_for_tiling_opportunity(stmt_ls2))
        return(1);
      break;
    case S_IF:
      stmt_ls2 = T_THEN(T_IF(stmt_ls));
      if(search_for_tiling_opportunity(stmt_ls2))
        return(1);
      stmt_ls2 = T_ELSE(T_IF(stmt_ls));
      if(search_for_tiling_opportunity(stmt_ls2))
        return(1);
      expr = T_IFCOND(T_IF(stmt_ls));
      if(search_expr_for_tiling_opp(expr))
        return(1);
      break;
    case S_LOOP:
      {
        stmt_ls2 = T_BODY(T_LOOP(stmt_ls));
        if(search_for_tiling_opportunity(stmt_ls2))
          return(1);

        switch (T_TYPE(T_LOOP(stmt_ls))) {
        case L_DO:
          expr = T_IVAR(T_LOOP(stmt_ls));
          if(search_expr_for_tiling_opp(expr))
            return(1);
          expr = T_START(T_LOOP(stmt_ls));
          if(search_expr_for_tiling_opp(expr))
            return(1);
          expr = T_STOP(T_LOOP(stmt_ls));
          if(search_expr_for_tiling_opp(expr))
            return(1);
          expr = T_STEP(T_LOOP(stmt_ls));
          if(search_expr_for_tiling_opp(expr))
            return(1);
          break;
        case L_WHILE_DO:
        case L_REPEAT_UNTIL:
          expr = T_LOOPCOND(T_LOOP(stmt_ls));
          if(search_expr_for_tiling_opp(expr))
            return(1);
          break;
        default:
          INT_FATAL(stmt_ls, "bad loop type (%d) in "
            "search_for_tiling_opportunity",T_TYPE(T_LOOP(stmt_ls)));
        }
      }
      break;

    default:
      break;
    }
    stmt_ls = T_NEXT(stmt_ls);
  }

  return(0);
}


static int reg_good_for_tiling(mloop_t* mloop) {
  int numdims;
  expr_t* reg;

  /* do not tile 1d regions */
  numdims = T_MLOOP_RANK(mloop);
  if (numdims < 2) {
    return 0;
  }

  /* do not tile sparse regions */
  reg = T_MLOOP_REG(mloop);
  if (!reg || !expr_is_dense_reg(reg)) {
    return 0;
  }

  /* do not tile mloops containing any random access BIAT's */
  if (stmtls_contains_uat(T_MLOOP_BODY(mloop), AT_RANDACC)) {
    return 0;
  }

  /* do not tile mloops containing zmacros */
  if (stmtls_contains_zmacro(T_MLOOP_BODY(mloop))) {
    return 0;
  }

  /* if either of the inner dims is flat, do not tile */
  if (RMSRegDimDegenerate(T_MLOOP_ORDER(mloop,numdims),0) ||
      RMSRegDimDegenerate(T_MLOOP_ORDER(mloop,numdims-1),0)) {
    return 0;
  }

  return 1;
}


static void floodtile (statement_t* stmt) {
  mloop_t* mloop;
  int numdims;

  mloop = T_MLOOP(stmt);
  if (!mloop)
    return;
  numdims = T_MLOOP_RANK(mloop);
  if (S_MLOOP==T_TYPE(stmt)) {
    if (reg_good_for_tiling(mloop) && 
	search_for_tiling_opportunity(T_MLOOP_BODY(mloop))) {
      T_MLOOP_TILE(mloop,T_MLOOP_ORDER(mloop,numdims)) = TILESIZE;
      T_MLOOP_TILE(mloop,T_MLOOP_ORDER(mloop,numdims-1)) = TILESIZE;
    }
  } /* if mloop */
} /* floodtile(stmt) */


int call_floodtile (module_t* mod, char* s) {
  module_t* modls;
  symboltable_t* pst;
  statement_t *stmt;
  if (tile_opt) {
    RMSInit();
    for (modls = mod; modls != NULL; modls = T_NEXT (modls)) {
      for (pst = T_DECL (modls); pst != NULL; pst = S_SIBLING (pst)) {
        if (S_CLASS (pst) == S_SUBPROGRAM) {
          stmt = T_STLS (S_FUN_BODY (pst));
          if (stmt != NULL)
            traverse_stmtls (T_STLS (T_CMPD (stmt)), 0, floodtile, NULL, NULL);
        }
      } /* for pst */
    } /* for modls */
    RMSFinalize();
  } /* if tile_opt */
  return 0;
}
