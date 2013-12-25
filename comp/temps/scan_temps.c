/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* rea - copied from the bluechip/z/src/comm directory on 7-8-93 to
   add in temps for scans and reduces */

#include <stdio.h>
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/createvar.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/red2mloops.h"
#include "../include/rmstack.h"
#include "../include/statement.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"


int GlobalTempCounter = 0;     

static statement_t *shard_stmt=NULL;


static void scan_temps_expr(expr_t*);


static void handle_function(expr_t *expr) {
  expr_t *actuals;
  symboltable_t *formals;
  expr_t *fnexpr;

  /* Traverse the parameter list of this function to find scans/reductions. */

  fnexpr = T_OPLS(expr);
  formals = T_DECL(D_FUN_BODY(T_TYPE(T_IDENT(fnexpr))));
  actuals = T_NEXT(fnexpr);
  while (actuals != NULL && formals != NULL) {
    if (S_SUBCLASS(formals) != SC_INOUT) { /** IN and OUT **/
      scan_temps_expr(actuals);
    }

    actuals = T_NEXT(actuals);
    do {  /* skip past non params */
      formals = S_SIBLING(formals);
    } while (formals && !S_FG_PARAM(formals));
  }
}


/******************************************************************
  do_scan(expression)
  Ruth Anderson    7-8-93, modified 3-7-94
  
  Create Temps and assignment statements for temps for "complex" scan and
  reduces.

  INPUT:  a ptr to an expression of type SCANPLUS, REDUCEMAX etc.
  
  ASSUMPTIONS:  
  1) That these are _complete_ reduces.
  2) Scan/reduces of simple expressions are accepted and will be passed
  on to kepart's scan/reduce Cgen routines.  He will generate
  code for things such as:

         x := +\A+B;
	 x := +\A*B-C;
	 A := +\\A-B;
	 etc.

  Basically the Scangen routines will work with a scan or reduction on 
  any expression that gen_expr() in Cgen.c will work easily with. 

  I will have pulled any scans/reductions during this pass.
  I will have pulled out function calls returning an ensemble in the
  function_temps pass.

  Scalar function calls will not have been removed.  Thus if the scalar
  function has been called with an ensemble, then the function will be called
  point by point during the codegen for the scan/reduction itself and no
  temp will be created.


  3) BIATs in scan/reduction expressions seem to be handled correctly by
  codegen, so we will allow them for now.  That is it appears that the
  comm pass is inserting communication for @s that appear inside of
  scan/reduction expressions.

  What this routine does:
  create a name for the temp
  FIND OUT WHAT TYPE THE TEMP IS
  insert the temp into the symbol table
  modify the current statement to include reference to new temp
  create and add assignment statements for the temp to the AST


  INPUT:
  old_lhs = scan_reduce() + otherstuff;
  
  OUTPUT:
  scan_reduce_type    temp;
  
  temp = scan_reduce();
  old_lhs = temp + otherstuff;

  ******************************************************************/

