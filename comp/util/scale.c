/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "../include/callsite.h"
#include "../include/coverage.h"
#include "../include/datatype.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/scale.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"

#define DIRIND  "        "
#define ARRIND  "          "
#define INTIND  "            "
#define INARIND "              "
#define DETIND  "                "
#define REGIND  "                  "

#define SDL 29
#define SUM 25

#ifndef abs
#define abs(x) (((x) >=0) ? (x) : (-(x)))
#endif

static int found_answer;
static int reg_stride_ans[MAXRANK];
static int dir_val_ans[MAXRANK];
static int numdims;


/* local prototypes */

static void reg_find_stride(expr_t* reg,int stride[]) {
  expr_t* dir;
  symboltable_t* dirpst;
  int i;
  int success;

  if (reg == NULL || T_TYPE(reg) == SBE || expr_is_qreg(reg)) {
    for (i=0; i<numdims; i++) {
      stride[i] = 1;
    }
    return;
  }
  
  switch (T_TYPE(reg)) {
  case CONSTANT:
  case VARIABLE:
    reg_find_stride(IN_VAL(S_INIT(T_IDENT(reg))), stride);
    break;
  case BIDOT:
  case ARRAY_REF:
    found_answer = -1;
    break;
  case BIPREP:
    switch (T_SUBTYPE(reg)) {
    case OF_REGION:
    case BY_REGION:
    case IN_REGION:
    case AT_REGION:
    case SPARSE_REGION:
      reg_find_stride(T_OPLS(reg),stride);
      break;
    default:
      INT_FATAL(NULL,"Unknown region type in reg_find_stride()\n");
    }
    if (T_SUBTYPE(reg) == BY_REGION) {
      DBS0(SDL,REGIND"Region is strided\n");
      dir = T_NEXT(T_OPLS(reg));
      dirpst = expr_find_root_pst(dir);
      DBS0(SDL,REGIND"Region is strided by non-multi direction\n");
      for (i=0; i<numdims; i++) {
	/* BC+SD: Assumes direction is a literal */
	stride[i] *= abs(dir_comp_to_int(dir, i, &success));
	if (!success) {
	  found_answer = -1;
	}
      }
    }
    break;
  default:
    INT_FATAL(NULL, "unexpected regiontype in reg_find_stride");
  }
}


int reg_dir_info(expr_t* expr,int ret_regscale[],int ret_dirscale[]) {
  expr_t* reg;
  expr_t* arr_expr;
  expr_t* dir;
  symboltable_t* dirpst;
  int dir_offset=0;
  int orig_dir_offset;
  symboltable_t* arrpst = NULL;
  int i;
  genlist_t* arr_alias;
  callsite_t* callsite;
  int reg_local;
  int loc_reg_stride[MAXRANK];
  int success;

  found_answer = 0;
  if (T_TYPE(expr) != BIAT) {
    INT_FATAL(T_STMT(expr),"Sending a non-BIAT expression into reg_dir_info");
  }
  numdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr)));

  reg = RMSCurrentRegion();

  if (T_IDENT(reg) == pst_qreg[0]) {
    T_IDENT(reg) = pst_qreg[RankOfCover(T_PARFCN(T_STMT(expr)))];
  }

  if (reg == NULL) {
    INT_FATAL(T_STMT(expr),"rmstack must be on for reg_dir_info() to work");
  }

  IFDB(SDL) {
    DBS1(SDL,"      Checking [%s] ... ",S_IDENT(T_IDENT(reg)));
    dbg_gen_expr(stdout,expr);
    DBS0(SDL,"\n");
  }
  
  dir = T_NEXT(T_OPLS(expr));
  dirpst = expr_find_root_pst(dir);
  DBS0(SDL,DIRIND"Direction is non-multi\n");
  for (i=0; i< numdims; i++) {
    dir_val_ans[i] = dir_comp_to_int(dir, i, &success);
    if (!success) {
      return -1;
    }
  }

  /* at this point either:
       loc_dirscale = 1    => direction was not multi-
       dirindexpr is non-NULL => direction's expression was locally known
       dirindexpr is NULL     => direction's expression must be inherited from 
                              arr_expr's aliases

       arrpst is non-NULL  => check aliases of arrpst
  */

  reg_find_stride(reg,loc_reg_stride);

  if (expr_contains_qreg(reg) || 
      (T_TYPEINFO(reg) && datatype_reg_inherits(T_TYPEINFO(reg)))) {
    reg_local = 0;
    if (expr_is_qreg(reg)) {
      DBS0(SDL,INTIND"Local region is a quote region\n");
    } else {
      int depth = 1;
      int cov_reg_stride[MAXRANK];
      expr_t* covreg;

      DBS0(SDL,INTIND"Local region contains a quote region\n");

      covreg = RMSRegionAtDepth(depth);
      while (!expr_is_qreg(covreg)) {
	reg_find_stride(covreg,cov_reg_stride);
	for (i=0; i<numdims; i++) {
	  loc_reg_stride[i] *= cov_reg_stride[i];
	}

	DBS1(SDL,INTIND"Whose covering region stride is: by [%d",
	     loc_reg_stride[0]);
	for (i=1; i<numdims; i++) {
	  DBS1(SDL,",%d",loc_reg_stride[i]);
	}
	DBS0(SDL,"]\n");

	if (expr_contains_qreg(reg)) {
	  DBS0(SDL,INTIND"And in turn contains a quote region\n");
	  depth++;
	  covreg = RMSRegionAtDepth(depth);
	} else {
	  DBS0(SDL,INTIND"And turns out to be local\n");
	  /* turned out to be local after all */
	  reg_local = 1;
	  break;
	}
      }
    }
  } else {
    DBS0(SDL,INTIND"Covering region is locally known\n");
    DBS0(SDL,INTIND"Covering region is not multiregion\n");
    reg_local = 1;
  }

  DBS1(SDL,INTIND"Local region's stride is: by [%d",loc_reg_stride[0]);
  for (i=1; i<numdims; i++) {
    DBS1(SDL,",%d",loc_reg_stride[i]);
  }
  DBS0(SDL,"]\n");


  orig_dir_offset = dir_offset;

  if (arrpst != NULL) {
    arr_alias = S_ACTUALS(arrpst);
    callsite = T_CALLINFO(T_PARFCN(T_STMT(expr)));
    while (arr_alias != NULL) {
      arr_expr = G_EXPR(arr_alias);
      reg = COV_REG(CALL_COVER(callsite));

      if (reg == NULL) {
	DBS0(SDL,INTIND"Alias' covering region is unknown");
      } else if (expr_contains_qreg(reg)) {
	DBS0(SDL,INTIND"Alias is covered by inherited region?");
      } else {
	IFDB(SDL) {
	  DBS1(SDL,INTIND"Alias: [%s] ... ",S_IDENT(T_IDENT(reg)));
	  dbg_gen_expr(stdout,arr_expr);
	  DBS0(SDL,"\n");
	}

      }

      arr_alias = G_NEXT(arr_alias);
      callsite = CALL_NEXT(callsite);
    }
  }
  if (reg_local) {
    DBS0(SDL,INTIND"Copying local region stride in");
    for (i=0; i<numdims; i++) {
      reg_stride_ans[i] = loc_reg_stride[i];
    }
  } else {
    DBS0(SDL,INTIND"Scaling inherited region stride by local region stride");
    for (i=0; i<numdims; i++) {
      reg_stride_ans[i] *= loc_reg_stride[i];
    }
  }

  /* if neither are multi and we haven't been stumped it's a piece of cake */
  if (found_answer == 0) {
    found_answer = 1;
  }
  
  DBS0(SDL,"\n");

  if (found_answer == 1) {
    for (i=0; i<numdims; i++) {
      ret_regscale[i] = reg_stride_ans[i];
      ret_dirscale[i] = dir_val_ans[i];
    }
    return 0;
  } else {
    return -1;
  }
}

