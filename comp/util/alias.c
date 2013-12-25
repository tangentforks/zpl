/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include "../include/alias.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/parsetree.h"
#include "../include/runtime.h"
#include "../include/struct.h"
#include "../include/symmac.h"
#include "../include/treemac.h"

static int symtabs_alias(symboltable_t *pst1,symboltable_t *pst2,expr_t *root1,
			 expr_t *root2) {
  int check_alias1=0;
  int check_alias2=0;
  function_t *fn1=NULL;
  function_t *fn2=NULL;
  genlist_t *alias1=NULL;
  genlist_t *alias2=NULL;

  if (pst1 == NULL || pst2 == NULL) {
    return 0;
  }
  if (pst1 == pst2) {
    return 1;
  }
  if (S_SUBCLASS(pst1) == SC_INOUT) {
    check_alias1 = 1;
    alias1 = S_ACTUALS(pst1);
    fn1 = S_FUN_BODY(S_PARENT(pst1));
  }
  if (S_SUBCLASS(pst2) == SC_INOUT) {
    check_alias2 = 1;
    alias2 = S_ACTUALS(pst2);
    fn2 = S_FUN_BODY(S_PARENT(pst2));
  }
  if (check_alias1 || check_alias2) {
    if (check_alias1 && check_alias2) {
      if (fn1 != fn2) {
	INT_FATAL(NULL,
		  "How could we be comparing parameters for different fns?");
	/* would need to do n^2 comparisons here */
      }
      while (alias1 != NULL) {
	if (alias2 == NULL) {  /* this shouldn't happen, but may if people
				  don't update alias information as retens.c
				  currently does */
	  return 1;
	}
	if (exprs_alias(G_EXPR(alias1),G_EXPR(alias2))) {
	  return 1;
	}
	alias1 = G_NEXT(alias1);
	alias2 = G_NEXT(alias2);
      }
    } else {
      if (check_alias1) {
	while (alias1 != NULL) {
	  if (exprs_alias(G_EXPR(alias1),root2)) {
	    return 1;
	  }
	  alias1 = G_NEXT(alias1);
	}
      } else {
	while (alias2 != NULL) {
	  if (exprs_alias(root1,G_EXPR(alias2))) {
	    return 1;
	  }
	  alias2 = G_NEXT(alias2);
	}
      }
    }
  }
  return 0;
}


static expr_t *expr_at_depth(expr_t *expr,int target_depth) {
  static int depth;
  expr_t *retval=NULL;

  if (expr == NULL || !expr_is_lvalue(expr)) {
    depth = 0;
  } else {
    retval = expr_at_depth(T_OPLS(expr),target_depth);
    if (!retval && T_TYPE(expr) != BIAT) {
      depth++;
      if (depth == target_depth) {
	retval = expr;
      }
    }
  }
  return retval;
}


static int exprs_nonequal_shift(expr_t *varexpr,expr_t *biexp) {
  int retval = 0;
  expr_t *lexpr;
  expr_t *rexpr;
  expr_t *offexpr = NULL;
  
  /* check for var+c, c+var, var-c, where c != 0 */
  switch (T_TYPE(biexp)) {
  case BIPLUS:
  case BIMINUS:
    lexpr = T_OPLS(biexp);
    rexpr = T_NEXT(T_OPLS(biexp));
    if (expr_equal(varexpr,lexpr)) {
      offexpr = rexpr;
    } 
    if (offexpr != NULL) {
      if (expr_computable_const(offexpr)) {
	if (expr_intval(offexpr) != 0) {
	  retval = 1;
	}
      }
    }
    break;
  default:
    break;
  }

  return retval;
}


static int exprs_struct_alias(expr_t *expr1,expr_t *expr2) {
  static int depth;
  expr_t *compexpr;
  int retval = 1;
  expr_t *ind1expr;
  expr_t *ind2expr;
  long ind1;
  long ind2;
  expr_t *varexpr;
  expr_t *biexpr;

  if (expr1 == NULL) {
    depth = 0;
  } else {
    switch (T_TYPE(expr1)) {
    case CONSTANT:
    case VARIABLE:
    case BIAT:
    case BIDOT:
    case ARRAY_REF:
      retval = exprs_struct_alias(T_OPLS(expr1),expr2);
      break;
    default:
      INT_FATAL(NULL,"Unexpected expression type in exprs_struct_alias");
    }
    if (retval && T_TYPE(expr1) != BIAT) {
      depth++;
      compexpr = expr_at_depth(expr2,depth);
      if (T_TYPE(expr1) != T_TYPE(compexpr)) {
	retval = 0;
      } else {
	switch (T_TYPE(expr1)) {
	case CONSTANT:
	case VARIABLE:
	  /* we've already verified this case by checking the pst's */
	  break;
	case BIDOT:
	  if (T_IDENT(expr1) != T_IDENT(compexpr)) {
	    retval = 0;
	  }
	  break;
	case ARRAY_REF:
	  /* the only case we're interested in is when the indices differ;
	     if we cannot tell that they differ, we must assume they alias */
	  ind1expr = T_NEXT(T_OPLS(expr1));
	  ind2expr = T_NEXT(T_OPLS(compexpr));
	  while (ind1expr && ind2expr) {
	    /* are the indices distinct constants? */
	    if (expr_computable_const(ind1expr) && 
		expr_computable_const(ind2expr)) {
	      ind1 = expr_intval(ind1expr);
	      ind2 = expr_intval(ind2expr);
	      if (ind1 != ind2) {
		retval = 0;
		break;
	      }
	    }
	    /* check for var vs. var +/- const */
	    if (expr_is_atom(ind1expr) != expr_is_atom(ind2expr)) {
	      if (expr_is_atom(ind1expr)) {
		varexpr = ind1expr;
		biexpr = ind2expr;
	      } else {
		varexpr = ind2expr;
		biexpr = ind1expr;
	      }
	      if (exprs_nonequal_shift(varexpr,biexpr)) {
		retval = 0;
		break;
	      }
	    }
	    ind1expr = T_NEXT(ind1expr);
	    ind2expr = T_NEXT(ind2expr);
	  }
	  break;
	default:
	  INT_FATAL(NULL,"Unexpected expression type in exprs_struct_alias");
	}
      }
    }
  }
  return retval;
}


