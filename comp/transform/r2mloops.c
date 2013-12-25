/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/contraction.h"
#include "../include/coverage.h"
#include "../include/db.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/macros.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/statement.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/traverse.h"
#include "../include/stmtutil.h"


/* BLC: I conjecture that dominant_dim could be removed or replaced
   with a less global variable  3/17/2003 */
static int dominant_dim = 0;    /* Counts the dims in a region */
static int insert_mloop;	/*** used in check_functions ***/
static statement_t *IN_MSCAN = NULL;
static statement_t *SHARD_ACTIVE = NULL;
static statement_t *INNER_NLOOP = NULL;


static statement_t *build_mloop_wrapper(expr_t *reg,statement_t *stmt,
					int rank,expr_t *mask,int with) {
  function_t *fn;

  fn = T_PARFCN(stmt);
  if (expr_is_qmask(mask)) {
    mask = BestCoveringMask(fn);
  }
  return build_mloop_statement(reg,stmt,rank,mask,with,T_LINENO(stmt),
			       T_FILENAME(stmt));
}


static int helperup(expr_t* expr) {
  if (expr == NULL) {
    return 0;
  }
  else if (expr_at_ensemble_root(expr) && T_RANDACC(expr) != NULL) {
    /* T_TYPE(expr) == BIAT && T_SUBTYPE(expr) == AT_RANDACC */
    return 1;
  }
  else if (T_PARENT(expr) != NULL && T_OPLS(T_PARENT(expr)) == expr) {
    return helperup(T_PARENT(expr));
  }
  else {
    return 0;
  }
}

static int helperdown(expr_t* expr) {
  if (expr == NULL) {
    return 0;
  }
  else if (expr_at_ensemble_root(expr) && T_RANDACC(expr) != NULL) {
    /* T_TYPE(expr) == BIAT && T_SUBTYPE(expr) == AT_RANDACC */
    return 1;
  }
  else {
    return helperdown(T_OPLS(expr));
  }
}

static int expr_is_of_randaccuat(expr_t* expr) {
  return helperup(expr) || helperdown(T_OPLS(expr));
}



/* rea 7-30   Modifications made so that all functions in a module are
   processed.  */



/********************************************************************
  parse_structure(expr)
  Derrick Weathersby/Ruth Anderson 9-21-93

  Traverses an expression to determine if it contains a reference to
  an ensemble.  If so, then set dominant_dim = # of dims in ensemble.
  dominant dim is a global variable in r2loops.c
  Also returns the num_dims

  Modified by Ruth to traverse arrays properly, since arrays of
  ensembles are possible.

  Assumptions:  there are no nested ensembles.
  ********************************************************************/
static void parse_structure(expr_t *expr) {
  symboltable_t *symtab_p = NULL;  /* symbol table pointer */
  int num_dims = 0;
  datatype_t *dt, *dt_ptr;

  if (expr_is_of_randaccuat(expr)) {
    if (T_RANDACC(expr) != NULL &&
	T_RANDACC(expr) != (expr_t*)0x1) { /* Brad's sentinel value */
      traverse_exprls(T_RANDACC(expr),TRUE, parse_structure); 
    }
  }
  symtab_p = T_IDENT(expr);  /* get symbol table pointer */
  if (symtab_p == NULL)  /* operator */
    {
      return;
    }
  dt = S_DTYPE(symtab_p);
  if (!dt)
    return;
  /* for each expression, get the variables used and dimensions of each */
  dt_ptr = dt;
  while (dt_ptr != NULL) {
    switch (D_CLASS(dt_ptr)) {
    case DT_ARRAY:
      dt_ptr = D_ARR_TYPE(dt_ptr);
      break;
    case DT_ENSEMBLE:
      num_dims= D_ENS_NUM(dt_ptr);
      if (num_dims > dominant_dim)
	dominant_dim = num_dims;
      dt_ptr = NULL;  /* Found an ensemble, can be no more ensembles in
			 this datatype ptr. */
      break;
    case DT_GENERIC_ENSEMBLE:
      DBS0(1, "DT_GENERIC_ENSEMBLE found.  We don't know what rank this is.")
      dt_ptr = NULL;  /* Found an ensemble, can be no more ensembles in
			 this datatype ptr. */
      break;
    default:
      dt_ptr = NULL;
      break;
    }
  }
  return;
}

