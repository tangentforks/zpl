/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/createvar.h"
#include "../include/datatype.h"
#include "../include/dbg_code_gen.h"
#include "../include/dimension.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/red2mloops.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/buildsym.h"
#include "../include/main.h"

static int part_red_buff_num=0;
static int full_red_buff_num=0;


static void append_sr_pdt(char* name,datatype_t* pdt) {
  switch (D_CLASS(pdt)) {
  case DT_CHAR:
  case DT_BOOLEAN:
  case DT_SIGNED_BYTE:
  case DT_INTEGER:
  case DT_SHORT:
    sprintf(name,"%s_int",name);
    break;
  case DT_UNSIGNED_BYTE:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_INT:
    sprintf(name,"%s__uint",name);
    break;
  case DT_LONG:
    sprintf(name,"%s_long",name);
    break;
  case DT_UNSIGNED_LONG:
    sprintf(name,"%s__ulong",name);
    break;
  case DT_REAL:
    sprintf(name,"%s_float",name);
    break;
  case DT_DOUBLE:
    sprintf(name,"%s_double",name);
    break;
  case DT_QUAD:
    sprintf(name,"%s__zquad",name);
    break;
  case DT_ARRAY:
    append_sr_pdt(name,D_ARR_TYPE(pdt));
    break;
  case DT_COMPLEX:
    sprintf(name,"%s_fcomplex",name);
    break;
  case DT_DCOMPLEX:
    sprintf(name,"%s_dcomplex",name);
    break;
  case DT_QCOMPLEX:
    sprintf(name,"%s_qcomplex",name);
    break;
  default:
    INT_FATAL(NULL, "Unexpected datatype in append_sr_pdt(): %d",D_CLASS(pdt));
  }
}


static statement_t* build_ident_expr(expr_t* redexpr,datatype_t* pdt, expr_t* tempexpr) {
  char name[256];
  char *id;
  symboltable_t* identpst;
  expr_t* identexpr;
  expr_t* assignexpr;
  statement_t* assignstmt;

  switch (T_SUBTYPE(redexpr)) {
  case PLUS:
    sprintf(name,"_ADD");
    break;
  case TIMES:
    sprintf(name,"_MULT");
    break;
  case MIN:
    sprintf(name,"_MIN");
    break;
  case MAX:
    sprintf(name,"_MAX");
    break;
  case AND:
    sprintf(name,"_AND");
    break;
  case OR:
    sprintf(name,"_OR");
    break;
  case BAND:
    sprintf(name,"_BAND");
    break;
  case BOR:
    sprintf(name,"_BOR");
    break;
  case BXOR:
    sprintf(name,"_XOR");
    break;
  case USER:
    id = get_function_name(T_IDENT(redexpr), NULL, 1);
    if (id == NULL) {
      id = get_function_name(T_IDENT(redexpr), tempexpr, 1);
      identpst = lu(id);
      if (id == NULL || S_CLASS(S_FUN_TYPE(identpst)) != DT_VOID) {
	USR_FATALX(T_LINENO(T_STMT(redexpr)), T_FILENAME(T_STMT(redexpr)),
		   "Missing identity function for user defined reduction");
      }
      identexpr = build_Nary_op(FUNCTION, build_0ary_op(VARIABLE, check_var(id)), tempexpr);
      assignstmt = build_expr_statement(identexpr,T_LINENO(T_STMT(redexpr)),T_FILENAME(T_STMT(redexpr)));
      return assignstmt;
    }
    if (get_function_name(T_IDENT(redexpr), tempexpr, 1)) {
      USR_FATALX(T_LINENO(T_STMT(redexpr)), T_FILENAME(T_STMT(redexpr)),
		 "Multiple identity functions for user defined reduction");
    }
    identpst = lu(id);
    if (!(equiv_datatypes(pdt, S_FUN_TYPE(identpst)))) {
      USR_FATALX(T_LINENO(T_STMT(redexpr)), T_FILENAME(T_STMT(redexpr)),
		 "Identity function for user defined reduction returns wrong type");
    }
    identexpr = build_Nary_op(FUNCTION, build_0ary_op(VARIABLE, check_var(id)), NULL);
    assignexpr = build_typed_binary_op(BIASSIGNMENT, identexpr, tempexpr);
    assignstmt = build_expr_statement(assignexpr,T_LINENO(T_STMT(redexpr)),T_FILENAME(T_STMT(redexpr)));
    return assignstmt;
    break;
  default:
    sprintf(name,"_NOP");
    break;
  }
  append_sr_pdt(name,pdt);
  sprintf(name,"%s_IDENTITY",name);
  identpst = lookup(S_VARIABLE,name);
  if (identpst == NULL) {
    INT_FATAL(T_STMT(redexpr),"Couldn't find identity %s\n",name);
  }
  identexpr = build_typed_0ary_op(CONSTANT,identpst);
  assignexpr = build_typed_binary_op(BIASSIGNMENT, identexpr, tempexpr);
  assignstmt = build_expr_statement(assignexpr,T_LINENO(T_STMT(redexpr)),T_FILENAME(T_STMT(redexpr)));

  return assignstmt;
}


