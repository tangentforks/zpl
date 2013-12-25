/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/db.h"
#include "../include/error.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"

static void traverse_wrap_g(wrap_t *,void (*) (statement_t *),
			    void (*) (statement_t *),void (*) (expr_t *),
			    void (*) (symboltable_t *));
static void traverse_comm_g(comm_t *,void (*) (statement_t *),
			   void (*) (statement_t *),void (*) (expr_t *),
			   void (*) (symboltable_t *));
static void traverse_if_g(if_t *,void (*) (statement_t *),
			  void (*) (statement_t *),void (*) (expr_t *),
			  void (*) (symboltable_t *));
static void traverse_loop_g(loop_t *,void (*) (statement_t *),
			    void (*) (statement_t *),void (*) (expr_t *),
			    void (*) (symboltable_t *));
static void traverse_region(region_t *,int postorder,void (*) (statement_t *),
			    void (*) (expr_t *),void (*) (symboltable_t *));
static void traverse_region_g(region_t *,void (*) (statement_t *),
			      void (*) (statement_t *),void (*) (expr_t *),
			      void (*) (symboltable_t *));
static void traverse_mloop_g(mloop_t *,void (*) (statement_t *),
			     void (*) (statement_t *),void (*) (expr_t *),
			     void (*) (symboltable_t *));
static void traverse_nloop_g(nloop_t *,void (*) (statement_t *),
			     void (*) (statement_t *),void (*) (expr_t *),
			     void (*) (symboltable_t *));
static void traverse_wrap(wrap_t *,int,void (*) (statement_t *),
			  void (*) (expr_t *),void (*) (symboltable_t *));
static void traverse_comm(comm_t *,int,void (*) (statement_t *),
			  void (*) (expr_t *),void (*) (symboltable_t *));
static void traverse_if(if_t *,int,void (*) (statement_t *),
			void (*) (expr_t *),void (*) (symboltable_t *));
static void traverse_mloop(mloop_t *,int,void (*) (statement_t *),
			   void (*) (expr_t *),void (*) (symboltable_t *));
static void traverse_nloop(nloop_t *,int,void (*) (statement_t *),
			   void (*) (expr_t *),void (*) (symboltable_t *));
static void traverse_loop(loop_t *,int,void (*) (statement_t *),
			  void (*) (expr_t *),void (*) (symboltable_t *));




int traverse_modules(module_t *mod,int	postorder,void	(*mod_fcn)(module_t *),
		     void	(*f_fcn)(function_t *)) {
  module_t *modls;
  module_t *next;
  function_t	*f;

  DB1(30,"{traverse_modules(,%d,,)\n",postorder);
  modls = mod;
  while (modls != NULL) {
    if (!postorder && (mod_fcn != NULL))
      (*mod_fcn)(modls);
    f = T_FCNLS(modls);
    if (f_fcn != NULL)
      while (f != NULL) {
	(*f_fcn)(f);
	f = T_NEXT(f);
      }
    next = T_NEXT(modls);
    if (postorder && (mod_fcn != NULL))
      (*mod_fcn)(modls);
    modls = next;
  }
  DB1(40,"}traverse_modules(,%d,,)\n",postorder);
  return 0;
}


int
traverse_functions(int (*s_fcn)(function_t *)) {
  function_t *f;

  if (pstMAIN == NULL) {
    INT_FATAL(NULL, "pstMAIN is NULL.");
  }

  for (f = S_FUN_BODY(pstMAIN); f != NULL; f = T_NEXT(f)) {
    (*s_fcn)(f);
  }

  return 0;
}


void traverse_stmtls(statement_t *stmtls,int postorder,
		     void (*f) (statement_t *),void (*e) (expr_t *),
		     void (*d) (symboltable_t *)) {
  statement_t	*next;

  DB1(50,"{traverse_stmtls(,%d,,,)\n",postorder);

  for (; stmtls != NULL; stmtls = next) {
    next = T_NEXT(stmtls);
    traverse_stmt(stmtls,postorder,f,e,d);
  }
  DB1(55,"}returning from traverse_stmtls(,%d,,,)\n",postorder);
}


