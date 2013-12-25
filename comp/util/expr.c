/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/buildstmt.h"
#include "../include/buildsym.h"
#include "../include/datatype.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/nudge.h"
#include "../include/parsetree.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/stmtutil.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"
#include "../include/zutil.h"

#define T_PRECEDENCE(x)  (expr_precedence[T_TYPE(x)])
#define T_ASSOCIATIVE(x) (expr_associative[T_TYPE(x)])
#define T_COMMUTATIVE(x) (expr_commutative[T_TYPE(x)])

/*** expr_precedence[] contains relative precedence levels between all expr's
     this array must correspond directly to OpName[] in include/Cname.h
     and the enumeration exprtype in include/parsetree.h ***/
static int expr_precedence[] = {
  9,    /* NULLEXPR      */
  9,    /* SBE           */
  9,    /* VARIABLE      */
  9,    /* CONSTANT      */
  9,    /* SIZE          */
  9,    /* GRID          */
  9,    /* DISTRIBUTION        */
  7,    /* UNEGATIVE     */
  7,    /* UPOSITIVE     */
  9,    /* NUDGE         */
  7,    /* REDUCE        */
  7,    /* SCAN          */
  7,    /* FLOOD         */
  9,    /* PERMUTE       */
  7,    /* UCOMPLEMENT   */
  9,    /* BIAT           */
  5,    /* BIPLUS        */
  5,    /* BIMINUS       */
  6,    /* BITIMES       */
  6,    /* BIDIVIDE      */
  6,    /* BIMOD         */
  8,    /* BIEXP         */
  1,    /* BIOP_GETS     */
  4,    /* BIGREAT_THAN  */
  4,    /* BILESS_THAN   */
  4,    /* BIG_THAN_EQ   */
  4,    /* BIL_THAN_EQ   */
  4,    /* BIEQUAL       */
  4,    /* BINOT_EQUAL   */
  3,    /* BILOG_AND     */
  2,    /* BILOG_OR      */
  1,    /* BIASSIGNMENT  */
  9,    /* BIDOT         */
  9,    /* ARRAY_REF     */
  9     /* FUNCTION      */
};

/*** expr_associative[] contains one if the expr is associative
     this array must correspond directly to OpName[] in include/Cname.h
     and the enumeration exprtype in include/parsetree.h ***/
static int expr_associative[] = {
  0,    /* NULLEXPR      */
  0,    /* SBE           */
  0,    /* VARIABLE      */
  0,    /* CONSTANT      */
  0,    /* SIZE          */
  0,    /* GRID          */
  0,    /* DISTRIBUTION        */
  0,    /* UNEGATIVE     */
  0,    /* UPOSITIVE     */
  0,    /* NUDGE         */
  0,    /* REDUCE        */
  0,    /* SCAN          */
  0,    /* FLOOD         */
  0,    /* PERMUTE       */
  0,    /* UCOMPLEMENT   */
  0,    /* BIAT           */
  1,    /* BIPLUS        */
  0,    /* BIMINUS       */
  1,    /* BITIMES       */
  0,    /* BIDIVIDE      */
  0,    /* BIMOD         */
  0,    /* BIEXP         */
  0,    /* BIOP_GETS     */
  0,    /* BIGREAT_THAN  */
  0,    /* BILESS_THAN   */
  0,    /* BIG_THAN_EQ   */
  0,    /* BIL_THAN_EQ   */
  0,    /* BIEQUAL       */
  0,    /* BINOT_EQUAL   */
  1,    /* BILOG_AND     */
  1,    /* BILOG_OR      */
  0,    /* BIASSIGNMENT  */
  0,    /* BIDOT         */
  0,    /* ARRAY_REF     */
  0     /* FUNCTION      */
};

/*** expr_commutative[] contains one if the expr is commutative
     this array must correspond directly to OpName[] in include/Cname.h
     and the enumeration exprtype in include/parsetree.h ***/

#ifdef _NOT_USED
static int expr_commutative[] = {
  0,    /* NULLEXPR      */
  0,    /* SBE           */
  0,    /* VARIABLE      */
  0,    /* CONSTANT      */
  0,    /* SIZE          */
  0,    /* GRID          */
  0,    /* DISTRIBUTION        */
  0,    /* UNEGATIVE     */
  0,    /* UPOSITIVE     */
  0,    /* NUDGE         */
  0,    /* REDUCE        */
  0,    /* SCAN          */
  0,    /* FLOOD         */
  0,    /* PERMUTE       */
  0,    /* UCOMPLEMENT   */
  0,    /* BIAT           */
  1,    /* BIPLUS        */
  0,    /* BIMINUS       */
  1,    /* BITIMES       */
  0,    /* BIDIVIDE      */
  0,    /* BIMOD         */
  0,    /* BIEXP         */
  0,    /* BIOP_GETS     */
  0,    /* BIGREAT_THAN  */
  0,    /* BILESS_THAN   */
  0,    /* BIG_THAN_EQ   */
  0,    /* BIL_THAN_EQ   */
  1,    /* BIEQUAL       */
  1,    /* BINOT_EQUAL   */
  1,    /* BILOG_AND     */
  1,    /* BILOG_OR      */
  0,    /* BIASSIGNMENT  */
  0,    /* BIDOT         */
  0,    /* ARRAY_REF     */
  0     /* FUNCTION      */
};
#endif


int expr_const(expr_t *expr) {
  if (expr == NULL) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
    return 1;
  case VARIABLE:
    if (S_IS_CONSTANT(T_IDENT(expr))) {
      return 1;
    } else {
      return 0;
    }
  case DISTRIBUTION:
  case GRID:
    return 0;
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    return expr_const(T_OPLS(expr));
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
    return (expr_const(T_OPLS(expr)) && expr_const(T_NEXT(T_OPLS(expr))));
  case ARRAY_REF:
  case BIDOT:
    return (expr_const(T_OPLS(expr)));
  default:
    return 0;
  }
}


int expr_computable_const(expr_t *expr) {
  symboltable_t *pst;
  initial_t *init;

  if (expr == NULL) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
    return 1;
  case VARIABLE:
    pst = T_IDENT(expr);
    if (S_IS_CONSTANT(pst) &&
	!(S_STYPE(pst) & SC_EXTERN) &&
	!(S_STYPE(pst) & SC_CONFIG)) {
      init = S_VAR_INIT(pst);
      if (init) {
	return expr_computable_const(IN_VAL(init));
      } else {
	return 0;
      }
    } else {
      return 0;
    }
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    return expr_computable_const(T_OPLS(expr));
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
    return (expr_computable_const(T_OPLS(expr)) && 
	    expr_computable_const(T_NEXT(T_OPLS(expr))));
  case BIMOD:
  case BIEXP:
    return (datatype_int(T_TYPEINFO(expr)) &&
	    expr_computable_const(T_OPLS(expr)) && 
	    expr_computable_const(T_NEXT(T_OPLS(expr))));
  default:
    return 0;
  }
}


