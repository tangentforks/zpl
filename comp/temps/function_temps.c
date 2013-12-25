/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* rea - copied from the function_temps directory on 7-20-93 to create temps for
   function calls to functions with embedded mloops */
/* rea - modified 2-18-94 so that this pass comes BEFORE mloops are inserted.*/


#include <stdio.h>
#include "../include/error.h"
#include "../include/struct.h"
#include "../include/macros.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/const.h"
#include "../include/global.h"
#include "../include/db.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/generic_stack.h"
#include "../include/stmtutil.h"
#include "../include/createvar.h"
#include "../include/function_temps.h"
#include "../include/genlist.h"
#include "../include/traverse.h"
#include "../include/datatype.h"
#include "../include/expr.h"

static void function_temps(module_t *);
static void function_temps_do_module(module_t *);
static void function_temps_statement(statement_t *);
static void function_temps_expr(expr_t *);
static void do_function_call(expr_t *);



int call_function_temps(module_t *mod,char *s) {
  function_temps(mod);
  return 0;
}


static void function_temps(module_t *mod) {
  traverse_modules(mod, TRUE, function_temps_do_module, NULL);
}


static void function_temps_do_module(module_t *mod) {
  function_t *current_function;
  
  /* call the function function_temps_expr() on every statement of each module */
  /* rea - Traverse all functions in each module */
  current_function = T_FCNLS(mod);
  
  while (current_function != NULL) {
    traverse_stmtls_g(T_STLS(current_function),
		      function_temps_statement, NULL,
		      NULL, NULL);
    current_function = T_NEXT(current_function);
  }
}

static void function_temps_statement(statement_t *s) {
  switch (T_TYPE(s)) {
  case S_EXPR:
    traverse_exprls_g(T_OPLS(T_EXPR(s)), NULL, function_temps_expr);
    break;
  case S_IF:
    traverse_exprls_g(T_IFCOND(T_IF(s)), NULL, function_temps_expr);
    break;
  case S_LOOP:
    switch (T_TYPE(T_LOOP(s))) {
    case L_WHILE_DO:
    case L_REPEAT_UNTIL:
      traverse_exprls_g(T_LOOPCOND(T_LOOP(s)), NULL, function_temps_expr);
      break;
    case L_DO:
      traverse_exprls_g(T_IVAR(T_LOOP(s)), NULL, function_temps_expr);
      traverse_exprls_g(T_START(T_LOOP(s)), NULL, function_temps_expr);
      traverse_exprls_g(T_STOP(T_LOOP(s)), NULL, function_temps_expr);
      traverse_exprls_g(T_STEP(T_LOOP(s)), NULL, function_temps_expr);
      break;
    default:
      break;
    }
    break;
  case S_RETURN:
    traverse_exprls_g(T_RETURN(s), NULL, function_temps_expr);
    break;
  case S_REGION_SCOPE:
    break;
  case S_MLOOP:
  case S_NLOOP:
  case S_NULL:
  case S_EXIT:
  case S_COMPOUND:
  case S_MSCAN:
  case S_HALT:
  case S_CONTINUE:
  case S_IO:
  case S_END:
  case S_WRAP:
  case S_REFLECT:
    break;
  default:
    INT_FATAL(s, "function_temps.c: Unrecognized statement type %d", T_TYPE(s));
    break;
  }
  
}


/******************************************************************
  function_temps_expr(e)
  Ruth Anderson    7-29-93, modified 3-6-94
  
  Creates a temp for all function calls with ENSEMBLE RETURN TYPES 
  that are not part of a simple assignment statement.

  Example 1:  converts:

     A := B + fun_call();

  into:
     ensemble_temp_type  temp;

     temp := fun_call();
     A := B + temp;

  R2mloops will wrap an mloop around the last statement.

  Example 2:  Will not create a temp in this simple assignment statement:

  A := fun_call()
  
  ************************
  
  Traverses parameter lists of function calls as well.
  
  Also note that traverse_exprls_g will traverse the operators of 
  scans and reductions.  So if the operators of the scan or reduction
  contain function calls that will require temps, this
  will be caught here.
  

  Assumptions:  1) That the actual region associated with a function
                   return type of type ensemble will be known at
		   compile time.

   TO DO: search for a BIAT on the lhs among complex structures.
          (see below)
  
  ******************************************************************/

