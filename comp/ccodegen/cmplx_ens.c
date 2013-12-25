/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdlib.h>
#include <string.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/buildstmt.h"
#include "../include/buildsym.h"
#include "../include/buildzplstmt.h"
#include "../include/cmplx_ens.h"
#include "../include/datatype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/macros.h"
#include "../include/name.h"
#include "../include/runtime.h"
#include "../include/stmtutil.h"
#include "../include/symbol.h"

#define CMPLX_ENSEMBLE 1
#define SCALAR_PARAM   2

typedef struct _fakeinfotype {
  expr_t *expr;
  symboltable_t *pst;
  int type;
  int BIATSonly;
  expr_t *reg;
  int rank;
} fakeinfotype;


/* variables from global.h */

int brad_no_access = FALSE;


static int num_fakes_allocated = 0;
static int num_fakes = 0;
static fakeinfotype *fakeinfo = NULL;
static int globuatsonly;
static int pass_null_arrayref=0;


expr_t *SearchForFunctionCall(expr_t *expr) {
  expr_t *left;
  expr_t *right;

  switch (T_TYPE(expr)) {
  case FUNCTION:
    if (T_IDENT(T_OPLS(expr)) == chksave ||
	T_IDENT(T_OPLS(expr)) == chkrecover ||
	T_IDENT(T_OPLS(expr)) == chklabel) {
      return NULL;
    }
    return expr;
  case ARRAY_REF:
    if (nthoperand(expr, 2) != NULL)	/*** sungeun ***/
      return SearchForFunctionCall(nthoperand(expr,2));
    else		/*** fake array ref for nloop generation ***/
      return NULL;
  case BIDOT:
  case BIAT:
    return SearchForFunctionCall(T_OPLS(expr));
  default:
    if (T_IS_UNARY(T_TYPE(expr))) {
      return SearchForFunctionCall(T_OPLS(expr));
    }
    if (T_IS_BINARY(T_TYPE(expr))) {
      left = SearchForFunctionCall(left_expr(expr));
      right = SearchForFunctionCall(right_expr(expr));
      if (left != NULL) {
	if (right != NULL) {
	  return expr;
	} else {
	  return left;
	}
      } else {
	if (right != NULL) {
	  return right;
	} else {
	  return NULL;
	}
      }
    }
    return NULL;
  }
}



static int valid_expr(expr_t *exp) {
  expr_t *tempexp;
  int exitflag;

  if (T_TYPE(exp) == VARIABLE || 
      (pass_null_arrayref && T_TYPE(exp) == ARRAY_REF && 
       nthoperand(exp,2) == NULL)) {
    return 0;
  } else {
    tempexp = exp;

    exitflag = 0;
    while (!exitflag) {
      switch (T_TYPE(tempexp)) {
      case CONSTANT:
      case VARIABLE:
	exitflag = 1;
	break;
      case BIAT:
      case BIDOT:
      case ARRAY_REF:
	if (T_TYPE(tempexp) != BIAT) {
	  globuatsonly = 0;
	}
	if (T_TYPEINFO_REG(T_OPLS(tempexp)) == NULL) {
	  exitflag = 1;
	} else {
	  if (expr_null_arrayref(tempexp) && tempexp == exp) {
	    exp = T_OPLS(tempexp);
	  }
	  tempexp = T_OPLS(tempexp);
	}
	break;
      default:
	return 0;
	/*
	INT_FATAL(NULL, "Unexpected expression type in "
		  "cmplx_ens.c:valid_expr!! (%d)", T_TYPE(tempexp));
	*/
      }
    }
    return (tempexp != exp);
  }
}