int expr_rt_const(expr_t *expr) {
  symboltable_t *pst;

  if (expr == NULL) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
    return 1;
  case VARIABLE:
    pst = T_IDENT(expr);
    if (!(S_IS_CONSTANT(pst))) {
      return 0;
    }
    /* fall through */
  case CONSTANT:
    pst = T_IDENT(expr);
    return (S_LEVEL(pst) == 0);
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    return expr_rt_const(T_OPLS(expr));
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
    return (expr_rt_const(T_OPLS(expr)) && expr_rt_const(T_NEXT(T_OPLS(expr))));
  case ARRAY_REF:
    {
      expr_t* inds;
      inds = T_NEXT(T_OPLS(expr));
      while (inds != NULL) {
	if (!expr_rt_const(inds)) {
	  return 0;
	}
	inds = T_NEXT(inds);
      }
    }
    /* fall through */
  case BIDOT:
    return (expr_rt_const(T_OPLS(expr)));
  default:
    return 0;
  }
}


int expr_is_free(expr_t* expr) {
  expr_t* tmp;

  if (expr == NULL) {
    return 0;
  }

  if (T_FREE(expr)) {
    return 1;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
    return 0;
  case VARIABLE:
    return (S_STYPE(T_IDENT(expr)) & SC_FREE);
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    return expr_is_free(T_OPLS(expr));
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
    return (expr_is_free(T_OPLS(expr)) || expr_is_free(T_NEXT(T_OPLS(expr))));
  case ARRAY_REF:
    tmp = T_OPLS(expr);
    while (tmp) {
      if (expr_is_free(tmp)) {
	return 1;
      }
      tmp = T_NEXT(tmp);
    }
    return 0;
  case BIDOT:
    return (expr_is_free(T_OPLS(expr)));
  case FUNCTION:
    if (expr_is_free(T_OPLS(expr))) {
      return 1;
    } else {
      /*** return free if free actual is passed to any
           unfree formal, assume promotion is correct from typechecking */
      symboltable_t* formals = NULL;
      expr_t* actuals = NULL;
      do {
        actuals = T_NEXT(T_OPLS(expr));
      } while (actuals != NULL && T_TYPE(actuals) == NULLEXPR);
      if (T_OPLS(expr) &&
          T_IDENT(T_OPLS(expr)) &&
          T_TYPE(T_IDENT(T_OPLS(expr))) &&
          D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))))
        formals = T_DECL(D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))));
      while (actuals && formals) {
        if (!(S_STYPE(formals) & SC_FREE) &&
            expr_is_free(actuals)) {
          return 1;
        }
        do {
          actuals = T_NEXT(actuals);
        } while (actuals != NULL && T_TYPE(actuals) == NULLEXPR);
        do {
          formals = S_SIBLING(formals);
        } while (formals && !S_FG_PARAM(formals));
      }
      return 0;
    }
  case REDUCE:
    if (T_REGMASK(expr) == NULL) {
      return 0;
    }
    return expr_is_free(T_OPLS(expr));
  default:
    return 0;
  }
}


long expr_intval(expr_t *expr) {
  if (expr == NULL) {
    return 0;
  }
  if (!expr_computable_const(expr)) {
    return 0;
  }
  switch (T_TYPE(expr)) {
  case NULLEXPR:
    return 0;
  case CONSTANT:
    return atoi(S_IDENT(T_IDENT(expr)));
  case VARIABLE:
    if (S_IS_CONSTANT(T_IDENT(expr))) {
      return expr_intval(IN_VAL(S_VAR_INIT(T_IDENT(expr))));
    } else {
      return 0;
    }
  case UNEGATIVE:
    return -(expr_intval(T_OPLS(expr)));
  case UPOSITIVE:
    return expr_intval(T_OPLS(expr));
  case UCOMPLEMENT:
    return !(expr_intval(T_OPLS(expr)));
  case BIPLUS:
    return (expr_intval(T_OPLS(expr)) + expr_intval(T_NEXT(T_OPLS(expr))));
  case BIMINUS:
    return (expr_intval(T_OPLS(expr)) - expr_intval(T_NEXT(T_OPLS(expr))));
  case BITIMES:
    return (expr_intval(T_OPLS(expr)) * expr_intval(T_NEXT(T_OPLS(expr))));
  case BIDIVIDE:
    return (expr_intval(T_OPLS(expr)) / expr_intval(T_NEXT(T_OPLS(expr))));
  case BIMOD:
    return (expr_intval(T_OPLS(expr)) % expr_intval(T_NEXT(T_OPLS(expr))));
  case BIEXP:
    {
      int i;
      long retval=1;
      long base;
      long exp;

      base = expr_intval(T_OPLS(expr));
      exp = expr_intval(T_NEXT(T_OPLS(expr)));

      for (i=0;i<exp;i++) {
	retval *= base;
      }
      return retval;
    }
  case BIGREAT_THAN:
    return (expr_intval(T_OPLS(expr)) > expr_intval(T_NEXT(T_OPLS(expr))));
  case BILESS_THAN:
    return (expr_intval(T_OPLS(expr)) < expr_intval(T_NEXT(T_OPLS(expr))));
  case BIG_THAN_EQ:
    return (expr_intval(T_OPLS(expr)) >= expr_intval(T_NEXT(T_OPLS(expr))));
  case BIL_THAN_EQ:
    return (expr_intval(T_OPLS(expr)) <= expr_intval(T_NEXT(T_OPLS(expr))));
  case BIEQUAL:
    return (expr_intval(T_OPLS(expr)) == expr_intval(T_NEXT(T_OPLS(expr))));
  case BINOT_EQUAL:
    return (expr_intval(T_OPLS(expr)) != expr_intval(T_NEXT(T_OPLS(expr))));
  case BILOG_AND:
    return (expr_intval(T_OPLS(expr)) && expr_intval(T_NEXT(T_OPLS(expr))));
  case BILOG_OR:
    return (expr_intval(T_OPLS(expr)) || expr_intval(T_NEXT(T_OPLS(expr))));
  default:
    return 0;
  }
}


