/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/db.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/statement.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"
#include "../include/buildzplstmt.h"

/*
 *  This is an ORDERED enumeration
 *  i.e. the order matters
 */
typedef enum {
   lboolean=1,
   lchar,
   luchar, /* unsigned char */
   lshortint,
   lushortint, /* unsigned short */
   lint,
   luint,	/* unsigned int */
   llongint,
   lulongint,	/* unsigned long */
   lfloat,
   ldouble,
   lquad,
   lcomplex,
   ldcomplex,
   lqcomplex,
   lstruct,
   lensemble,
   larray
} conv_type;


/* typeinfo global */

static int unbound_index;
static datatype_t *param_ens=NULL;


/* typeinfo function prototypes */

static void typeinfo_stmtls(statement_t *);
static void typeinfo_repls (replacement_t *);


static datatype_t* compute_prep_reg_type(statement_t* stmt,
					 regionclass regtype, 
					 datatype_t* regdt, 
					 datatype_t* dirdt) {
  int numdims;
  dimtypeclass newregtype[MAXRANK];
  int i;
  dimtypeclass regdimtype;
  signclass dirdimtype;
  int regtypesame;

  if (D_CLASS(regdt) != DT_REGION || D_CLASS(dirdt) != DT_DIRECTION) {
    return NULL;
  }
  numdims = D_REG_NUM(regdt);
  if (D_DIR_NUM(dirdt) != numdims) {
    return NULL;
  }
  for (i=0; i<MAXRANK; i++) {
    regdimtype = D_REG_DIM_TYPE(regdt, i);
    dirdimtype = D_DIR_SIGN(dirdt, i);

    if (regdimtype == DIM_INHERIT) {
      regdimtype = RMSCurrentDimType(i);
    }

    switch (regtype) {
    case OF_REGION:
    case IN_REGION:
      switch (regdimtype) {
      case DIM_DYNAMIC:
      case DIM_SPARSE:
	newregtype[i] = regdimtype;
	break;
      case DIM_FLOOD:
      case DIM_GRID:
	switch (dirdimtype) {
	case SIGN_ZERO:
	case SIGN_UNKNOWN:
	  newregtype[i] = regdimtype;
	  break;
	case SIGN_POS:
	case SIGN_NEG:
	case SIGN_POSONE:
	case SIGN_NEGONE:
	  USR_FATAL(stmt, 
		    "Cannot apply non-zero direction component to flood/grid dimensions\n"
		    "using of/in region operators");
	}
	break;
      case DIM_INHERIT:
      case DIM_RANGE:
      case DIM_FLAT:
	switch (dirdimtype) {
	case SIGN_ZERO:
	case SIGN_UNKNOWN:
	  newregtype[i] = regdimtype;
	  break;
	case SIGN_POS:
	case SIGN_NEG:
	  newregtype[i] = DIM_RANGE;
	  break;
	case SIGN_POSONE:
	case SIGN_NEGONE:
	  newregtype[i] = DIM_FLAT;
	  break;
	}
	break;
      }
      break;
    case BY_REGION:
    case AT_REGION:
      newregtype[i] = regdimtype;
      break;
    default:
      return NULL;
    }
  }

  regtypesame = 1;
  for (i=0; i<numdims; i++) {
    if (newregtype[i] != D_REG_DIM_TYPE(regdt, i)) {
      regtypesame = 0;
    }
  }
  if (!regtypesame) {
    regdt = alloc_dt(DT_REGION);
    for (i=0; i<numdims; i++) {
      D_REG_DIM_TYPE(regdt, i) = newregtype[i];
    }
    D_REG_NUM(regdt) = numdims;
  }
  return regdt;
}


