/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/error.h"
#include "../include/db.h"
#include "../include/runtime.h"
#include "../include/symbol.h"
#include "../include/buildsym.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/genlist.h"
#include "../include/stmtutil.h"
#include "../include/allocstmt.h"
#include "../include/traverse.h"
#include "../include/passes.h"


static symboltable_t * Gnewparam;
static int doing_indarray=0;

static symboltable_t * AddParamExPostFacto(char * name,datatype_t * type,
					   symboltable_t * fn) {
  symboltable_t * new;

  new = alloc_st(S_PARAMETER,name);
  S_PAR_TYPE(new) = type;
  S_PAR_CLASS(new) = SC_INOUT;     /* echris: for some reason was SC_VALUE */
  S_PARENT(new) = fn;
  S_FG_PARAM(new) = 1;
  S_FG_ACTIVE(new) = S_INACTIVE;
  S_LEVEL(new)=1;  

  return new;
}


static void ChangeFunctionReturnType(symboltable_t * pst) {
  D_FUN_TYPE(S_DTYPE(pst))=pdtVOID;
}


static void MakeReturnValueParam(datatype_t * retpdt,function_t * thefunction) {
  symboltable_t * newparam;
  genlist_t * glp;
  symboltable_t * formals;
  
  /* could possibly use build/duplicate.c except that it seems to make
     them SC_REFER rather than SC_VALUE and assumes that they'll be
     of type pdtINT */
  newparam=AddParamExPostFacto("_LHS_ens",retpdt,T_FCN(thefunction));
  Gnewparam = newparam;

  glp = alloc_gen();
  G_IDENT(glp) = newparam;
  T_PARAMLS(thefunction) = cat_genlist_ls(T_PARAMLS(thefunction),glp);
  T_NUM_PARAMS(thefunction)++;
  
  formals = T_DECL(thefunction);
  if (formals == NULL) {
    T_DECL(thefunction) = newparam;
  } else {
    while (S_SIBLING(formals) != NULL && S_FG_PARAM(S_SIBLING(formals))) {
      formals = S_SIBLING(formals);
    }
    if (S_SIBLING(formals) == NULL) {
      S_SIBLING(formals) = newparam;
    } else {
      S_SIBLING(newparam) = S_SIBLING(formals);
      S_SIBLING(formals) = newparam;
    }
  }
}


static void ChangeReturnsToAssigns(statement_t * s) {
  expr_t * retval;
  expr_t * new_expr;
  expr_t * new_expr2;
  expr_t * compexpr;
  expr_t * assgnexpr;
  statement_t * assgnstmt;
  statement_t * ifstmt;
  
  if (T_TYPE(s)==S_RETURN) {
    retval=T_RETURN(s);
    T_RETURN(s)=NULL;

    new_expr = build_0ary_op(VARIABLE, Gnewparam);
    new_expr2 = copy_expr(new_expr);
    if (doing_indarray) {
      /* insert a blank array reference 
	 (BUG: should be possibly more than one if this is a nested array) */
      new_expr2 = build_Nary_op(ARRAY_REF, new_expr2, NULL);
    }

    compexpr=build_Nary_op(FUNCTION,
			   build_0ary_op(VARIABLE,
					 lookup(S_SUBPROGRAM,"_T_GOOD_ARR")),
			   new_expr);

    assgnexpr=build_binary_op(BIASSIGNMENT,retval,new_expr2);
    assgnstmt=build_expr_statement(assgnexpr,T_LINENO(s),T_FILENAME(s));

    ifstmt=build_if_statement(compexpr,assgnstmt,NULL,NULL,T_LINENO(s),
			      T_FILENAME(s));

    insertbefore_stmt(ifstmt,s);

    DBS0(10, "*** Changed a return statement!! \n");

  }
}


static void FindFunctionsWhichReturnEnsembles(module_t * mod) {
  function_t *current_function;
  symboltable_t * pst;
  datatype_t * retpdt;
  
  current_function = T_FCNLS(mod);

  while (current_function != NULL) {
    pst = T_FCN(current_function);
    retpdt = D_FUN_TYPE(S_DTYPE(pst));
    if (retpdt == NULL) {
      INT_FATAL(NULL,
		"Function '%s' has not return datatype pointer (If this function is in the standard context, it should still get a return datatype structure)",
		S_IDENT(pst));
    }
    if (D_CLASS(retpdt)==DT_ENSEMBLE || D_CLASS(retpdt) == DT_ARRAY) {

      doing_indarray = (D_CLASS(retpdt) == DT_ARRAY);

      DBS1(10, "Found %s\n",S_IDENT(pst));
      DBS0(10, "   *** And it returns an array!!!\n");

      ChangeFunctionReturnType(pst);

      DBS0(10, "   *** Changed it to return a void!!!\n");

      MakeReturnValueParam(retpdt,current_function);

      DBS0(10, "   *** Added it as a parameter!!!\n");

      traverse_stmtls_g(T_STLS(current_function),ChangeReturnsToAssigns,NULL,
			NULL, NULL);

      doing_indarray = 0;

    }
    current_function = T_NEXT(current_function);
  }
}