void traverse_stmt(statement_t *stmt,int postorder,void (*f) (statement_t *),
		   void (*e) (expr_t *),void (*d) (symboltable_t *)) {
  DB1(60,"traverse_stmt(,%d,,,)\n",postorder);

  if (stmt == NULL) {
    return;
  }
  if ((!postorder) && (f != NULL))
    (*f)(stmt);
  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_HALT:
  case S_CONTINUE:
  case S_ZPROF: /* MDD */
    break;
  case S_WRAP:
  case S_REFLECT:
    traverse_wrap(T_WRAP(stmt), postorder, f, e, d);
    break;
  case S_COMM:
    traverse_comm(T_COMM(stmt), postorder, f, e, d);
    break;
  case S_REGION_SCOPE:
    RMSPushScope(T_REGION(stmt));
    traverse_region(T_REGION(stmt),postorder,f,e,d);
    RMSPopScope(T_REGION(stmt));
    break;
  case S_MLOOP:
    traverse_mloop(T_MLOOP(stmt),postorder,f,e,d);
    break;
  case S_NLOOP:
    traverse_nloop(T_NLOOP(stmt),postorder,f,e,d);
    break;
  case S_COMPOUND:
  case S_MSCAN:
    if (d != NULL)
      (*d)(T_CMPD_DECL(stmt));
    traverse_stmtls(T_CMPD_STLS(stmt),postorder,f,e,d);
    break;
  case S_EXPR:
    if (e != NULL)
      (*e)(T_EXPR(stmt));
    break;
  case S_IF:
    traverse_if(T_IF(stmt),postorder,f,e,d);
    break;
  case S_LOOP:
    traverse_loop(T_LOOP(stmt),postorder,f,e,d);
    break;
  case S_RETURN:
    if (e != NULL && T_RETURN(stmt) != NULL)
      (*e)(T_RETURN(stmt));
    break;
  case S_IO:
    if (e != NULL)
      (*e)(IO_EXPR1(T_IO(stmt)));
    break;
  default:
    DB1(70,"Type of structure == %d\n",ST_TYPE(stmt));
    INT_FATAL(stmt, "Bad statementtype (%d) in traverse_stmt()",T_TYPE(stmt));
  }
  if ((postorder) && (f != NULL))
    (*f)(stmt);
}



/******  New routines between these comments **********/

/*  Don't apply the functions here...  They will be applied in the call
    to traverse_stmt_g.  This function is a general traversal routine
    that allows functions to be applied before (pre) and after (post)
    a statement (pre_s, post_s) or an expression (pre_e, post_e) is
    visited.  It is more general than the traverse_stmtls() call.
    It works in part with traverse_stmt_g() call.  The _g signifies
    genericity.
    */

void traverse_stmtls_g(statement_t *stmtls,void (*pre_s) (statement_t *),
		       void (*post_s) (statement_t *),void (*e) (expr_t *),
		       void (*d) (symboltable_t *)) {
  statement_t	*next;

  DB0(50,"{traverse_stmtls_g(,,,,)\n");

  for (; stmtls != NULL; stmtls = next) {
    next = T_NEXT(stmtls);
    traverse_stmt_g(stmtls,pre_s,post_s,e,d);
  }
  DB0(55,"}returning from traverse_stmtls_g(,,,,)\n");
}


