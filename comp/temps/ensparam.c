/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/createvar.h"
#include "../include/datatype.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/glist.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"

static int expr_needs_tmp(expr_t * actual) {
  int retVal=1;

  if (actual == NULL) {
    return 0;
  }
  
  switch(T_TYPE(actual)) {
  case CONSTANT:
  case VARIABLE:
  case BIAT:
  case BIDOT:
  case ARRAY_REF:
  case NULLEXPR:

  case BIOP_GETS:

  case BIASSIGNMENT:
    
    retVal = 0;
    break;

  case FUNCTION:
    
  case REDUCE:

  case SCAN:

  case FLOOD:      /* BLC -- assuming these will work the same */

  case UNEGATIVE:
  case UPOSITIVE:

  case UCOMPLEMENT:

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
    
  default:
    retVal = 1;
    break;
  }

  return retVal;
}


/**
 **  Merged functions from param_temps below this mark
 **  These functions should handle traversing a module
 **  and placing parameter temps where necessary
 **  The major modifications made where to remove unecessary sections of
 **  code which were not needed.  
 **/


/*
 *  The parameters to make copies of are checking in ensparam_expr
 *  The parameters to make copies of a passed via a global variable,
 *    all_tmp_params
 */


static statement_t *build_temp_assign(expr_t *lhsexpr,expr_t *rhsexpr,
				      datatype_t *formaldt,int lineno,
				      char *filename) {
  expr_t * new_expr;
  statement_t *new_stmt;
  int isliteral;

  switch (D_CLASS(formaldt)) {
  case DT_ARRAY:
    new_stmt = build_temp_assign(lhsexpr,rhsexpr,D_ARR_TYPE(formaldt),lineno,
				 filename);

    break;
  case DT_ENSEMBLE:
    new_stmt = build_temp_assign(lhsexpr,rhsexpr,D_ENS_TYPE(formaldt),lineno,
				 filename);

    if (new_stmt && T_SUBTYPE(D_ENS_REG(formaldt)) != INTERNAL_REGION &&
	!expr_is_qreg(D_ENS_REG(formaldt))) {
      new_stmt = build_reg_mask_scope(D_ENS_REG(formaldt), NULL, MASK_NONE,
				      new_stmt, lineno, filename);
    }
    break;
  case DT_STRUCTURE:  /* should eventually do something smart here */
  default:
    isliteral = 0;
    if (T_IDENT(rhsexpr) != NULL) {
      isliteral = S_CLASS(T_IDENT(rhsexpr)) == S_LITERAL;
    }
    if (!typecheck_compat(T_TYPEINFO(lhsexpr),T_TYPEINFO(rhsexpr),isliteral,
			  strict_check,1)) {
      /* This would report an error in typechecking, so we leave it a param */
      return NULL;
    }
    new_expr = build_binary_op(BIASSIGNMENT,rhsexpr,lhsexpr);
    new_stmt = build_expr_statement(new_expr, lineno, filename);
  }

  if (new_stmt) {
    T_LINENO(new_stmt) = lineno;
  }
  return new_stmt;
}


/******************************************************************
 insert_param_temps(e, dt)
 Ruth Anderson 3-5-94
  
  Create temps and assignment statements for parameters to function
  calls.
  
  ASSUMPTIONS:  
  Assumes that the exact region associated with a formal parameter
  is known at compile time.  In reality we will may only know the  
  rank of the region.  This being the case, we will have to malloc space for
  the temp at compile time, or use the brackets trick or something other
  than just inserting the temp into the symbol table. 
  
  WHAT THIS ROUTINE DOES:
  create a name for the temp - access the global temp counter
  FIND OUT WHAT TYPE THE TEMP IS - needs derricks info to do this.
  insert the temp into the symbol table
  modify the current statement to include reference to new temp
  create and add assignment statements for the temp to the AST
 
  INPUT:
  func_call(parm);
  
  OUTPUT:
  param_type    new_temp;
  
  new_temp = parm;
  func_call(new_temp);


  TO DO: use derrick's type info to create temp.  (i.e. we could
  be calling a scalar function with an ensemble.)

expr_t       *e;   : expression to be passed as an actual parameter.
datatype_t   *dt;  : datatype of the formal parameter this expression is for.
symboltable_t*fpst : symboltable pointer for the formal

  ******************************************************************/