static datatype_t* compute_with_reg_type(statement_t* stmt, regionclass with,
					 expr_t* expr) {
  expr_t* basereg;
  expr_t* mask;
  int numdims;
  datatype_t* regdt;
  int i;

  basereg = T_OPLS(expr);
  mask = T_NEXT(T_OPLS(expr));
  numdims = D_REG_NUM(T_TYPEINFO(basereg));
  if (numdims == 0 && T_TYPEINFO_REG(mask)) {
    numdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(mask)));
  }
  switch (with) {
  case WITH_REGION:
  case WITHOUT_REGION:
    regdt = T_TYPEINFO(basereg);
  case SPARSE_WITH_REGION:
  case SPARSE_WITHOUT_REGION:
    regdt = alloc_dt(DT_REGION);
    for (i=0; i<numdims; i++) {
      D_REG_DIM_TYPE(regdt, i) = DIM_SPARSE;
    }
    D_REG_NUM(regdt) = numdims;
    break;
  default:
    INT_FATAL(NULL, "Unexpected region type in compute_with_reg_type");
  }
  return regdt;
}


static datatype_t *get_subinfo(datatype_t *pdt) {

  if (pdt == NULL) {
    return 0;
  }
  switch (D_CLASS(pdt)) {
  case DT_ARRAY:
    pdt = D_ARR_TYPE(pdt);
    break;
  case DT_ENSEMBLE:                /* found the ensemble */
    pdt = D_ENS_TYPE(pdt);
    break;
  case DT_STRUCTURE:
    pdt = S_DTYPE(D_STRUCT(pdt));
    break;
  default:
    return 0;
  }
  return pdt;
}


static datatype_t *convertBinary(datatype_t* lhs,datatype_t* rhs,exprtype op) {
  datatype_t* current;
  int i;
  conv_type first_type = (conv_type)-1;
  conv_type second_type = (conv_type)-1;
  
  /* if same type */
  if ((lhs == NULL)) {
    return rhs;
  } else if ((rhs == NULL)) {
    return lhs;
  }
  /* guaranteed not null in this case */
  if (D_CLASS(lhs) == D_CLASS(rhs)) {
    return lhs; /* arbitrary choice here */
  }
  current = lhs;
  for (i=0; i<2; i++) {
    switch (D_CLASS(current)) {
    case DT_ENUM:
      if (first_type == -1) {
	first_type = lint;
      }
      else {
	second_type = lint;
      }
      break;
    case DT_UNSIGNED_INT:
      if (first_type == -1) {
	first_type = luint;
      } else if (first_type == lint) {
	second_type = lint;
      } else {
	second_type = luint;
      }
      break;
    case DT_INTEGER:
      if (first_type == -1) {
	first_type = lint;
      } else {
	second_type = lint;
      }
      break;
    case DT_UNSIGNED_LONG:
      if (first_type == -1) {
	first_type = lulongint;
      } else if (first_type == llongint) {
	second_type = llongint;
      } else {
	second_type = lulongint;
      }
      break;
    case DT_LONG:
      if (first_type == -1) {
	first_type = llongint;
      } else {
	second_type = llongint;
      }
      break;
    case DT_UNSIGNED_SHORT:
      if (first_type == -1) {
	first_type = lushortint;
      } else if (first_type == lshortint) {
	second_type = lshortint;
      } else {
	second_type = lushortint;
      }
      break;
    case DT_SHORT:
      if (first_type == -1) {
	first_type = lshortint;
      } else {
	second_type = lshortint;
      }
      break;
    case DT_REAL:
      if (first_type == -1) {
	first_type = lfloat;
      } else {
	second_type = lfloat;
      }
      break;
    case DT_DOUBLE:
      if (first_type == -1) {
	first_type = ldouble;
      } else {
	second_type = ldouble;
      }
      break;
    case DT_QUAD:
      if (first_type == -1) {
	first_type = lquad;
      } else {
	second_type = lquad;
      }
      break;
    case DT_UNSIGNED_BYTE:
      if (first_type == -1) {
	first_type = luchar;
      } else if (first_type == lchar) {
	second_type = lchar;
      } else  {
	second_type = luchar;
      }
      break;
    case DT_BOOLEAN:
      if (first_type == -1) {
	first_type = lboolean;
      } else {
	second_type = lboolean;
      }
      break;
    case DT_SIGNED_BYTE:
    case DT_CHAR:
      if (first_type == -1) {
	first_type = lchar;
      } else {
	second_type = lchar;
      }
      break;
    case DT_COMPLEX:
      if (first_type == -1) {
	first_type = lcomplex;
      } else {
	second_type = lcomplex;
      }
      break;
    case DT_DCOMPLEX:
      if (first_type == -1) {
	first_type = ldcomplex;
      } else {
	second_type = ldcomplex;
      }
      break;
    case DT_QCOMPLEX:
      if (first_type == -1) {
	first_type = lqcomplex;
      } else {
	second_type = lqcomplex;
      }
      break;
    case DT_REGION:
    case DT_ARRAY:
      if (current == lhs) {
	/* check if rhs is the same and its subtype takes precedence
	   or if rhs is an ensemble, then it takes precedence.
	   */
	if ((rhs != NULL) && (D_CLASS(rhs) == DT_ENSEMBLE)) {
	  return rhs;
	} else {
	  return lhs;
	}
      } else {
	return current;
      }
    case DT_ENSEMBLE: /* regardless of other type, this wins out for now */
      return current;
    case DT_STRUCTURE:
      return current;
    case DT_VOID:
      if (current == lhs) {
	return rhs;
      } else {
	return lhs;
      }
    default:
      if (current == lhs) {
	return rhs;
      } else {
	return lhs;
      }
    }
    current = rhs;
  }
  if (first_type > second_type) {
    return lhs;
  } else {
    return rhs;
  }
}


