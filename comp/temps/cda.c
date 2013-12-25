/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/Cgen.h"
#include "../include/alias.h"
#include "../include/allocstmt.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/comm.h"
#include "../include/createvar.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/temps.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"


/* global variables */

static int replaced_prime_at;
static expr_t* target_expr;
static expr_t* replacement_expr;


/*************************************************
  datatype_t  *cda_build_ensemble_type( base, region, rank)	
  Ruth Anderson
  6-1-94

  Builds an ensemble datatype.  Works for rank-only defined ensembles,
  where the region is unknown.

  Assumptions:
             1) relies on typeinfo.
  *************************************************/
/* WHY not build_ensemble_type? SJD */
static datatype_t *cda_build_ensemble_type(datatype_t *base,
					   expr_t *region,int rank) {
  datatype_t *new;

  if (region) {
    DB1(30, "cda_build_ensemble_type - %s\n", S_IDENT(T_IDENT(region)));
    INT_COND_FATAL((D_CLASS(T_TYPEINFO(region)) == DT_REGION), NULL, 
		   "must pass in a region");
  }
  
  HandleAnonymousRecords(base);
  
  new = alloc_dt(DT_ENSEMBLE);
  D_ENS_TYPE(new) = base;
  D_ENS_REG(new)  = region;
  D_ENS_DNR(new) = 1;
  if (region) {
    D_ENS_NUM(new)  = D_REG_NUM(T_TYPEINFO(region));
  } else { /* rank must be passed in from somewhere */
    D_ENS_NUM(new)  = rank;
  }
  D_ENS_COPY(new) = NULL;
  return new;
}


/*************************************************
  symboltable_t *cda_create_new_var()
  Derrick Weathersby
  12-13-93
  Comments and modifications by rea: 12-15-93, 1-23-94
    
  Comments:
  
  1) This is not fully general for the case of records and arrays.
  It currently handles ensembles of records but nothing more
  complex is guaranteed.
  
  2) For the case of ensembles of records, only create a temp that is an
  ensemble of the type of the field that is being accessed.  That is
  for: A.x := A@east.x; 
  if field x is a float, and A is an ensemble of vectors.
  create cda_temp1 as an ensemble of floats, rather than as an ensemble 
  of vectors.
  *************************************************/
static symboltable_t *cda_create_new_var(datatype_t *pdt,expr_t *expr) {
  char temp[32];
  symboltable_t *new;
  function_t *pft;
  
  pft = T_PARFCN(T_STMT(expr));
  sprintf(temp,"_cda_temp%d",GlobalTempCounter++);
  new = create_named_local_var(pdt,pft,temp);
  
  /* assumptions:
     cda temp will not need fluff info.
     */

  return new;
}


static void replace_prime_ats(expr_t* expr) {
  expr_t* direxpr;

  if (T_TYPE(expr) == BIAT && T_SUBTYPE(expr) == AT_PRIME) {
    if (expr_equal(T_OPLS(expr),target_expr)) {
      /* replace the argument of the prime-@ */
      direxpr = T_NEXT(T_OPLS(expr));
      T_OPLS(expr) = copy_expr(replacement_expr);
      T_NEXT(T_OPLS(expr)) = direxpr;
      T_PARENT(T_OPLS(expr)) = expr;

      replaced_prime_at = 1;
    }
  }
}


static statement_t* build_init_cda_primeat_temp(statement_t* scan,
						expr_t* tempexpr,
						expr_t* old_expr,
						expr_t* reg) {
  expr_t* assign_expr;
  statement_t* assign_stmt;

  assign_expr = build_binary_op(BIASSIGNMENT,copy_expr(old_expr),
				copy_expr(tempexpr));
  assign_stmt = build_expr_statement(assign_expr,T_LINENO(scan),
				     T_FILENAME(scan));
  assign_stmt = build_reg_mask_scope(reg,NULL,MASK_NONE,assign_stmt,
				     T_LINENO(scan),T_FILENAME(scan));
  insertbefore_stmt(assign_stmt, scan);

  return assign_stmt;
}