static void insert_param_temps(expr_t *expr,datatype_t *dt,
			       symboltable_t *fpst) {
  statement_t *new_stmt;                /* Assignment statement for new temp */
  statement_t *insertion_pt;		/* insertion point */
  char  temp[20];
  symboltable_t  *new;
  expr_t *new_expr, *old_expr;
  expr_t *new_expr_lhs;
  int i;
  genlist_t *newgls;
  int tmpnumdims;
  int orignumdims;
  datatype_t* lhspdt;
  datatype_t* rhspdt;
  expr_t* temprhsexpr;
  expr_t* parent;
  int loop;
  
  /**************************************************************
    Create a temp as a local variable and place it in the symbol table. 
    Access the global temp counter for this:

    New Declaration:
    dt    new_temp;
    *************************************************************/
  sprintf(temp,"_paramtemp%d", GlobalTempCounter++);   

  if (REPORT(REPORT_TEMPS)) {
    printf("Inserting paramtemp\n");
  }

  DBS1(1, "\n Inserting %s in ensparam\n", temp);

  INT_COND_FATAL((T_STMT(expr)!=NULL), T_STMT(expr), "T_STMT(expr) was NULL");
  if (!datatype_find_ensemble(dt) && T_TYPEINFO_REG(expr)) {
    /* this is a promoted call, so we pretend that the parameter is an
       array, hoping that contraction will contract it.  This is easier
       than trying to insert a scalar temp and getting it inserted into
       the MLOOP properly */

    /* We should do the similar thing for function calls promoted by 
       array arguments as in test_array_param_promoted1a and 3a.z 
         -BLC 12/14/00
    */

    tmpnumdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr)));
    dt = build_ensemble_type(dt,buildqregexpr(tmpnumdims),0,1,T_LINENO(T_STMT(expr)),T_FILENAME(T_STMT(expr)));
  } else {
    tmpnumdims = datatype_rank(dt);
  }
  if (T_TYPEINFO_REG(expr) == NULL) {
    orignumdims = tmpnumdims;
  } else {
    orignumdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr)));
  }

  /* if we have a typechecking problem, let's leave it for the typecheck
     pass to handle */
  if (tmpnumdims != orignumdims) {
    return;
  }

  new = create_named_local_var(dt, T_PARFCN(T_STMT(expr)), temp); 

  if (fpst != NULL) {
    for (i=0;i<MAXRANK;i++) {
      S_FLUFF_LO(new,i) = S_FLUFF_LO(fpst,i);
      S_FLUFF_HI(new,i) = S_FLUFF_HI(fpst,i);
      S_UNK_FLUFF_LO(new,i) = S_UNK_FLUFF_LO(fpst,i);
      S_UNK_FLUFF_HI(new,i) = S_UNK_FLUFF_HI(fpst,i);
      S_WRAPFLUFF_LO(new,i) = S_WRAPFLUFF_LO(fpst,i);
      S_WRAPFLUFF_HI(new,i) = S_WRAPFLUFF_HI(fpst,i);
    }
  }

  /*************************************************************
    Build a new expresion for this temp and replace e
    in current statement with this temp:

    fun_call(e);  --> fun_call(new_temp);
    *************************************************************/
  new_expr = build_typed_0ary_op(VARIABLE, new);

  T_PARENT(new_expr) = T_PARENT(expr);
  T_STMT(new_expr) = T_STMT(expr);
  T_NEXT(new_expr) = T_NEXT(expr);
  T_PREV(new_expr) = T_PREV(expr);

  old_expr = copy_expr(expr);
  new_expr_lhs = copy_expr(new_expr);

  /**************************************************************
   Build an assignment statement for the new temp of the form:
           new_temp = e;
   *************************************************************/
  insertion_pt =  T_STMT(expr);

  lhspdt = dt;
  while (D_CLASS(lhspdt) == DT_ARRAY || D_CLASS(lhspdt) == DT_ENSEMBLE) {
    if (D_CLASS(lhspdt) == DT_ENSEMBLE) {
      lhspdt = D_ENS_TYPE(lhspdt);
    } else {
      new_expr_lhs = build_typed_Nary_op(ARRAY_REF,new_expr_lhs,NULL);
      lhspdt = D_ARR_TYPE(lhspdt);
    }
  }
  temprhsexpr = expr_find_root_expr(old_expr);
  if (temprhsexpr) {  /* if this has a base var, let's make sure it doesn't
			 need more blank array references */
    rhspdt = S_DTYPE(T_IDENT(temprhsexpr));
    loop = 1;
    while (loop) {
      parent = T_PARENT(temprhsexpr);
      switch (D_CLASS(rhspdt)) {
      case DT_ENSEMBLE:
	rhspdt = D_ENS_TYPE(rhspdt);
	if (parent && T_TYPE(parent) == BIAT) {
	  temprhsexpr = parent;
	}
	break;
      case DT_ARRAY:
	rhspdt = D_ARR_TYPE(rhspdt);
	if (parent && T_TYPE(parent) == ARRAY_REF) {
	  temprhsexpr = parent;
	} else {
	  temprhsexpr = build_typed_Nary_op(ARRAY_REF,temprhsexpr,NULL);
	  if (parent) {
	    T_OPLS(parent) = temprhsexpr;
	  }
	  T_PARENT(temprhsexpr) = parent;
	}
	break;
      case DT_STRUCTURE:
	/* search for field so we can update rhspdt */
	rhspdt = expr_bidot_find_field_dt(parent,rhspdt);
	if (rhspdt == NULL) {
	  loop = 0;
	} else {
	  temprhsexpr = parent;
	}
	break;
      default:
	loop = 0;
	break;
      }
    }
  } else {
    temprhsexpr = old_expr;
  }

  new_stmt = build_temp_assign(new_expr_lhs,temprhsexpr,dt,
			       T_LINENO(insertion_pt),T_FILENAME(insertion_pt));
  if (new_stmt == NULL) { /* this case occurs if we determine the assignment
			     will cause typechecking problems later */
    return;
  }

  /* exchange temp as actual parameter */
  *(expr) = *(new_expr); 

  /* insert assignment to temp */
  insertbefore_stmt(new_stmt, insertion_pt);
 
  S_SETUP(new) = 0;

  newgls = alloc_gen();
  G_IDENT(newgls) = new;
  if (T_TYPE(new_stmt) == S_REGION_SCOPE) {
    new_stmt = T_BODY(T_REGION(new_stmt));
  }
  T_PRE(new_stmt) = cat_genlist_ls(T_PRE(new_stmt),newgls);
  
  newgls = alloc_gen();
  G_IDENT(newgls) = new;
  T_POST(insertion_pt) = cat_genlist_ls(T_POST(insertion_pt),newgls);

}