void traverse_stmt_g(statement_t *stmt,void (*pre_s) (statement_t *),
		     void (*post_s) (statement_t *),void (*e) (expr_t *),
		     void (*d) (symboltable_t *)) {
  DB0(60,"traverse_stmt_g(,,,,)\n");

  if (stmt == NULL) {
    return;
  }
  if (pre_s)
    (*pre_s)(stmt);
  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_HALT:
  case S_CONTINUE:
  case S_ZPROF: /* MDD */
    break;
  case S_WRAP:
  case S_REFLECT:
    traverse_wrap_g(T_WRAP(stmt), pre_s, post_s, e, d);
    break;
  case S_COMM:
    traverse_comm_g(T_COMM(stmt), pre_s, post_s, e, d);
    break;
  case S_REGION_SCOPE:
    RMSPushScope(T_REGION(stmt));
    traverse_region_g(T_REGION(stmt),pre_s,post_s,e,d);
    RMSPopScope(T_REGION(stmt));
    break;
  case S_MLOOP:
    traverse_mloop_g(T_MLOOP(stmt),pre_s,post_s,e,d);
    break;
  case S_NLOOP:
    traverse_nloop_g(T_NLOOP(stmt),pre_s,post_s,e,d);
    break;
  case S_COMPOUND:
  case S_MSCAN:
    if (d != NULL)
      (*d)(T_CMPD_DECL(stmt));
    traverse_stmtls_g(T_CMPD_STLS(stmt),pre_s,post_s,e,d);
    break;
  case S_EXPR:
    if (e != NULL)
      (*e)(T_EXPR(stmt));
    break;
  case S_IF:
    traverse_if_g(T_IF(stmt),pre_s,post_s,e,d);
    break;
  case S_LOOP:
    traverse_loop_g(T_LOOP(stmt),pre_s,post_s,e,d);
    break;
  case S_RETURN:
    if (e != NULL && T_RETURN(stmt) != NULL)
      (*e)(T_RETURN(stmt));
    break;
  case S_IO:
    if (e != NULL)
      (*e)(IO_EXPR1(T_IO(stmt)));
    break;
  default:
    DB1(70,"Type of structure == %d\n",ST_TYPE(stmt));
    INT_FATAL(stmt, "Bad statementtype (%d) in traverse_stmt_g()",T_TYPE(stmt));
  }
  if (post_s)
    (*post_s)(stmt);
}


void traverse_exprls_g(expr_t *exprls,void (*pre_e) (expr_t *),
		  void (*post_e) (expr_t *)) {
  expr_t *next;

  DB0(70,"{traverse_exprls_g(,,);\n");

  if (pre_e == NULL && post_e == NULL) {
    DB0(75,"}returning from traverse_exprls_g(,,);\n");
    return;
  }
  for (; exprls != NULL; exprls = next) {
    next = T_NEXT(exprls);
    traverse_expr_g(exprls,pre_e,post_e);
  }
  DB0(75,"}returning from traverse_exprls_g(,,);\n");
}


void traverse_expr_g(expr_t *expr,void (*pre_e) (expr_t *),
		     void (*post_e) (expr_t *)) {
  DB0(70,"traverse_expr_g(,,);\n");

  if ((expr == NULL) || ((pre_e == NULL) && (post_e == NULL))) {
    return;
  }

  if (pre_e != NULL)
    (*pre_e)(expr);
  traverse_exprls_g(T_OPLS(expr),pre_e,post_e);
  if (post_e != NULL)
    (*post_e)(expr);
}


static void traverse_wrap_g(wrap_t *wrapstmt,void (*pre_s) (statement_t *),
			    void (*post_s) (statement_t *),void (*e) (expr_t *),
			    void (*d) (symboltable_t *)) {
  expr_t *expr;

  if (e != NULL)
    for (expr = T_OPLS(wrapstmt); expr != NULL; expr = T_NEXT(expr))
      (*e)(expr);
}


static void traverse_comm_g(comm_t *commstmt,void (*pre_s) (statement_t *),
			    void (*post_s) (statement_t *),void (*e) (expr_t *),
			    void (*d) (symboltable_t *)) {
  comm_info_t *c;

  if (e != NULL)
    for (c = T_COMM_INFO_STRUCT(commstmt); c != NULL; c = T_COMM_INFO_NEXT(c))
      (*e)(T_COMM_INFO_ENS(c));
}


static void traverse_if_g(if_t *ifstmt,void (*pre_s) (statement_t *),
			  void (*post_s) (statement_t *),void (*e) (expr_t *),
			  void (*d) (symboltable_t *)) {
  DB0(80,"{traverse_if_g(,,,,);\n");

  if (ifstmt == NULL) {
    INT_FATAL(NULL, "Null ifstmt in traverse_if_g()");
  }
  if (e != NULL) {
    (*e)(T_IFCOND(ifstmt));
  }
  traverse_stmtls_g(T_THEN(ifstmt),pre_s,post_s,e,d);
  traverse_stmtls_g(T_ELSE(ifstmt),pre_s,post_s,e,d);
  DB0(85,"}returning from traverse_if_g();\n");
}