static void check_functions(expr_t *expr) {
  if (insert_mloop == FALSE)
    return;

  switch (T_TYPE(expr)) {
  case FUNCTION:
    if ((T_STLS(S_FUN_BODY(T_IDENT(T_OPLS(expr)))) != NULL) &&
	(T_PARALLEL(S_FUN_BODY(T_IDENT(T_OPLS(expr)))))) {
      /*** not an external or parallel function ***/
      /*** no mloop needed ***/
      insert_mloop = FALSE;
    }
    break;
  default:
    break;
  }

}



/*********************************************************
fixup_scan(stmt)
Ruth Anderson 9-22-93

Adds info to the scan/reduce structure:
   - rank of the scan_reduce
   - region associated with the scan/reduce if known
  *******************************************************/
static void fixup_scan(statement_t *stmt) {
  int *rank=0;  
 
 /* Traverse expression to get rank of scan/reduce */
  traverse_exprls(T_OPLS(T_NEXT(T_OPLS(T_EXPR(stmt)))),
		  TRUE, parse_structure); 
  rank = (int *)PMALLOC(sizeof(int));
  *rank = dominant_dim;
  T_RANK(T_NEXT(T_OPLS(T_EXPR(stmt)))) = rank;
}





/***********************************************
build_expr_mloop(stmt)
Derrick Weathersby/ Ruth Anderson 9-21-93

Builds an mloop around stmt.  Generates mloops 
  for assignment expressions that:
  - do not contain a scan or reduce
  - are not imbedded inside of a where statement
  *****************************************************/

static statement_t *build_expr_mloop(statement_t *stmt) {
  statement_t *mloop;
  expr_t *reg;
  expr_t *mask;
  int with;
  
  if (dominant_dim) {                   /* we have an ensemble */
    reg = RMSCurrentRegion();
    if (expr_is_qreg(reg) == -1) {
      reg = buildqregexpr(RankOfCover(T_PARFCN(stmt)));
    }
    mask = RMSCurrentMask(&with);
    mloop = build_mloop_wrapper(reg,stmt,dominant_dim,mask,with);
    /* insert mloop before stmt */
    insertbefore_stmt(mloop, stmt);
    /* remove stmt from AST, clear, and then place into mloop body */
    remove_stmt(stmt);
    clear_stmt(stmt);
    T_PARENT(mloop) = T_PARENT(stmt); /* original parent of stmt*/
    T_PARENT(stmt) = mloop; /* new parent of stmt */
    T_PARFCN(mloop) = T_PARFCN(stmt); /* function_t pointer copied*/
    T_PRE(mloop) = T_PRE(stmt);
    T_PRE(stmt) = NULL;
    T_POST(mloop) = T_POST(stmt);
    T_POST(stmt) = NULL;
  }
  return mloop;
}


/**************************************************
datatype_t *ensemble_in_dt(dt)
Ruth Anderson
3-30-94

Takes a datatype ptr as input and returns a ptr to a datatype
ptr of type DT_ENSEMBLE if one exists in this dt.  Returns NULL
if no DT_ENSEMBLE is found.

**************************************************/