/*
 *  static int
 *  make_may_tmp()
 *
 *  Returns 1 if an actual parameter should have a temp copy of itself made
 *  and to have the temp passed to the function instead of the actual
 *  value(s) of the actual.
 */
static int make_may_tmp(expr_t *actual) {
  int retVal;

  if (actual == NULL)
    return 1;

  retVal = 1;
  switch(T_TYPE(actual)) {
    
  case VARIABLE:
    /*
     * Check if this variable is a constant
     */
    if (T_IDENT(actual) != NULL) {
      switch(S_CLASS(T_IDENT(actual))) {
      case S_TYPE:
      case S_LITERAL:
      case S_SUBPROGRAM:
      case S_UNKNOWN:
      case S_ENUMTAG:
      case S_STRUCTURE:
      case S_VARIANT:
      case S_ENUMELEMENT:
	retVal = 0;
	break;
	
      case S_PARAMETER:
      case S_COMPONENT:
      case S_VARIABLE:
      default:
	if (S_IS_CONSTANT(T_IDENT(actual))) {
	  retVal = 0;
	}
	retVal = 1;
	break;
      }
    }
    break;

  case CONSTANT:
    retVal = 0;
    break;

  case BIAT:
  case BIDOT:
  case ARRAY_REF:
    /* don't use fake, use a temp, due to aliasing */
    /* may change later */
    
  case NULLEXPR:
    
  case UNEGATIVE:
  case UPOSITIVE:

  case REDUCE:

  case SCAN:    

  case FLOOD:      /* BLC -- assuming these will work the same */
    
  case UCOMPLEMENT:
    
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:

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
    
  case FUNCTION:
  default:
    retVal = 1;
    break;
  }
  
  return retVal;
}