int exprs_alias(expr_t *expr1,expr_t *expr2) {
  int retval;

  IFDB(2) {
    printf("exprs_alias called with ");
    dbg_gen_expr(stdout,expr1);
    printf(" and ");
    dbg_gen_expr(stdout,expr2);
    printf("\n");
  }
  if (expr1 == NULL || expr2 == NULL) {
    DBS0(2,"...returning 0 (NULL)\n");
    return 0;
  }
  if (!expr_is_lvalue(expr1)) {
    switch(T_TYPE(expr1)) {
    case UNEGATIVE:
    case UPOSITIVE:
    case UCOMPLEMENT:
      retval = exprs_alias(T_OPLS(expr1),expr2);
      DBS1(2,"...returning %d (arg 1 unary)\n",retval);
      return retval;
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
      retval = (exprs_alias(T_OPLS(expr1),expr2) || 
		exprs_alias(T_NEXT(T_OPLS(expr1)),expr2));
      DBS1(2,"...returning %d (arg1 binary)\n",retval);
      return retval;
    default:
      DBS0(2,"...returning 0 (arg1 complex)\n");
      return 0;
    }
  }
  if (!expr_is_lvalue(expr2)) {
    switch(T_TYPE(expr2)) {
    case UNEGATIVE:
    case UPOSITIVE:
    case UCOMPLEMENT:
      retval = exprs_alias(expr1,T_OPLS(expr2));
      DBS1(2,"...returning %d (arg2 unary)\n",retval);
      return retval;
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
      retval = (exprs_alias(expr1,T_OPLS(expr2)) || 
		exprs_alias(expr1,T_NEXT(T_OPLS(expr2))));
      DBS1(2,"...returning %d (arg2 binary)\n",retval);
      return retval;
    default:
      DBS0(2,"...returning 0 (arg2 complex)\n");
      return 0;
    }
  }
  if (!symtabs_alias(expr_find_root_pst(expr1),expr_find_root_pst(expr2),
		     expr1,expr2)) {
    DBS0(2,"...returning 0 (symtabs don't alias)\n");
    return 0;
  }
  retval = exprs_struct_alias(expr1,expr2);
  DBS1(2,"...returning %d (comparison of structures)\n",retval);
  return retval;
}


static int exprs_cda_alias_help(expr_t *lhs,expr_t *rhs,int consider_primeats) {
  int retval;
  expr_t *lhs_at_expr;
  expr_t *rhs_at_expr;
  expr_t *lhs_dir=NULL;
  expr_t *rhs_dir=NULL;

  IFDB(2) {
    printf("exprs_cda_alias called with ");
    dbg_gen_expr(stdout,lhs);
    printf(" and ");
    dbg_gen_expr(stdout,rhs);
    printf("\n");
  }
  if (lhs == NULL || rhs == NULL) {
    DBS0(2,"...returning 0 (NULL)\n");
    return 0;
  }
  if (!expr_is_lvalue(lhs)) {
    INT_FATAL(NULL,"lvalue assumption error in exprs_cda_alias_help");
  }
  if (!expr_is_lvalue(rhs)) {
    switch(T_TYPE(rhs)) {
    case UNEGATIVE:
    case UPOSITIVE:
    case UCOMPLEMENT:
      retval = exprs_cda_alias_help(lhs,T_OPLS(rhs),consider_primeats);
      DBS1(2,"...returning %d (rhs unary)\n",retval);
      return retval;
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
      retval = (exprs_cda_alias_help(lhs,T_OPLS(rhs),consider_primeats) || 
		exprs_cda_alias_help(lhs,T_NEXT(T_OPLS(rhs)),consider_primeats));
      DBS1(2,"...returning %d (rhs binary)\n",retval);
      return retval;
    default:
      DBS0(2,"...returning 0 (rhs complex)\n");
      return 0;
    }
  }
  lhs_at_expr = expr_find_at(lhs);
  rhs_at_expr = expr_find_at(rhs);
  if (lhs_at_expr) {
    lhs_dir = T_NEXT(T_OPLS(lhs_at_expr));
  }
  if (rhs_at_expr) {
    /* although this seems like a good idea, contraction won't order the
       Mscan loops properly, as it needs an inter-statement dependence to
       get it right */
    if (consider_primeats==0 && T_SUBTYPE(rhs_at_expr)==AT_PRIME) {
      DBS0(2,"...returning 0 (primed reference)\n");
      return 0;
    }
    rhs_dir = T_NEXT(T_OPLS(rhs_at_expr));
  }
  if (expr_equal(lhs_dir,rhs_dir)) {
    DBS0(2,"...returning 0 (dirs match)\n");
    return 0;
  }
  /* directions are different, so let's see if these things alias */
  retval = exprs_alias(lhs,rhs);
  DBS1(2,"...returning %d (lhs and rhs alias)\n",retval);
  return retval;
}


int exprs_cda_alias(expr_t *lhs,expr_t *rhs) {
  return exprs_cda_alias_help(lhs,rhs,1);
}


int exprs_really_cda_alias(expr_t* lhs,expr_t* rhs) {
  return exprs_cda_alias_help(lhs,rhs,0);
}