static statement_t* find_insertion_pt(statement_t* stmt) {
  statement_t* parent;

  parent = T_PARENT(stmt);
  if (parent && T_TYPE(parent) == S_MSCAN) {
    return parent;
  } else {
    return stmt;
  }
}


void insert_cda_temp(expr_t *LHS_expr,statement_t *stmt,exprtype et,
		     int consider_primeats) {
  datatype_t *new_type;
  symboltable_t *new_symbol;
  expr_t *new_expr;
  expr_t *old_expr;
  expr_t *assign_expr;
  statement_t *new_assign_stmt;
  expr_t *reg;
  int numdims;
  genlist_t *newgls;
  binop_t assigntype;
  statement_t* insertion_pt;

  /* Create the new cda_temp# */

  numdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(LHS_expr)));
  reg = RMSCurrentRegion();
  if (T_IDENT(reg) == pst_qreg[0]) {
    reg = buildqregexpr(numdims);
  }
  new_type = cda_build_ensemble_type(T_TYPEINFO(LHS_expr),reg,numdims); 
  new_symbol = cda_create_new_var(new_type,LHS_expr);
  
  new_expr = build_typed_0ary_op(VARIABLE, new_symbol);
  INT_COND_FATAL((new_expr != NULL), NULL, 
		 "new_expr ptr is null in cda temp create");
  /* Replace LHS with new cda_temp# 
     example:  cda_temp# := OLD_RHS
     note: the expr type is BIASSIGNMENT under all circumstances */
  T_TYPE(T_EXPR(stmt)) = BIASSIGNMENT;                        /*echris*/
  assigntype = T_SUBTYPE(T_EXPR(stmt));
  T_SUBTYPE(T_EXPR(stmt)) = 0;
  T_PARENT(new_expr) = T_PARENT(LHS_expr);
  T_STMT(new_expr) = T_STMT(LHS_expr);
  T_NEXT(new_expr) = T_NEXT(LHS_expr);
  T_PREV(new_expr) = T_PREV(LHS_expr);

  /* replace prime-@'s on RHS as necessary and update the temp's region
     if any were found */
  if (consider_primeats) {
    replacement_expr = new_expr;
    target_expr = LHS_expr;
    replaced_prime_at = 0;
    traverse_expr(T_NEXT(T_OPLS(T_EXPR(stmt))),0,replace_prime_ats);
    if (replaced_prime_at) {
      expr_t* basereg = T_TYPEINFO_REG(LHS_expr);
    
      if (reg != basereg) {
	new_type=cda_build_ensemble_type(T_TYPEINFO(LHS_expr),basereg,numdims);
	S_DTYPE(new_symbol) = new_type;
      }
    }
  }

  /* replace the LHS expression */
  old_expr = copy_expr(LHS_expr);
  *(LHS_expr) = *(new_expr);

  
  typeinfo_expr(new_expr);
  while (D_CLASS(T_TYPEINFO(new_expr)) == DT_ARRAY) {
    new_expr = build_typed_Nary_op(ARRAY_REF,new_expr,NULL);
    typeinfo_expr(new_expr);
  }
  
  insertion_pt = find_insertion_pt(T_STMT(LHS_expr));

  /* Build assignment statement for the new cda_temp#
     example:       LHS := cda_temp#                 
     note: the expr type (eg BIPLUS_GETS, BIASSIGNMENT, etc) 
     is the same as in the original expr (et)             */
  assign_expr = build_typed_binary_op(et, new_expr, old_expr); /*echris*/
  T_SUBTYPE(assign_expr) = assigntype;
  INT_COND_FATAL((assign_expr != NULL), NULL, "assign_expr null in cda");
  new_assign_stmt = build_expr_statement(assign_expr,T_LINENO(stmt),
					 T_FILENAME(stmt));
  T_STMT(assign_expr) = new_assign_stmt;
  INT_COND_FATAL((new_assign_stmt != NULL), NULL, 
		 "new expr ptr is null in cda");
  
  /* insert statement immediately above current statement */
  
  /*
    dbg_gen_stmt(stdout,insertion_pt);
    dbg_gen_stmt(stdout,new_assign_stmt);
  */

  insertafter_stmt(new_assign_stmt, insertion_pt);

  if (consider_primeats && replaced_prime_at) {
    insertion_pt = build_init_cda_primeat_temp(insertion_pt,new_expr,old_expr,
					       D_ENS_REG(S_DTYPE(new_symbol)));
  }

  S_SETUP(new_symbol) = 0;

  newgls = alloc_gen();
  G_IDENT(newgls) = new_symbol;
  T_PRE(insertion_pt) = cat_genlist_ls(T_PRE(insertion_pt),newgls);

  newgls = alloc_gen();
  G_IDENT(newgls) = new_symbol;
  T_POST(new_assign_stmt) = cat_genlist_ls(T_POST(new_assign_stmt),newgls);
}