/******************************************************************
  ensparam_expr()
  Ruth Anderson    3-5-94
  
  Insert temps for parameters to function calls if the actual
  parameter is:

  1) A expression involving ensembles. (If the formal parameter is
  an ensemble)
  2) A single function call (not involved in an expression).
  3) A single scan or reduction (not involved in an expression).

  Function calls and scans and reductions involved in expressions will
  be removed eventually as the expression will be removed by case 1).
  Then it is assumed that the function_temps and scan_temps passes
  will remove function calls and scans/reductions from complex 
  expressions when necessary.

  Also note that traverse_exprls_g will be called to traverse the parameters.

  The order of these passes should be:

  1) param_temps
  2) function_temps or scan_temps

  insertcomm, cda, and r2mloops should come after this.
  
implementor's notes:
G_IDENT(paramlist) gives a symtab_ptr
T_TYPE(symtab_P)  gives the datatype_ptr

********************************************************************/

static void ensparam_expr(expr_t *e) {
  expr_t       *actuals;		/* actual parameter list */
  genlist_t    *formals;                /* formal parameter list */
  expr_t       *actual_param;           /* ref to actual param in actuals */
  int          numops;
  int	       insertedFlag;
  int 	       i;

  if (!e)
    return;

  switch(T_TYPE(e)) {
  case FUNCTION:     
    numops = numoperands(e);

    /* This is a function with parameters. */
    if (numops > 1) {

      /*
       *  Check that this is a function we found we need to put temps
       *  in for.
       */
      
      DBS1(1, "*** ensparam: Checking out function '%s'\n",
	   S_IDENT(T_IDENT(T_OPLS(e))));

      if (S_DTYPE(T_IDENT(T_OPLS(e))) != NULL) { /* not external function */
	/******************************************************
	 ******** NOT an external function            ********
	 *****************************************************/
	for (actuals = nthoperand(e, 2),
	     formals = T_PARAMLS(S_FUN_BODY(T_IDENT(T_OPLS(e))));
	     ((actuals != NULL) && (formals != NULL)); 
	     actuals = T_NEXT(actuals), formals = G_NEXT(formals)) {
	  
	  actual_param = actuals;
	  
          /*
           * inserted flag used to insert only one temp
           */
	  insertedFlag = FALSE;
	  
	  if (D_IS_ANY_ENSEMBLE(S_DTYPE(G_IDENT(formals))) ||
	      D_CLASS(S_DTYPE(G_IDENT(formals))) == DT_ARRAY) {
	    if (S_VAR_VCLASS(G_IDENT(formals)) == SC_IN ||
		S_VAR_VCLASS(G_IDENT(formals)) == SC_CONST) {

	      /*
	       *  Insert MAY temps
	       *  Check to see if we need to insert a may them if:
	       *    - didn't insert a must temp
	       *    - this function has may temps
	       *    - safety check that pointer to list of may temps valid
	       *    - need to make a temp for this type of expression
	       *
	       */
	      if (!insertedFlag && make_may_tmp(actual_param)) {
		DBS1(1, 
		     "Inserting may temp for '%s'\n",
		     S_IDENT(G_IDENT(formals)));
		
		insert_param_temps(actual_param, T_TYPE(G_IDENT(formals)),
				   G_IDENT(formals));	
		insertedFlag = TRUE;
	      }
	    } /* end of if a formal value parameter cases */
	    
	    /*
	     *  Insert INDEX temps
	     *	- construct temp that stores index values to
	     *          pass to function
	     *	- If there is no symbol table entry present, then
	     *	  we are not dealing with an Indexi variable.
	     *	  continue.
	     *
	     */
	    if (!insertedFlag && T_IDENT(actual_param)) {
	      i = symtab_is_indexi(T_IDENT(actual_param));
	      if (i) {
		if (S_FG_PARAM(G_IDENT(formals)) &&
		    ((S_VAR_VCLASS(G_IDENT(formals)) == SC_INOUT) ||
		     (S_VAR_VCLASS(G_IDENT(formals)) == SC_OUT))) {
		  USR_FATAL_CONT(T_STMT(e),
				 "When calling '%s' illegal to pass "
				 "Index%d by inout or out, line %d",
				 S_IDENT(S_PARENT(G_IDENT(formals))), 
				 i, 
				 T_LINENO(T_STMT(e)));
		} else {
		  DBS0(1, "** Found index\n");
		  insert_param_temps(actual_param,T_TYPE(G_IDENT(formals)),
				     G_IDENT(formals));
		  insertedFlag = TRUE;
		}
	      }
	    }
	    
	  } /* end of if an ensemble formal parameter */

	  insertedFlag = FALSE;

	} /* end of for which iterates over actual and formal parameters */
      } else { /* external function */
	/******************************************************
	 ********    An external function             ********
	 *****************************************************/
	for (actuals = nthoperand(e, 2); (actuals != NULL); 
	     actuals = T_NEXT(actuals)) {
	  
	  /* Traverse the parameter list */
	  traverse_exprls_g(actuals, NULL, ensparam_expr);
	  
	}
      }
    }
    break;                   
  case FLOOD:
    {
      datatype_t *newdt;
      
      /* build flood argument temps using the typeinfo of the rhs
	 assignment so that all memcpy's will work properly.  In 
	 fact, we need a temp if the two types don't match. */

      if (expr_needs_tmp(T_OPLS(e)) ||
	  ((T_TYPE(T_PARENT(e)) == BIASSIGNMENT) &&
	   (datatype_base(T_TYPEINFO(T_OPLS(e))) != 
	    datatype_base(T_TYPEINFO(T_OPLS(T_PARENT(e))))))) {
	newdt = build_ensemble_type(T_TYPEINFO(T_OPLS(T_PARENT(e))),
				    T_REGION_SYM(T_REGMASK(e)),0,1,
				    T_LINENO(T_STMT(e)),T_FILENAME(T_STMT(e)));
	insert_param_temps(T_OPLS(e),newdt,NULL);
      }
    }
    break;
  default:
    break;
  }
}