static void traverse_loop_g(loop_t *loop,void (*pre_s) (statement_t *),
			    void (*post_s) (statement_t *),void (*e) (expr_t *),
			    void (*d) (symboltable_t *)) {
  DB0(80,"traverse_loop_g(,,,,);\n");

  if (loop == NULL) {
    INT_FATAL(NULL, "Null loop in traverse_loop_g()");
    return;
  }

  switch (T_TYPE(loop)) {
  case L_DO:
    if (e != NULL) {
      (*e)(T_IVAR(loop));
      (*e)(T_START(loop));
      (*e)(T_STOP(loop));
      if (T_STEP(loop) != NULL) {
	(*e)(T_STEP(loop));
      }
    }
    traverse_stmtls_g(T_BODY(loop),pre_s,post_s,e,d);
    break;
  case L_WHILE_DO:
  case L_REPEAT_UNTIL:
    if (e != NULL) {
      (*e)(T_LOOPCOND(loop));
    }
    traverse_stmtls_g(T_BODY(loop),pre_s,post_s,e,d);
    break;
  default:
    INT_FATAL(NULL, "Bad looptype (%d) in traverse_loop_g()",T_TYPE(loop));
  }
#ifdef DEBUG
  fflush(stdout);
#endif
}


static void traverse_region(region_t *region,int postorder,
			    void (*f) (statement_t *),void (*e) (expr_t *),
			    void (*d) (symboltable_t *)) {
  DB1(80,"traverse_region(,%d,,,);\n",postorder);

  if (region == NULL) {
    INT_FATAL(NULL, "Null region in traverse_region()");
  }
  if (e) (*e)(T_MASK_EXPR(region));
  traverse_stmtls(T_BODY(region),postorder,f,e,d);
}


static void traverse_region_g(region_t *region,void (*pre_s) (statement_t *),
			      void (*post_s) (statement_t *),
			      void (*e) (expr_t *),
			      void (*d) (symboltable_t *)) {
  DB0(80,"traverse_region_g(,,,,);\n");

  if (region == NULL) {
    INT_FATAL(NULL, "Null region in traverse_region_g()");
  }
  if (e) (*e)(T_MASK_EXPR(region));
  traverse_stmtls_g(T_BODY(region),pre_s,post_s,e,d);
}


static void traverse_mloop_g(mloop_t *mloop,void (*pre_s) (statement_t *),
			     void (*post_s) (statement_t *),
			     void (*e) (expr_t *),void (*d) (symboltable_t *)) {
  DB0(80,"traverse_mloop_g(,,,,);\n");

  if (mloop == NULL) {
    INT_FATAL(NULL, "Null loop in traverse_mloop_g()");
  }
  traverse_stmtls_g(T_BODY(mloop),pre_s,post_s,e,d);
}


static void traverse_nloop_g(nloop_t *nloop,void (*pre_s) (statement_t *),
			     void (*post_s) (statement_t *),
			     void (*e) (expr_t *),void (*d) (symboltable_t *)) {
  DB0(80,"traverse_nloop_g(,,,,);\n");

  if (nloop == NULL) {
    INT_FATAL(NULL, "Null loop in traverse_nloop_g()");
  }
  traverse_stmtls_g(T_NLOOP_BODY(nloop),pre_s,post_s,e,d);
}

/******  New routines between these comments **********/


void traverse_exprls(expr_t *exprls,int postorder,void (*e) (expr_t *)) {
  expr_t *next;

  DB1(70,"{traverse_exprls(,%d,);\n",postorder);

  if (e == NULL) {
    DB1(75,"}returning from traverse_exprls(,%d,);\n",postorder);
    return;
  }
  for (; exprls != NULL; exprls = next) {
    next = T_NEXT(exprls);
    traverse_expr(exprls,postorder,e);
  }
  DB1(75,"}returning from traverse_exprls(,%d,);\n",postorder);
}