static void typeinfo_exprls(expr_t *exprls) {
  for (; exprls != NULL; exprls = T_NEXT(exprls)) {
    typeinfo_expr(exprls);
  }
}


static datatype_t *param_is_ensemble_type(datatype_t *dt) {
  switch (D_CLASS(dt)) {
  case DT_ARRAY:
    return param_is_ensemble_type(D_ARR_TYPE(dt));
  case DT_GENERIC_ENSEMBLE:
  case DT_ENSEMBLE:
  case DT_REGION:
    return dt;
  default:
    return NULL;
  }
}


static datatype_t *getensemble_subtype(datatype_t *dt) {
  if (dt == NULL) {
    return dt;
  }
  if (D_CLASS(dt) != DT_ENSEMBLE) {
    return dt;
  }
  return D_ENS_TYPE(dt);
}

datatype_t *typeinfo_expr(expr_t *expr) {
  datatype_t *dt_ptr;
  expr_t *reg;
  int indexirank;
  region_t *scope;
  
  if (expr == NULL) {
    return NULL;
  }
  
  switch(T_TYPE(expr)) {
  case VARIABLE:
  case CONSTANT:
    {
      symboltable_t *pst;
      
      pst = T_IDENT(expr);
      if (pst == NULL) {
	INT_FATAL(NULL,"NULL symboltable pointer in VARIABLE expression");
      }
      if (T_IDENT(expr) == NULL) {
	return T_TYPEINFO(expr);
      }
      dt_ptr = S_DTYPE(T_IDENT(expr));
      
      T_TYPEINFO(expr) = getensemble_subtype(dt_ptr);
      if (dt_ptr && (D_CLASS(dt_ptr) == DT_ENSEMBLE)) {
	T_TYPEINFO_REG(expr) = D_ENS_REG(dt_ptr);
      }
      else {
	T_TYPEINFO_REG(expr) = NULL;
      }

      /* regions shouldn't store themselves as TYPEINFO_REG, rather TYPEINFO
	 else if (dt_ptr && (D_CLASS(dt_ptr) == DT_REGION)) {
	T_TYPEINFO_REG(expr) = expr;
      }
      */
      indexirank = symtab_is_indexi(pst);
      if (indexirank) {
	reg = NULL;
	if (param_ens != NULL) {
	  reg = D_ENS_REG(param_ens);
	}
	if (reg == NULL) {
	  reg = RMSCurrentRegion();
	}
	if (reg == NULL) {
	  reg = stmt_find_reg(T_STMT(expr));
	}
	if (reg == NULL) {
	  unbound_index = indexirank;
	}
	if (reg != NULL) {
	  T_TYPEINFO_REG(expr) = reg;
	}
      }
      return T_TYPEINFO(expr);
    }

    /* unary operators */
  case BIAT:
    typeinfo_expr(T_NEXT(T_OPLS(expr)));
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:

    T_TYPEINFO(expr) = getensemble_subtype(typeinfo_expr(T_OPLS(expr)));
    /* active*/

    if (T_TYPE(expr) == BIAT && T_SUBTYPE(expr) == AT_RANDACC) {
      T_TYPEINFO_REG(expr) = NULL;
    }
    else {
      T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));
    }
    return T_TYPEINFO(expr);

    /* scan/reduce */
  case REDUCE:
    {
      if (D_CLASS(typeinfo_expr(T_OPLS(expr))) == DT_ARRAY) {
	T_SET1(T_STMT(expr));
      }
      scope = T_REGMASK(expr);
      if (scope) {
	RMSPushScope(scope);
      }
      if (T_SUBTYPE(expr) == USER) {
	if (T_PARENT(expr) && T_IS_ASSIGNOP(T_TYPE(T_PARENT(expr))) && T_PREV(expr)) {
	  T_TYPEINFO(expr) = typeinfo_expr(T_PREV(expr));
	}
      }
      else {
	/* I assume this will resolve to the basic type that we 
	   are reducing.  Therefore, the ENSEMBLE field should
	   propagate upward appropriately.
	*/
	/* NOT::: Reduction clears the ensemble field */
	/* That's not right? SD 10/8/03 */
	T_TYPEINFO(expr) = getensemble_subtype(typeinfo_expr(T_OPLS(expr)));
      }
      /*T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));*/
      if (T_REGMASK(expr) && T_REGION_SYM(T_REGMASK(expr))) {
	T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr)); /* wrong; placeholder */
      } else {
	T_TYPEINFO_REG(expr) = NULL;
      }
      if (scope) {
	RMSPopScope(scope);
      }
      return T_TYPEINFO(expr);
    }
    
  case SCAN:
    {
      /* scans will always return ensembles */
      T_TYPEINFO(expr) = getensemble_subtype(typeinfo_expr(T_OPLS(expr)));
      /* active*/
      T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));
      return T_TYPEINFO(expr);
    }
    
  case FLOOD:
    scope = T_REGMASK(expr);
    if (scope) {
      RMSPushScope(scope);
    }
    /* BLC -- this is incorrect, but is serving as a placeholder */
    /* floods will return ensembles of different shapes */
    T_TYPEINFO(expr) = getensemble_subtype(typeinfo_expr(T_OPLS(expr)));
    /* active */
    T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));
    if (scope) {
      RMSPopScope(scope);
    }
    return T_TYPEINFO(expr);
  case PERMUTE:
    {
      expr_t *map;
      
      map = T_MAP(expr);
      while (map != NULL) {
	typeinfo_expr(map);
	map = T_NEXT(map);
      }
      T_TYPEINFO(expr) = getensemble_subtype(typeinfo_expr(T_OPLS(expr)));
      T_TYPEINFO_REG(expr) = RMSCurrentRegion();
      return T_TYPEINFO(expr);
    }

    /* n-ary operators */
  case ARRAY_REF:
    if (T_SUBTYPE(expr) == ARRAY_IND) {
      expr_t *original_expr = expr;
      datatype_t *arrdt;
      datatype_t *basedt;
      
      arrdt = typeinfo_expr(T_OPLS(expr));
      basedt = get_subinfo(arrdt);
      /* NULL array reference */
      if (expr_null_arrayref(expr)) { 
	T_TYPEINFO(expr) = arrdt;
      } else if (basedt == NULL) {  /* overindexing an array, e.g. */
	T_TYPEINFO(expr) = getensemble_subtype(arrdt);
      } else {
	T_TYPEINFO(expr) = getensemble_subtype(basedt);
      }

      /* The next_p pointer of the expression from the array
	 reference is possibly part of an expression that
	 is nexted within the array ref.  For example,
	 B[A[i]] is such a case.  T_OPLS(expr) = A, 
	 T_NEXT(") = ARRAY_REF i.  Therefore, we need to 
	 recursively test the next fields 
	 */
      
      {
	expr_t *texpr = T_NEXT(T_OPLS(expr));
	expr_t *typeinfo_reg = NULL; /*** reg typeinfo ***/
	
	while (texpr) {
	  T_TYPEINFO(texpr) = typeinfo_expr(texpr);
	  if (T_TYPEINFO(texpr)) {
	    if (D_CLASS(T_TYPEINFO(texpr)) == DT_ENSEMBLE) {
	      T_TYPEINFO_REG(texpr) = D_ENS_REG(T_TYPEINFO(texpr));
	      T_TYPEINFO(texpr) = getensemble_subtype(T_TYPEINFO(texpr));
	    }
	    if (typeinfo_reg == NULL) {
	      /*** sungeun *** for now, save the first one found ***
	       *** NOTE: THIS SHOULD CHANGE IF WE DECIDED
	       ***       TO MAKE TYPEINFO_REG() MORE ACCURATE ***/
	      typeinfo_reg = T_TYPEINFO_REG(texpr);
	    }
	  } else {
	    T_TYPEINFO_REG(texpr) = NULL;
	  }
	  texpr = T_NEXT(texpr);
	}

	/* array ref one down*/
	/* active*/
	if (basedt && (D_CLASS(basedt) == DT_ENSEMBLE)) {
	  T_TYPEINFO_REG(expr) = D_ENS_REG(basedt);
	} else if (typeinfo_reg) {
	  T_TYPEINFO_REG(expr) = typeinfo_reg;
	} else {
	  T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));
	}

      }

      expr = original_expr;
      /* Process the parameters to the function */
      
    } else {
      expr_t *arrexpr;
      datatype_t *pdt;

      typeinfo_expr(T_OPLS(expr));
      arrexpr = T_OPLS(expr);
      T_TYPEINFO(expr) = T_TYPEINFO(arrexpr);
      pdt = S_DTYPE(T_IDENT(arrexpr));
      if (pdt && D_CLASS(pdt) == DT_ENSEMBLE) {
	T_TYPEINFO_REG(expr) = D_ENS_REG(pdt);
      }
    }
    return T_TYPEINFO(expr);
    
  case FUNCTION:
    {
      datatype_t *retdt;
      
      /* This is how it should be, but datatype NULL on function */
      retdt = NULL;
      if ((T_OPLS(expr) != NULL) && (T_IDENT(T_OPLS(expr))) != NULL) {
	retdt = S_FUN_TYPE(T_IDENT(T_OPLS(expr)));
      }
      /* type of function is return type */
      if (retdt != NULL) {
	T_TYPEINFO(expr) = getensemble_subtype(retdt);
	if (S_CLASS(retdt) == DT_ENSEMBLE) {
	  T_TYPEINFO_REG(expr) = D_ENS_REG(retdt);
	}
      }

      /* Process the parameters to the function */
      {
	expr_t *actuals;
	int numops = numoperands(expr);
	symboltable_t *formals;
	int promoted_function=0;
	int indexirank;
	int saveindexirank=0;
	datatype_t *formdt;
	datatype_t *save_param_ens;
	
	if (numops > 1) {  /* This is a function with parameters. */
	  formals = T_DECL(D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))));
	  for (actuals = nthoperand(expr, 2);
	       ((actuals != NULL) && (formals != NULL)); 
	       actuals = T_NEXT(actuals)) {
	    formdt = S_DTYPE(formals);
	    
	    save_param_ens = param_ens;
	    param_ens = NULL;
	    /* this code seems a bit fragile with -O on alphas.
	       Without the preceding line, the assignment to param_ens
	       seems to disappear, causing erroneous error messages to
	       be printed */
	    if (D_CLASS(formdt) == DT_ENSEMBLE) {
	      param_ens = formdt;
	    }
	    typeinfo_expr(actuals);
	    param_ens = save_param_ens;

	    /* if return type of function was generic, let's see if we
	       can guess the return type by using an actual parameter */
	    if ((S_CLASS(retdt) == DT_GENERIC) && 
		(S_CLASS(formdt) == DT_GENERIC)) {
	      retdt = T_TYPEINFO(actuals);
	      T_TYPEINFO(expr) = getensemble_subtype(retdt);
	    }

	    /*check if function is promoted and set ENSEMBLE field*/
	    indexirank = expr_contains_indexi(actuals);
	    if (!saveindexirank && indexirank) {
	      saveindexirank = indexirank;
	    }
	    if ((T_TYPEINFO_REG(actuals) != NULL || indexirank) &&
		param_is_ensemble_type(formdt) == NULL) {
	      DBS1(45, "PARM IS ENSEMBLE, make function promoted: %s\n", 
		   S_IDENT(T_IDENT(T_OPLS(expr))));
	      T_SET_FLAG(T_PROMOTED_FUNC(expr));
	      promoted_function=1;
	      if (T_TYPEINFO_REG(actuals) != NULL) {
		T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(actuals);
	      }
	    }
	    formals = S_SIBLING(formals);
	  }
	  if (promoted_function && saveindexirank && T_TYPEINFO_REG(expr) == NULL) {
	    /* function was promoted using Indexi only */
	    
	    reg = stmt_find_reg(T_STMT(expr));
	    if (reg == NULL) {
	      unbound_index = indexirank;
	    } else {
	      T_TYPEINFO_REG(expr) = RMSCurrentRegion();
	    }
	  }
	}
      }
      return T_TYPEINFO(expr);
    }
    
    /* binary operators */
    
  case BIASSIGNMENT:
    {
      /* get type information from left hand side. */
      T_TYPEINFO(expr) = getensemble_subtype(typeinfo_expr(T_OPLS(expr)));
      /* active*/
      T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));
      (void *)typeinfo_expr(T_NEXT(T_OPLS(expr)));
      return T_TYPEINFO(expr);
    }
    
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
    /*
     *  Typeinfo for a comparitor is boolean, no matter what
     *  the types of the operands are 
     */
    T_TYPEINFO(expr) = pdtBOOLEAN;
    
    /*
     *  Generate typeinfo for LHS and RHS
     */
    typeinfo_expr(T_OPLS(expr));
    typeinfo_expr(T_NEXT(T_OPLS(expr)));
    
    /* the ENSEMBLE field is set to whichever of its children's
       ENSEMBLE fields is non-null;  if they are both non-null,
       it is set to the first; it is set to null otherwise (echris) */
    
    if (T_OPLS(expr) && T_TYPEINFO_REG(T_OPLS(expr))) {
      T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));
    } else if (T_OPLS(expr) && T_NEXT(T_OPLS(expr))) {
      T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_NEXT(T_OPLS(expr)));
    }    

    return T_TYPEINFO(expr);

  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:

  case BIOP_GETS:
    
  case BILOG_AND:
  case BILOG_OR:

  case BIEXP:

    {
      /* the ENSEMBLE field is set to whichever of its children's
	 ENSEMBLE fields is non-null;  if they are both non-null,
	 it is set to the first; it is set to null otherwise (echris) */
      
      T_TYPEINFO(expr) = getensemble_subtype(convertBinary(typeinfo_expr(T_OPLS(expr)),
							   typeinfo_expr(T_NEXT(T_OPLS(expr))),
							   T_TYPE(expr)));

      if (T_OPLS(expr) && T_TYPEINFO_REG(T_OPLS(expr)))
	T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));
      else if (T_OPLS(expr) &&
	       T_NEXT(T_OPLS(expr)))
	T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_NEXT(T_OPLS(expr)));
      
      return T_TYPEINFO(expr);
    }
    
  case BIDOT:
    {
      /* special case binary operator */
      /* handle right side of BIDOT */
      
      /* This will be the result from this BIDOT */
      /* I expect this to never be null */
      T_TYPEINFO(expr) = getensemble_subtype(S_DTYPE(T_IDENT(expr)));
      if (D_CLASS(S_DTYPE(T_IDENT(expr))) == DT_ENSEMBLE)
	{
	  /* active*/
	  T_TYPEINFO_REG(expr) = D_ENS_REG(S_DTYPE(T_IDENT(expr)));
	}
      /* left hand side, I simply ignore the results at this level
	 but I must traverse down to update the subtree fields */
      
      /* check left side of BIDOT   A.r, where A is left side */
      /* The result is unimportant*/
      (void *)typeinfo_expr((expr_t*)T_OPLS(expr));
      if (D_CLASS(S_DTYPE(T_IDENT(expr))) != DT_ENSEMBLE) {
	T_TYPEINFO_REG(expr) = T_TYPEINFO_REG(T_OPLS(expr));
      }
      return T_TYPEINFO(expr);
    }

  case SBE:
    {
      break;
    }

  case BIPREP:
    {
      datatype_t* regdt = typeinfo_expr(T_OPLS(expr));
      datatype_t* dirdt = typeinfo_expr(T_NEXT(T_OPLS(expr)));
      T_TYPEINFO(expr) = compute_prep_reg_type(T_STMT(expr), T_SUBTYPE(expr), 
					       regdt, dirdt);
      
      return T_TYPEINFO(expr);
      break;
    }

  case BIWITH:
    typeinfo_expr(T_NEXT(T_OPLS(expr)));
    typeinfo_expr(T_OPLS(expr));
    T_TYPEINFO(expr) = compute_with_reg_type(T_STMT(expr), T_SUBTYPE(expr),
					     expr);
    return T_TYPEINFO(expr);
    break;

  default:
    break;
  }
  return NULL;
}


