/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "../include/Repgen.h"
#include "../include/buildstmt.h"
#include "../include/expr.h"
#include "../include/macros.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"


void add_replacement(mloop_t* mloop,expr_t* origexpr,expr_t* newexpr) {
  replacement_t* newrepl;

  newrepl = (replacement_t*)PMALLOC(sizeof(replacement_t));
  newrepl->origexpr = origexpr;
  newrepl->newexpr = newexpr;
  newrepl->next = T_MLOOP_REPLS(mloop);
  T_MLOOP_REPLS(mloop) = newrepl;
}


int assignment_replace(mloop_t* mloop,expr_t* assignexpr) {
  expr_t* leftexpr;
  expr_t* rightexpr;
  int match = 0;
  replacement_t* repl;

  leftexpr = T_OPLS(assignexpr);
  rightexpr = T_NEXT(leftexpr);
  repl = T_MLOOP_REPLS(mloop);
  while ((repl != NULL) && (match==0)) {
    match |= (expr_equal(leftexpr,repl->newexpr) && 
	      expr_equal(rightexpr,repl->origexpr));
    match |= (expr_equal(rightexpr,repl->newexpr) &&
	      expr_equal(leftexpr,repl->origexpr));

    repl = repl->next;
  }
  return !match;
}


expr_t* find_replacement(mloop_t* mloop,expr_t* expr) {
  replacement_t* repl;

  repl = T_MLOOP_REPLS(mloop);
  while (repl != NULL) {
    if (expr_equal(expr,repl->origexpr)) {
      return repl->newexpr;
    }

    repl = repl->next;
  }
  return NULL;
}


void test_replacements(mloop_t* mloop) {
  /*
    add_replacement(mloop,
		    build_unary_at_op(build_0ary_op(VARIABLE,
						    lu_only("A")),NULL,AT_NORMAL),
		    build_0ary_op(VARIABLE,alloc_st(S_VARIABLE,"replaceA@NULL")));
  */
  /*
    add_replacement(mloop,
		    build_0ary_op(VARIABLE,lu_only("A")),
		    build_0ary_op(VARIABLE,alloc_st(S_VARIABLE,"replaceA")));
  */
  /*
    add_replacement(mloop,
                    build_0ary_op(CONSTANT,lu_only("0.5")),
		    build_unary_at_op(build_0ary_op(VARIABLE,
						    lu_only("A")),NULL,AT_NORMAL)
		    );
  */
  add_replacement(mloop,
		  build_0ary_op(CONSTANT,lu_only("0.5"))
		  ,
		  build_0ary_op(VARIABLE,alloc_st(S_VARIABLE,"replace0.5")));
  add_replacement(mloop,
		  build_unary_at_op(build_0ary_op(VARIABLE,
						  lu_only("B")),NULL,AT_NORMAL),
		  build_0ary_op(VARIABLE,alloc_st(S_VARIABLE,"replaceB@NULL")));
}