static int lastlineno = -1;
static int lineno = -1;
static symboltable_t* currfn;
static int teston;

static void test_scale_expr(expr_t* expr) {
  int retval;
  int regscale[MAXRANK];
  int dirval[MAXRANK];
  int i;

  IFDB(SUM) {
    if (teston) {
      if (expr && T_TYPE(expr) == BIAT) {
	if (currfn) {
	  printf("\n\nFN %s()\n",S_IDENT(currfn));
	  currfn = NULL;
	}
	if (lineno != lastlineno) {
	  printf("\n");
	  printf("  Stmt %d:\n", lineno);
	  printf("  --------------\n");
	  lastlineno = lineno;
	}
	
	for (i=1; i<MAXRANK; i++) {
	  regscale[i] = 0;
	  dirval[i] = 0;
	}
	retval = reg_dir_info(expr,regscale,dirval);
	
	printf("    ");
	dbg_gen_expr(stdout,expr);
	printf(": ");
	
	if (retval == -1) {
	  printf("unable to classify\n");
	} else {
	  
	  printf("[%d",regscale[0]);
	  for (i=1; i<MAXRANK; i++) {
	    printf(",%d",regscale[i]);
	  }
	  printf("]   ... @[%d",dirval[0]);
	  for (i=1; i<MAXRANK; i++) {
	    printf(",%d",dirval[i]);
	  }
	  printf("]\n");
	  fflush(stdout);
	}
      }
    }
  }
}


static void test_scale_stmt_pre(statement_t* stmt) {
  lineno = T_LINENO(stmt);
  if (T_TYPE(stmt) == S_COMM) {
    teston = 0;
  }
}


static void test_scale_stmt_post(statement_t* stmt) {
  if (T_TYPE(stmt) == S_EXPR) {
    traverse_exprls_g(T_EXPR(stmt),test_scale_expr,NULL);
  }
  teston = 1;
}


static void test_scale_module(module_t* mod) {
  function_t* fn;

  RMSInit();
  fn = T_FCNLS(mod);
  while (fn != NULL) {
    currfn = T_FCN(fn);
    traverse_stmtls_g(T_STLS(fn),test_scale_stmt_pre,test_scale_stmt_post,
		      test_scale_expr,NULL);
    fn = T_NEXT(fn);
  }
  RMSFinalize();
}


int call_test_scale(module_t* mod,char* flags) {
  traverse_modules(mod,TRUE,test_scale_module,NULL);

  return 0;
}