static expr_t *find_base_ens_exp(FILE* outfile, expr_t *exp,
				      expr_t ** base_exp, int printflag) {
  expr_t *dir = NULL;

  switch (T_TYPE(exp)) {
  case VARIABLE:
    *base_exp = exp;
    if (printflag) {
      gen_expr(outfile, exp);
      fprintf(outfile,"))");
    }
    break;
  case BIDOT:
  case ARRAY_REF:
  case BIAT:
    if (T_TYPEINFO_REG(T_OPLS(exp)) == NULL) {
      *base_exp = exp;
      if (printflag) {
	gen_expr(outfile, exp);
	fprintf(outfile,"))");
      }
    } else {
      dir = find_base_ens_exp(outfile, T_OPLS(exp),base_exp,printflag);
      switch (T_TYPE(exp)) {
      case BIAT:
	dir = T_NEXT(T_OPLS(exp));
	break;
      case BIDOT:
	if (printflag) {
	  fprintf(outfile,"%s%s",OpName[(int)T_TYPE(exp)],
		  S_IDENT(T_IDENT(exp)));
	}
	break;
      case ARRAY_REF:
	if (printflag) {
	  gen_arrayref(outfile, nthoperand(exp,2),T_TYPEINFO(nthoperand(exp,1)));
	}
	break;
      default:
	break;
      }
    }
    break;
  default:
    INT_FATAL(NULL, "Unexpected expression type in cmplx_ens.c!! -- "
	      "%d",T_TYPE(exp));
  }
  return dir;
}


static void ExpandFakeInfo(void) {
  int temp_num;
  int i;
  char name[32];

  temp_num = (int)(num_fakes_allocated *1.5) + 1; /* 1, 2, 4, 7, ... */
  if (fakeinfo != NULL) {
    fakeinfo = (void *)realloc((void *)fakeinfo,temp_num*sizeof(fakeinfotype));
  } else {
    fakeinfo = (void *)PMALLOC(temp_num*sizeof(fakeinfotype));
  }
  INT_COND_FATAL((fakeinfo!=NULL), NULL, "Realloc failed in ExpandFakeInfo()");
  
  for (i=num_fakes_allocated;i<temp_num;i++) {
    sprintf(name,"_fake_ens_param_%d",i);
    fakeinfo[i].pst = alloc_st(S_VARIABLE,name);
  }
  num_fakes_allocated = temp_num;
}


static void DoStartFakesPrinting(FILE* outfile) {
  expr_t *exp;
  expr_t *base_exp;
  datatype_t *cast_type;
  expr_t *dir;
  int i;

  brad_no_access = TRUE;

  fprintf(outfile,"{\n");
  for (i=0;i<num_fakes;i++) {
    fprintf(outfile,"  int _offset%d;\n",i);
    fprintf(outfile,"  _arr_info _fep_info_%d;\n",i);
    fprintf(outfile,"  _array_fnc _fake_ens_param_%d=&(_fep_info_%d);\n",i,
	    i);
    if (fakeinfo[i].type == SCALAR_PARAM) {
      gen_pdt(outfile,T_TYPEINFO(fakeinfo[i].expr),PDT_GLOB);
      fprintf(outfile,"_fep_scalar_%d;\n",i);
    }
  }
  fprintf(outfile,"\n");
  
  for (i=0;i<num_fakes;i++) {
    switch (fakeinfo[i].type) {
    case CMPLX_ENSEMBLE:
      exp = fakeinfo[i].expr;
      base_exp = exp;
      while (base_exp && T_TYPEINFO_REG(base_exp) != NULL) {
	cast_type = T_TYPEINFO(base_exp);
	base_exp = T_OPLS(base_exp);
      }
      fprintf(outfile,"\n");
      fprintf(outfile,"_offset%d = ",i);
      if (!fakeinfo[i].BIATSonly) {
	fprintf(outfile,"(char *)(&((");
	fprintf(outfile,"*");
	fprintf(outfile,"((");
	gen_pdt(outfile,cast_type,PDT_PCST);
	fprintf(outfile,"*)");
	dir = find_base_ens_exp(outfile, exp,&base_exp,1);
	fprintf(outfile,")) - (char *)(&(");
	fprintf(outfile,"*");
	fprintf(outfile,"((");
	gen_pdt(outfile,cast_type,PDT_PCST);
	fprintf(outfile,"*)");
	gen_expr(outfile, base_exp);
	fprintf(outfile,")))");
      } else {
	dir = find_base_ens_exp(outfile, exp,&base_exp,0);
	fprintf(outfile,"0");
      }
      fprintf(outfile,";\n");
      fprintf(outfile,"  _CreateOffsetEnsemble(_fake_ens_param_%d,",i);
      gen_expr(outfile, base_exp);
      fprintf(outfile,",_offset%d,",i);
      if (dir) {
	gen_expr(outfile,dir);
      } else {
	fprintf(outfile,"NULL");
      }
      fprintf(outfile,",%d);\n",fakeinfo[i].BIATSonly);
      break;
    case SCALAR_PARAM:
      fprintf(outfile,"_fep_scalar_%d = ",i);
      gen_expr(outfile, fakeinfo[i].expr);
      fprintf(outfile,";\n");

      fprintf(outfile,"  _PromoteScalarToEnsemble(&(_fep_scalar_%d",i);
      fprintf(outfile,"),_fake_ens_param_%d,sizeof(_fep_scalar_%d",i,i);
      fprintf(outfile,"),");
      if (fakeinfo[i].reg != NULL) {
	priv_reg_access(outfile, fakeinfo[i].reg);
      } else {
	genStackReg(outfile);
      }
      fprintf(outfile,");\n");
      break;
    default:
      INT_FATAL(NULL, "BRAD -- The impossible has happened!! Wowza!");
    }
  }
  brad_no_access = FALSE;
}