static void do_scan(expr_t *expr,expr_t *reg) {
  statement_t    *new_stmt;             /* Assignment statement for new temp */
  statement_t    *insertion_pt;		/* insertion point */
  char           temp[20];
  symboltable_t  *new;
  expr_t         *new_expr, *old_expr, *tempassign_expr;
  genlist_t      *newgls;
  expr_t         *replacement_expr;
  statement_t    *new_stmt_copy;
  statement_t    *pre_location=NULL;
  datatype_t* basedt;

  /* don't insert temps for scans/reduces in shards, cause they're illegal */
  if (shard_stmt != NULL) {
    return;
  }
  
  /**************************************************************
    Create a temp and place it in the symbol table. Access the 
    global temp counter for this.
    
    declare:
     scan_reduce_type    temp;
    *************************************************************/
  sprintf(temp,"_scantemp%d", GlobalTempCounter++);   

  if (REPORT(REPORT_TEMPS)) {
    printf("Inserting scantemp\n");
  }

  INT_COND_FATAL((T_STMT(expr)!=NULL), NULL,
		 "T_STMT(expr) was NULL");

  insertion_pt = T_STMT(expr);

  /* If this is an ensemble, then create an ensemble temp. */
  if (!reg) {
    reg = T_TYPEINFO_REG(expr);
  }

  basedt = T_TYPEINFO(expr);
  if (basedt == NULL && T_TYPE(expr) == REDUCE && T_SUBTYPE(expr) == USER) {
    USR_FATAL(T_STMT(expr), "User-defined reductions may only be used in simple assignment statements");
  }
  basedt = ensure_good_scanred_type(basedt);
  if (reg) {
    datatype_t *newdt;
    newdt = build_ensemble_type(basedt,reg,0,1,T_LINENO(T_STMT(expr)),T_FILENAME(T_STMT(expr)));
    if (D_ENS_NUM(newdt) == 0) {
      D_ENS_NUM(newdt) = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr)));
    }
    new = create_named_local_var(newdt,T_PARFCN(insertion_pt), temp);
  } else {
    new = create_named_local_var(basedt, T_PARFCN(insertion_pt), temp);
  }
  
  /*************************************************************
    Build a new expression for this temp and replace expression
    in current statement with this temp:
         old_lhs = temp + otherstuff;
    *************************************************************/
  new_expr = build_typed_0ary_op(VARIABLE, new);
  T_PARENT(new_expr) = T_PARENT(expr);
  T_STMT(new_expr) = insertion_pt;
  T_NEXT(new_expr) = T_NEXT(expr);
  T_PREV(new_expr) = T_PREV(expr);
  
  old_expr = copy_expr(expr);
  {
    datatype_t *pdt;

    replacement_expr = copy_expr(new_expr);
    T_PARENT(replacement_expr) = T_PARENT(new_expr);
    T_STMT(replacement_expr) = insertion_pt;
    T_NEXT(replacement_expr) = T_NEXT(new_expr);
    T_PREV(replacement_expr) = T_PREV(new_expr);
    pdt = S_DTYPE(new);
    do {
      if (D_CLASS(pdt) == DT_ENSEMBLE) {
	pdt = D_ENS_TYPE(pdt);
      }
      if (D_CLASS(pdt) == DT_ARRAY) {
	/* add null ref */
	replacement_expr = build_typed_Nary_op(ARRAY_REF,replacement_expr,NULL);
	new_expr = build_typed_Nary_op(ARRAY_REF,new_expr,NULL);

	T_STMT(replacement_expr) = insertion_pt;

	pdt = D_ARR_TYPE(pdt);
      }
    } while (D_CLASS(pdt) == DT_ARRAY);
  }
  *(expr) = *(replacement_expr);
  /**************************************************************
    Build an assignment statement for the new temp of the form:
       temp = scan_reduce();
   **************************************************************/
  tempassign_expr = build_typed_binary_op(BIASSIGNMENT, old_expr, new_expr);
  new_stmt = build_expr_statement(tempassign_expr,T_LINENO(insertion_pt),
				  T_FILENAME(insertion_pt));
  if (reg != NULL && T_SUBTYPE(reg) != INTERNAL_REGION) {
    pre_location = new_stmt;
    new_stmt = build_reg_mask_scope(reg,NULL,MASK_NONE,new_stmt,
				    T_LINENO(new_stmt),T_FILENAME(new_stmt));
  }
  
  /* Insert statement immediately above current statement. */
  if ((T_TYPE(insertion_pt)==S_LOOP) && (T_TYPE(T_LOOP(insertion_pt))!=L_DO)) {
    cat_stmt_ls(T_BODY(T_LOOP(insertion_pt)),new_stmt);
    if (T_TYPE(T_LOOP(insertion_pt))==L_WHILE_DO) {
      new_stmt_copy = copy_stmt(new_stmt);
      insertbefore_stmt(new_stmt_copy,insertion_pt);
    }
  } else {
    insertbefore_stmt(new_stmt, insertion_pt);
  }

  S_SETUP(new) = 0;

  /* insert PRE --- location depends on context */
  if (pre_location == NULL) {
    if ((T_TYPE(insertion_pt)==S_LOOP) && (T_TYPE(T_LOOP(insertion_pt))!=L_DO)) {
      if (T_TYPE(T_LOOP(insertion_pt)) == L_WHILE_DO) {
	/* insert pre above assignment outside of loop */
	pre_location = new_stmt_copy;
      } else {
	/* insert pre outside of loop */
	pre_location = insertion_pt;
      }
    } else {
      /* insert pre above assignment to temp */
      pre_location = new_stmt;
    }
  }
  newgls = alloc_gen();
  G_IDENT(newgls) = new;
  T_PRE(pre_location) = cat_genlist_ls(T_PRE(pre_location),newgls);

  /* insert post after use of temp */
  newgls = alloc_gen();
  G_IDENT(newgls) = new;
  T_POST(insertion_pt) = cat_genlist_ls(T_POST(insertion_pt),newgls);
}