datatype_t* ensure_good_scanred_type(datatype_t* pdt) {
  datatype_t* basetype;
  datatype_t* newtype;

  switch (D_CLASS(pdt)) {
  case DT_CHAR:
  case DT_BOOLEAN:
  case DT_SIGNED_BYTE:
  case DT_INTEGER:
  case DT_SHORT:
    return pdtINT;
    break;
  case DT_UNSIGNED_BYTE:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_INT:
    return pdtUNSIGNED_INT;
    break;
  case DT_LONG:
    return pdtLONG;
    break;
  case DT_UNSIGNED_LONG:
    return pdtUNSIGNED_LONG;
    break;
  case DT_REAL:
    return pdtFLOAT;
    break;
  case DT_DOUBLE:
    return pdtDOUBLE;
    break;
  case DT_QUAD:
    return pdtQUAD;
    break;
  case DT_COMPLEX:
    return pdtCOMPLEX;
    break;
  case DT_DCOMPLEX:
    return pdtDCOMPLEX;
    break;
  case DT_QCOMPLEX:
    return pdtQCOMPLEX;
    break;
  case DT_ARRAY:
    basetype = D_ARR_TYPE(pdt);
    newtype = ensure_good_scanred_type(basetype);
    if (newtype == basetype) {
      return pdt;
    } else {
      /* sharing dimension list */
      return build_array_type(D_ARR_DIM(pdt),newtype);
    }
    break;
  default:
    INT_FATAL(NULL, "Unexpected datatype in ensure_good_scanred_type(): %d",
	      D_CLASS(pdt));
  }
  return NULL;
}