unsigned long expr_uintval(expr_t *expr) {
  if (expr == NULL) {
    return 0;
  }
  if (!expr_computable_const(expr)) {
    return 0;
  }
  switch (T_TYPE(expr)) {
  case NULLEXPR:
    return 0;
  case CONSTANT:
    return atoi(S_IDENT(T_IDENT(expr)));
  case VARIABLE:
    if (S_IS_CONSTANT(T_IDENT(expr))) {
      return expr_uintval(IN_VAL(S_VAR_INIT(T_IDENT(expr))));
    } else {
      return 0;
    }
  case UNEGATIVE:
    return -(expr_uintval(T_OPLS(expr)));
  case UPOSITIVE:
    return expr_uintval(T_OPLS(expr));
  case UCOMPLEMENT:
    return !(expr_uintval(T_OPLS(expr)));
  case BIPLUS:
    return (expr_uintval(T_OPLS(expr)) + expr_uintval(T_NEXT(T_OPLS(expr))));
  case BIMINUS:
    return (expr_uintval(T_OPLS(expr)) - expr_uintval(T_NEXT(T_OPLS(expr))));
  case BITIMES:
    return (expr_uintval(T_OPLS(expr)) * expr_uintval(T_NEXT(T_OPLS(expr))));
  case BIDIVIDE:
    return (expr_uintval(T_OPLS(expr)) / expr_uintval(T_NEXT(T_OPLS(expr))));
  case BIMOD:
    return (expr_uintval(T_OPLS(expr)) % expr_uintval(T_NEXT(T_OPLS(expr))));
  case BIEXP:
    {
      int i;
      long retval=1;
      long base;
      long exp;

      base = expr_uintval(T_OPLS(expr));
      exp = expr_uintval(T_NEXT(T_OPLS(expr)));

      for (i=0;i<exp;i++) {
	retval *= base;
      }
      return retval;
    }
  case BIGREAT_THAN:
    return (expr_uintval(T_OPLS(expr)) > expr_uintval(T_NEXT(T_OPLS(expr))));
  case BILESS_THAN:
    return (expr_uintval(T_OPLS(expr)) < expr_uintval(T_NEXT(T_OPLS(expr))));
  case BIG_THAN_EQ:
    return (expr_uintval(T_OPLS(expr)) >= expr_uintval(T_NEXT(T_OPLS(expr))));
  case BIL_THAN_EQ:
    return (expr_uintval(T_OPLS(expr)) <= expr_uintval(T_NEXT(T_OPLS(expr))));
  case BIEQUAL:
    return (expr_uintval(T_OPLS(expr)) == expr_uintval(T_NEXT(T_OPLS(expr))));
  case BINOT_EQUAL:
    return (expr_uintval(T_OPLS(expr)) != expr_uintval(T_NEXT(T_OPLS(expr))));
  case BILOG_AND:
    return (expr_uintval(T_OPLS(expr)) && expr_uintval(T_NEXT(T_OPLS(expr))));
  case BILOG_OR:
    return (expr_uintval(T_OPLS(expr)) || expr_uintval(T_NEXT(T_OPLS(expr))));
  default:
    return 0;
  }
}