static void ensparam_before_statement(statement_t *s) {
  switch (T_TYPE(s)) {
  case S_EXPR:
    traverse_exprls_g(T_EXPR(s), NULL, ensparam_expr);
    break;
  case S_IF:
    traverse_exprls_g(T_IFCOND(T_IF(s)), NULL, ensparam_expr);
    break;
  case S_LOOP:
    switch (T_TYPE(T_LOOP(s))) {
    case L_WHILE_DO:
    case L_REPEAT_UNTIL:
      traverse_exprls_g(T_LOOPCOND(T_LOOP(s)), NULL, ensparam_expr);
      break;
    case L_DO:
      traverse_exprls_g(T_IVAR(T_LOOP(s)), NULL, ensparam_expr);
      traverse_exprls_g(T_START(T_LOOP(s)), NULL, ensparam_expr);
      traverse_exprls_g(T_STOP(T_LOOP(s)), NULL, ensparam_expr);
      traverse_exprls_g(T_STEP(T_LOOP(s)), NULL, ensparam_expr);
      break;
    default:
      break;
    }
    break;
  case S_RETURN:
    traverse_exprls_g(T_RETURN(s), NULL, ensparam_expr);
    break;
  case S_IO:
    {
      datatype_t *ensdt;
      expr_t *reg;
      expr_t *ioexpr;

      ioexpr = IO_EXPR1(T_IO(s));
      if (ioexpr) {
	reg = T_TYPEINFO_REG(ioexpr);
	if (reg != NULL && expr_needs_tmp(ioexpr)) {
	  expr_t* tmpreg;

	  tmpreg = RMSCurrentRegion();
	  if (T_IDENT(tmpreg) == pst_qreg[0]) {
	    tmpreg = buildqregexpr(D_REG_NUM(T_TYPEINFO(reg)));
	  }
	  ensdt = build_ensemble_type(T_TYPEINFO(ioexpr), tmpreg, 0, 1, T_LINENO(s), T_FILENAME(s));
	  insert_param_temps(ioexpr,ensdt,NULL);
	} else if (expr_contains_par_fns(ioexpr)) { /* pull all par fns */
	  insert_param_temps(ioexpr,S_FUN_TYPE(T_IDENT(T_OPLS(ioexpr))),NULL);
	} else if (IO_IS_BIN(T_IO(s)) && !expr_is_lvalue(ioexpr)) {
	  insert_param_temps(ioexpr,T_TYPEINFO(ioexpr),NULL);
	}
      }
    }
    break;
  case S_REGION_SCOPE:
    break;
  case S_MLOOP:
  case S_NLOOP:
    break;
  case S_MSCAN:
  case S_COMPOUND:
  case S_NULL:
  case S_EXIT:
  case S_HALT:
  case S_CONTINUE:
  case S_END:
  case S_WRAP:
  case S_REFLECT:
    break;
  default:
    INT_FATAL(s, "ensparam.c: Unrecognized statement type %d", T_TYPE(s));
    break;
  }

}