static expr_t* create_red_reg(int numdims,expr_t* dstreg,
			      expr_t* srcreg,int lineno,
			      char* filename,int* staticreg) {
  expr_t* newreg;
  int redtype;
  int i;
  int dimbit;
  dimension_t* newdim=NULL;
#ifdef SKIP_THIS_FOR_NOW
  dimension_t* srcdim=NULL;
  dimension_t* dstdim=NULL;
#endif
  int dstregappropriate;
  int reg_dynamic = 0;
  
  if (expr_is_strided_reg(srcreg,1) || expr_is_strided_reg(dstreg,1)) {
    redtype = -1;
  } else {
    redtype = RMSClassifyPartialReduce(numdims);
  }
  /* do all reductions dynamically for now */
  if (redtype != -1) {
    dstregappropriate = 1;  /* assume we can reuse the dstreg */

    dimbit = 1;
    for (i=0; i<numdims; i++) {
      if (redtype & dimbit) {
#ifdef SKIP_THIS_FOR_NOW
	switch (D_REG_DIM_TYPE(T_TYPEINFO(srcreg),i)) {
	case DIM_RANGE:
	  if (!expr_rt_const(DIM_HI(srcdim))) {
	    reg_dynamic = 1;
	  }
	  /* fall through */
	case DIM_FLAT:
	  if (!expr_rt_const(DIM_LO(srcdim))) {
	    reg_dynamic = 1;
	  }
	  break;
	default:
	  break;
	}
	newdim = append_dim_list(newdim, copy_dimension(srcdim));
#endif

	/* the dstreg must match in this dim, so we don't check it */
      } else {
#ifdef SKIP_THIS_FOR_NOW
	newdim = append_dim_list(newdim,build_dimension(DIM_GRID));
#endif
	  
	/* see if the dstreg is appropriate for this dimension */
	switch (D_REG_DIM_TYPE(T_TYPEINFO(dstreg),i)) {
	case DIM_FLOOD:
	case DIM_GRID:
	  /* it's appropriate -- do nothing */
	  break;
	default:
	  dstregappropriate = 0;
	  break;
	}
      }
      
#ifdef SKIP_THIS_FOR_NOW
      dstdim = DIM_NEXT(dstdim);
      srcdim = DIM_NEXT(srcdim);
#endif
      dimbit = dimbit << 1;
    }

    if (dstregappropriate) {
      *staticreg = 1;
      return dstreg;
    }
  }
  /* if we can't use the destination region, we make a dynamic one for now */
  reg_dynamic = 1;
  if (!reg_dynamic) {
    /* newdim == NULL */
    expr_t* sbe = build_sbe_expr(newdim);
    newreg = convert_sbe_to_region(sbe, "", lineno, filename);
  } else {
    newreg = build_0ary_op(CONSTANT, pstAutoRedReg[numdims]);
  }
    
  if (reg_dynamic == 1) {
    *staticreg = 0;
  } else {
    *staticreg = (redtype != -1);
  }
  return newreg;
}