/*************************************************
  void  do_statement(s)
  
  Derrick Weathersby
  12-13-93
  Comments and modifications by rea: 12-15-93, 1-23-94
  
  
  General Idea of the CDA pass:
  
  This routine looks for dependencies in statements.  A dependency may
  exist if there is an @ on both the left-hand-side and right-hand-side
  of an assignment statement.  If the @s both refer to the same ensemble
  then we do further tests.  Finally, a dependence is shown to exist if
  we are writing a new value on the LHS before we have had a chance to
  read the old value from the same location on the RHS.  If we find such
  a dependence, then we will modify the LHS so writing is done into a temp
  variable and then the temp is later copied back into the LHS variable at
  the appropriate location.
  
  Comments:
  1) This will currently only compare if the base VARIABLE name
     is the same, it will not compare if the same field in a record 
     or same element in an array has been accessed. 
     Thus we are being conservative here and adding in more temps than
     may be necessary.
  Assumptions:
  1) search_upfor_ensemble_dt() will return NULL if no ensemble exists
  in a particular variable.
*************************************************/

static void cda_statement(statement_t *s) {
  expr_t            *LHS_expr, *RHS_expr;
  expr_t            *assignment_expr;
  /* below is used for inserting the temporary */
  exprtype et;
  
  if (T_TYPE(s) != S_EXPR) {
    return;
  }

  et = T_TYPE(T_EXPR(s));

  if (!((et == BIASSIGNMENT) || (et == BIOP_GETS))) {
    return;
  }
  
  assignment_expr = T_EXPR(s);            /* assignment expression */
  LHS_expr = T_OPLS(assignment_expr);     /* lhs of assignment expression */
  RHS_expr = T_NEXT(LHS_expr);            /* rhs of assignment expression */


  /*************************************
    Set LHS Fields
  *************************************/
  if (!T_TYPEINFO_REG(LHS_expr)) {
    return;
  }
  
  /* Next, determine if there are truly dependencies and then add in a
     temporary variable.  We do this in two stages.  First we add a
     temp for anything that really, truly needs it.  Then we add some
     additional temps to ensure that every MLOOP with the same
     expression on the LHS and RHS has a temp inserted so that the
     MLOOP generation can get the order right, because it isn't smart
     enough to do it if there's just one statement for some reason.
     Ideally, only this first insertion should exist.  

     There's some chance that this second conditional may fire
     unnecessarily, such as if the first conditional inserts a temp
     but not because of any prime-ats (replaced_prime_at == 0).
     However, I haven't found any such cases yet, so haven't optimized
     this case.  -BLC 01/2001 */

  if (exprs_really_cda_alias(LHS_expr,RHS_expr)) {
    insert_cda_temp(LHS_expr,s,et,1);
  }
  if (exprs_cda_alias(LHS_expr,RHS_expr)) {
    insert_cda_temp(LHS_expr,s,et,0);
  }
}


static void cda_module(module_t *mod) {
  function_t *current_function = NULL;

  current_function = T_FCNLS(mod);
  while (current_function != NULL) {
    traverse_stmtls_g(T_STLS(current_function),cda_statement,NULL,NULL,NULL);
    current_function = T_NEXT(current_function);
  }
}


static void cda_temps(module_t *mod) {
  /* call the function cda_module() on each of the modules */
  traverse_modules(mod, TRUE, cda_module, NULL);
}


int call_cda_temps(module_t *mod,char *s) {
  RMSInit();
  cda_temps(mod);
  RMSFinalize();

  return 0;
}
