/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/genlist.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/main.h"

/* globals */

static int currlev = 1;
static statement_t **Lastdo;
static statement_t* global_last_stmt;

static void fix_params(function_t *f) {
  genlist_t *glp;
  glp = T_PARAMLS(f);
  while (glp != NULL) {
    if (S_CLASS(T_TYPE(G_IDENT(glp))) == DT_ARRAY) {
	/*      S_PAR_CLASS(G_IDENT(glp)) = SC_REFER;*/ /* ignoring in REFER to INOUT change */
    }
    glp = G_NEXT(glp);
  }
}


static void fix_expr(expr_t *expr,expr_t *parent,statement_t *stmt,
		     expr_t *prev) {

  if (expr == NULL) {
    return;
  }

  T_STMT(expr) = stmt;
  T_PARENT(expr) = parent;
  T_PREV(expr) = prev;
  fix_expr(T_NEXT(expr),parent,stmt,expr);
  fix_expr(T_OPLS(expr),expr,stmt,NULL);
  switch T_TYPE(expr) {
  case PERMUTE:
    fix_expr(T_MAP(expr),NULL,stmt,NULL);
    if (T_SUBTYPE(expr) == P_PERMUTE) {
      
    }
    break;
  default:
    break;
  }
}


static void fix_scope(statement_t *stmtls,statement_t *scope,
		      statement_t *parent,function_t *parfcn) {
  statement_t	*prev;

  prev = NULL;
  while (stmtls != NULL) {
    T_PREV(stmtls) = prev;
 
    T_PARENT(stmtls) = scope;

    T_PARFCN(stmtls) = parfcn;
    fix_stmt(stmtls);
    prev = stmtls;
    stmtls = T_NEXT(stmtls);
  }
}


static void fix_if(if_t *ifstmt,statement_t *parent) {
  if (ifstmt == NULL) {
    INT_FATAL(NULL, "Null ifstmt in fix_if()");
  }
  
  fix_expr(T_IFCOND(ifstmt),NULL,parent,NULL);
  fix_stmtls(T_THEN(ifstmt),parent,T_PARFCN(parent));
  fix_stmtls(T_ELSE(ifstmt),parent,T_PARFCN(parent));
}


static void fix_loop(loop_t *loop,statement_t *parent) {
  if (loop == NULL) {
    INT_FATAL(NULL, "Null loop in fix_loop()");
  }
  
  switch (T_TYPE(loop)) {
  case L_DO:
    fix_expr(T_IVAR(loop),NULL,parent,NULL);
    fix_expr(T_START(loop),NULL,parent,NULL);
    fix_expr(T_STOP(loop),NULL,parent,NULL);
    fix_expr(T_STEP(loop),NULL,parent,NULL);
		
		
    *Lastdo = parent;
    Lastdo = &T_LNEXTDO(loop);
    break;
  case L_WHILE_DO:
  case L_REPEAT_UNTIL:
    fix_expr(T_LOOPCOND(loop),NULL,parent,NULL);
    break;
  default:
    INT_FATAL(NULL, "bad type (%d) in fix_loop",T_TYPE(loop));
  }
  currlev++;
  fix_stmtls(T_BODY(loop),parent,T_PARFCN(parent));
  currlev--;
}