static datatype_t *ensemble_in_dt(datatype_t *dt) {

  int            done;
  datatype_t     *result_dt;
  symboltable_t  *field;

  while (dt) {
    switch (D_CLASS(dt)) {
    case DT_ARRAY:
      dt = D_ARR_TYPE(dt);
      break;
    case DT_ENSEMBLE:                /* found the ensemble */
      return dt;
    case DT_GENERIC_ENSEMBLE:                /* found the ensemble */
      DBS0(1, "Returning a ptr to a DT_GENERIC_ENSEMBLE. I think this is o.k.");
      return dt;
    case DT_STRUCTURE: 
      /* Search down all of the fields. Exit when an ensemble is found. */
      done = FALSE;
      field = D_STRUCT(dt); /* Get the first field. */
      
      while (field && !done) {
	dt = S_DTYPE(field);
	result_dt = ensemble_in_dt(dt);
	/* If result_dt is non-null, we found an ensemble. */
	if (result_dt) {
	  dt = result_dt;
	  done = TRUE;
	}
	/* Get the next field in the structure. */
	field = S_SIBLING(field);
      }
      break;
    case DT_GENERIC:
    case DT_BOOLEAN:
    case DT_CHAR:
    case DT_SHORT:
    case DT_LONG:
    case DT_DOUBLE:
    case DT_QUAD:
    case DT_INTEGER:
    case DT_FILE:
    case DT_STRING:
    case DT_REAL:   /* Terminate the search down this datatype, no 
		       further expansion is possible from this point.*/
    case DT_UNSIGNED_INT:
    case DT_UNSIGNED_SHORT:
    case DT_UNSIGNED_LONG:
    case DT_UNSIGNED_BYTE:
    case DT_SIGNED_BYTE:
    case DT_OPAQUE:
    case DT_ENUM:
    case DT_COMPLEX:
    case DT_DCOMPLEX:
    case DT_QCOMPLEX:
      dt = NULL;
      break;
    case DT_REGION:
      dt = NULL;
      break;
    default:
      INT_FATAL(NULL, "datatype %d not handled in ensemble_in_dt",
		D_CLASS(dt));
      dt = NULL;
      break;
    }
  }
  
  return dt;
}


/*******************************************************

  ASSUMPTIONS:  
  1) external functions cannot return ensembles.

*******************************************************/