static void DoStopFakePrinting(FILE* outfile) {
  fprintf(outfile,"}\n");
}


static expr_t *StickInAFake(expr_t ** old_param,int type,int uatsonly,
			    expr_t *reg,int rank) { 
  expr_t *new_expr;

  if (num_fakes == num_fakes_allocated) {
    ExpandFakeInfo();
  }
  fakeinfo[num_fakes].expr = *old_param;
  fakeinfo[num_fakes].type = type;
  fakeinfo[num_fakes].BIATSonly = uatsonly;
  fakeinfo[num_fakes].reg = reg;
  fakeinfo[num_fakes].rank = rank;
  
  new_expr = copy_expr(*old_param);
  T_TYPE(new_expr) = VARIABLE;
  T_IDENT(new_expr) = fakeinfo[num_fakes].pst;
  T_NEXT(new_expr) = T_NEXT(*old_param);
  T_TYPEINFO(new_expr) = T_TYPEINFO(*old_param);
  T_TYPEINFO_REG(new_expr) = T_TYPEINFO_REG(*old_param);
  
  *old_param = new_expr;
  num_fakes++;
  return new_expr;
}


static void PullOutAFake(expr_t ** fake_param) {
  *fake_param = fakeinfo[num_fakes].expr;
  num_fakes++;
}


static void ProcessParameters(expr_t ** exprls,symboltable_t *function) {
  expr_t *actual;
  expr_t *prev = NULL;
  genlist_t *formal;
  symboltable_t *pst;
  int expecting_ensemble;
  int sending_ensemble;
  int type;

  actual = *exprls;
  formal = T_PARAMLS(S_FUN_BODY(function));
  while (actual != NULL && formal != NULL) {
    pst = G_IDENT(formal);

    sending_ensemble = T_TYPEINFO_REG(actual) != NULL;
    expecting_ensemble = (D_CLASS(S_DTYPE(pst)) == DT_GENERIC_ENSEMBLE ||
			  (D_CLASS(S_DTYPE(pst)) == DT_ENSEMBLE));

    type = 0;
    globuatsonly = 1;

    if (sending_ensemble && expecting_ensemble && valid_expr(actual)) {

      /* the function is expecting an ensemble and we are sending it a 
	 complex one */

      type = CMPLX_ENSEMBLE;

    } else if (!sending_ensemble && expecting_ensemble) {
      
      if (T_TYPE(actual) == VARIABLE && T_IDENT(actual) == pst_NULL) {
	/* in this case, we're probably dealing with an ensemble that is
	   returning an ensemble but it's not being assigned.  No temp
	   needed. */
      } else {

	/* the function is expecting an ensemble and we are sending it a
	   scalar */
	type = SCALAR_PARAM;
      }
    }
    if (T_IDENT(actual) && S_CLASS(T_IDENT(actual)) == S_VARIABLE && !S_IS_CONSTANT(T_IDENT(actual)) && D_CLASS(S_DTYPE(T_IDENT(actual))) == DT_GENERIC_ENSEMBLE) {
      type = 0;
    }
    if (type != 0) {
      if (prev == NULL) {
	prev=StickInAFake(exprls,type,globuatsonly,D_ENS_REG(S_DTYPE(pst)),
			  D_ENS_NUM(S_DTYPE(pst)));
      } else {
	prev=StickInAFake(&(T_NEXT(prev)),type,globuatsonly,
			  D_ENS_REG(S_DTYPE(pst)),D_ENS_NUM(S_DTYPE(pst)));
      }
    } else {
      prev = actual;
    }

    actual = T_NEXT(actual);
    formal = G_NEXT(formal);
  }
  if (strncmp(S_IDENT(function), "_ZMAC_", 6) != 0 &&
      (actual != NULL || formal != NULL)) {
    INT_FATAL(NULL, "Number of formals does not equal number of actuals "
	      "in function: %s",S_IDENT(function));
  }
}