double expr_doubleval(expr_t *expr) {
  if (expr == NULL) {
    return 0.0;
  }
  if (!expr_computable_const(expr)) {
    return 0.0;
  }
  switch (T_TYPE(expr)) {
  case NULLEXPR:
    return 0.0;
  case CONSTANT:
    return (double) atof(S_IDENT(T_IDENT(expr)));
  case VARIABLE:
    if (S_IS_CONSTANT(T_IDENT(expr))) {
      return expr_doubleval(IN_VAL(S_VAR_INIT(T_IDENT(expr))));
    } else {
      return 0.0;
    }
  case UNEGATIVE:
    return -(expr_doubleval(T_OPLS(expr)));
  case UPOSITIVE:
    return expr_doubleval(T_OPLS(expr));
  case UCOMPLEMENT:
    return !(expr_doubleval(T_OPLS(expr)));
  case BIPLUS:
    return (expr_doubleval(T_OPLS(expr)) + expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BIMINUS:
    return (expr_doubleval(T_OPLS(expr)) - expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BITIMES:
    return (expr_doubleval(T_OPLS(expr)) * expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BIDIVIDE:
    return (expr_doubleval(T_OPLS(expr)) / expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BIGREAT_THAN:
    return (expr_doubleval(T_OPLS(expr)) > expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BILESS_THAN:
    return (expr_doubleval(T_OPLS(expr)) < expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BIG_THAN_EQ:
    return (expr_doubleval(T_OPLS(expr)) >= expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BIL_THAN_EQ:
    return (expr_doubleval(T_OPLS(expr)) <= expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BIEQUAL:
    return (expr_doubleval(T_OPLS(expr)) == expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BINOT_EQUAL:
    return (expr_doubleval(T_OPLS(expr)) != expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BILOG_AND:
    return (expr_doubleval(T_OPLS(expr)) && expr_doubleval(T_NEXT(T_OPLS(expr))));
  case BILOG_OR:
    return (expr_doubleval(T_OPLS(expr)) || expr_doubleval(T_NEXT(T_OPLS(expr))));
  default:
    return 0.0;
  }
}


symboltable_t *expr_is_var(expr_t *expr) {
  symboltable_t *pst;

  switch (T_TYPE(expr)) {
  case VARIABLE:
    pst = T_IDENT(expr);
    if (S_IS_CONSTANT(pst)) {
      return NULL;
    } else {
      return pst;
    }
  case BIAT:
    return NULL;
  default:
    return NULL;
  }
}


symboltable_t *expr_find_ens_pst(expr_t *expr) {
  symboltable_t *field;

  switch(T_TYPE(expr)) {
  case VARIABLE:
    if (datatype_find_ensemble(S_DTYPE(T_IDENT(expr)))) {
      return T_IDENT(expr);
    }
    else {
      return NULL;
    }
  case BIAT:
  case ARRAY_REF:
    return expr_find_ens_pst(T_OPLS(expr));
  case BIDOT:
    field = T_IDENT(expr);
    if (datatype_find_ensemble(S_DTYPE(field))) {
      return field;
    } else {
      return expr_find_ens_pst(T_OPLS(expr));
    }
  default:
    return NULL;
  }
}


expr_t* expr_find_root_expr(expr_t* expr) {
  switch (T_TYPE(expr)) {
  case CONSTANT:
  case VARIABLE:
    return expr;
  case BIAT:
  case BIDOT:
  case ARRAY_REF:
    return expr_find_root_expr(T_OPLS(expr));
  default:
    return NULL;
  }
}


symboltable_t *expr_find_root_pst(expr_t *expr) {
  expr_t* rootexpr;

  rootexpr = expr_find_root_expr(expr);
  if (rootexpr != NULL) {
    return T_IDENT(rootexpr);
  } else {
    return NULL;
  }
}


expr_t **expr_find_root_exprptr(expr_t *expr) {
  static expr_t *foundit = NULL;
  expr_t **retval;

  switch (T_TYPE(expr)) {
  case CONSTANT:
  case VARIABLE:
    return &foundit;
  case BIAT:
  case BIDOT:
  case ARRAY_REF:
    retval = expr_find_root_exprptr(T_OPLS(expr));
    if (retval == &foundit) {
      return &(T_OPLS(expr));
    } else {
      return retval;
    }
  default:
    return NULL;
  }
}


int expr_is_global(expr_t *expr) {
  if (expr == NULL) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case VARIABLE:
    return symtab_is_global(T_IDENT(expr));

  case BIDOT:
  case ARRAY_REF:
    return expr_is_global(T_OPLS(expr));

  default:
    return 0;
  }
}


int expr_is_static(expr_t* expr) {
  if (expr == NULL) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case VARIABLE:
    return symtab_is_static(T_IDENT(expr));

  case BIDOT:
  case ARRAY_REF:
    return expr_is_static(T_OPLS(expr));

  default:
    return 0;
  }
}


int expr_is_qmask(expr_t *expr) {
  int i;

  for (i=1;i<=MAXRANK;i++) {
    if (expr == pexpr_qmask[i]) {
      return i;
    }
  }
  if (expr == pexpr_qmask[0]) {
    return -1;
  }
  return 0;
}


int expr_is_indexi(expr_t *expr) {
  if (T_TYPE(expr) == VARIABLE) {
    return symtab_is_indexi(T_IDENT(expr));
  } else {
    return 0;
  }
}


int expr_contains_indexi(expr_t *expr) {
  int retval;

  if (expr == NULL) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
  case SIZE:
  case DISTRIBUTION:
  case GRID:
  case SBE:
    return 0;

  case VARIABLE:
    return expr_is_indexi(expr);

  case UNEGATIVE:
  case UPOSITIVE:
  case BIAT:
  case REDUCE:
  case SCAN:
  case FLOOD:
  case PERMUTE:
  case UCOMPLEMENT:
    return expr_contains_indexi(T_OPLS(expr));
    
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIOP_GETS:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
  case BIASSIGNMENT:
  case BIPREP:
    return (expr_contains_indexi(T_OPLS(expr)) ||
	    expr_contains_indexi(T_NEXT(T_OPLS(expr))));

  case BIDOT:
    return expr_contains_indexi(T_OPLS(expr));

  case ARRAY_REF:
  case FUNCTION:
    retval = FALSE;
    expr = T_OPLS(expr);
    while (expr) {
      retval = retval || expr_contains_indexi(expr);
      expr = T_NEXT(expr);
    }
    return retval;

  default:
    INT_FATAL(T_STMT(expr),
	      "Unkown expression type in expr_contains_indexi()");
    return 0;  /* can't get here, but compiler doesn't know that */
  }
}


int expr_is_ia_index(expr_t* expr) {
  if (T_TYPE(expr) == VARIABLE) {
    return symtab_is_ia_index(T_IDENT(expr));
  } else {
    return 0;
  }
}


int expr_contains_ia_indexi(expr_t *expr) {
  int retval;

  if (expr == NULL) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
  case SIZE:
  case DISTRIBUTION:
  case GRID:
    return 0;

  case VARIABLE:
    return expr_is_ia_index(expr);

  case UNEGATIVE:
  case UPOSITIVE:
  case BIAT:
  case REDUCE:
  case SCAN:
  case FLOOD:
  case PERMUTE:
  case UCOMPLEMENT:
    return expr_contains_ia_indexi(T_OPLS(expr));
    
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIOP_GETS:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
  case BIASSIGNMENT:
    return (expr_contains_ia_indexi(T_OPLS(expr)) ||
	    expr_contains_ia_indexi(T_NEXT(T_OPLS(expr))));

  case BIDOT:
    return expr_contains_ia_indexi(T_OPLS(expr));

  case ARRAY_REF:
  case FUNCTION:
    retval = FALSE;
    expr = T_OPLS(expr);
    while (expr) {
      retval = retval || expr_contains_ia_indexi(expr);
      expr = T_NEXT(expr);
    }
    return retval;

  default:
    INT_FATAL(T_STMT(expr),
	      "Unkown expression type in expr_contains_ia_indexi()");
    return 0;  /* can't get here, but compiler doesn't know that */
  }
}


int expr_equal(expr_t *expr1,expr_t *expr2) {
  int retval;
  int i;

  expr_t* tmp1, * tmp2;

  if (expr1 == NULL) {
    if (expr2 == NULL) {
      return 1;
    } else {
      return 0;
    }
  } else {
    if (expr2 == NULL) {
      return 0;
    }
  }

  if (expr1 == expr2) {
    return 1;
  }

  if (T_TYPE(expr1) != T_TYPE(expr2)) {
    return 0;
  }

  switch (T_TYPE(expr1)) {
  case NULLEXPR:
    return 1;

  case CONSTANT:
  case SIZE:
    /*    return !(strcmp(S_IDENT(T_IDENT(expr1)),S_IDENT(T_IDENT(expr2))));*/
    return T_IDENT(expr1) == T_IDENT(expr2);

  case VARIABLE:
    return (T_IDENT(expr1) == T_IDENT(expr2) &&
	    T_INSTANCE_NUM(expr1) == T_INSTANCE_NUM(expr2));

  case BIAT:
    if (expr_equal(T_NEXT(T_OPLS(expr1)),T_NEXT(T_OPLS(expr2))) &&
	expr_equal(T_OPLS(expr1),T_OPLS(expr2))) {
      if (T_SUBTYPE(expr1) != T_SUBTYPE(expr2)) {
	return 0;
      }
      if (T_SUBTYPE(expr1) == AT_RANDACC) {
	tmp1 = T_MAP(expr1);
	tmp2 = T_MAP(expr2);
	for (i = 0; i < T_MAPSIZE(expr1); i++) {
	  if (!(expr_equal(tmp1, tmp2))) {
	    return 0;
	  }
	  tmp1 = T_NEXT(expr1);
	  tmp2 = T_NEXT(expr2);
	}
      }
      if (T_IS_NUDGED(expr1)) {
	if (T_IS_NUDGED(expr2)) {
	  for (i = 0; i < MAXRANK; i++) {
	    if (T_GET_NUDGE(expr1,i) != T_GET_NUDGE(expr2,i)) {
	      return 0;
	    }
	  }
	  return 1;
	}
	else {
	  return 0;
	}
      }
      else if (T_IS_NUDGED(expr2)) {
	return 0;
      }
      else {
	return 1;
      }
    }
    else return 0;
  case NUDGE:
    for (i=0; i<MAXRANK; i++) {
      if (T_GET_NUDGE(expr1,i) != T_GET_NUDGE(expr2,i)) {
	return 0;
      }
    }
    return expr_equal(T_OPLS(expr1), T_OPLS(expr2));
    break;
  case UNEGATIVE:
  case UPOSITIVE:
  case REDUCE:
  case SCAN:
  case FLOOD:
  case PERMUTE:
  case UCOMPLEMENT:
    return expr_equal(T_OPLS(expr1),T_OPLS(expr2));
    
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIOP_GETS:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
  case BIASSIGNMENT:
  case BIPREP:
  case BIWITH:
    return (expr_equal(T_OPLS(expr1),T_OPLS(expr2)) &&
	    expr_equal(T_NEXT(T_OPLS(expr1)),T_NEXT(T_OPLS(expr2))));

  case BIDOT:
    return ((T_IDENT(expr1) == T_IDENT(expr2)) &&
	    expr_equal(T_OPLS(expr1),T_OPLS(expr2)));

  case ARRAY_REF:
  case FUNCTION:
    retval = (T_IDENT(expr1) == T_IDENT(expr2));
    expr1 = T_OPLS(expr1);
    expr2 = T_OPLS(expr2);
    while (expr1 || expr2) {
      retval = retval && expr_equal(expr1,expr2);
      expr1 = expr1?T_NEXT(expr1):NULL;
      expr2 = expr2?T_NEXT(expr2):NULL;
    }
    return retval;

  default:
    INT_FATAL(T_STMT(expr1),
	      "Unkown expression type in expr_equal()");
    return 0;  /* won't get here, but compiler doesn't know that */
  }
}


expr_t *expr_find_at_up(expr_t *expr) {
  if (expr == NULL) {
    return NULL;
  }
  switch (T_TYPE(expr)) {
  case BIAT:
    return expr;

  case NULLEXPR:
  case CONSTANT:
  case VARIABLE:
  case BIDOT:
  case ARRAY_REF:
    return expr_find_at_up(T_PARENT(expr));

  default:
    return NULL;
  }
}


expr_t *expr_find_at(expr_t *expr) {
  if (expr == NULL) {
    return NULL;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
  case VARIABLE:
    return NULL;
  case BIAT:
    return expr;
  case UNEGATIVE:
  case UPOSITIVE:
  case REDUCE:
  case SCAN:
  case FLOOD:
  case PERMUTE:
  case UCOMPLEMENT:
    return expr_find_at(T_OPLS(expr));
    
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIOP_GETS:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
  case BIASSIGNMENT:
    return NULL;

  case BIDOT:
  case ARRAY_REF:
    return expr_find_at(T_OPLS(expr));

  case FUNCTION:
    return NULL;

  default:
    INT_FATAL(T_STMT(expr),
	      "Unkown expression type in expr_find_at()");
    return NULL;  /* won't get here, but compiler doesn't know that */
  }
}


int expr_rank(expr_t *expr) {
  int rank;

  if (!expr) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
  case SIZE:
    return 0;
  case VARIABLE:
    return symtab_rank(T_IDENT(expr));
  case REDUCE:
  case FLOOD:
    if (T_REGMASK(expr)) {
      return expr_rank(T_REGION_SYM(T_REGMASK(expr)));
    }
  case SCAN:
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    return expr_rank(T_OPLS(expr));

  case NUDGE:
    return expr_rank(T_OPLS(expr));
  case BIAT:
    if (T_SUBTYPE(expr) == AT_RANDACC) {
      if (T_RANDACC(expr) != NULL) {
	int rank = 0, new;
	expr_t* map = T_RANDACC(expr);

	while (map != NULL) {
	  new = expr_rank(map);
	  if (rank == 0) {
	    rank = new;
	  }
	  else if (rank != new) {
	    return 0;
	    /*** sungeun *** rank = -1; ***/
	  }
	  map = T_NEXT(map);
	}
	/*** sungeun *** return (rank == -1) ? 0 : rank; ***/
	return rank;
      }
      else {
	return 0;
      }
    }
    else {
      return expr_rank(T_OPLS(expr));
    }
    
  case PERMUTE:
    return 0;

  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIOP_GETS:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
  case BIASSIGNMENT:
    rank = expr_rank(T_OPLS(expr));
    if (!rank) {
      rank = expr_rank(T_NEXT(T_OPLS(expr)));
    }
    return rank;

  case BIDOT:
    rank = expr_rank(T_OPLS(expr));
    if (!rank) {
      rank = symtab_rank(T_IDENT(expr));
    }
    return rank;

  case ARRAY_REF:
    rank = expr_rank(T_OPLS(expr));
    if (!rank) {
      for (expr = T_NEXT(T_OPLS(expr)); expr != NULL; expr = T_NEXT(expr)) {
	rank = expr_rank(expr);
	if (rank) {
	  break;
	}
      }
    }
    return rank;

  case FUNCTION:
    if (T_TYPEINFO_REG(expr)) {
      return D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr)));
    }
    return 0;

  default:
    INT_FATAL(T_STMT(expr),"Unkown expression type in expr_rank(): %d",
	      T_TYPE(expr));
    return 0; /* won't get here, but compiler doesn't know that */
  }
}


int expr_parallel(expr_t *expr) {
  int rank;

  rank = expr_rank(expr);
  if (!rank) {
    rank = expr_contains_indexi(expr);
  }
  return rank;
}

static int parallelindim_pst(expr_t* pst, int dim) {
  datatype_t* regdt;

  if (!pst) {
    return 0;
  }
  regdt = T_TYPEINFO(pst);
  if (D_REG_DIM_TYPE(regdt, dim) == DIM_FLOOD ||
      D_REG_DIM_TYPE(regdt, dim) == DIM_GRID) {
    return 0;
  }
  return 1;
}

int parallelindim_expr(expr_t* expr, int dim) {
  expr_t* tmp;

  if (!expr) {
    return 0;
  }
  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
  case SIZE:
    return 0;
  case VARIABLE:
    if (expr_is_indexi(expr)) {
      if (dim == expr_is_indexi(expr) - 1) {
	return 1;
      }
      else {
	return 0;
      }
    }
  case REDUCE:
  case PERMUTE:
    return parallelindim_pst(T_TYPEINFO_REG(expr), dim);
  case FLOOD:
    return parallelindim_pst(T_REGION_SYM(T_REGMASK(expr)), dim);
  case SCAN:
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    return parallelindim_expr(T_OPLS(expr), dim);
  case BIAT:
    if (T_SUBTYPE(expr) == AT_RANDACC) {
      for (tmp = T_RANDACC(expr); tmp != NULL; tmp = T_NEXT(tmp)) {
	if (parallelindim_expr(tmp, dim)) {
	  return 1;
	}
      }
    }
    return parallelindim_pst(T_TYPEINFO_REG(expr), dim);
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIOP_GETS:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
  case BIASSIGNMENT:
    return
      parallelindim_expr(T_OPLS(expr), dim) ||
      parallelindim_expr(T_NEXT(T_OPLS(expr)), dim);
  case BIDOT:
    return 
      parallelindim_expr(T_OPLS(expr), dim) ||
      parallelindim_pst(T_TYPEINFO_REG(expr), dim);
  case ARRAY_REF:
    if (parallelindim_pst(T_TYPEINFO_REG(expr), dim)) {
      return 1;
    }
  case FUNCTION:
    for (tmp = T_NEXT(T_OPLS(expr)); tmp != NULL; tmp = T_NEXT(tmp)) {
      if (parallelindim_expr(tmp, dim)) {
	return 1;
      }
    }
    return 0;
  default:
    INT_FATAL(T_STMT(expr),"Unkown expression type: %d", T_TYPE(expr));
    return 0;
  }
}

int expr_contains_par_fns(expr_t *expr) {
  int contains;
  
  if (!expr) {
    return 0;
  }

  switch(T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
  case VARIABLE:
    return 0;
  case REDUCE:
  case FLOOD:
  case SCAN:
  case PERMUTE:
  case BIAT:
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    return expr_contains_par_fns(T_OPLS(expr));
    
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIOP_GETS:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
  case BIASSIGNMENT:
    contains = expr_contains_par_fns(T_OPLS(expr));
    if (!contains) {
      contains = expr_contains_par_fns(T_NEXT(T_OPLS(expr)));
    }
    return contains;

  case BIDOT:
    return 0;

  case ARRAY_REF:
    contains = 0;
    {
      expr_t *arr_ind;

      arr_ind = T_NEXT(T_OPLS(expr));
      while (arr_ind != NULL && !contains) {
	contains = expr_contains_par_fns(arr_ind);
	arr_ind = T_NEXT(arr_ind);
      }
    }
    if (!contains) {
      contains = expr_contains_par_fns(T_OPLS(expr));
    }
    return contains;
      

  case FUNCTION:
    {
      symboltable_t *pst;
      function_t *func;
      
      pst = T_IDENT(T_OPLS(expr));
      if (pst) {
	func = S_FUN_BODY(pst);
	if (func) {
	  if (T_PARALLEL(func)) {
	    return 1;
	  }
	}
      }
      return 0;
    }

  default:
    INT_FATAL(T_STMT(expr),"Unkown expression type in expr_rank(): %d",
	      T_TYPE(expr));
    return 0; /* won't get here, but compiler doesn't know that */
  }
}


expr_t *expr_find_reg(expr_t *expr) {
  expr_t *reg;
  expr_t *exprptr;
  
  reg = T_TYPEINFO_REG(expr);
  if (reg == NULL) {
    if (T_TYPE(expr) == BIAT && T_SUBTYPE(expr) == AT_RANDACC) {
      exprptr = T_RANDACC(expr);
    }
    else {
      exprptr = T_OPLS(expr);
    }
    while (exprptr != NULL) {
      reg = T_TYPEINFO_REG(exprptr);
      if (reg != NULL) {
	break;
      }
      exprptr = T_NEXT(exprptr);
    }
    if (reg == NULL) {
      exprptr = T_OPLS(expr);
      while (exprptr != NULL) {
	reg = expr_find_reg(exprptr);
	if (reg != NULL) {
	  break;
	}
	exprptr = T_NEXT(exprptr);
      }
    }
  }
  return reg;
}


int expr_null_arrayref(expr_t *expr) {
  if (!expr) {
    return 0;
  }
  if ((T_TYPE(expr) == ARRAY_REF) && (nthoperand(expr,2) == NULL)) {
    return 1;
  }
  return 0;
}


int expr_c_legal_init(expr_t *expr) {
  if (expr == NULL) {
    return 0;
  }

  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
    if (D_CLASS(S_DTYPE(T_IDENT(expr))) == DT_DIRECTION) {
      return symtab_dir_c_legal_init(T_IDENT(expr));
    } else {
      return 1;
    }
  case VARIABLE:
    return 0;
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    return expr_c_legal_init(T_OPLS(expr));
  case BIEXP:
    return 0;
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
    return (expr_c_legal_init(T_OPLS(expr)) && 
	    expr_c_legal_init(T_NEXT(T_OPLS(expr))));
  default:
    return 0;
  }
}