static void scan_temps_expr_ls(expr_t *expr) {
  while (expr != NULL) {
    scan_temps_expr(expr);
    expr = T_NEXT(expr);
  }
}


/******************************************************************
  scan_temps_expr(e)
  Ruth Anderson    7-29-93, modified 3-6-94
  
  Creates a temp for all scans/reductions that are not just
  part of a simple assignment statement.

  Example 1:  converts:

     A := B + scan_reduce();

  into:
     scan_reduce_type  temp;

     temp := scan_reduce();
     A := B + temp;

  R2mloops will wrap an mloop around the last statement.

  Example 2:  Will not create a temp in this simple assignment statement:

  A := scan_reduce();
  
  ************************
  
  Traverses parameter lists of function calls as well.


  Note that this should cover if_statements as well: 
       if (+/ A)  . . .
  should be replaced with:
       temp = +/A; 
       if (temp)   . . 

  Assumptions:  1) That the expression type stuff works.
                   (Derrick's new pass )
		2) Reductions are complete (to a scalar) reductions.

  ******************************************************************/


static void scan_temps_expr(expr_t *e) {
  region_t *scope;
  expr_t *tos;

  if (!e) {
    return;
  }

  switch(T_TYPE(e)) {
  case FUNCTION:
    /* Q: Is this necessary????? Doesn't it do this in param_temps?? rea */
    /* A: it does, but if the expression is scan + scan, it won't put in
       temps for the scan expressions, so this is necessary in addition */

    handle_function(e);
    break;
  case REDUCE:
    /* rea to do:  think about how function parameter will get traversed here.*/
    /* NOTE: This assumes that reductions are all COMPLETE(i.e. to a scalar).*/
    /* If either this IS NOT just a simple assignment statement, OR
       if (it IS a simple assignment statement AND there is an ensemble
       on the lhs), then a temp is needed as well. 
       The first test is for an if or while statement. */

    scope = T_REGMASK(e);
    if (scope) {
      RMSPushScope(scope);
    }

    scan_temps_expr(T_OPLS(e));
    
    if (scope) {
      RMSPopScope(scope);
    }

    if ((expr_is_free(T_OPLS(e)) && !expr_parallel(T_OPLS(e))) ||
        !T_PARENT(e) || /* IF this is an if or while statment */
	(T_TYPE(T_PARENT(e)) != BIASSIGNMENT ) || /* anything but a biassign */
	((T_TYPE(T_PARENT(e)) == BIASSIGNMENT ) && 
	 (((T_TYPEINFO_REG(left_expr(T_PARENT(e)))) &&
	   (!T_TYPEINFO_REG(e))) ||
	  (T_SUBTYPE(e) != USER && ensure_good_scanred_type(T_TYPEINFO(left_expr(T_PARENT(e)))) !=
	   T_TYPEINFO(left_expr(T_PARENT(e))))))) {
      /* this is a biassign, but there is an ensemble on the LHS. */ 
      if (scope) {
	do_scan(e,RMSCurrentRegion()); /* part red needs ens temp */
      } else {
	do_scan(e,NULL);                      /* full red needs scalar temp */
      }
    }
    break;                     /* rea */
  case SCAN:
  /* rea to do:  think about how function parameter will get traversed here. */
    scan_temps_expr(T_OPLS(e));

    if (!T_PARENT(e) || (T_TYPE(T_PARENT(e)) != BIASSIGNMENT)) {
      do_scan(e,RMSCurrentRegion()); 
    }
    break;                     /* rea */
  case PERMUTE:
    scan_temps_expr(T_OPLS(e)); /*** SCATTER problem: how to insert temps for complex scatters? ***/

    if (!T_PARENT(e) || (T_TYPE(T_PARENT(e)) != BIASSIGNMENT && T_TYPE(T_PARENT(e)) != BIOP_GETS)) {
      do_scan(e,RMSCurrentRegion());
    }
    break;
  case FLOOD: /* BLC -- copied from the scan cases without thinking too hard */
    
    scope = T_REGMASK(e);
    if (scope) {
      RMSPushScope(scope);
    }

    scan_temps_expr(T_OPLS(e));

    if (scope) {
      RMSPopScope(scope);
    }

    if (!T_PARENT(e) || (T_TYPE(T_PARENT(e)) != BIASSIGNMENT)) {
      tos = RMSCurrentRegion();
      if (expr_is_qreg(tos)) {
	do_scan(e,build_0ary_op(CONSTANT, pst_mfreg));
      } else {
	do_scan(e,tos);
      }
    }
    break;                     /* rea */
  default:
    scan_temps_expr_ls(T_OPLS(e));
    break;
  }
}