static void r2mloops_pre_statement(statement_t *stmt) {
  genlist_t               *formal;
  expr_t                  *actual;
  int                     promoted;

  if (IN_MSCAN) {  /* no MLOOPs within an MSCAN */
    return;
  }
  
  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_HALT:
  case S_CONTINUE:
      if (SHARD_ACTIVE)
         return;
    break;
  case S_MSCAN:
      /* this doesn't work:  truncates the stmtls as if there was just one */
      /*      T_CMPD_STLS(stmt) = build_expr_mloop(T_CMPD_STLS(stmt));*/
      {
	statement_t *newstmt;
	statement_t *body;
	expr_t *mask;
	int with;
	expr_t* reg;

	reg = RMSCurrentRegion();
	if (expr_is_qreg(reg) == -1) {
	  reg = buildqregexpr(RankOfCover(T_PARFCN(stmt)));
	}
	mask = RMSCurrentMask(&with);
	body = T_CMPD_STLS(stmt);

	newstmt = build_mloop_wrapper(reg,body,D_REG_NUM(T_TYPEINFO(reg)),mask,with);
	IN_MSCAN = newstmt;

	/* old way */
/*	T_CMPD_STLS(stmt) = newstmt;*/

	/* new way -- replace MSCAN with MLOOP by direct replacement */
	/* on the way out so that we hit the right case and recur properly */
      }
      break;
  case S_COMPOUND:
      if (SHARD_ACTIVE)
         return;
    break;
  case S_REGION_SCOPE:
    INNER_NLOOP = NULL;
    break;
  case S_EXPR:
    dominant_dim = 0;

    if (!(T_TYPE(T_EXPR(stmt)) == FUNCTION && 
	  (T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_GATHER_GD_LOOP") ||
	   T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_GATHER_GD_LOOP_BAIL") ||
	   T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_SCATTER_PD_LOOP") ||
	   T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_SCATTER_PD_LOOP_BAIL") ||
	   T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_SCATTER_NI_PD") ||
	   T_IDENT(T_OPLS(T_EXPR(stmt))) == lu_pst("_GATHER_NI_GD")))) {
      traverse_exprls(T_EXPR(stmt),TRUE, parse_structure); 
    }
    if (SHARD_ACTIVE)
       return;
    
    /*****************************************
      THIS IS AN ASSIGNMENT OPERATOR
      *****************************************/
    if (T_IS_ASSIGNOP(T_TYPE(T_EXPR(stmt))))  {
      
      /*  This IS an assignment of a: scan or reduce, set rank and region. 
          Do not insert an MLOOP. */
      /***************************************
	ASSINGMENT OF A SCAN/REDUCE.  
	**************************************/
      if ((T_IS_CCOMM(T_TYPE(T_NEXT(T_OPLS(T_EXPR(stmt)))))) ||
	  (T_IS_CCOMM(T_TYPE(T_OPLS(T_EXPR(stmt)))))) {
	fixup_scan(stmt);
      }
      
      /***************************************
	ASSINGMENT OF ANYTHING OTHER THAN AN SCAN/REDUCE or a FUNCTION
	CALL RETURNING AN ENSEMBLE.
	**************************************/
      /* This IS NOT an assignment of a:  scan or reduce.
	 If it is NOT the case that this is (an assignment of a function_call 
	 and the function call returns an ensemble) then insert an mloop. */
      else {
	insert_mloop = TRUE;
	traverse_exprls_g(right_expr(T_EXPR(stmt)),
			  check_functions, NULL);
	if (insert_mloop == TRUE)
	  build_expr_mloop(INNER_NLOOP?INNER_NLOOP:stmt);
      }
    }
    /*****************************************
      THIS IS _NOT_ AN ASSIGNMENT OPERATOR
      *****************************************/
    /* Check if this a function call, not part of an assignment stmt. */
    /* If this is a user or external function that has scalar parameters, 
       but is being passed in an ensemble, then an mloop needs to be 
       inserted.
       For each actual param: (begin loop)
         If actual param is an ensemble, then 
	    if formal param is a scalar, then
	      insert an mloop;
	      exit loop;
	 end loop;
	 */
    else if (T_TYPE(T_EXPR(stmt)) == FUNCTION ) {
      /* If this is a user-defined function then we have formal param info,
	 otherwise it is an external function and an mloop will be inserted
	 (if ensembles were present - dominant_dim >0 ). */
      if (T_TYPE(T_IDENT(T_OPLS(T_EXPR(stmt))))) {
	formal = T_PARAMLS(S_FUN_BODY(T_IDENT(T_OPLS(T_EXPR(stmt)))));
	actual = T_NEXT(T_OPLS(T_EXPR(stmt)));
	promoted = FALSE;
	while (!promoted && actual) {
	  /* linc: not sure what the following comment means.  The code that
	     inserts null BIATs no longer resides in this file. */
	  /* Skip over BIAT if this is the first item in the actual expr, 
	     as typeinfo will not yet be propagated to the null BIATs just 
	     inserted above. Null BIATs existing further down the expr tree 
	     are not a problem as the typeinfo field should be correct 
	     from a previous call to the typeinfo pass. */	
	  if ( T_TYPE(actual) == BIAT) {
	    actual = T_OPLS(actual);
	  }
	  if (T_TYPEINFO_REG(actual)) {
	    /* It is o.k. to search only the dt ptr off of the formal,
	       because there is only one dt_ptr in this expression.
	       i.e. Formals will only be A or b not A.r or A[2] or A@west 
	       Thus this should find any ensembles in the formal. */
	    if (formal && !(ensemble_in_dt(T_TYPE(G_IDENT(formal))))) {
	      build_expr_mloop(INNER_NLOOP?INNER_NLOOP:stmt);
	      promoted = TRUE;
	    }
	  }
	  if (formal)
	     formal = G_NEXT(formal);
	  actual = T_NEXT(actual);
	}
      }
      else { /* If this is an external function, then insert an mloop
		if ensembles are present. */
	build_expr_mloop(INNER_NLOOP?INNER_NLOOP:stmt);    
      }
    }
    break;
  case S_COMM:
      {
	INT_FATAL(NULL,"How did we get here?");
      }
      break;
  case S_WRAP:
  case S_REFLECT:
    {
      
      /* Note: each WRAP statement has a send and a receive node */
      /*******************************************************
	Find rank of wrap/reflects stmt by looking at ensembles it
	will be applied to.  Insert null BIAT nodes.
	*******************************************************/
      dominant_dim = 0;
      traverse_exprls(T_OPLS(T_WRAP(stmt)),TRUE,parse_structure); 
      if (SHARD_ACTIVE)
         return;
      /* Set the wrap expr_p ptr to point to the newly inserted null BIATs.  */
      if (T_PARENT(T_OPLS(T_WRAP(stmt))) != NULL) {
	T_OPLS(T_WRAP(stmt)) = T_PARENT(T_OPLS(T_WRAP(stmt)));
      }
      
      INT_COND_FATAL((dominant_dim > 0),  stmt,
		     "No ensembles found for a wrap/reflects statement.");
    }
    break;
  case S_IF:
      {
      int local_dominant_dim = 0;

      if (SHARD_ACTIVE) {
         return;
      }
      local_dominant_dim = stmt_is_shard(stmt);
      if (local_dominant_dim) {
	if (local_dominant_dim == -1) {
	  local_dominant_dim = RankOfCover(T_PARFCN(stmt));
	}
	 /* assign to global used in build_expr_mloop */
	 dominant_dim = local_dominant_dim;
	 build_expr_mloop(INNER_NLOOP?INNER_NLOOP:stmt); /* Insert an MLOOP */
	 SHARD_ACTIVE = stmt;
      }
      }
      break;
  case S_LOOP:
      {
      int local_dominant_dim = 0;

      switch(T_TYPE(T_LOOP(stmt))) {
	 case L_WHILE_DO:
	 case L_REPEAT_UNTIL:
	 case L_DO:
	   break;
      }
      if (SHARD_ACTIVE)
         return;
      local_dominant_dim = stmt_is_shard(stmt);
      if (local_dominant_dim) {
	if (local_dominant_dim == -1) {
	  local_dominant_dim = RankOfCover(T_PARFCN(stmt));
	}
	 /* assign to global used in build_expr_mloop */
	 dominant_dim = local_dominant_dim;
	 build_expr_mloop(INNER_NLOOP?INNER_NLOOP:stmt); /* Insert an MLOOP. */
	 SHARD_ACTIVE = stmt;
      }
      }
    break;
  case S_NLOOP:
    if (INNER_NLOOP)
      return;
    if ((T_NLOOP_MLOOP_PROXIMITY(T_NLOOP(stmt)) == NLOOP_INSIDE) &&
	(stmtls_length(T_NLOOP_BODY(T_NLOOP(stmt))) == 1)) {
      INNER_NLOOP = stmt;
    }
    break;
  case S_RETURN:
      if (SHARD_ACTIVE)
         return;
    break;
  case S_IO:
      if (SHARD_ACTIVE)
         return;
    break;
  default:
    break;
  }
}