static void ensparam_do_module(module_t *mod) {
  function_t *current_function;
  
  /* call the function ensparam_expr() on every statement of each module */
  /* rea - Traverse all functions in each module */
  current_function = T_FCNLS(mod);
  
  while (current_function != NULL) {
    traverse_stmtls_g(T_STLS(current_function), ensparam_before_statement, 
		      NULL, NULL, NULL);
    current_function = T_NEXT(current_function);
  }
}


/*
 *  ensparam
 *	Traverse through each module and apply get_ensparam_fcn() to each
 *	function of the module.
 */
static void ensparam(module_t *module) {
  function_t *ftp;
	  
  DB0(20,"ensparam()\n");

  RMSInit();
  for (; module != NULL; module = T_NEXT(module)) {
    ftp = T_FCNLS(module);
    if (ftp == NULL) {
      INT_FATAL(NULL, "No functions in module in ensparam()");
    }

    /*
     *  Drop in all the temps
     *  A modified version of param_temps
     *  Thank you rea
     */
    ensparam_do_module(module);
  }
  RMSFinalize();
}



/*
 *  call_ensparam
 *	Entry routine for ensparam pass.
 *	Ensparam makes sure that passing ensemble parameters happens correctly
 *	by copying the ensemble into temp and passing the temp into 
 *	the function.  Only those ensembles which could possibly be 
 *	changed within the function are copied to temps.  Temp insertion is
 *	done by routines similar to param_temps.
 *
 */
int call_ensparam(module_t *mod, char *s) {
  FATALCONTINIT();
  ensparam(mod);
  FATALCONTDONE();
  return 0;
}