static void typeinfo_if(if_t *ifstmt) {
  if (ifstmt == NULL) {
    INT_FATAL(NULL, "Null ifstmt in typeinfo_if()");
  }
  
  typeinfo_expr(T_IFCOND(ifstmt));
  typeinfo_stmtls(T_THEN(ifstmt));
  typeinfo_stmtls(T_ELSE(ifstmt));
}


static void typeinfo_loop(loop_t *loop) {
  if (loop == NULL) {
    INT_FATAL(NULL, "Null loop in typeinfo_loop()");
  }
  
  switch (T_TYPE(loop)) {
  case L_DO:
    typeinfo_expr(T_IVAR(loop));
    typeinfo_expr(T_START(loop));
    typeinfo_expr(T_STOP(loop));
    typeinfo_expr(T_STEP(loop));
    
    break;
  case L_WHILE_DO:
  case L_REPEAT_UNTIL:
    typeinfo_expr(T_LOOPCOND(loop));
    break;
  default:
    INT_FATAL(NULL, "bad type (%d) in typeinfo_loop",T_TYPE(loop));
  }
  typeinfo_stmtls(T_BODY(loop));
}


static void typeinfo_stmt_guts(statement_t *stmt,int depth) {
  expr_t *e;
  comm_info_t *c;
  int i;
  
  if (stmt == NULL) {
    return;
  }
  unbound_index = 0;
  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_CONTINUE:
  case S_ZPROF: /* MDD */
    break;
  case S_HALT:
    typeinfo_stmtls(T_HALT(stmt));
    break;
  case S_COMM:
    for (c = T_COMM_INFO(stmt); c!= NULL; c = T_COMM_INFO_NEXT(c))
      typeinfo_expr(T_COMM_INFO_ENS(c));
    break;
  case S_IO:
    typeinfo_expr(IO_EXPR1(T_IO(stmt)));
    if (IO_FILE(T_IO(stmt)))
      typeinfo_expr(IO_FILE(T_IO(stmt)));
    break;
  case S_WRAP:
  case S_REFLECT:	
    typeinfo_exprls(T_OPLS(T_WRAP(stmt)));
    break;
  case S_MLOOP:
    typeinfo_stmtls(T_BODY(T_MLOOP(stmt)));
    typeinfo_repls (T_MLOOP_REPLS (T_MLOOP (stmt)));
    for (i = -1; i < T_MLOOP_RANK (T_MLOOP (stmt)); i++) {
      typeinfo_stmtls (T_MLOOP_PREPRE (T_MLOOP (stmt), i));
      typeinfo_stmtls (T_MLOOP_PRE (T_MLOOP (stmt), i));
      typeinfo_stmtls (T_MLOOP_POST (T_MLOOP (stmt), i));
      typeinfo_stmtls (T_MLOOP_POSTPOST (T_MLOOP (stmt), i));
    }
    break;
  case S_NLOOP:
    typeinfo_stmtls(T_NLOOP_BODY(T_NLOOP(stmt)));
    break;
  case S_REGION_SCOPE:
    typeinfo_expr(T_REGION_SYM(T_REGION(stmt)));
    e = T_MASK_EXPR(T_REGION(stmt));
    if (e) {
      typeinfo_expr(T_MASK_EXPR(T_REGION(stmt)));
    }
    RMSPushScope(T_REGION(stmt));
    typeinfo_stmtls(T_BODY(T_REGION(stmt)));
    RMSPopScope(T_REGION(stmt));
    break;
  case S_COMPOUND:
  case S_MSCAN:
    typeinfo_stmtls(T_CMPD_STLS(stmt));
    break;
  case S_EXPR:
    typeinfo_expr(T_EXPR(stmt));
    break;
  case S_IF:
    typeinfo_if(T_IF(stmt));
    break;
  case S_LOOP:
    typeinfo_loop(T_LOOP(stmt));
    break;
  case S_RETURN:
    typeinfo_expr(T_RETURN(stmt));
    break;
  default:
    INT_FATAL(stmt, "Bad statement type (%d) in typeinfo_stmt()",T_TYPE(stmt));
  }
  if (unbound_index) {
    if (depth == 0) { 
      /* make one more pass over statement to bind all Index */
      typeinfo_stmt_guts(stmt,1);
    } else {
      if (ran_a2nloops) {
	/* these can sometimes crop up before we've resolved blank array
	   refs, and should be ignored until they are resolved */
	USR_FATAL(stmt,"Unable to infer size of Index%d",unbound_index);
      }
    }
  }
}