/* FUNCTION: expr_is_lvalue - return true if argument has lvalue
 * echris 2/11/98
 */

int expr_is_lvalue(expr_t * expr) {
  int retval=1;

  if (expr) {
    switch (T_TYPE(expr)) {
    case CONSTANT:
    case VARIABLE:
    case BIAT:
    case BIDOT:
    case ARRAY_REF:
    case PERMUTE:
      retval=1;
      break;
    default:
      retval=0;
      break;
    }
  }

  return retval;
}


int expr_is_atom(expr_t *expr) {
  int retval=1;

  if (expr) {
    switch (T_TYPE(expr)) {
    case CONSTANT:
    case VARIABLE:
    case BIAT:
    case NUDGE:
    case BIDOT:
    case ARRAY_REF:
      retval=1;
      break;
    default:
      retval=0;
      break;
    }
  }

  return retval;
}
  


int expr_is_ever_strided_ens(expr_t *expr) {
  symboltable_t *pst;
  genlist_t *aliases;
  expr_t *reg;
  static int calldepth=0;

  pst = expr_find_ens_pst(expr);
  if (pst == NULL) {
    INT_FATAL(NULL,"BLC: Bad array assumption in expr.c");
  }
  reg = T_TYPEINFO_REG(expr);
  if (reg == NULL) {
    if (calldepth) {
      /* This is a silly little conservative guess.  We can only get
	 here when we're analyzing aliases, but our analysis of
	 aliases is somewhat simplistic (and I'm being too lazy to
	 improve it).  In particular, if a parameter is an array of
	 ensembles (say, X: array [1..10] of [R] of double), and we
	 are looking at X[3], and A is the actual parameter, we try to
	 get region typeinfo from A rather than from A[3].  The latter
	 can be achieved by doing some careful splicing of expressions
	 (I believe we pull this off in callanal), but it's fairly
	 painful, so for now I'm assuming this case will result in
	 strided behavior.  Note that in simple cases (passing a
	 parallel array to a parallel array, the region typeinfo for
	 the actual will not be null, and this will get the right
	 answer. */
      return 1;
    } else {
      INT_FATAL(NULL,"BLC: Bad assumption in expr.c");
    }
  }
  if (S_CLASS(pst) == S_PARAMETER) {
    if (S_SUBCLASS(pst) == SC_INOUT) { /** IN AND OUT **/
      /* First check to see if we can tell stridedness from the expr itself */
      if (expr_is_strided_reg(reg,0)) {
	return 1;
      }
      /* Next check and see if the parameter is a multi-parameter, in which
	 case we can (conservatively) assume that stride changes with diff.
	 parameters.  We really could be more careful and check, though. */
      /* if not, and its size is determined by the callsite, check aliases */
      if (expr_contains_qreg(reg)) {
	aliases = S_ACTUALS(pst);
	while (aliases != NULL) {
	  calldepth++;
	  if (expr_is_ever_strided_ens(G_EXPR(aliases))) {
	    calldepth--;
	    return 1;
	  } else {
	    calldepth--;
	  }
	  aliases = G_NEXT(aliases);
	}
      }
    } else {
      return 1;
      /* Would need to check region covers for more exact information */
      /* But how often do we care by by-val arrays anyway? */
    }
  } else {
    if (expr_is_strided_reg(reg,1)) {
      return 1;
    }
  }
  return 0;
}


