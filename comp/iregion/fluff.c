/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/allocstmt.h"
#include "../include/buildstmt.h"
#include "../include/buildsym.h"
#include "../include/datatype.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/passes.h"
#include "../include/runtime.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/traverse.h"
#include "../include/treemac.h"

static void extend_pst_core(symboltable_t* pst, expr_t* dir, attype_t attype) {
  int i;
  int comp;
  symboltable_t *dirpst;
  int rank;
  long val;
  int success;

  dirpst = expr_find_root_pst(dir);
  rank = D_DIR_NUM(T_TYPEINFO(dir));

  IFDB(1) {
    printf("Extending %s's fluff to the %s\n",S_IDENT(pst),S_IDENT(dirpst));
  }

  for (i=0; i<rank; i++) {
    val = dir_comp_to_int(dir, i, &success);
    if (!success) {
      if (rank > 0 && S_UNK_FLUFF_LO(pst, 0) == 0) {
	if (0) {
	  USR_WARN(T_STMT(dir), 
		   "Making an assumption that fluff halo of 1 is sufficient "
		   "for %s", S_IDENT(pst));
	}
      }
      S_UNK_FLUFF_LO(pst, i) = 1;
      S_UNK_FLUFF_HI(pst, i) = 1;
      if (attype == AT_WRAP) {
	S_WRAPFLUFF_LO(pst, i) = 1;
	S_WRAPFLUFF_HI(pst, i) = 1;
      }
    } else {
      comp = expr_intval(expr_direction_value(IN_VAL(S_VAR_INIT(dirpst)),i));
      if (comp < 0) {
	comp = -comp;
	if (comp > S_FLUFF_LO(pst,i)) {
	  S_FLUFF_LO(pst,i) = comp;
	}
	if (attype == AT_WRAP) {
	  S_WRAPFLUFF_LO(pst,i) = 1;
	}
	comp = -comp;
      } else if (comp > 0) {
	if (comp > S_FLUFF_HI(pst,i)) {
	  S_FLUFF_HI(pst,i) = comp;
	}
	if (attype == AT_WRAP) {
	  S_WRAPFLUFF_HI(pst,i) = 1;
	}
      }
    }
  }
}

static void extend_pst(symboltable_t *pst, expr_t *dir, attype_t attype) {
  datatype_t *pdt;
  expr_t *reg;

  pdt = S_DTYPE(pst);
  pdt = datatype_find_ensemble(pdt);
  reg = D_ENS_REG(pdt);
  if (!expr_is_dense_reg(reg)) { /* store sparse fluff with region, not arr */
    extend_pst_core(expr_find_root_pst(reg),dir,attype);

    /* We also need to propagate fluff info down through all preceding
       sparse regions so that when we set up this one's fluff, we can
       iterate over the previous sparse region's fluff.
       The alternative would be to iterate over the shallowest dense
       region when setting up fluff, but this seems like overkill.  
    */
    reg = T_OPLS(reg);
    while (reg && !expr_is_dense_reg(reg)) {
      extend_pst_core(expr_find_root_pst(reg),dir,attype);
      reg = T_OPLS(reg);
    }
  } else {
    extend_pst_core(pst,dir,attype);
  }
}


static void extend_storage(expr_t *expr,expr_t *dir,attype_t attype) {
  symboltable_t *pst;
  genlist_t *aliases;
  expr_t *iexpr;

  IFDB(2) {
    printf("Wanting to extend ");
    dbg_gen_expr(stdout,expr);
    printf("'s fluff to the %s\n",S_IDENT(expr_find_root_pst(dir)));
  }
  pst = expr_find_ens_pst(expr);
  if (pst == NULL) {
    INT_FATAL(NULL,"l-value misassumption");
  }
  if (S_CLASS(pst) == S_PARAMETER) {
    aliases = S_ACTUALS(pst);
    while (aliases != NULL) {
      pst = expr_find_ens_pst(G_EXPR(aliases));
      while (T_TYPE(expr) != ARRAY_REF &&
	     T_TYPE(expr) != VARIABLE) {
	expr = T_OPLS(expr);
      }
      if (T_TYPE(expr) == ARRAY_REF) {
	iexpr = T_NEXT(T_OPLS(expr));
      } else {
	iexpr = NULL;
      }
      extend_pst(pst, dir, attype);

      aliases = G_NEXT(aliases);
    }
  } else {
    extend_pst(pst,dir,attype);
  }
}


static void fluff_uat(expr_t *expr) {
  expr_t *dir;
  attype_t attype;

  if (T_TYPE(expr) == BIAT) {
    dir = T_NEXT(T_OPLS(expr));
    attype = (attype_t)T_SUBTYPE(expr);
    extend_storage(T_OPLS(expr),dir,attype);
  }
}


static void fluff_expr(expr_t *expr) {
  traverse_expr(expr,FALSE,fluff_uat);
}


static void fluff_module(module_t *mod) {
  function_t *fn;

  fn = T_FCNLS(mod);
  while (fn != NULL) {
    traverse_stmtls_g(T_STLS(fn),NULL,NULL,fluff_expr,NULL);
    fn = T_NEXT(fn);
  }
}


int call_fluff(module_t *mod,char *s) {
  traverse_modules(mod,TRUE,fluff_module,NULL);
  return 0;
}