static void r2mloops_post_statement(statement_t *stmt) {
  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_HALT:
  case S_CONTINUE:
    break;
  case S_MSCAN:
    T_TYPE(stmt) = S_MLOOP;
    T_MLOOP(stmt) = T_MLOOP(IN_MSCAN);
    if (stmtls_contains_uat(T_MLOOP_BODY(T_MLOOP(stmt)), AT_PRIME)) {
      T_SUBTYPE(stmt) = MLOOP_MSCAN;
    }
    IN_MSCAN = NULL;
    break;
  case S_COMPOUND:
    break;
  case S_REGION_SCOPE:
    break;
  case S_WRAP:
  case S_REFLECT:
  case S_COMM:
    break;
  case S_EXPR:
    break;
  case S_IF:
    if (SHARD_ACTIVE==stmt) {
      SHARD_ACTIVE = 0;
    }
    break;
  case S_LOOP:
    if (SHARD_ACTIVE==stmt) {
      SHARD_ACTIVE = 0;
    }
    break;
  case S_NLOOP:
    if (INNER_NLOOP == stmt) {
      INNER_NLOOP = NULL;
    }
    break;
  case S_RETURN:
    break;
  case S_IO:
    break;
  default:
    break;
  }
}


static void r2mloops_module(module_t *mod) {
  function_t *current_function;

  /* rea - Traverse all functions in each module */
  current_function = T_FCNLS(mod);

  while (current_function != NULL) {
    traverse_stmtls_g(T_STLS(current_function), 
		      r2mloops_pre_statement, 
		      r2mloops_post_statement,
		      NULL,
		      NULL);
    current_function = T_NEXT(current_function);
  }
  set_mloop(mod); /* set T_IN_MLOOP() pointer */
}


static void regions_to_mloops(module_t *module_list) {
  RMSInit();
  traverse_modules(module_list,TRUE,r2mloops_module,NULL);
  RMSFinalize();
}


int call_r2mloops(module_t *mod,char *s) {
  regions_to_mloops(mod);

  return 0;
}