int expr_is_scan_reduce(expr_t *expr) {
  if (expr == NULL) {
    return 0;
  }
  switch (T_TYPE(expr)) {
  case REDUCE:
  case SCAN:
    return 1;
  default:
    return 0;
  }
}


expr_t* expr_sibling(expr_t* expr) {
  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to expr_sibling");
  }

  if (T_PARENT(expr) == NULL) {
    INT_FATAL(NULL, "parentless expr passed to expr_sibling");
  }

  if (T_OPLS(T_PARENT(expr)) == NULL) {
    INT_FATAL(NULL, "poorly formed expr passed to expr_sibling");
  }

  if (T_NEXT(T_OPLS(T_PARENT(expr))) == NULL) {
    INT_FATAL(NULL, "unary expr passed to expr_sibling");
  }

  if (T_OPLS(T_PARENT(expr)) == expr) {
    return T_NEXT(T_OPLS(T_PARENT(expr)));
  }
  else {
    return T_OPLS(T_PARENT(expr));
  }
}

int expr_requires_parens(expr_t* expr) {
  if (expr == NULL) {
    INT_FATAL(NULL, "expr_requires_parens: expr arg is null");
  }

  return !(T_PARENT(expr) == NULL ||
	   T_TYPE(T_PARENT(expr)) == FUNCTION ||
	   expr_is_atom(T_PARENT(expr)) ||
	   T_TYPE(expr) == FUNCTION ||
	   expr_is_atom(expr) ||
	   T_PRECEDENCE(expr) > T_PRECEDENCE(T_PARENT(expr)) ||
	   (T_TYPE(expr) == T_TYPE(T_PARENT(expr)) && T_ASSOCIATIVE(expr)) ||
	   (T_PRECEDENCE(expr) == T_PRECEDENCE(T_PARENT(expr)) &&
	    expr == T_OPLS(T_PARENT(expr))));
}