static void MoveLHSToParamList(expr_t * LHS,expr_t * fncall) {
  expr_t * temp;

  temp=T_OPLS(fncall);
  while (T_NEXT(temp)!=NULL) {
    temp = T_NEXT(temp);
  }
  T_NEXT(temp)=LHS;

  T_PREV(LHS)=temp;
  T_PARENT(LHS)=T_PARENT(temp);
  T_NEXT(LHS)=NULL;
}


static void ChangeBiAssignEnsRetFns(statement_t * s) {
  expr_t * exp;
  expr_t * LHS;
  expr_t * RHS;
  symboltable_t * pst;
  datatype_t * retpdt;

  if (T_TYPE(s) == S_EXPR) {
    exp=T_EXPR(s);
    if (T_TYPE(exp) == BIASSIGNMENT) {
      LHS = T_OPLS(exp);
      RHS = T_NEXT(LHS);
      
      if (T_TYPE(RHS) == FUNCTION) {
	pst = T_IDENT(T_OPLS(RHS));

	DBS1(10, "Found an assignment using %s\n",S_IDENT(pst));

	if (S_DTYPE(pst)==NULL) {

	  DBS0(10, "  *** Undefined return type (external call?)!!!\n");

	} else {
	  
	  retpdt = D_FUN_TYPE(S_DTYPE(pst));
	  if (retpdt == NULL) {
	    INT_FATAL(s, "Function '%s' has no return datatype pointer (If this function is in the standard context, it should still get a return datatype structure)",
		      S_IDENT(pst));
	  }
	  if (D_CLASS(retpdt)==DT_ENSEMBLE || D_CLASS(retpdt) == DT_ARRAY) {

	    DBS0(10, "   *** And it returns an array!!!\n");

	    MoveLHSToParamList(LHS,RHS);

	    DBS0(10, "   *** Moved its LHS to be a parameter\n");
	    
	    T_EXPR(s)=RHS;
	  }
	}
      }
    } else if (T_TYPE(exp) == FUNCTION) {
      pst = T_IDENT(T_OPLS(exp));

      DBS1(10, "Found a simple call to %s\n",S_IDENT(pst));

      if (S_DTYPE(pst)==NULL) {

	DBS0(10, "  *** Undefined return type (external call?)!!!\n");

      } else {
	retpdt = D_FUN_TYPE(S_DTYPE(pst));
	if (retpdt == NULL) {
	  INT_FATAL(s, "Function '%s' has no return datatype pointer (If this function is in the standard context, it should still get a return datatype structure)",
		    S_IDENT(pst));
	}
	if (D_CLASS(retpdt)==DT_ENSEMBLE || D_CLASS(retpdt)==DT_ARRAY) {

	  DBS0(10, "   *** And it returns an array!!!\n");
	  
	  MoveLHSToParamList(build_0ary_op(VARIABLE,lookup(S_VARIABLE,"_ZPL_NULL")),
			     exp);

	  DBS0(10, "  *** Added _NULL as a parameter\n");

	}
      }
    }
  }
}


static void ChangeCallsToEnsReturningFns(module_t * mod) {
  function_t * current_function;

  current_function = T_FCNLS(mod);

  while (current_function != NULL) {
    
    traverse_stmtls_g(T_STLS(current_function),ChangeBiAssignEnsRetFns,NULL,
		      NULL,NULL);

    current_function = T_NEXT(current_function);
  }
}

static void ChangeFunctionsWhichReturnEnsembles(module_t * mod) {
  traverse_modules(mod, TRUE, ChangeCallsToEnsReturningFns, NULL);

  DBS0(10, "------------\n");

  traverse_modules(mod, TRUE, FindFunctionsWhichReturnEnsembles, NULL);
}

int call_return_ensembles(module_t * mod,char * s) {
  ChangeFunctionsWhichReturnEnsembles(mod);
  return 0;
}