static void UndoParameters(expr_t ** exprls) {
  expr_t *actual;
  expr_t *prev=NULL;

  actual = *exprls;
  while (actual != NULL && num_fakes < num_fakes_allocated) {
    if (T_IDENT(actual) == fakeinfo[num_fakes].pst && 
	T_IDENT(actual) != NULL) {
      T_NEXT(fakeinfo[num_fakes].expr) = T_NEXT(actual);
      if (prev != NULL) {
	T_NEXT(prev) = fakeinfo[num_fakes].expr;
      } else {
	*exprls = fakeinfo[num_fakes].expr;
      }
      num_fakes++;
    }

    prev=actual;
    actual = T_NEXT(actual);
  }
}


static void StartExpr(expr_t *expr) {
  expr_t *exprls;
  expr_t *func;

  switch (T_TYPE(expr)) {
  case VARIABLE:
  case CONSTANT:
  case REDUCE:   /* assuming any functions in here will be temped out */
  case SCAN:
  case FLOOD:
  case PERMUTE:
    break;
  case ARRAY_REF:
    StartExpr(nthoperand(expr,1));
    StartExpr(nthoperand(expr,2));
    break;
  case BIDOT:
    StartExpr(T_OPLS(expr));
    break;
  case BIAT:
    StartExpr(T_OPLS(expr));
    break;
  case FUNCTION:  /* This is the meaty case we are looking for... */

    /* first, do what we've been doing recursively to find nested functions */

    exprls = nthoperand(expr,2);
    while (exprls != NULL) {
      func = SearchForFunctionCall(exprls);
      if (func != NULL) {
	StartExpr(func);
      }
      exprls = T_NEXT(exprls);
    }

    /* then process the parameters of this call to see if there are any
       cases we need to deal with */

    ProcessParameters(&(T_NEXT(T_OPLS(expr))),T_IDENT(nthoperand(expr,1)));
    break;
  default:
    if (T_IS_BINARY(T_TYPE(expr))) {
      func = SearchForFunctionCall(left_expr(expr));
      StartExpr(func);
      func = SearchForFunctionCall(right_expr(expr));
      StartExpr(func);
    } else if (T_IS_UNARY(T_TYPE(expr))) {
      func = SearchForFunctionCall(T_OPLS(expr));
      StartExpr(func);
    } else {
      INT_FATAL(T_STMT(expr), "Unexpected expression type %d in StartExpr",
		T_TYPE(expr));
    }
  }
}


void TestExprForComplexEnsParams(FILE* outfile, expr_t *expr) {
  if (num_fakes) {
    INT_FATAL(NULL, "INTERNAL ASSUMPTION ERROR:  Contact Brad about "
	      "TestExprForComplexEnsParams()");
  }
  StartExpr(expr);
  if (num_fakes) {
    DoStartFakesPrinting(outfile);
  }
}