static void function_temps_expr(expr_t *e) {
  expr_t       *exprls;			/*** expr list of arguments ***/
  int          numops;


  if (!e)
    return;
  
  switch(T_TYPE(e)) {
  case FUNCTION:    
    
    /* Is this necessary????? Doesn't it do this in param_temps?? rea rea */

    /* Traverse the parameter list of this function to find function calls. */
    numops = numoperands(e);
    
    if (numops > 1) {  /* This is a function with parameters. */
      for (exprls = nthoperand(e, 2);
	   (exprls != NULL); exprls = T_NEXT(exprls)) {
	function_temps_expr(exprls);
      }
    }

    /** no function temp for free functions **/
    if (expr_is_free(e)) {
      break;
    }
    
/* rea to do:  think about how function parameter will get traversed here. */
    /* Create a temp for this function call if 1) it is NOT just part
       of a simple bi-assignment statement and 2) its return type is an
       ensemble. */

    /* If there is no parent ptr, then this may be part of a conditional 
       test.  For now we will assume that functions inside of condtional
       tests do not need to have temps created for them. */
    /* Are there other cases where we might have no parent ptr??? */
    if (T_PARENT(e) ) {
      if (datatype_complex(T_TYPEINFO(e))) {
	if (!expr_is_scan_reduce(T_PARENT(e))) {
	  do_function_call(e);
	}
      } else if (!T_IS_ASSIGNOP(T_TYPE(T_PARENT(e)))) { 
	/* Check to see if this is a non-external function.
	   External functions have a null datatype ptr. They also cannot
	   return ensembles. */
	if ((S_DTYPE(T_IDENT(T_OPLS(e))) != NULL) &&
	    /* do function if return type is ensemble */
	    (D_IS_ENSEMBLE(S_FUN_TYPE(T_IDENT(T_OPLS(e))))) ) {
	  if (T_TYPE(T_PARENT(e))!=FUNCTION) { /* handled well in ensparam.c */
	    do_function_call(e);
	  }
	} else if (T_TYPEINFO_REG(e) == NULL &&
		   T_TYPEINFO(e) != NULL && /* make sure typeinfo didn't 
					       simply ignore this expression */
		   T_TYPE(T_STMT(e)) == S_EXPR &&
		   T_IS_ASSIGNOP(T_TYPE(T_EXPR(T_STMT(e)))) &&
		   T_TYPEINFO_REG(T_OPLS(T_EXPR(T_STMT(e)))) != NULL) {
	  do_function_call(e);
	} else if (do_checkpoint && (T_TYPE(T_STMT(e)) == S_RETURN)) {
	  /*** this is a return statement with a function ***/
	  do_function_call(e);
	}
      }
      /* This was the RHS of a simple BIASSIGNMENT. 
	 If the LHS of the biassignment contains a (guaranteed non-null) BIAT,
	 and the return type of the function is an ensemble,
	 then a temp will still be required. */
      
      /* TO DO:  rea rea rea - This needs to search for 
	 a BIAT among complex structures. */
      
      else {
	if( (T_TYPE(T_OPLS(T_PARENT(e))) == BIAT) &&
	   /* Check to see if this is a non-external function.
	      External functions have a null datatype ptr. They also cannot
	      return ensembles. */
	   (S_DTYPE(T_IDENT(T_OPLS(e))) != NULL) &&
	   (D_IS_ENSEMBLE(S_FUN_TYPE(T_IDENT(T_OPLS(e))))) ) {
	  do_function_call(e);
	} else if (T_TYPEINFO_REG(T_OPLS(T_PARENT(e))) != NULL &&
		   T_TYPEINFO_REG(T_NEXT(T_OPLS(T_PARENT(e)))) == NULL) {
	  do_function_call(e);
	} else if (T_TYPE(T_PARENT(e)) != BIASSIGNMENT  &&
		   !T_PROMOTED_FUNC(e)) { /* handle += ... */
	  do_function_call(e);
	}
      }
    } else if (do_checkpoint &&
	       (T_TYPE(T_STMT(e)) == S_RETURN)) {
      /*** this is a return statement with a function ***/
      do_function_call(e);
    }
    break;                   
  default:
    break;
  }
}


/******************************************************************
  do_function_call()
  Ruth Anderson    7-20-93
  
  Create temps and assignment statements for temps for function calls.
  
  ASSUMPTIONS:  
  Assumes that the exact region associated with the return type of a function
  is known at compile time.  In reality we may only know the  
  rank of the region.  This being the case, we will have to malloc space for
  the temp at compile time, or use the brackets trick or something other
  than just inserting the temp into the symbol table.
  
  WHAT THIS ROUTINE DOES:
  create a name for the temp - access the global temp counter
  find out what type the temp is
  insert the temp into the symbol table
  modify the current statement to include reference to new temp
  create and add assignment statements for the temp to the AST
  
  INPUT:
  old_lhs = function_call() + otherstuff;
  
  OUTPUT:
  returntype    new_temp;
  
  new_temp = function_call();
  old_lhs = new_temp + otherstuff;
  
  ******************************************************************/