symboltable_t* expr_bidot_find_field(expr_t* expr,datatype_t* record_dt) {
  symboltable_t* field;

  if (expr == NULL || T_TYPE(expr) != BIDOT || 
      D_CLASS(record_dt) != DT_STRUCTURE) {
    return NULL;
  }
  field = D_STRUCT(record_dt);
  while (field) {
    if (field == T_IDENT(expr)) {
      return field;
    }
    
    field = S_SIBLING(field);
  }
  return NULL;
}


datatype_t* expr_bidot_find_field_dt(expr_t* expr,datatype_t* record_dt) {
  symboltable_t* field;

  field = expr_bidot_find_field(expr,record_dt);
  if (field == NULL) {
    return NULL;
  } else {
    return S_DTYPE(field);
  }
}


expr_t* new_const_int(int i)
{
  sprintf(buffer, "%d", i);
  return build_typed_0ary_op(CONSTANT, build_int(i));
}

expr_t* new_const_string(char *s)
{
  return build_typed_0ary_op(CONSTANT, combine(NULL, vstr(s)));
}

expr_t*  build_function_call(symboltable_t* pst, int num_args, ...) {
  expr_t* fcn;
  expr_t* expr;
  va_list ap;
  int i;

  fcn = NULL;
  va_start(ap, num_args);
  for (i=0; i<num_args; i++) {
    if (fcn == NULL) {
      fcn = copy_exprls(va_arg(ap, expr_t*), NULL);
    }
    else {
      expr = fcn;
      while (T_NEXT(expr) != NULL) {
	expr = T_NEXT(expr);
      }
      T_NEXT(expr) = copy_exprls(va_arg(ap, expr_t*), NULL);
    }
  }
  va_end(ap);
  expr = build_0ary_op(VARIABLE,pst);
  fcn = build_Nary_op(FUNCTION,expr,fcn);
  return fcn;
}


int expr_is_char(expr_t* expr, int unknown) {
  datatype_t* pdt;
  symboltable_t* pst;

  pdt = T_TYPEINFO(expr);
  if (pdt == NULL) {
    switch (T_TYPE(expr)) {
    case CONSTANT:
    case VARIABLE:
      pst = T_IDENT(expr);
      pdt = S_DTYPE(pst);
      break;
    default:
      pdt = NULL;
    }
  }
  if (pdt) {
    if (D_CLASS(pdt) == DT_CHAR) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return unknown;
  }
}


int expr_at_ensemble_root(expr_t* expr) {
  if (expr == NULL) {
    INT_FATAL(T_STMT(expr), "unexpected case in expr_at_ensemble_start");
  }
  if (T_PARENT(expr) != NULL && 
      expr_is_atom(T_PARENT(expr)) &&
      expr_at_ensemble_root(T_PARENT(expr))) {
    return 0;
  }
  switch (T_TYPE(expr)) {
  case VARIABLE:
    if (expr_is_indexi(expr)) {
      return 0;
    } else {
      return (T_TYPEINFO_REG(expr) != NULL);
    }
  case BIDOT:
  case ARRAY_REF:
    return (T_TYPEINFO_REG(expr) != NULL && 
	    T_TYPEINFO_REG(T_OPLS(expr)) == NULL &&
	    !expr_is_promoted_indarr(expr));
  default:
    return 0;
  }
}


expr_t* expr_find_ensemble_root(expr_t* expr) {
  if (expr == NULL) {
    INT_FATAL(T_STMT(expr), "unexpected case in expr_at_ensemble_start");
  }
  switch (T_TYPE(expr)) {
  case VARIABLE:
    if (!expr_at_ensemble_root(expr)) {
      return NULL;
    } else {
      return expr;
    }
  case BIDOT:
  case ARRAY_REF:
  case BIAT:
    if (expr_at_ensemble_root(expr)) {
      return expr;
    } else {
      return expr_find_ensemble_root(T_OPLS(expr));
    }
  default:
    return NULL;
  }
}   


expr_t* expr_find_an_ensemble_root(expr_t* expr) {
  expr_t* retval;

  if (T_IS_BINARY(T_TYPE(expr))) {
    retval = expr_find_ensemble_root(T_OPLS(expr));
    if (retval == NULL) {
      retval = expr_find_ensemble_root(T_NEXT(T_OPLS(expr)));
    }
  } else {
    retval = expr_find_ensemble_root(expr);
  }

  return retval;
}


static int expr_is_region(expr_t* expr) {
  if (expr == NULL) {
    return 0;
  }
  if (T_TYPEINFO(expr) == NULL) {
    INT_FATAL(NULL, "NULL typeinfo in expr_is_region");
  }
  if (D_CLASS(T_TYPEINFO(expr)) != DT_REGION) {
    INT_FATAL(NULL, "expression is not region in expr_is_region");
  }
  return 1;
}


int expr_is_qreg(expr_t* expr) {
  if (!expr_is_region(expr)) {
    return 0;
  }
  if (T_TYPE(expr) == CONSTANT || T_TYPE(expr) == VARIABLE) {
    return symtab_is_qreg(T_IDENT(expr));
  } else {
    return 0;
  }
}


int expr_contains_qreg(expr_t* expr) {
  if (!expr_is_region(expr)) {
    return 0;
  }
  if (T_TYPE(expr) == CONSTANT || T_TYPE(expr) == VARIABLE) {
    return expr_is_qreg(expr);
  } else if (T_TYPE(expr) == BIPREP || T_TYPE(expr) == BIWITH) {
    return expr_contains_qreg(T_OPLS(expr));
  } else {
    return 0;
  }
}