static void FinishExpr(expr_t *expr) {
  expr_t *exprls;
  expr_t *func;

  switch (T_TYPE(expr)) {
  case VARIABLE:
  case CONSTANT:
  case REDUCE:   /* assuming any functions in here will be temped out */
  case SCAN:
  case FLOOD:
  case PERMUTE:
    break;
  case ARRAY_REF:
    FinishExpr(nthoperand(expr,1));
    FinishExpr(nthoperand(expr,2));
    break;
  case BIDOT:
    FinishExpr(T_OPLS(expr));
    break;
  case BIAT:
    FinishExpr(T_OPLS(expr));
    break;
  case FUNCTION:  /* This is the meaty case we are looking for... */
    
    /* first, do what we've been doing recursively to find nested functions */
    
    exprls = nthoperand(expr,2);
    while (exprls != NULL) {
      func = SearchForFunctionCall(exprls);
      if (func != NULL) {
	FinishExpr(func);
      }
      exprls = T_NEXT(exprls);
    }
    
    /* then process the parameters of this call to see if there are any
       cases we need to deal with */
    
    UndoParameters(&(T_NEXT(T_OPLS(expr))));
    break;
  default:
    if (T_IS_BINARY(T_TYPE(expr))) {
      func = SearchForFunctionCall(left_expr(expr));
      FinishExpr(func);
      func = SearchForFunctionCall(right_expr(expr));
      FinishExpr(func);
    } else if (T_IS_UNARY(T_TYPE(expr))) {
      func = SearchForFunctionCall(T_OPLS(expr));
      FinishExpr(func);
    } else {
      INT_FATAL(T_STMT(expr), "Unexpected expression type %d in FinishExpr",
		T_TYPE(expr));
    }
  }
}


void FinishExprComplexParams(FILE* outfile, expr_t *expr) {
  if (num_fakes) {
    num_fakes = 0;
    FinishExpr(expr);
    DoStopFakePrinting(outfile);
  }
  num_fakes = 0;
}


void TestIOStmtForComplexEnsParams(FILE* outfile, statement_t *s) {
  expr_t *exp;
  expr_t *reg;
 
  if (num_fakes) {
    INT_FATAL(s, "INTERNAL ASSUMPTION ERROR:  Contact Brad about "
	      "TestIOStmtForComplexEnsParams()");
  }
  exp = IO_EXPR1(T_IO(s));
  
  if (exp!=NULL) {
    reg = T_TYPEINFO_REG(exp);
    globuatsonly = 1;
/*    globuatsonly = (D_CLASS(T_TYPEINFO(exp)) == DT_ARRAY ||
		    D_CLASS(T_TYPEINFO(exp)) == DT_STRUCTURE);*/
    if (reg != NULL && valid_expr(exp)) {
      StickInAFake(&(IO_EXPR1(T_IO(s))),CMPLX_ENSEMBLE,globuatsonly,NULL,0);
      DoStartFakesPrinting(outfile);
    }
  }
}


void FinishIOStmt(FILE* outfile, statement_t *s) {
  if (num_fakes) {
    num_fakes = 0;
    PullOutAFake(&(IO_EXPR1(T_IO(s))));
    DoStopFakePrinting(outfile);
  }
  num_fakes=0;
}


expr_t *TestMaskStmtForComplexEnsMask(FILE* outfile, statement_t *s) {
  expr_t *exp;
  expr_t *reg;
  expr_t *retval;
  expr_t *newexpr;

  retval = NULL;
  if (num_fakes) {
    INT_FATAL(s, "INTERNAL ASSUMPTION ERROR:  Contact Brad about "
	      "TestMaskStmtForComplexEnsMask()");
  }
  exp = T_MASK_EXPR(T_REGION(s));

  if (exp!=NULL) {
    reg = T_TYPEINFO_REG(exp);
    globuatsonly = 1;
    if (reg != NULL && valid_expr(exp)) {
      StickInAFake(&(T_MASK_EXPR(T_REGION(s))),CMPLX_ENSEMBLE,globuatsonly,
		   NULL,0);
      newexpr = T_MASK_EXPR(T_REGION(s));
      DoStartFakesPrinting(outfile);
      if (num_fakes) {
	if (num_fakes != 1) {
	  INT_FATAL(s, "INTERNAL ASSUMPTION ERROR: Contact Brad about "
		    "TestMaskStmtForComplexEnsMask() (2)");
	} else {
	  num_fakes = 0;
	  PullOutAFake(&retval);
	  num_fakes = 0;
	}
      }
    }
  }
  return retval;
}


void FinishMaskStmt(FILE* outfile, statement_t *s,expr_t *savedexpr) {
  if (savedexpr != NULL) {
    DoStopFakePrinting(outfile);
    T_MASK_EXPR(T_REGION(s)) = savedexpr;
  }
}