void typeinfo_stmt(statement_t *stmt) {
  typeinfo_stmt_guts(stmt,0);
}


static void typeinfo_stmtls(statement_t *stmtls) {
  while (stmtls != NULL) {
    typeinfo_stmt(stmtls);
    stmtls = T_NEXT(stmtls);
  }
}

/*
 * typeinfo pass on replacement list
 */
static void typeinfo_repls (replacement_t* repl) {
  if (repl != NULL) {
    typeinfo_expr (repl->origexpr);
    typeinfo_expr (repl->newexpr);
    typeinfo_repls (repl->next);
  }
}

static void get_typeinfo_fcn(function_t *fcn) {
  typeinfo_stmtls(T_STLS(fcn));
}


static void typeinfo(module_t *module) {
  function_t	*ftp;
  
  DB0(45,"typeinfo()\n");

  for (; module != NULL; module = T_NEXT(module)) {
    ftp = T_FCNLS(module);
    if (ftp == NULL) {
      INT_FATAL(NULL, "No functions in module in typeinfo()");
    }
    while (ftp != NULL) {
      /*T_MODULE(ftp) = module;*/
      get_typeinfo_fcn(ftp);
      ftp = T_NEXT(ftp);
    }
  }
}


int call_typeinfo(module_t *mod, char *s) {
  RMSInit();
  typeinfo(mod);
  RMSFinalize();

  return 0;
}