int expr_is_grid_scalar(expr_t* expr) {
  if (expr == NULL) {
    return 0;
  }
  if (T_TYPE(expr) == CONSTANT || T_TYPE(expr) == VARIABLE) {
    return symtab_is_grid_scalar(T_IDENT(expr));
  } else {
    return 0;
  }
}


int expr_is_dense_reg(expr_t* expr) {
  if (!expr_is_region(expr)) {
    return 0;
  }
  return datatype_is_dense_reg(T_TYPEINFO(expr));
}


int expr_is_strided_reg(expr_t* expr, int unsure) {
  int numdims;
  static int regstackdepth=0;
  int retval;
  expr_t* covreg;

  if (!expr_is_region(expr)) {
    return 0;
  }
  switch (T_TYPE(expr)) {
  case CONSTANT:
  case VARIABLE:
    numdims = expr_is_qreg(expr);
    if (numdims) {
      covreg = RMSRegionAtDepth(regstackdepth);
      regstackdepth++;
      if (covreg == NULL) {
	retval = unsure;
      } else {
	retval = expr_is_strided_reg(covreg, unsure);
      }
      regstackdepth--;
    } else {
      if (S_VAR_INIT(T_IDENT(expr))) {
	expr_t* initexpr = IN_VAL(S_VAR_INIT(T_IDENT(expr)));
	if (T_TYPE(initexpr) == SBE) {
	  retval = dimlist_inherits(T_SBE(initexpr));
	} else {
	  typeinfo_expr(initexpr);
	  retval = expr_is_strided_reg(initexpr, unsure);
	}
      } else {
	retval = unsure;
      }
    }
    break;
  case BIPREP:
    if (T_SUBTYPE(expr) == BY_REGION) {
      return 1;
    } else {
      return expr_is_strided_reg(T_OPLS(expr), unsure);
    }
  case BIWITH:
    return expr_is_strided_reg(T_OPLS(expr), unsure);
  default:
    /* this is conservative; could look deeper... */
    return 1;
  }

  return retval;
}


int expr_dyn_reg(expr_t* reg) {
  if (!expr_is_region(reg)) {
    return 0;
  }
  switch (T_TYPE(reg)) {
  case CONSTANT:
  case VARIABLE:
    if (T_SUBTYPE(reg) == DYNAMIC_REGION) {
      return 1;
    } else {
      return 0;
    }
  case BIPREP:
    if (!expr_rt_const(T_NEXT(T_OPLS(reg)))) {
      return 1;
    }
    /* fall through */
  case BIWITH:
    return expr_dyn_reg(T_OPLS(reg));
  default:
    return 0;
  }
}


static expr_t* expr_reg_find_sparsity(expr_t* reg) {
  expr_t* retval;
  int numdims;
  static int regstackdepth=0;

  if (!expr_is_region(reg)) {
    return 0;
  }
  retval = NULL;
  if (T_TYPE(reg) == BIWITH) {
    switch (T_SUBTYPE(reg)) {
    case SPARSE_REGION:
      retval = reg;
      break;
    case AT_REGION:
    case OF_REGION:
    case IN_REGION:
    case BY_REGION:
      retval = expr_reg_find_sparsity(T_OPLS(reg));
      break;
    case INTERNAL_REGION:
      numdims = expr_is_qreg(reg);
      if (numdims) {
	retval = expr_reg_find_sparsity(RMSRegionAtDepth(regstackdepth++));
	regstackdepth--;
      }
      break;
    default:
      break;
    }
  }
  return retval;
}


int expr_regs_share_sparsity(expr_t* reg1, expr_t* reg2) {
  expr_t* basereg1;
  expr_t* basereg2;

  if (!expr_is_region(reg1) || !expr_is_region(reg2)) {
    return 0;
  }
  basereg1 = expr_reg_find_sparsity(reg1);
  basereg2 = expr_reg_find_sparsity(reg2);

  return expr_equal(basereg1, basereg2);
}


int expr_is_sparse_reg(expr_t* reg) {
  if (!expr_is_region(reg)) {
    return 0;
  } else {
    return !(datatype_is_dense_reg(T_TYPEINFO(reg)));
  }
}


int expr_is_ofreg(expr_t* reg) {
  if (!expr_is_region(reg)) {
    return 0;
  }
  switch (T_TYPE(reg)) {
  case CONSTANT:
  case VARIABLE:
    return expr_is_ofreg(IN_VAL(S_VAR_INIT(T_IDENT(reg))));
  case BIPREP:
    switch (T_SUBTYPE(reg)) {
    case OF_REGION:
      return 1;
    default:
      return 0;
    }
  default:
    return 0;
  }
}


int expr_is_clean_sparse_reg(expr_t* reg) {
  datatype_t* regpdt;
  int numdims;
  int i;

  if (!expr_is_region(reg)) {
    return 0;
  }
  if ((reg == NULL) || (D_CLASS(T_TYPEINFO(reg)) != DT_REGION) ||
      (expr_is_dense_reg(reg)) || T_TYPE(reg) != BIWITH || 
      T_SUBTYPE(reg) != SPARSE_REGION) {
    return 0;
  } else {
    regpdt = T_TYPEINFO(reg);
    numdims = D_REG_NUM(regpdt);
    for (i=0; i<numdims; i++) {
      if (S_FLUFF_LO(T_IDENT(reg),i) != 0 || S_FLUFF_HI(T_IDENT(reg),i) != 0 ||
	  S_UNK_FLUFF_LO(T_IDENT(reg),i) != 0 || S_UNK_FLUFF_HI(T_IDENT(reg),i) != 0) {
	return 0;
      }
    }
    return 1;
  }
}


int expr_prep_reg(expr_t* reg) {
  if (!expr_is_region(reg)) {
    return 0;
  }
  return (T_TYPE(reg) == BIPREP);
}


int expr_is_promoted_indarr(expr_t* expr) {
  expr_t* arrexpr;
  datatype_t* arrdt;

  if (T_TYPE(expr) != ARRAY_REF) {
    return 0;
  }
  arrexpr = T_OPLS(expr);
  arrdt = D_ARR_TYPE(T_TYPEINFO(arrexpr));

  return (datatype_find_ensemble(arrdt) == NULL);
}

/*** returns initial value assigned to a direction ***/
expr_t* expr_direction_value(expr_t* expr, int dim) {
  dimension_t* dimlist;
  int i;

  if (expr_const(expr) && T_TYPE(expr) == ARRAY_REF) {
    return expr_direction_value(IN_VAL(S_VAR_INIT(T_IDENT(T_OPLS(expr)))), dim);
  }

  if (T_TYPE(expr) != SBE) {
    INT_FATAL(NULL, "error in expr_direction_value");
  }

  dimlist = T_SBE(expr);
  for (i = 0; i < dim; i++) {
    dimlist = DIM_NEXT(dimlist);
  }
  return DIM_LO(dimlist);
}