static void process_reduction(expr_t* redexpr) {
  statement_t* stmt;
  int lineno;
  char* filename;
  statement_t* nextstmt;
  expr_t* assignexpr;
  expr_t* rhs;
  expr_t* lhs;
  int numdims = 0;
  datatype_t* pdt;
  char tempvarname[64];
  symboltable_t* tempvar;
  expr_t* tempexpr;
  statement_t* assignstmt;
  int complete;
  expr_t* rhsreg;
  expr_t* tempreg=NULL;
  datatype_t* temparrdt;
  expr_t* dstreg;
  expr_t* srcreg;
  expr_t* dstmask;
  int dstmaskbit;
  expr_t* srcmask;
  int srcmaskbit;
  int staticreg;
  int try_using_dest_as_temp=1;
  int using_dest_as_temp=0;
  char* id;
  expr_t* tmp;
  statement_t* newstmt;
  statement_t* laststmt;
  int free=0;
 
  stmt = T_STMT(redexpr);
  lineno = T_LINENO(stmt);
  filename = T_FILENAME(stmt);
  nextstmt = T_NEXT(stmt);
  assignexpr = T_EXPR(stmt);
  if (!T_IS_ASSIGNOP(T_TYPE(assignexpr))) {
    return;
  }
  /* if the assignment is +=, etc. we can't use the dest as the temp storage */
  if (T_TYPE(assignexpr) != BIASSIGNMENT) {
    try_using_dest_as_temp=0;
  }
  lhs = T_OPLS(assignexpr);
  rhs = T_NEXT(lhs);
  if (rhs != redexpr) {
    INT_FATAL(stmt,"Unexpected placement of reduction within statement");
  }
  rhs = T_OPLS(rhs);
  rhsreg = T_TYPEINFO_REG(rhs);

  if (rhsreg == NULL) {
    /* if rhsreg is NULL, either the expression is a scalar (which is
       illegal and will be handled by typechecking), or it's an
       indexed array of arrays which has not yet had its indices
       inserted by a2nloops, and therefore evaluates as a scalar
       array.  In this latter case, we search deeply to try and find
       something that tells the rank. */
    numdims = expr_rank(rhs);
    if (numdims == 0) {
      free = expr_is_free(rhs);
      if (!free) {
	USR_FATAL_CONT(stmt,"Reduce of scalar expression");
	return; 
      }
    }
  } else {
    numdims = D_REG_NUM(T_TYPEINFO(rhsreg));
  }

  dstreg = RMSCurrentRegion();
  dstmask = RMSCurrentMask(&dstmaskbit);
  if (T_IDENT(dstreg) == pst_qreg[0]) { /* unresolved quote region */
    if (free) {
      T_IDENT(dstreg) = pst_free;
    } else {
      T_IDENT(dstreg) = pst_qreg[numdims];
    }
  }
  /* save destination region away for later use at codegen time since we
     will be playing games with scopes */
  T_REGMASK2(redexpr) = T_REGION(build_reg_mask_scope(dstreg,NULL,0,NULL,
						      lineno,filename));
  complete = (T_REGMASK(redexpr) == NULL);
  if (complete) {
    srcreg = dstreg;
    srcmask = dstmask;
    srcmaskbit = dstmaskbit;
    dstreg = NULL;
    tempreg = NULL;
  } else {
    RMSPushScope(T_REGMASK(redexpr));
    if (!RMSLegalReduce(stmt,numdims)) {
      RMSPopScope(T_REGMASK(redexpr));
      return;
    }

    srcreg = RMSCurrentRegion();
    srcmask = RMSCurrentMask(&srcmaskbit);
    if (T_IDENT(srcreg) == pst_qreg[0]) { /* unresolved quote region */
      T_IDENT(srcreg) = pst_qreg[numdims];
    }

    /* build temp region */
    tempreg = create_red_reg(numdims,dstreg,srcreg,lineno,filename,&staticreg);

    RMSPopScope(T_REGMASK(redexpr));
  }

  pdt = T_TYPEINFO(lhs);
  if (T_SUBTYPE(redexpr) != USER) {
    pdt = ensure_good_scanred_type(pdt);
  }

  if ((try_using_dest_as_temp) && (tempreg == dstreg)) {
    symboltable_t* lhsroot;
    datatype_t* lhsensdt;

    lhsroot = expr_find_root_pst(lhs);
    lhsensdt = datatype_find_ensemble(S_DTYPE(lhsroot));
    if ((tempreg == NULL && lhsensdt == NULL) ||
	(tempreg != NULL && lhsensdt != NULL && 
	 expr_equal(D_ENS_REG(lhsensdt), tempreg) &&  /* must be size of tempreg */
	 expr_find_ensemble_root(lhs) == lhs)) {  /* must be whole array access */
      using_dest_as_temp = 1;
      tempexpr = copy_expr(lhs);
    }
  }
  if (!using_dest_as_temp) {
    if (complete) {
      /* build scalar (grid) temp */
      sprintf(tempvarname,"_red_data%d",full_red_buff_num++);
      tempvar = create_named_local_var(pdt,T_PARFCN(stmt),tempvarname);
    } else {
      /* build temp array */
      sprintf(tempvarname, "_Red_data%d", part_red_buff_num++);
      temparrdt = build_ensemble_type(pdt,tempreg,0,1,lineno,filename);
      if (staticreg) {  /* make global if we can */
	tempvar = LU_INS(tempvarname);
	S_DTYPE(tempvar) = temparrdt;
      } else {  /* otherwise, make it local */
	tempvar = create_named_local_var(temparrdt,T_PARFCN(stmt),tempvarname);
	S_SETUP(tempvar) = 0;
      }
    }

    /* build expression for the reduction temp */
    tempexpr = build_typed_0ary_op(VARIABLE,tempvar);
  }

  /* switch reduction argument, tag reduction's rank */
  T_OPLS(redexpr) = tempexpr;
  T_RED_RANK(redexpr) = numdims;
  tempexpr = copy_expr(tempexpr);

  if (!using_dest_as_temp) {
    /* add on blank array references */
    while (D_CLASS(pdt) == DT_ARRAY) {
      tempexpr = build_typed_Nary_op(ARRAY_REF, tempexpr, NULL);
      T_TYPEINFO_REG(tempexpr) = build_0ary_op(CONSTANT, pstGRID_SCALAR[numdims]);
      pdt = D_ARR_TYPE(pdt);
    }
  }

  /* build assignment to the reduction temp */
  assignstmt = build_ident_expr(redexpr,pdt,tempexpr);

  /* wrap a scope around it for partial reduction */
  if (!complete) {
    if (using_dest_as_temp) {
      /* use dest region */
      assignstmt = build_reg_mask_scope(dstreg,NULL,MASK_NONE,assignstmt,
					lineno,filename);
      /*      assignstmt = build_mloop_statement(dstreg, assignstmt, 
					 D_REG_NUM(T_TYPEINFO(dstreg)),
					 NULL, MASK_NONE, lineno, filename);*/
    } else {
      /* use temporary region that we created to describe internal temp */
      genlist_t* newgls;
      /* open region scope */
      
      assignstmt = build_reg_mask_scope(tempreg,NULL,MASK_NONE,assignstmt,
					lineno,filename);
      /*      assignstmt = build_mloop_statement(tempreg, assignstmt, 
					 D_REG_NUM(T_TYPEINFO(tempreg)),
					 NULL, MASK_NONE, lineno, filename);*/
      if (!staticreg) {
	newgls = alloc_gen();
	G_IDENT(newgls) = tempvar;
	T_PRE(assignstmt) = newgls;
      }
    }
  }
  newstmt = assignstmt;

  /*
  dbg_gen_stmtls(stdout, newstmt);
  printf("-------\n");
  */

  
  /* setup temp reduction region */
  if (!complete && !staticreg && !using_dest_as_temp) {
    expr_t* fncall;
    expr_t* argexpr;
    expr_t* newarg;

    argexpr = copy_expr(srcreg);
    newarg = copy_expr(dstreg);
    T_NEXT(newarg) = argexpr;
    argexpr = newarg;
    newarg = copy_expr(tempreg);
    T_NEXT(newarg) = argexpr;
    argexpr = newarg;
    fncall = build_typed_Nary_op(FUNCTION,build_0ary_op(VARIABLE,
							pstCalcRedReg),argexpr);
    
    assignstmt = build_expr_statement(fncall,lineno,filename);
    insertafter_stmt(newstmt,assignstmt);
    newstmt = assignstmt;
    /*
    dbg_gen_stmtls(stdout, newstmt);
    printf("-------\n");
    */
  }

  /* build local part of reduction */
  tempexpr = copy_expr(tempexpr);
  {
    expr_t* tmp;
    tmp = tempexpr;
    while (tmp) {
      T_FREE(tmp) = TRUE;
      tmp = T_OPLS(tmp);
    }
  }
  rhs = copy_exprls(rhs, T_PARENT(rhs));
  if (T_SUBTYPE(redexpr) != USER) {
    assignexpr = build_typed_binary_op(BIOP_GETS, rhs, tempexpr);
    T_SUBTYPE(assignexpr) = T_SUBTYPE(redexpr);
  }
  else {
    symboltable_t* pst;
    tmp = rhs;
    while (T_NEXT(tmp) != NULL) {
      tmp = T_NEXT(tmp);
    }
    T_NEXT(tmp) = copy_expr(tempexpr);
    id = get_function_name(T_IDENT(redexpr), rhs, 1);
    if (id == NULL) {
      USR_FATALX(lineno, filename, "Missing local function for user defined reduction");
    }
    assignexpr = build_typed_Nary_op(FUNCTION, build_0ary_op(VARIABLE, check_var(id)), rhs);
    pst = lu(id);
    if (pst != NULL) {
      subclass sc;
      symboltable_t* decls;

      decls = T_DECL(S_FUN_BODY(pst));
      while (decls != NULL && S_CLASS(decls) != S_PARAMETER) {
	decls = S_SIBLING(decls);
      }
      while (decls != NULL) {
	sc = S_PAR_CLASS(decls);
	decls = S_SIBLING(decls);
	while (decls != NULL && S_CLASS(decls) != S_PARAMETER) {
	  decls = S_SIBLING(decls);
	}
      }
      if (sc == SC_IN || sc == SC_CONST) {
	assignexpr = build_typed_binary_op(BIASSIGNMENT, assignexpr, tempexpr);
	if (!(equiv_datatypes(S_FUN_TYPE(pst), T_TYPEINFO(tempexpr)))) {
	  USR_FATALX(lineno, filename, "local function for user defined reduction returns wrong type");
	}
      }
      else {
	if (S_CLASS(S_FUN_TYPE(pst)) != DT_VOID) {
	  USR_FATALX(lineno, filename, "by reference local function for user defined reduction must not return a value");
	}
      }
    }
  }
  assignstmt = build_expr_statement(assignexpr,lineno,filename);
  if (numdims != 0) { /* don't put mloop around free reduce */
    /*    assignstmt = build_mloop_statement(srcreg, assignstmt,
				       D_REG_NUM(T_TYPEINFO(srcreg)),
				       srcmask, srcmaskbit, lineno, filename);*/
  }

  insertbefore_stmt(newstmt, assignstmt);
  /*
  dbg_gen_stmtls(stdout, newstmt);
  printf("-------\n");
  */

  /* create actual reduction statement */
  assignstmt = build_expr_statement(redexpr,lineno,filename);
  insertbefore_stmt(newstmt, assignstmt);
  /*
  dbg_gen_stmtls(stdout, newstmt);
  printf("-------\n");
  */

  /* wrap a scope around it for partial reduction */
  if (!complete) {
    newstmt = build_reg_mask_scope(srcreg,srcmask,srcmaskbit,newstmt,
				   lineno,filename);
  }
  insertbefore_stmt(newstmt,stmt);
  laststmt = stmt;

  /*
  dbg_gen_stmtls(stdout, newstmt);
  printf("-------\n");
  */


  if (!using_dest_as_temp) {
    /* build assignment of reduction result */
    tempexpr = copy_expr(tempexpr);
    lhs = copy_expr(lhs);
    assignexpr = build_typed_binary_op(BIASSIGNMENT, tempexpr, lhs);
    assignstmt = build_expr_statement(assignexpr,lineno,filename);
    if (!complete) {
      /*      assignstmt = build_mloop_statement(dstreg, assignstmt, 
					 D_REG_NUM(T_TYPEINFO(dstreg)),
					 dstmask, dstmaskbit, lineno, filename);*/
    }
    if (nextstmt == NULL) {
      T_NEXT(stmt) = assignstmt;
      T_PREV(assignstmt) = stmt;
      T_PARFCN(assignstmt) = T_PARFCN(stmt);
    } else {
      insertbefore_stmt(assignstmt,nextstmt);
      laststmt = nextstmt;
    }
  }

  /* deallocate local array if there was one */
  if (!complete && !staticreg && !using_dest_as_temp) {
    genlist_t* newgls;

    newgls = alloc_gen();
    G_IDENT(newgls) = tempvar;
    T_POST(assignstmt) = newgls;
  }

  /* convert original statement into a simple reduction expression */
  /* T_EXPR(stmt) = redexpr;*/
  T_PRE(newstmt) = cat_genlist_ls(T_PRE(newstmt), T_PRE(stmt));
  T_POST(laststmt) = cat_genlist_ls(T_POST(laststmt), T_POST(stmt));
  remove_stmt(stmt);
  /* for user defined reductions, find or create a global reduction function */
  if (T_SUBTYPE(redexpr) == USER) {
    expr_t* copyexpr;
    char gfn[256];
    statement_t* body;
    statement_t* comp_stmt;
    symboltable_t* pst;
    expr_t* oldarg1;
    expr_t* newarg1;
    expr_t* oldarg2;
    expr_t* newarg2;

    sprintf(gfn, "_ZPLGLOBALREDUCE_%s", S_IDENT(T_IDENT(redexpr)));
    pst = lu(gfn);
    if (pst != NULL) {
      T_IDENT(redexpr) = pst;
    }
    else {
      symboltable_t* locals;

      tempexpr = copy_expr(tempexpr);
      copyexpr = copy_expr(tempexpr);
      T_NEXT(tempexpr) = copyexpr;
      id = get_function_name(T_IDENT(redexpr), tempexpr, 1);
      if (id == NULL) {
	USR_FATALX(lineno, filename, "Missing global function for user defined reduction");
      }
      pst = lu(id);
      if (pst == NULL) {
	USR_FATALX(lineno, filename, "Missing global function for user defined reduction");
      }
      locals = T_DECL(S_FUN_BODY(pst));
      oldarg1 = build_typed_0ary_op(VARIABLE, T_DECL(S_FUN_BODY(pst)));
      oldarg2 = build_typed_0ary_op(VARIABLE, S_SIBLING(T_DECL(S_FUN_BODY(pst))));
      body = copy_stmtls(T_STLS(S_FUN_BODY(pst)));
      pst = insert_function(gfn,pdtVOID,2,T_TYPEINFO(tempexpr),SC_INOUT,
			    T_TYPEINFO(tempexpr),SC_INOUT);
      S_STD_CONTEXT(pst) = FALSE;
      if (T_TYPE(body) != S_COMPOUND) {
	comp_stmt = build_compound_statement( NULL, body, lineno, filename);
      } else {
	comp_stmt = body;
      }
      S_SIBLING(S_SIBLING(T_DECL(S_FUN_BODY(pst)))) = locals;
      T_DECL(T_CMPD(comp_stmt)) = T_DECL(S_FUN_BODY(pst));
      T_STLS(S_FUN_BODY(pst)) = comp_stmt;
      fix_stmtls(comp_stmt, NULL, S_FUN_BODY(pst));
      newarg1 = build_typed_0ary_op(VARIABLE, T_DECL(S_FUN_BODY(pst)));
      newarg2 = build_typed_0ary_op(VARIABLE, S_SIBLING(T_DECL(S_FUN_BODY(pst))));
      replaceall_stmtls(body, oldarg1, newarg1);
      replaceall_stmtls(body, oldarg2, newarg2);

      /* we can do this because we know it is not recursive! */
      body = returns2assigns(body, newarg2);






      T_IDENT(redexpr) = pst;
    }
  }
}


static void red2mloops_expr(expr_t* expr) {
  if (expr == NULL) {
    return;
  }
  if (T_TYPE(expr) == REDUCE) {
    process_reduction(expr);
  }
}


static void red2mloops_stmt(statement_t* stmt) {
  if (stmt != NULL) {
    if (T_TYPE(stmt) == S_EXPR) {
      traverse_exprls(T_EXPR(stmt),0,red2mloops_expr);
    }
  }
}


static void red2mloops_mod(module_t* mod) {
  function_t* fn;

  fn = T_FCNLS(mod);

  while (fn != NULL) {
    traverse_stmtls(T_STLS(fn),0,red2mloops_stmt,NULL,NULL);
    fn = T_NEXT(fn);
  }
}


int call_red2mloops(module_t* mod,char* s) {
  FATALCONTINIT();
  RMSInit();
  traverse_modules(mod,TRUE,red2mloops_mod,NULL);
  RMSFinalize();
  FATALCONTDONE();

  return 0;
}
