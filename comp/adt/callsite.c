/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/callsite.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/parsetree.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symmac.h"
#include "../include/treemac.h"

callsite_t *new_callsite(cover_t cover,expr_t **actuals,int numactuals) {
  int i;
  callsite_t *new;
  genlist_t *actuallist;

  new = malloc(sizeof(callsite_t));
  COV_REG(CALL_COVER(new)) = COV_REG(cover);
  COV_MSK(CALL_COVER(new)) = COV_MSK(cover);
  COV_WTH(CALL_COVER(new)) = COV_WTH(cover);
  actuallist = malloc(numactuals*sizeof(genlist_t));
  for (i=0;i<numactuals;i++) {
    G_EXPR(&(actuallist[i])) = copy_expr(actuals[i]);
    G_NEXT(&(actuallist[i])) = NULL;
  }
  CALL_ACTUALS(new) = actuallist;
  CALL_NEXT(new) = NULL;
  return new;
}


int match_callsite(callsite_t *node,cover_t cover,expr_t **aliases,
		   int numaliases) {
  int i;
  expr_t **newptr;
  genlist_t *oldptr;

  if (COV_REG(cover) != COV_REG(CALL_COVER(node))) {
    DBS0(3,"region didn't match\n");
    return 0;
  }
  if (!expr_equal(COV_MSK(cover),COV_MSK(CALL_COVER(node)))) {
    DBS0(3,"mask didn't match\n");
    return 0;
  }
  if ((COV_MSK(cover) != NULL) && 
      (COV_WTH(cover) != COV_WTH(CALL_COVER(node)))) {
    DBS0(3,"withs didn't match\n");
    return 0;
  }
  newptr = aliases;
  oldptr = CALL_ACTUALS(node);
  for (i=0;i<numaliases;i++) {
    if (!expr_equal(newptr[i],G_EXPR(&(oldptr[i])))) {
      DBS1(3,"arg %d didn't match:\n",i+1);
      IFDB(4) {
	dbg_gen_expr(stdout,newptr[i]);
	printf(" != ");
	dbg_gen_expr(stdout,G_EXPR(&(oldptr[i])));
	printf("\n");
      }
      return 0;
    }
  }
  return 1;
}


#if (!RELEASE_VER)
void print_callsite(function_t *fn,callsite_t *callinfo) {
  expr_t *reg;
  expr_t *mask;
  int with;
  genlist_t *actuals;
  int i;

  reg = COV_REG(CALL_COVER(callinfo));
  mask = COV_MSK(CALL_COVER(callinfo));
  with = COV_WTH(CALL_COVER(callinfo));
  if (reg != NULL || mask != NULL) {
    printf("[");
    if (reg != NULL) {
      printf("%s",S_IDENT(T_IDENT(reg)));
    }
    if (mask != NULL) {
      if (with) {
	printf(" with ");
      } else {
	printf(" without ");
      }
      dbg_gen_expr(stdout,mask);
    }
    printf("] ");
  }
  printf("%s(",S_IDENT(T_FCN(fn)));
  actuals = CALL_ACTUALS(callinfo);
  for (i=0;i<T_NUM_PARAMS(fn);i++) {
    if (i) {
      printf(",");
    }
    dbg_gen_expr(stdout,G_EXPR(&(actuals[i])));
  }
  printf(");\n");
}
#endif