void TestWRForComplexEns(FILE* outfile, wrap_t *wrap) {
  expr_t *exprptr;
  expr_t *prev=NULL;

  pass_null_arrayref = 1;
  if (num_fakes) {
    INT_FATAL(NULL, "ASSERTION FAILURE:  Contact Brad about "
	      "TestWRForCmplexEns()");
  }
  
  exprptr = T_OPLS(wrap);
  while (exprptr != NULL) {
    if (valid_expr(exprptr)) {
      if (prev == NULL) {
	prev = StickInAFake(&(T_OPLS(wrap)),CMPLX_ENSEMBLE,globuatsonly,NULL,0);
      } else {
	prev = StickInAFake(&(T_NEXT(prev)),CMPLX_ENSEMBLE,globuatsonly,NULL,0);
      }
    } else {
      prev = exprptr;
    }
    exprptr = T_NEXT(exprptr);
  }

  if (num_fakes) {
    DoStartFakesPrinting(outfile);
  }
  pass_null_arrayref = 0;
}
  

void FinishWRForComplexEns(FILE* outfile, wrap_t *wrap) {
  expr_t *exprptr;
  expr_t *prev=NULL;

  if (num_fakes) {
    num_fakes = 0;
    
    exprptr = T_OPLS(wrap);
    while (exprptr != NULL && num_fakes < num_fakes_allocated) {
      if (T_IDENT(exprptr) == fakeinfo[num_fakes].pst &&
	  T_IDENT(exprptr) != NULL) {
	T_NEXT(fakeinfo[num_fakes].expr) = T_NEXT(exprptr);
	if (prev != NULL) {
	  T_NEXT(prev) = fakeinfo[num_fakes].expr;
	} else {
	  T_OPLS(wrap) = fakeinfo[num_fakes].expr;
	}
	num_fakes++;
      }
      
      prev = exprptr;
      exprptr = T_NEXT(exprptr);
    }
    DoStopFakePrinting(outfile);
  }
  num_fakes = 0;
}


void TestFloodForComplexEns(FILE* outfile, expr_t *expr) {
  expr_t *exprptr;

  if (num_fakes) {
    INT_FATAL(NULL, "ASSERTION FAILURE:  Contact Brad about "
	      "TestFloodForCmplexEns()");
  }
  
  exprptr = T_OPLS(expr);
  if (exprptr != NULL) {
    if (valid_expr(exprptr)) {
      StickInAFake(&(T_OPLS(expr)),CMPLX_ENSEMBLE,globuatsonly,NULL,0);
    }
  }
  exprptr = T_OPLS(T_NEXT(T_OPLS(expr)));
  if (exprptr != NULL) {
    if (valid_expr(exprptr)) {
      StickInAFake(&(T_OPLS(T_NEXT(T_OPLS(expr)))),CMPLX_ENSEMBLE,globuatsonly,
		   NULL,0);
    }
  }

  if (T_TYPEINFO_REG(exprptr) == NULL) {  /* argument is a scalar */
    StickInAFake(&(T_OPLS(T_NEXT(T_OPLS(expr)))),SCALAR_PARAM,globuatsonly,
		 T_REGION_SYM(T_REGMASK(T_NEXT(T_OPLS(expr)))),0);
  }

  if (num_fakes) {
    DoStartFakesPrinting(outfile);
  }
}
  

void FinishFloodForComplexEns(FILE* outfile, expr_t *expr) {
  expr_t *exprptr;

  if (num_fakes) {
    num_fakes = 0;
    
    exprptr = T_OPLS(expr);
    if (T_IDENT(exprptr) == fakeinfo[num_fakes].pst &&
	T_IDENT(exprptr) != NULL) {
      T_NEXT(fakeinfo[num_fakes].expr) = T_NEXT(exprptr);
      T_OPLS(expr) = fakeinfo[num_fakes].expr;
      num_fakes++;
    }

    if (num_fakes < num_fakes_allocated) {
      exprptr = T_OPLS(T_NEXT(T_OPLS(expr)));
      if (T_IDENT(exprptr) == fakeinfo[num_fakes].pst &&
	  T_IDENT(exprptr) != NULL) {
	T_NEXT(fakeinfo[num_fakes].expr) = T_NEXT(exprptr);
	T_OPLS(T_NEXT(T_OPLS(expr))) = fakeinfo[num_fakes].expr;
	num_fakes++;
      }
    }
    
    DoStopFakePrinting(outfile);
  }
  
  num_fakes = 0;
}