static void scan_temps_before_statement(statement_t *s) {
  switch (T_TYPE(s)) {
  case S_EXPR:
    scan_temps_expr_ls(T_EXPR(s));
    break;
  case S_IF:
    scan_temps_expr_ls(T_IFCOND(T_IF(s)));
    break;
  case S_LOOP:
    switch (T_TYPE(T_LOOP(s))) {
    case L_WHILE_DO:
    case L_REPEAT_UNTIL:
      scan_temps_expr_ls(T_LOOPCOND(T_LOOP(s)));
      break;
    case L_DO:
      scan_temps_expr_ls(T_IVAR(T_LOOP(s)));
      scan_temps_expr_ls(T_START(T_LOOP(s)));
      scan_temps_expr_ls(T_STOP(T_LOOP(s)));
      scan_temps_expr_ls(T_STEP(T_LOOP(s)));
      break;
    default:
      break;
    }
    break;
  case S_RETURN:
    scan_temps_expr_ls(T_RETURN(s));
    break;
  case S_IO:
    scan_temps_expr_ls(IO_EXPR1(T_IO(s)));
    break;
  case S_REGION_SCOPE:
    break;
  case S_MLOOP:
  case S_NLOOP:
    /* keep track of the outer most loop construct */
    /* 3- 6-94 no longer used. rea */
    break;
  case S_NULL:
  case S_EXIT:
  case S_HALT:
  case S_CONTINUE:
  case S_COMPOUND:
  case S_MSCAN:
  case S_END:
  case S_WRAP:
  case S_REFLECT:
    break;
  default:
    INT_FATAL(s, "scan_temps.c: Unrecognized statement type %d", T_TYPE(s));
    break;
  }
  if (shard_stmt==NULL && stmt_is_shard(s)) {
    shard_stmt = s;
  }
}


static void scan_temps_after_statement(statement_t *s) {
  if (shard_stmt == s) {
    shard_stmt = NULL;
  }
}


static void scan_temps_do_module(module_t *mod) {
  function_t *current_function;
 
  /* rea - Traverse all functions in each module */
  current_function = T_FCNLS(mod);
  
  while (current_function != NULL) {
    traverse_stmtls_g(T_STLS(current_function),scan_temps_before_statement,
		      scan_temps_after_statement,NULL, NULL);
    current_function = T_NEXT(current_function);
  }
}


static void scan_temps(module_t *mod) {
  traverse_modules(mod, TRUE, scan_temps_do_module, NULL);
}


int call_scan_temps(module_t *mod,char *s) {
  RMSInit();
  scan_temps(mod);
  RMSFinalize();
  return 0;
}