void traverse_expr(expr_t *expr,int postorder,void (*e) (expr_t *)) {
  DB1(70,"traverse_expr(,%d,);\n",postorder);

  if ((expr == NULL) || (e == NULL)) {
    return;
  }

  if (!postorder)
    (*e)(expr);
  traverse_exprls(T_OPLS(expr),postorder,e);
  if (postorder)
    (*e)(expr);
}


static void traverse_wrap(wrap_t *wrapstmt,int postorder,
			  void (*f) (statement_t *),void (*e) (expr_t *),
			  void (*d) (symboltable_t *)) {
  expr_t *expr;

  if (e != NULL)
    for (expr = T_OPLS(wrapstmt); expr != NULL; expr = T_NEXT(expr))
      (*e)(expr);
}


/*** this looks just like traverse_comm_g() ***/
/*** I don't know what the difference is ***/
/*** what's with paramenters postorder, f() and d()? ***/

static void traverse_comm(comm_t *commstmt,int postorder,
			 void (*f) (statement_t *),void (*e) (expr_t *),
			 void (*d) (symboltable_t *)) {
  comm_info_t *c;

  if (e != NULL)
    for (c = T_COMM_INFO_STRUCT(commstmt); c != NULL; c = T_COMM_INFO_NEXT(c))
      (*e)(T_COMM_INFO_ENS(c));

}


static void traverse_if(if_t *ifstmt,int postorder,void (*f) (statement_t *),
			void (*e) (expr_t *),void (*d) (symboltable_t *)) {
  DB1(80,"{traverse_if(,%d,,,);\n",postorder);

  if (ifstmt == NULL) {
    INT_FATAL(NULL, "Null ifstmt in traverse_if()");
  }
  if (e != NULL) {
    (*e)(T_IFCOND(ifstmt));
  }
  traverse_stmtls(T_THEN(ifstmt),postorder,f,e,d);
  traverse_stmtls(T_ELSE(ifstmt),postorder,f,e,d);
  DB0(85,"}returning from traverse_if();\n");
}


static void traverse_mloop(mloop_t *mloop,int postorder,
			   void (*f) (statement_t *),void (*e) (expr_t *),
			   void (*d) (symboltable_t *)) {
  DB1(80,"traverse_mloop(,%d,,,);\n",postorder);

  if (mloop == NULL) {
    INT_FATAL(NULL, "Null loop in traverse_mloop()");
  }
  traverse_stmtls(T_BODY(mloop),postorder,f,e,d);
}


static void traverse_nloop(nloop_t *nloop,int postorder,
			   void (*f) (statement_t *),void (*e) (expr_t *),
			   void (*d) (symboltable_t *)) {
  DB1(80,"traverse_nloop(,%d,,,);\n",postorder);

  if (nloop == NULL) {
    INT_FATAL(NULL, "Null loop in traverse_nloop()");
  }
  traverse_stmtls(T_NLOOP_BODY(nloop),postorder,f,e,d);
}


static void traverse_loop(loop_t *loop,int postorder,void (*f) (statement_t *),
			  void (*e) (expr_t *),void (*d) (symboltable_t *)) {
  DB1(80,"traverse_loop(,%d,,,);\n",postorder);

  if (loop == NULL) {
    INT_FATAL(NULL,"Null loop in traverse_loop()");
  }

  switch (T_TYPE(loop)) {
  case L_DO:
    if (e != NULL) {
      (*e)(T_IVAR(loop));
      (*e)(T_START(loop));
      (*e)(T_STOP(loop));
      if (T_STEP(loop) != NULL) {
	(*e)(T_STEP(loop));
      }
    }
    traverse_stmtls(T_BODY(loop),postorder,f,e,d);
    break;
  case L_WHILE_DO:
  case L_REPEAT_UNTIL:
    if (e != NULL) {
      (*e)(T_LOOPCOND(loop));
    }
    traverse_stmtls(T_BODY(loop),postorder,f,e,d);
    break;
  default:
    INT_FATAL(NULL, "Bad looptype (%d) in traverse_loop()",T_TYPE(loop));
  }
#ifdef DEBUG
  fflush(stdout);
#endif
}