static void do_function_call(expr_t *expression) {
  statement_t *new_stmt;                /* Assignment statement for new temp */
  statement_t *insertion_pt;		/* insertion point */
  char  temp[20];
  symboltable_t  *new;
  expr_t *new_expr, *old_expr, *tempassign_expr;
  expr_t *new_expr_lhs;
  datatype_t *newdt;
  genlist_t *newgls;
  expr_t *reg;
  statement_t *pre_location=NULL;
  
  /**************************************************************
    Create a temp and place it in the symbol table. Access the 
    global temp counter for this.
    *************************************************************/
  sprintf(temp,"_fntemp%d", GlobalTempCounter++);   

  if (REPORT(REPORT_TEMPS)) {
    printf("Inserting fntemp\n");
  }
  
  DBS1(100, "\n Inserting %s in do_function_call\n", temp);
  INT_COND_FATAL((T_STMT(expression)!=NULL), T_STMT(expression),
		 "T_STMT(expression) was NULL");
  
  /* The first operand on the T_OPLS is a variable expression that stands
     for the name of the function being called. T_IDENT gives us the
     symboltable pointer for the function.  S_FUN_TYPE gives us the
     return type of the function. */


  reg = T_TYPEINFO_REG(expression);
  if (reg == NULL) {
    newdt = T_TYPEINFO(expression);
  } else {
    newdt = build_ensemble_type(T_TYPEINFO(expression),reg,0,1,T_LINENO(T_STMT(expression)),
				T_FILENAME(T_STMT(expression)));
  }
  INT_COND_FATAL(newdt!=NULL,T_STMT(expression),
		 "Return type of function is null.");
    
  new = create_named_local_var(newdt,T_PARFCN(T_STMT(expression)), temp); 
  if (expr_is_free(expression)) {
    S_STYPE(new) |= SC_FREE;
  }

  /* This assumes that the data type of the return type of a function
     is known at compile time (including the region associated with it).
     This may not be the case.  The rank of the region of the return 
     type (if the return type is an ensemble) will be known, but the 
     actual region may not be known.  In this case the output will not be 
     correct.  We will need to malloc space for the temps or something
     like that.  */
  

  DBS2(100, "do_function:: call to function : %s  parent function is = %s\n",
       S_IDENT(T_IDENT(T_OPLS(expression))),
       S_IDENT(T_FCN(T_PARFCN(T_STMT(expression)))));

  /*************************************************************
    Build a new expression for this temp and replace expression
    in current statement with this temp:
         old_lhs = new_temp;
    *************************************************************/
  new_expr = build_typed_0ary_op(VARIABLE, new);
  T_PARENT(new_expr) = T_PARENT(expression);
  T_STMT(new_expr) = T_STMT(expression);
  T_NEXT(new_expr) = T_NEXT(expression);
  T_PREV(new_expr) = T_PREV(expression);

  old_expr = copy_expr(expression);
  *(expression) = *(new_expr);
  new_expr_lhs = copy_expr(new_expr);

  /**************************************************************
   Build an assignment statement for the new temp of the form:
           new_temp = function_call();
   *************************************************************/
  tempassign_expr = build_typed_binary_op(BIASSIGNMENT, old_expr, new_expr_lhs);
  new_stmt = build_expr_statement(tempassign_expr,T_LINENO(T_STMT(expression)),
				  T_FILENAME(T_STMT(expression)));
  if (reg != NULL && T_SUBTYPE(reg) != INTERNAL_REGION) {
    pre_location = new_stmt;
    new_stmt = build_reg_mask_scope(reg,NULL,MASK_NONE,new_stmt,
				    T_LINENO(new_stmt),T_FILENAME(new_stmt));
  }
  
  insertion_pt =  T_STMT(expression);
  while (T_PREV(insertion_pt) == NULL && 
	 T_PARENT(insertion_pt) != NULL &&
	 T_TYPE(T_PARENT(insertion_pt)) == S_NLOOP) {
    insertion_pt = T_PARENT(insertion_pt);
  }
  insertbefore_stmt(new_stmt, insertion_pt);

  S_SETUP(new) = 0;

  if (pre_location == NULL) {
    if ((T_TYPE(insertion_pt)==S_LOOP) && (T_TYPE(T_LOOP(insertion_pt))!=L_DO)) {
      pre_location = insertion_pt;
    } else {
      pre_location = new_stmt;
    }
  }
  
  newgls = alloc_gen();
  G_IDENT(newgls) = new;
  if (T_TYPE(new_stmt) == S_REGION_SCOPE) {
    new_stmt = T_BODY(T_REGION(new_stmt));
  }
  T_PRE(pre_location) = cat_genlist_ls(T_PRE(pre_location),newgls);

  newgls = alloc_gen();
  G_IDENT(newgls) = new;
  T_POST(insertion_pt) = cat_genlist_ls(T_POST(insertion_pt),newgls);
}