void fix_stmt(statement_t *stmt) {
  comm_info_t *c;

  if (stmt == NULL)
    return;
  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_HALT:
  case S_CONTINUE:
  case S_ZPROF: /* MDD */
    break;
  case S_IO:
    DB0(30, "fixing up an IO stmt. \n");
    fix_expr(IO_EXPR1(T_IO(stmt)),NULL,stmt,NULL);
    break;
  case S_COMM:
    DB0(30, "fixing up a COMM stmt. \n");
    for (c = T_COMM_INFO(stmt); c != NULL; c = T_COMM_INFO_NEXT(c))
      fix_expr(T_COMM_INFO_ENS(c), NULL, stmt, NULL);
    break;
  case S_WRAP:
  case S_REFLECT:	
    DB0(30, "fixing up a WRAP/REFLECT stmt. \n");
    fix_expr(T_OPLS(T_WRAP(stmt)),NULL,stmt,NULL);
    break;
  case S_MLOOP:
    /*** sungeun ***/
    /*** Since mloops do not dictate control flow, ***/
    /*** currlev need not be tampered with ***/
    DB0(30, "fixing up a MLOOP stmt. \n");
    fix_scope(T_BODY(T_MLOOP(stmt)),stmt,T_PARENT(stmt),T_PARFCN(stmt));
    break;
  case S_NLOOP:
    /*** sungeun ***/
    /*** Since nloops do not dictate control flow, ***/
    /*** currlev need not be tampered with ***/
    DB0(30, "fixing up a NLOOP stmt. \n");
    fix_scope(T_BODY(T_NLOOP(stmt)),stmt,T_PARENT(stmt),T_PARFCN(stmt));
    break;
  case S_REGION_SCOPE:
    /*** sungeun ***/
    /*** Since region scopes do not dictate control flow, ***/
    /*** currlev need not be tampered with ***/
    DB0(30, "fixing up a REGION_SCOPE stmt. \n");
    fix_scope(T_BODY(T_REGION(stmt)),stmt,T_PARENT(stmt),T_PARFCN(stmt));
    break;
  case S_COMPOUND:
  case S_MSCAN:
    currlev++;
    DB0(30, "fixing up a COMPOUND stmt. \n");
    fix_stmtls(T_CMPD_STLS(stmt),stmt,T_PARFCN(stmt));
    currlev--;
    break;
  case S_EXPR:
    DB0(30, "fixing up an EXPR stmt. \n");
    fix_expr(T_EXPR(stmt),NULL,stmt,NULL);
    break;
  case S_IF:
    fix_if(T_IF(stmt),stmt);
    break;
  case S_LOOP:
    fix_loop(T_LOOP(stmt),stmt);
    break;
  case S_RETURN:
    fix_expr(T_RETURN(stmt),NULL,stmt,NULL);
    break;
  default:
    INT_FATAL(stmt, "Bad statement type (%d) in fix_stmt()",T_TYPE(stmt));
  }
}


void fix_stmtls(statement_t *stmtls,statement_t *parent, function_t *parfcn) {
  statement_t *prev;

  prev = NULL;
  while (stmtls != NULL) {
    T_PREV(stmtls) = prev;
    T_PARENT(stmtls) = parent;
    T_PARFCN(stmtls) = parfcn;
    fix_stmt(stmtls);
    prev = stmtls;
    stmtls = T_NEXT(stmtls);
  }
}


static void tmpfix(statement_t *s) {
  T_PREV_LEX(s) = global_last_stmt;
  global_last_stmt = s;
}


static void fix_lex(function_t *fcn) {
  statement_t *s;
  statement_t *next;
  

  if (fcn == NULL) {
    INT_FATAL(NULL, "fcn == NULL in fix_lex()");
  }

  global_last_stmt = NULL;

  traverse_stmtls(T_STLS(fcn),0,tmpfix,NULL,NULL);
  DB0(50,"past traverse_stmtls\n");
  
  if (global_last_stmt == NULL) {
    DB1(40,"No statements in function '%s'\n",S_IDENT(T_FCN(fcn)));
    return;
  }

  next = global_last_stmt;
  T_NEXT_LEX(next) = NULL;
  s = T_PREV_LEX(next);
  while (s != NULL) {
    T_NEXT_LEX(s) = next;
    next = s;
    s = T_PREV_LEX(next);
  }
}


static void fix_fcn(function_t *fcn,function_t *prevfcn) {
  currlev = 1;
  T_PREV(fcn) = prevfcn;
	
  fix_params(fcn);
  Lastdo = &T_FIRSTDO(fcn);
  fix_stmtls(T_STLS(fcn),NULL,fcn);
  *Lastdo = NULL;

  fix_lex(fcn);

  DB(30) {
    statement_t	*p;

    DBS0(30, "DO LOOPS\n");
    for (p = T_FIRSTDO(fcn); p; p = T_NEXTDO(p)) {
      DBS1(30, "Loop at line %d\n", T_LINENO(p));
      dbg_gen_stmt(stderr,p);
    }
    DBS0(30, "NO MORE DO LOOPS\n");
  }
}


static void fixup(module_t *module) {
  function_t *ftp;
  function_t *prev=NULL;

  DB0(20,"fixup()\n");


  for (; module != NULL; module = T_NEXT(module)) {
    ftp = T_FCNLS(module);
    if (ftp == NULL) {
      INT_FATAL(NULL, "No functions in module in fixup()");
    }
    while (ftp != NULL) {
      T_MODULE(ftp) = module;
      fix_fcn(ftp,prev);
      prev = ftp;
      ftp = T_NEXT(ftp);
    }
  }
}


int call_fixup(module_t *mod, char *s)
{
  fixup(mod);
  return 0;
}
