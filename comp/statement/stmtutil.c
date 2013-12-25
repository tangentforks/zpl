/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../include/error.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/global.h"
#include "../include/stmtutil.h"
#include "../include/buildsymutil.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/allocstmt.h"
#include "../include/macros.h"
#include "../include/callgraph.h" /* MDD */
#include "../include/db.h"
#include "../include/genlist.h"
#include "../include/traverse.h"
#include "../include/runtime.h"
#include "../include/buildstmt.h"
#include "../include/buildsym.h"
#include "../include/expr.h"
#include "../include/set.h"
#include "../include/setmac.h"

#ifdef DEBUG
#define DBLVL	21
#endif

static statement_t *insert_stmt(statement_t*, statement_t*);
static statement_t *copy_if_stmt(statement_t *);
static statement_t *copy_loop_stmt(statement_t *);
static statement_t *copy_region_stmt(statement_t *);
static statement_t *copy_wrap_stmt(statement_t *);
static statement_t *copy_mloop_stmt(statement_t *);
static statement_t *copy_nloop_stmt(statement_t *);

static statement_t *New_stmt, *Old_stmt; /* MDD */

/* FN: first_stmt_in_stmtls - given a statement s, return the first
 *                            statement in the list that contains s
 * 11-24-99 echris
 */

statement_t *first_stmt_in_stmtls(statement_t *s)
{
  if (!s) return (NULL);

  while(T_PREV(s)) {
    s = T_PREV(s);
  }

  return (s);
}

/* FN: last_stmt_in_stmtls - given a statement s, return the last
 *                           statement in the list that contains s
 * 11-24-99 echris
 */

statement_t *last_stmt_in_stmtls(statement_t *s)
{
  if (!s) return (NULL);

  while(T_NEXT(s)) {
    s = T_NEXT(s);
  }

  return (s);
}

statement_t *cat_stmt_ls(statement_t *stmtls,statement_t *stmt) {
  DB0(40,"cat_stmt_ls(,);\n");

  if (stmt == NULL) {
    return stmtls;
  } else if (stmtls == NULL) {
    return stmt;
  } else {
    statement_t	*last = last_stmt_in_stmtls(stmtls);
    
    T_NEXT(last) = stmt;
    T_PREV(stmt) = last;
    return stmtls;
  }
}

expr_t *cat_expr_ls(expr_t *ls,expr_t *e) {
  expr_t *prev;

  DB0(30,"cat_expr_ls(,);\n");

  if (e == NULL) {
    return ls;
  }
  if (ls == NULL) {
    return e;
  } else {
    for ( prev = ls; T_NEXT(prev) != NULL; prev = T_NEXT(prev)) {
    }
    T_NEXT(prev) = e;
    T_PREV(e) = prev;
    return ls;
  }
}

statement_t* reverse_stmtls (statement_t* stmtls)
{
  statement_t* tmp;

  if (stmtls == NULL) {
    INT_FATAL (NULL, "null stmtls passed to reverse_stmtls");
  }

  tmp = stmtls;
  if (T_NEXT(stmtls) != NULL) {
    tmp = reverse_stmtls (T_NEXT(stmtls));
    T_NEXT(stmtls) = NULL;
    T_PREV(tmp) = NULL;
    stmtls = cat_stmt_ls (tmp, stmtls);
  }
  return tmp;
}

/* cat_io_expr()-- catenate a control string and an expression */

expr_t	*cat_io_expr(expr_t *string,expr_t *e) {
  expr_t *prev;

  if (string == NULL) {
    INT_FATAL( NULL, "NULL expression in cat_io_expr(): arg1");
  }
  
  if (e == NULL) {
    DB0(1, "NULL expression in cat_io_expr(): arg2\n");
  }
  
  T_FLAG1(string) = TRUE;	/* Mark this expression as a Control String */
  for ( prev = string; T_NEXT(prev) != NULL; prev = T_NEXT(prev)) {
  }
  T_NEXT(prev) = e;
  T_PREV(e) = prev;
  
  return string;
}


function_t *cat_fcn_ls(function_t *ls,function_t *f) {
  function_t *prev;

  if (f == NULL) {
    return ls;
  } else if (ls == NULL) {
    return f;
  } else {
    for ( prev = ls; T_NEXT(prev) != NULL; prev = T_NEXT(prev)) {
    }
    T_NEXT(prev) = f;
    T_PREV(f) = prev;
    return ls;
  }
}


symboltable_t	*cat_symtab_ls(symboltable_t *ls,symboltable_t *s) {
  symboltable_t	*prev;

  if (ls == NULL) {
    return s;
  } else if (s == NULL) {
    return ls;
  } else {
    for ( prev = ls; S_SIBLING(prev) != NULL; prev = S_SIBLING(prev)) {
    }
    S_SIBLING(prev) = s;
    S_SIBPREV(s) = prev;
    return ls;
  }
}


genlist_t *cat_genlist_ls(genlist_t *ls,genlist_t *s) {
  genlist_t *prev;

  if (ls == NULL) {
    return s;
  } else if (s == NULL) {
    return ls;
  } else {
    prev = ls;
    while (G_NEXT(prev) != NULL) {
      prev = G_NEXT(prev); 
    }
    G_NEXT(prev) = s;
    return ls;
  }
}

int numoperands(expr_t *expr) {
  int i = 0;

  for (expr = T_OPLS(expr); expr != NULL; expr = T_NEXT(expr)) {
    i++;
  }

  return i;
}


expr_t	*nthoperand(expr_t *expr,int n) {
  for (expr = T_OPLS(expr); (expr != NULL) && (n>1); n--) {
    expr = T_NEXT(expr);
  }
  if (n > 1) {
    INT_FATAL(NULL, "Not enough operands in expr in nthoperand()");
  }
	
  return expr;
}


expr_t *left_expr(expr_t *expr) {
  return nthoperand(expr,1);
}


expr_t	*right_expr(expr_t *expr) {
  return nthoperand(expr,2);
}


/*
 *  inserts statement list "sls" before statement "stmt"
 */
void insertbefore_stmt(statement_t* sls, statement_t* stmt)
{
  insert_stmt(sls, stmt);
}


/*
 *  inserts statement list "sls" after statement "stmt"
 */
void insertafter_stmt(statement_t* sls, statement_t* stmt)
{
  insert_stmt(sls, stmt);
  remove_stmt(stmt);
  insert_stmt(stmt, sls);
}


/*
 *  inserts compound statement cs1 into compound statement cs2
 */
void insert_cmpd_stmt(statement_t* cs1, statement_t* cs2) {
  symboltable_t* pst;

  if (T_TYPE(cs1) != S_COMPOUND) {
    INT_FATAL(NULL, "Compound statement expected in insert_cmpd_stmt");
  }
  if (T_TYPE(cs2) != S_COMPOUND) {
    INT_FATAL(NULL, "Compound statement expected in insert_cmpd_stmt");
  }

  pst = T_CMPD_DECL(cs1);
  while (pst && S_SIBLING(pst)) {
    pst = S_SIBLING(pst);
  }
  if (pst) {
    S_SIBLING(pst) = T_CMPD_DECL(cs2);
    T_CMPD_DECL(cs2) = T_CMPD_DECL(cs1);
    T_CMPD_DECL(cs1) = NULL;
  }
  insertbefore_stmt(copy_stmtls(T_CMPD_STLS(cs1)), T_CMPD_STLS(cs2));
}


/*
 *  inserts statement list "sls" before statement "stmt"
 */
static statement_t *insert_stmt(statement_t* sls, statement_t* stmt)
{
  statement_t *last;
  statement_t *p;

  DB0(DBLVL,"insert_stmt(,)\n");

  if (sls == NULL) {
    INT_FATAL(sls, "insert_stmt: attempt to insert null statement list");
  }
  if (stmt == NULL) {
    INT_FATAL(sls, "insert_stmt: attempt to insert next to null statement");
  }
  
  p = sls;
  do {
    T_PARENT(p) = T_PARENT(stmt);
    T_PARFCN(p) = T_PARFCN(stmt);
    last = p;
    p = T_NEXT(p);
  } while (p != NULL);
  T_NEXT(last) = stmt;
  T_NEXT_LEX(last) = stmt;
  T_PREV(sls) = T_PREV(stmt);
  T_PREV_LEX(sls) = T_PREV_LEX(stmt);
  T_PREV(stmt) = last;
  T_PREV_LEX(stmt) = last;
  if (T_PREV(sls) != NULL) {
    T_NEXT(T_PREV(sls)) = sls;
  } else if (T_PARENT(stmt)) {
    p = T_PARENT(stmt);
    switch (T_TYPE(p)) {
    case S_REGION_SCOPE:
      if (stmt == T_BODY(T_REGION(p))) {
	T_BODY(T_REGION(p)) = sls;
      }
      break;
    case S_MLOOP:
      if (stmt == T_BODY(T_MLOOP(p))) {
	T_BODY(T_MLOOP(p)) = sls;
      }
      break;
    case S_NLOOP:		/*** sungeun ***/
      if (stmt == T_NLOOP_BODY(T_NLOOP(p))) {
	T_NLOOP_BODY(T_NLOOP(p)) = sls;
      }
      break;
    case S_MSCAN:
    case S_COMPOUND:
      if (stmt == T_CMPD_STLS(p)) {
	T_CMPD_STLS(p) = sls;
      }
      break;
    case S_IF:
      if (stmt == T_THEN(T_IF(p))) {
	T_THEN(T_IF(p)) = sls;
      }
      else if (stmt == T_ELSE(T_IF(p))) {
	T_ELSE(T_IF(p)) = sls;
      }
      break;
    case S_LOOP:
      if (stmt == T_BODY(T_LOOP(p))) {
	T_BODY(T_LOOP(p)) = sls;
      }
      else if (T_TYPE(T_LOOP(p)) == L_DO && stmt == T_PINIT(T_LOOP(p))) {
	T_PINIT(T_LOOP(p)) = sls;
      }
      else {
	INT_FATAL(stmt, "insert_stmt: bad loop");
      }
      break;
    default:
      INT_FATAL(sls, "insert_stmt: bad statement and/or type (%d)", T_TYPE(p));
    }
  }
  else if (T_PARFCN(stmt)) {
    T_STLS(T_PARFCN(stmt)) = sls;
  }
  return sls;
}


/* 
wdw:
for a statement that has been removed from the AST, we want to reset
invalid pointers to NULL so problems don't arise later.  This is a change
supporting the insertion of mloops.  We'd like to call remove_stmt(s),
clear_stmt(s), followed by build_mloop(...,stmt,...).
*/

statement_t	*clear_stmt(statement_t *s) {
  if (s == NULL) {
    INT_FATAL(NULL, "s == NULL in clear_stmt()");
  } 
  T_NEXT(s) = NULL;
  T_PREV(s) = NULL;
  T_PARENT(s) = NULL;
  return s;
}

statement_t *remove_stmt(statement_t *s) {
  statement_t *p;

  if (s == NULL) {
    INT_FATAL(NULL, "s == NULL in remove_stmt()");
  }
  if (T_NEXT(s) != NULL) {
    T_PREV(T_NEXT(s)) = T_PREV(s);
  }
  if (T_PREV(s) != NULL) {
    T_NEXT(T_PREV(s)) = T_NEXT(s);
  } else if (T_PARENT(s) != NULL) {
    p = T_PARENT(s);
    switch (T_TYPE(p)) {
    case S_NULL:
    case S_EXIT:
    case S_EXPR:
    case S_RETURN:
    case S_HALT:
    case S_CONTINUE:
    case S_IO:
    case S_END:
    case S_WRAP:
    case S_REFLECT:
    case S_COMM:
      INT_FATAL(s, "parent of s doesn't make sense in remove_stmt()");
      break;
    case S_REGION_SCOPE:	/*GHF new */	
      if (s == T_BODY(T_REGION(p))) {
	T_BODY(T_REGION(p)) = T_NEXT(s);
      }
      break;
    case S_MLOOP:		/*GHF new */
      if (s == T_BODY(T_MLOOP(p))) {
	T_BODY(T_MLOOP(p)) = T_NEXT(s);
      }
      break;
    case S_NLOOP:		/*** sungeun ***/
      if (s == T_NLOOP_BODY(T_NLOOP(p))) {
	T_NLOOP_BODY(T_NLOOP(p)) = T_NEXT(s);
      }
      break;
    case S_MSCAN:
    case S_COMPOUND:
      if (s == T_CMPD_STLS(p)) {
	T_CMPD_STLS(p) = T_NEXT(s);
      }
      break;
    case S_IF:
      {
	if_t	*t = T_IF(p);

	if (s == T_THEN(t))
	  T_THEN(t) = T_NEXT(s);
	else if (s == T_ELSE(t))
	  T_ELSE(t) = T_NEXT(s);
      }
      break;
    case S_LOOP:
      {
	loop_t	*t = T_LOOP(p);

	if (s == T_BODY(t)) {
	  T_BODY(t) = T_NEXT(s);
	}
	switch(T_TYPE(t)) {
	case L_DO:
	  if (s == T_PINIT(t)) {
	    T_PINIT(t) = T_NEXT(s);
	  }
	  break;
	case L_WHILE_DO:
	case L_REPEAT_UNTIL:
	  break;
	default:
	  INT_FATAL(s, "Bad looptype (%d) in remove_stmt()",T_TYPE(t));
	}
      }
      break;
    default:
      INT_FATAL(s, "Bad stmnttype (%d) in remove_stmt()",T_TYPE(p));
    }
  }
  T_PREV(s) = NULL;
  T_NEXT(s) = NULL;
  return s;
}

void delete_stmt(statement_t *s) {
  destroy_stmt(remove_stmt(s));
}

/* returns a copy of the decls (params = 1 then include parameters) */
symboltable_t *copy_decls(symboltable_t *stp, int params) {
  DB0(10,"copy_decls()\n");
  DB0(10,"WARNING : copy_decls() not implemented yet.\n");

  return stp;
}

statement_t	*copy_stmtls(statement_t *ls) {
  statement_t *newls;

  DB0(DBLVL,"copy_stmtls();\n");

  if (ls == NULL) {
    return NULL;
  }

  newls = copy_stmt(ls);
  ls = T_NEXT(ls);

  while (ls != NULL) {
    newls = cat_stmt_ls(newls,copy_stmt(ls));
    ls = T_NEXT(ls);
  }

  return newls;
}



/**************************************************************
 check_expr_mdd(expr_t)
 MDD 9-29-95
 this is a fix to restore the callgraph, which becomes stale 
 after copying old statements including procedure calls, without
 fixing calledge information

 proper solution would include changing the copy_stmt or the
 build_expr_mloop
 **************************************************************/
static void check_expr_mdd(expr_t *e) {

  symboltable_t *callee;
  expr_t *ef;
  callgraph_t *callee_node;
  calledge_t  *in_edge;

  if (T_TYPE(e) == FUNCTION) {
    ef = T_OPLS(e);
    callee = T_IDENT(ef);
    callee_node = CG_NODE(callee);
    if (callee_node) { /* callgraph constructed */
      in_edge = CG_IN_EDGE(callee_node);
      while (in_edge) {
	if (T_STMT(CG_CALL(in_edge)) == Old_stmt) {
	  T_STMT(CG_CALL(in_edge)) = New_stmt;
	  T_STMT(CG_CALL(CG_EDGE_SAME(in_edge))) = New_stmt;
	}
	in_edge = T_NEXT(in_edge);
      }
    }
  }

}


statement_t	*copy_stmt(statement_t *s) {
  statement_t *new = NULL;
  expr_t	*e;

  DB0(DBLVL,"copy_stmt();\n");
  
  if (s == NULL) {
    return NULL;
  }
  
  switch (T_TYPE(s)) {
  case S_NULL:
  case S_EXIT:
  case S_HALT:
  case S_CONTINUE:
    new = alloc_statement(T_TYPE(s),T_LINENO(s),T_FILENAME(s));
    break;
  case S_COMM:
    new = copy_comm_stmt(s);
    break;
  case S_MLOOP:
    new = copy_mloop_stmt(s);
    break;
  case S_NLOOP:
    new = copy_nloop_stmt(s);
    break;
  case S_REGION_SCOPE:
    new = copy_region_stmt(s);
    break;
  case S_WRAP:
  case S_REFLECT:		
    new = copy_wrap_stmt(s);
    break;
  case S_MSCAN:
  case S_COMPOUND:
    assert(T_CMPD(s));
    new = build_compound_statement(copy_decls(T_CMPD_DECL(s), 1),
				   copy_stmtls(T_CMPD_STLS(s)),
				   T_LINENO(s),
				   T_FILENAME(s));
    break;
  case S_EXPR:
    e = copy_expr(T_EXPR(s));
    new = build_expr_statement(e,T_LINENO(s),T_FILENAME(s));
    break;
  case S_IF:
    new = copy_if_stmt(s);
    break;
  case S_LOOP:
    new = copy_loop_stmt(s);
    break;
  case S_RETURN:
    e = copy_expr(T_RETURN(s));
    new = build_return_statement(e,T_LINENO(s),T_FILENAME(s));
    break;
  case S_IO:
    e = copy_expr(IO_EXPR1(T_IO(s)));
    new = build_1_io_statement(IO_TYPE(T_IO(s)), IO_FILE(T_IO(s)), 
			       IO_CONTROL(T_IO(s)), e, NULL,
			       T_LINENO(s), T_FILENAME(s));
    break;
  case S_END:
    new = alloc_statement(S_END,T_LINENO(s),T_FILENAME(s));
    T_END(new) = T_END(s);
    break;
  default:
    INT_FATAL(s, "Bad stmnttype (%d) in remove_stmt()",T_TYPE(s));
  }
  
  /*** not needed since the above do it *** T_TYPE(new) = T_TYPE(s); ***/
  T_SUBTYPE(new) = T_SUBTYPE(s);
  T_LINENO(new) = T_LINENO(s);
  T_LABEL(new) = T_LABEL(s);
  
  T_SEMANTICS(new) = T_SEMANTICS(s);
  
  T_PARFCN(new) = T_PARFCN(s);
  
  T_IN(new) = T_IN(s);
  T_OUT(new) = T_OUT(s);
  T_LIVE(new) = T_LIVE(s); /*** a little dicey as are the above 2 ***/
  
  T_FUNC_CALLS(new) = T_FUNC_CALLS(s);
  T_PARALLEL(new) = T_PARALLEL(s);

  T_PRE(new) = copy_genlist(T_PRE(s));
  T_POST(new) = copy_genlist(T_POST(s));

  T_IS_SHARD(new) = T_IS_SHARD(s);

  T_PROHIBITIONS(new) = T_PROHIBITIONS(s);

  Old_stmt = s; /* MDD */
  New_stmt = new; /* MDD */
  T_IN_MLOOP(new) = T_IN_MLOOP(s);
  if (T_TYPE(s) == S_EXPR) 
    traverse_exprls_g(T_EXPR(s), check_expr_mdd, NULL); 
  
  return new;
}


genlist_t *copy_genlist(genlist_t *glp) {
  genlist_t *new=NULL, *tnew, *temp=glp;
  while (temp) {
    tnew = alloc_gen();
    G_IDENT(tnew) = G_IDENT(temp);
    new = cat_genlist_ls( new, tnew);
    temp = G_NEXT(temp);
  }
  return new;
}

static expr_t *copy_expr_int(expr_t*, expr_t*);

expr_t *copy_exprls(expr_t *e, expr_t* parent) {
  expr_t *new;
	
  DB0(30,"copy_exprls()\n");

  if (e == NULL)
    return NULL;
  
  new = copy_expr_int(e, parent);
  T_NEXT(new) = copy_exprls(T_NEXT(e), parent);

  return new;
}

expr_t *copy_expr(expr_t *e) {
  expr_t *new;
  new = copy_expr_int (e, NULL);
  return new;
}


static expr_t *copy_expr_int(expr_t* e, expr_t* parent) {
  expr_t *new;
  int i;

  DB0(30,"copy_expr()\n");

  if (e == NULL) {
    return NULL;
  }

  new = alloc_expr(T_TYPE(e));
  T_NEXT(new) = NULL;
  T_SUBTYPE(new) = T_SUBTYPE(e);
  T_IDENT(new) = T_IDENT(e);
  T_REGMASK(new) = T_REGMASK(e);
  T_REGMASK2(new) = T_REGMASK2(e);
  T_OPLS(new) = copy_exprls(T_OPLS(e), new);
  T_PARENT(new) = parent;
  T_FLAG1(new) = T_FLAG1(e);
  T_FLAG2(new) = T_FLAG2(e);
  T_FLAG3(new) = T_FLAG3(e);
  T_FLAG4(new) = T_FLAG4(e);
  T_DUMMY(new) = T_DUMMY(e);
  if (T_MAP(e) == (expr_t*)0x1) {
    T_MAP(new) = T_MAP(e);
  }
  else {
    T_MAP(new) = copy_exprls(T_MAP(e), NULL);
  }
  T_MAPSIZE(new) = T_MAPSIZE(e);
  T_STMT(new) = T_STMT(e);
  
  /* Need these fields for scan/reduce exprs  - rea */
  T_DIMS(new) = T_DIMS(e);
  T_RANK(new) = T_RANK(e);
  
  /* Copy Derrick's type info */
  T_TYPEINFO(new) = T_TYPEINFO(e);
  T_TYPEINFO_REG(new) = T_TYPEINFO_REG(e);

  for (i = 0; i < MAXRANK; i++) {
    T_NUDGE (new)[i] = T_NUDGE (e)[i];
  }

  T_INSTANCE_NUM(new) = T_INSTANCE_NUM(e);

  T_FREE(new) = T_FREE(e);
  
  return new;
}


static statement_t *copy_if_stmt(statement_t *s) {
  if_t *t;

  DB0(DBLVL,"copy_if_stmt()\n");

  if (s == NULL) {
    return NULL;
  }

  if (T_TYPE(s) != S_IF) {
    INT_FATAL(s, "s is not if-statement in copy_loop()");
  }
  
  t = T_IF(s);
  return build_if_statement(copy_expr(T_IFCOND(t)),
			    copy_stmtls(T_THEN(t)), 
			    copy_stmtls(T_ELSE(t)),
			    (statement_t *)NULL, 
			    T_LINENO(s),
			    T_FILENAME(s));
}


static statement_t *copy_loop_stmt(statement_t *s) {
  statement_t *new = NULL;
  loop_t *l;

  DB0(DBLVL,"copy_loop_stmt()\n");

  if (s == NULL) {
    return NULL;
  }

  if (T_TYPE(s) != S_LOOP) {
    INT_FATAL(s, "s is not loop statement in copy_loop()");
  }

  l = T_LOOP(s);
  if (l == NULL) {
    INT_FATAL(s, "l should not be NULL in copy_loop()");
  }

  switch (T_TYPE(l)) {
  case L_DO:
    new = build_do_statement(T_TYPE(l),
			     T_UPDOWN(l),
			     T_ENDDO(l),
			     copy_expr(T_IVAR(l)),
			     copy_expr(T_START(l)),
			     copy_expr(T_STOP(l)),
			     copy_expr(T_STEP(l)),
			     copy_stmtls(T_BODY(l)),
			     T_LINENO(s),
			     T_FILENAME(s));
    break;
  case L_WHILE_DO:
    new = build_loop_statement(L_WHILE_DO,
			       copy_expr(T_LOOPCOND(l)),
			       copy_stmtls(T_BODY(l)),
			       T_LINENO(s),
			       T_FILENAME(s));
    break;
  case L_REPEAT_UNTIL:
    new = build_loop_statement(L_REPEAT_UNTIL,
			       copy_expr(T_LOOPCOND(l)),
			       copy_stmtls(T_BODY(l)),
			       T_LINENO(s),
			       T_FILENAME(s));
    break;
	default:
    INT_FATAL(s, "Bad looptype (%d) in copy_loop_stmt() list.c",T_TYPE(l));
  }
  return new;
}



/* WDW new copy function for regions */

/* copy new merged node */
static statement_t *copy_region_stmt(statement_t *s) {
  statement_t *new = NULL;
  region_t	*l;

  DB0(DBLVL,"copy_region_stmt()\n");
  
  if (s == NULL) {
    return NULL;
  }
  
  if (T_TYPE(s) != S_REGION_SCOPE) {
    INT_FATAL(s, "s is not region statement in copy_region()");
  }
  
  l = T_REGION(s); /* get region symbol */
  if (l == NULL) {
    INT_FATAL(s, "l should not be NULL in copy_region()");
  }
  
  /* The mask expression is shared and not copied */
  new = build_reg_mask_scope(T_REGION_SYM(l),T_MASK_EXPR(l),
			     T_MASK_BIT(l), copy_stmtls(T_BODY(l)), 
			     T_LINENO(s), T_FILENAME(s));

  return new;
}


static statement_t *copy_wrap_stmt(statement_t *s) {
  statement_t *new = NULL;
  wrap_t *l;
  expr_t *new_list;

  DB0(DBLVL,"copy_wrap_stmt()\n");

  if (s == NULL) {
    return NULL;
  }

  if ((T_TYPE(s) != S_WRAP) && (T_TYPE(s) != S_REFLECT)) {
    INT_FATAL(s, "s is not wrap/reflect statement in copy_wrap()");
  }
  
  l = T_WRAP(s);
  if (l == NULL) {
    INT_FATAL(s, "l should not be NULL in copy_wrap()");
  }
  
  new_list = copy_exprls(T_OPLS(l), NULL);
  new = build_wrap_statement(T_TYPE(s), new_list, T_LINENO(s), T_FILENAME(s));

  return new;
}

statement_t *copy_comm_stmt(statement_t *s) {
  statement_t *new = NULL;
  comm_info_t *c, *c2, *new_c;

  DB0(DBLVL,"copy_comm_stmt()\n");

  if (s == NULL)
    return NULL;

  if (T_TYPE(s) != S_COMM) {
    INT_FATAL(s, "s is not send/recv statement in copy_comm_stmt()");
    return NULL;
  }

  new = build_comm_statement(T_COMM_TYPE(s), T_COMM_LHSCOMM(s),
			     T_COMM_ID(s), T_COMM_REG(s),
			     T_COMM_INFO_ENS(T_COMM_INFO(s)),
			     T_COMM_INFO_DIR(T_COMM_INFO(s)),
			     T_COMM_INFO_DIRTYPES(T_COMM_INFO(s)),
			     T_LINENO(s),T_FILENAME(s));

  /*** copy the rest of the information ***/
  c2 = T_COMM_INFO(new);
  for (c = T_COMM_INFO_NEXT(T_COMM_INFO(s)); c!=NULL; c=T_COMM_INFO_NEXT(c)) {
    new_c = (comm_info_t *) PMALLOC(sizeof(comm_info_t));
    T_COMM_INFO_ENS(new_c) = copy_expr(T_COMM_INFO_ENS(c));
    T_COMM_INFO_DIR(new_c) = copy_genlist(T_COMM_INFO_DIR(c));
    T_COMM_INFO_DIRTYPES(new_c) = copy_genlist(T_COMM_INFO_DIRTYPES(c));
    T_COMM_INFO_NEXT(c2) = new_c;
    c2 = new_c;
  }
  T_COMM_INFO_NEXT(c2) = NULL;
  
  return new;
}


static statement_t *copy_mloop_stmt(statement_t *s) {
  statement_t *new = NULL;
  mloop_t *l;

  DB0(DBLVL,"copy_mloop_stmt()\n");

  if (s == NULL) {
    return NULL;
  }

  if (T_TYPE(s) != S_MLOOP) {
    INT_FATAL(s, "s is not mloop statement in copy_mloop_stmt()");
  }

  l = T_MLOOP(s);
  if (l == NULL) {
    INT_FATAL(s, "l should not be NULL in copy_mloop_stmt()");
  }
  
  new = build_mloop_statement(T_MLOOP_REG(l),copy_stmtls(T_BODY(l)),
			      T_MLOOP_RANK(l),
			      copy_expr(T_MLOOP_MASK(l)),
			      T_MLOOP_WITH(l),
			      T_LINENO(s), T_FILENAME(s));

  return new;
}


static statement_t *copy_nloop_stmt(statement_t *s) {
  statement_t *new = NULL;
  nloop_t *l;

  DB0(DBLVL,"copy_nloop_stmt()\n");

  if (s == NULL) {
    return NULL;
  }

  if (T_TYPE(s) != S_NLOOP) {
    INT_FATAL(s, "s is not nloop statement in copy_nloop_stmt()");
  }

  l = T_NLOOP(s);
  if (l == NULL) {
    INT_FATAL(s, "l should not be NULL in copy_nloop_stmt()");
  }

  new = build_nloop_statement(T_NLOOP_DIMS(l),
			      T_NLOOP_DIMLIST(l),
			      copy_stmtls(T_NLOOP_BODY(l)),
			      T_NLOOP_DEPTH(l),
			      T_NLOOP_MLOOP_PROXIMITY(l),
			      T_LINENO(s), T_FILENAME(s));
  return new;
}

expr_t* replace_expr (expr_t* old, expr_t* new)
{
  if (old == NULL) {
    INT_FATAL (NULL, "null old passed to replace_expr");
  }
  
  if (new == NULL) {
    INT_FATAL (NULL, "null new passed to replace_expr");
  }
  /*
  if (T_PARENT(old) != NULL) {
    if (T_OPLS(T_PARENT(old)) == old) {
      T_OPLS(T_PARENT(old)) = new;
    }
    else if (T_NEXT(T_OPLS(T_PARENT(old))) == old) {
      T_NEXT(T_OPLS(T_PARENT(old))) = new;
    }
    else {
      INT_FATAL (NULL, "replacing non-binary, non-unary tree node");
    }
  }
  */
  if (T_PARENT(old) != NULL) {
    if (T_OPLS(T_PARENT(old)) == old) {
      T_OPLS(T_PARENT(old)) = new;
    }
    else if (T_NEXT(T_OPLS(T_PARENT(old))) == old) {
      T_NEXT(T_OPLS(T_PARENT(old))) = new;
    }
  }
  if (T_NEXT(old) != NULL) {
    T_PREV(T_NEXT(old)) = new;
  }
  if (T_PREV(old) != NULL) {
    T_NEXT(T_PREV(old)) = new;
  }
  T_NEXT(new) = T_NEXT(old);
  T_PREV(new) = T_PREV(old);
  T_PARENT(new) = T_PARENT(old);
  T_NEXT(old) = NULL;
  T_PREV(old) = NULL;
  T_PARENT(old) = NULL;
  T_STMT(new)= T_STMT(old);

  T_MEANING(new) = copy_expr(old);
  return new;
}

expr_t* replaceall_expr(expr_t* super, expr_t* old, expr_t* new) {
  expr_t* tmp;
  int i, j;

  if (old == NULL) {
    INT_FATAL (NULL, "null old passed to replaceall_expr");
  }
  
  if (new == NULL) {
    INT_FATAL (NULL, "null new passed to replaceall_expr");
  }

  if (super == NULL) {
    return NULL;
  }

  if (expr_equal (super, old)) {
    return replace_expr (super, copy_expr (new));
  }
  else {
    i = 0;
    do {
      tmp = T_OPLS(super);
      for (j = 0; j < i; j++) {
	tmp = T_NEXT(tmp);
      }
      if (tmp != NULL) {
	replaceall_expr(tmp, old, new);
	i++;
      }
      else {
	i = 0;
      }
    } while (i != 0);
  }
  return super;
}


expr_t* replaceall_atom_exprs(expr_t* super, expr_t* old, expr_t* new) {
  expr_t* tmp;
  int i, j;

  if (old == NULL) {
    INT_FATAL (NULL, "null old passed to replaceall_atom_exprs");
  }
  
  if (new == NULL) {
    INT_FATAL (NULL, "null new passed to replaceall_atom_exprs");
  }

  if (super == NULL) {
    return NULL;
  }

  if (expr_equal (super, old)) {
    return replace_expr (super, copy_expr (new));
  }
  else if (!expr_is_atom(super)) {
    i = 0;
    do {
      tmp = T_OPLS(super);
      for (j = 0; j < i; j++) {
	tmp = T_NEXT(tmp);
      }
      if (tmp != NULL) {
	replaceall_atom_exprs(tmp, old, new);
	i++;
      }
      else {
	i = 0;
      }
    } while (i != 0);
  }
  return super;
}


void replaceall_exprls(expr_t* super, expr_t* old, expr_t* new) {
  expr_t* tmp;

  for (tmp = super; tmp != NULL; tmp = T_NEXT(tmp)) {
    tmp = replaceall_expr(tmp, old, new);
  }
}

void replaceall_stmtls(statement_t* stmt, expr_t* old, expr_t* new) {
  for (; stmt != NULL; stmt = T_NEXT(stmt)) {
    switch (T_TYPE(stmt)) {
    case S_NULL:
    case S_EXIT:
    case S_HALT:
    case S_CONTINUE:
    case S_COMM:
    case S_END:
      break;
    case S_MLOOP:
      replaceall_stmtls(T_BODY(T_MLOOP(stmt)), old, new);
      break;
    case S_NLOOP:
      replaceall_stmtls(T_NLOOP_BODY(T_NLOOP(stmt)), old, new);
      break;
    case S_REGION_SCOPE:
      replaceall_stmtls(T_BODY(T_REGION(stmt)), old, new);
      break;
    case S_WRAP:
    case S_REFLECT:
      replaceall_exprls(T_OPLS(T_WRAP(stmt)), old, new);
      break;
    case S_MSCAN:
    case S_COMPOUND:
      replaceall_stmtls(T_CMPD_STLS(stmt), old, new);
      break;
    case S_EXPR:
      T_EXPR(stmt) = replaceall_expr(T_EXPR(stmt), old, new);
      break;
    case S_IF:
      T_IFCOND(T_IF(stmt)) = replaceall_expr(T_IFCOND(T_IF(stmt)), old, new);
      replaceall_stmtls(T_THEN(T_IF(stmt)), old, new);
      replaceall_stmtls(T_ELSE(T_IF(stmt)), old, new);
      break;
    case S_LOOP:
      switch (T_TYPE(T_LOOP(stmt))) {
      case L_DO:
	T_IVAR(T_LOOP(stmt)) = replaceall_expr(T_IVAR(T_LOOP(stmt)), old, new);
	T_START(T_LOOP(stmt)) = replaceall_expr(T_START(T_LOOP(stmt)), old, new);
	T_STOP(T_LOOP(stmt)) = replaceall_expr(T_STOP(T_LOOP(stmt)), old, new);
	T_STEP(T_LOOP(stmt)) = replaceall_expr(T_STEP(T_LOOP(stmt)), old, new);
	replaceall_stmtls(T_BODY(T_LOOP(stmt)), old, new);
	break;
      case L_WHILE_DO: 
      case L_REPEAT_UNTIL:
	T_LOOPCOND(T_LOOP(stmt)) = replaceall_expr(T_LOOPCOND(T_LOOP(stmt)), old, new);
	replaceall_stmtls(T_BODY(T_LOOP(stmt)), old, new);
	break;
      default:
	INT_FATAL(NULL, "Bad looptype (%d) in replaceall_stmt()",T_TYPE(T_LOOP(stmt)));
      }
      break;
    case S_RETURN:
      T_RETURN(stmt) = replaceall_expr(T_RETURN(stmt), old, new);
      break;
    case S_IO:
      IO_EXPR1(T_IO(stmt)) = replaceall_expr(IO_EXPR1(T_IO(stmt)), old, new);
      IO_FILE(T_IO(stmt)) = replaceall_expr(IO_FILE(T_IO(stmt)), old, new);
      IO_CONTROL(T_IO(stmt)) = replaceall_expr(IO_CONTROL(T_IO(stmt)), old, new);
      break;
    default:
      INT_FATAL(stmt, "Bad stmnttype (%d) in replaceall_stmt()",T_TYPE(stmt));
    }
  }
}

statement_t* returns2assigns(statement_t* stmt, expr_t* assign) {
  expr_t* assignexpr;
  statement_t* assignstmt;
  statement_t* returnstmt;
  int first;

  returnstmt = NULL;
  for (first = 1; stmt != NULL; stmt = T_NEXT(stmt)) {
    switch (T_TYPE(stmt)) {
    case S_NULL:
    case S_EXIT:
    case S_HALT:
    case S_CONTINUE:
    case S_COMM:
    case S_END:
    case S_WRAP:
    case S_REFLECT:
    case S_EXPR:
    case S_IO:
      break;
    case S_MLOOP:
      T_BODY(T_MLOOP(stmt)) = returns2assigns(T_BODY(T_MLOOP(stmt)), assign);
      break;
    case S_NLOOP:
      T_NLOOP_BODY(T_NLOOP(stmt)) = returns2assigns(T_NLOOP_BODY(T_NLOOP(stmt)), assign);
      break;
    case S_REGION_SCOPE:
      T_BODY(T_REGION(stmt)) = returns2assigns(T_BODY(T_REGION(stmt)), assign);
      break;
    case S_MSCAN:
    case S_COMPOUND:
      T_CMPD_STLS(stmt) = returns2assigns(T_CMPD_STLS(stmt), assign);
      break;
    case S_IF:
      T_THEN(T_IF(stmt)) = returns2assigns(T_THEN(T_IF(stmt)), assign);
      T_ELSE(T_IF(stmt)) = returns2assigns(T_ELSE(T_IF(stmt)), assign);
      break;
    case S_LOOP:
      T_BODY(T_LOOP(stmt)) = returns2assigns(T_BODY(T_LOOP(stmt)), assign);
      break;
    case S_RETURN:
      assignexpr = build_typed_binary_op(BIASSIGNMENT, copy_expr(T_RETURN(stmt)),
					 copy_expr(assign));
      assignstmt = build_expr_statement(assignexpr,0,NULL);
      insertbefore_stmt(assignstmt, stmt);
      remove_stmt(stmt);
      stmt = assignstmt;
      break;
    default:
      INT_FATAL(stmt, "Bad stmnttype (%d) in replaceall_stmt()",T_TYPE(stmt));
    }
    if (first) {
      returnstmt = stmt;
      first = 0;
    }
  }
  return returnstmt;
}

void zeroreplace_expr (expr_t* expr)
{
  expr_t* zero;

  if (expr == NULL) {
    INT_FATAL (NULL, "null expr passed to zeroreplace_expr");
  }

  strcpy (buffer, "0"); /** for build_int **/
  zero = build_0ary_op (CONSTANT, build_int (0));
  replace_expr (expr, zero);
}

/*
 * factor_expr helper function
 */
static expr_t* find_factor (expr_t* expr, expr_t* mul)
{
  expr_t* factor;
  expr_t* factor2;

  if (expr == NULL) {
    INT_FATAL (NULL, "null expr passed to find_factor");
  }

  if (mul == NULL) {
    INT_FATAL (NULL, "null mul passed to find_factor");
  }

  if (T_TYPE(expr) == BITIMES) {
    if (expr_equal (T_OPLS(expr), mul)) {
      factor = T_NEXT(T_OPLS(expr));
      zeroreplace_expr (T_NEXT(T_OPLS(expr)));
      return factor;
    }
    else if (expr_equal (T_NEXT(T_OPLS(expr)), mul)) {
      factor = T_OPLS(expr);
      zeroreplace_expr (T_OPLS(expr));
      return factor;
    }
    else {
      return NULL;
    }
  }
  else if (T_TYPE(expr) == BIPLUS) {
    factor2 = T_NEXT(T_OPLS(expr));
    factor = find_factor (T_OPLS(expr), mul);
    factor2 = find_factor (factor2, mul);
    if (factor == NULL) {
      return factor2;
    }
    else if (factor2 == NULL) {
      return factor;
    }
    else {
      factor = build_binary_op (BIPLUS, factor, factor2);
      return factor;
    }
  }
  else {
    return NULL;
  }
}

/*
 * factor_expr helper function
 */
static void factor_expr_int (expr_t* super, expr_t* expr)
{
  expr_t* factor;

  if (expr == NULL) {
    INT_FATAL (NULL, "null expr passed to factor_expr_int");
  }

  if (T_TYPE(expr) == BITIMES) {
    factor_expr_int (T_OPLS(expr), T_OPLS(expr));
    factor_expr_int (T_NEXT(T_OPLS(expr)), T_NEXT(T_OPLS(expr)));
    factor = find_factor (super, T_OPLS(expr));
    if (factor != NULL) {
      replace_expr (T_NEXT(T_OPLS(expr)), factor);
    }
    else {
      factor = find_factor (super, T_NEXT(T_OPLS(expr)));
      if (factor != NULL) {
	replace_expr (T_OPLS(expr), factor);
      }
    }
  }
  else if (T_TYPE(expr) == BIPLUS) {
    factor_expr_int (super, T_OPLS(expr));
    factor_expr_int (super, T_NEXT(T_OPLS(expr)));
  }
}

/*
 * minimizes the number of multiplications by replacing
 *
 *        x * y + ... + z * y  with  (x + z) * y + ... + 0 * y
 *
 * the resultant expression is cleaned before it is returned.
 */
void factor_expr (expr_t* expr)
{
  if (expr == NULL) {
    INT_FATAL (NULL, "null expr passed to factor_expr");
  }

  factor_expr_int (expr, expr);
  clean_expr (expr);
}

/*
 * replace 1 * x with x, 0 * x with 0, 0 + x with x recursively
 * throughout expr
 */
expr_t* clean_expr (expr_t* expr)
{
  static expr_t* zero = NULL;
  static expr_t* one = NULL;

  if (expr == NULL) {
    INT_FATAL (NULL, "null expr passed to clean_expr");
  }

  if (zero == NULL || one == NULL) {
    sprintf (buffer, "0"); zero = build_0ary_op (CONSTANT, build_int (0));
    sprintf (buffer, "1"); one = build_0ary_op (CONSTANT, build_int (1));
  }

  if ((T_TYPE(expr) == BITIMES && expr_equal(T_OPLS(expr), zero)) ||
      (T_TYPE(expr) == BITIMES && expr_equal(T_NEXT(T_OPLS(expr)), one)) ||
      (T_TYPE(expr) == BIPLUS && expr_equal(T_NEXT(T_OPLS(expr)), zero))) {
    return clean_expr (replace_expr (expr, T_OPLS(expr)));
  }
  if ((T_TYPE(expr) == BITIMES && expr_equal(T_NEXT(T_OPLS(expr)), zero)) ||
      (T_TYPE(expr) == BITIMES && expr_equal(T_OPLS(expr), one)) ||
      (T_TYPE(expr) == BIPLUS && expr_equal(T_OPLS(expr), zero))) {
    return clean_expr (replace_expr (expr, T_NEXT(T_OPLS(expr))));
  }
  if (T_IS_BINARY(T_TYPE(expr))) {
    T_OPLS(expr) = clean_expr (T_OPLS(expr));
    T_NEXT(T_OPLS(expr)) = clean_expr (T_NEXT(T_OPLS(expr)));
  }
  return expr;
}


expr_t* plainify_expr (expr_t* expr)
{
  expr_t* tmp;

  if (expr == NULL) {
    INT_FATAL (NULL, "null expr passed to plainify_expr");
  }

  if (T_TYPE(expr) == BIMINUS) {
    sprintf (buffer, "-1");
    tmp = build_0ary_op(CONSTANT, build_int(-1));
    T_STMT(tmp) = T_STMT(expr);
    tmp = build_binary_op(BITIMES, T_NEXT(T_OPLS(expr)), tmp);
    tmp = build_binary_op(BIPLUS, T_OPLS(expr), tmp);
    return plainify_expr(replace_expr(expr, tmp));
  }
  /*
  if (T_TYPE(expr) == BIDIVIDE) {
    sprintf (buffer, "1");
    tmp = build_0ary_op(CONSTANT, build_int(1));
    T_STMT(tmp) = T_STMT(expr);
    tmp = build_binary_op(BIDIVIDE, T_NEXT(T_OPLS(expr)), tmp);
    tmp = build_binary_op(BITIMES, T_OPLS(expr), tmp);
    return replace_expr(expr, tmp);
  }
  */
  if (T_IS_BINARY(T_TYPE(expr))) {
    T_OPLS(expr) = plainify_expr(T_OPLS(expr));
    T_NEXT(T_OPLS(expr)) = plainify_expr(T_NEXT(T_OPLS(expr)));
  }
  return expr;
}

void compose_expr (expr_t* expr)
{
  expr_t* mult;
  expr_t* tmp1;
  expr_t* tmp2;
  expr_t* tmp;

  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to compose_expr");
  }

  if (T_TYPE(expr) == BITIMES) {
    if (T_TYPE(T_OPLS(expr)) == BIPLUS) {
      mult = T_NEXT(T_OPLS(expr));
      tmp1 = copy_expr(T_OPLS(T_OPLS(expr)));
      tmp2 = copy_expr(T_NEXT(T_OPLS(T_OPLS(expr))));
    }
    else if (T_TYPE(T_NEXT(T_OPLS(expr))) == BIPLUS) {
      mult = T_OPLS(expr);
      tmp1 = copy_expr(T_OPLS(T_NEXT(T_OPLS(expr))));
      tmp2 = copy_expr(T_NEXT(T_OPLS(T_NEXT(T_OPLS(expr)))));
    }
    else {
      tmp1 = NULL;
      tmp2 = NULL;
    }
    if (tmp1 != NULL && tmp2 != NULL) {
      tmp1 = build_binary_op(BITIMES, tmp1, copy_expr(mult));
      tmp2 = build_binary_op(BITIMES, tmp2, copy_expr(mult));
      tmp = build_binary_op(BIPLUS, tmp1, tmp2);
      expr = replace_expr(expr, tmp);
      if (T_PARENT(expr) != NULL) {
	compose_expr(T_PARENT(expr));
      }
    }
  }
  if (T_IS_BINARY(T_TYPE(expr))) {
    compose_expr(T_OPLS(expr));
    compose_expr(T_NEXT(T_OPLS(expr)));
  }
}

static expr_t* groupmult_consts (expr_t* expr)
{
  expr_t* tmp1;
  expr_t* tmp2;

  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to groupmult_consts");
  }

  if (T_TYPE(expr) != BITIMES) {
    INT_FATAL(NULL, "non bitimes passed to groupmult_consts");
  }

  if (T_TYPE(T_OPLS(expr)) == BITIMES) {
    tmp1 = groupmult_consts (T_OPLS(expr));
  }
  else if (expr_computable_const (T_OPLS(expr))) {
    tmp1 = copy_expr (T_OPLS(expr));
  }
  else {
    tmp1 = NULL;
  }

  if (T_TYPE(T_NEXT(T_OPLS(expr))) == BITIMES) {
    tmp2 = groupmult_consts (T_NEXT(T_OPLS(expr)));
  }
  else if (expr_computable_const (T_NEXT(T_OPLS(expr)))) {
    tmp2 = copy_expr (T_NEXT(T_OPLS(expr)));
  }
  else {
    tmp2 = NULL;
  }

  if (tmp1 == NULL) {
    return tmp2;
  }
  else if (tmp2 == NULL) {
    return tmp1;
  }
  else {
    return build_binary_op (BITIMES, tmp1, tmp2);
  }
}

static expr_t* groupmult_nonconsts (expr_t* expr)
{
  expr_t* tmp1;
  expr_t* tmp2;

  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to groupmult_consts");
  }

  if (T_TYPE(expr) != BITIMES) {
    INT_FATAL(NULL, "non bitimes passed to groupmult_consts");
  }

  if (T_TYPE(T_OPLS(expr)) == BITIMES) {
    tmp1 = groupmult_nonconsts (T_OPLS(expr));
  }
  else if (T_TYPEINFO_REG(T_OPLS(expr)) == NULL &&
	   !expr_computable_const(T_OPLS(expr))) {
    tmp1 = copy_expr (T_OPLS(expr));
  }
  else {
    tmp1 = NULL;
  }

  if (T_TYPE(T_NEXT(T_OPLS(expr))) == BITIMES) {
    tmp2 = groupmult_nonconsts (T_NEXT(T_OPLS(expr)));
  }
  else if (T_TYPEINFO_REG(T_NEXT(T_OPLS(expr))) == NULL &&
	   !expr_computable_const(T_NEXT(T_OPLS(expr)))) {
    tmp2 = copy_expr (T_NEXT(T_OPLS(expr)));
  }
  else {
    tmp2 = NULL;
  }

  if (tmp1 == NULL) {
    return tmp2;
  }
  else if (tmp2 == NULL) {
    return tmp1;
  }
  else {
    return build_binary_op (BITIMES, tmp1, tmp2);
  }
}

static expr_t* groupmult_loopds (expr_t* expr)
{
  expr_t* tmp1;
  expr_t* tmp2;

  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to groupmult_consts");
  }

  if (T_TYPE(expr) != BITIMES) {
    INT_FATAL(NULL, "non bitimes passed to groupmult_consts");
  }

  if (T_TYPE(T_OPLS(expr)) == BITIMES) {
    tmp1 = groupmult_loopds (T_OPLS(expr));
  }
  else if (T_TYPEINFO_REG(T_OPLS(expr)) != NULL) {
    tmp1 = copy_expr (T_OPLS(expr));
  }
  else {
    tmp1 = NULL;
  }

  if (T_TYPE(T_NEXT(T_OPLS(expr))) == BITIMES) {
    tmp2 = groupmult_loopds (T_NEXT(T_OPLS(expr)));
  }
  else if (T_TYPEINFO_REG(T_NEXT(T_OPLS(expr))) != NULL) {
    tmp2 = copy_expr (T_NEXT(T_OPLS(expr)));
  }
  else {
    tmp2 = NULL;
  }

  if (tmp1 == NULL) {
    return tmp2;
  }
  else if (tmp2 == NULL) {
    return tmp1;
  }
  else {
    return build_binary_op (BITIMES, tmp1, tmp2);
  }
}

expr_t* groupmult_expr (expr_t* expr)
{
  expr_t* tmp1;
  expr_t* tmp2;
  expr_t* tmp3;
  expr_t* tmp;

  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to groupmult_expr");
  }

  if (T_TYPE(expr) == BITIMES) {
    tmp1 = groupmult_consts (expr);
    tmp2 = groupmult_nonconsts (expr);
    tmp3 = groupmult_loopds (expr);

    if (tmp1 == NULL) {
      tmp = tmp2;
    }
    else if (tmp2 == NULL) {
      tmp = tmp1;
    }
    else {
      tmp = build_binary_op (BITIMES, tmp1, tmp2);
    }
    if (tmp == NULL) {
      tmp = tmp3;
    }
    else if (tmp3 == NULL) {
    }
    else {
      tmp = build_binary_op (BITIMES, tmp3, tmp);
    }
    if (tmp == NULL) {
      INT_FATAL(NULL, "bitimes of nada encountered");
    }
    return replace_expr(expr, tmp);
  }
  else if (T_IS_BINARY(T_TYPE(expr))) {
    T_OPLS(expr) = groupmult_expr(T_OPLS(expr));
    T_NEXT(T_OPLS(expr)) = groupmult_expr(T_NEXT(T_OPLS(expr)));
  }
  return expr;
}

expr_t* constfold_expr (expr_t* expr)
{
  expr_t* tmp;

  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to simplify_expr");
  }

  if (expr_computable_const(expr) && T_TYPE(expr) != CONSTANT) {
    if (D_CLASS(T_TYPEINFO(expr)) == DT_INTEGER) {
      sprintf(buffer, "%ld", expr_intval(expr));
      tmp = build_0ary_op(CONSTANT, build_int(expr_intval(expr)));
      return replace_expr(expr, tmp);
    }
  }
  if (T_IS_BINARY(T_TYPE(expr))) {
    T_OPLS(expr) = constfold_expr(T_OPLS(expr));
    T_NEXT(T_OPLS(expr)) = constfold_expr(T_NEXT(T_OPLS(expr)));
  }
  return expr;
}


int varequals_expr(expr_t* expr, symboltable_t* var)
{
  expr_t* tmp;
  if (T_TYPE(expr) == VARIABLE) {
    if (T_IDENT(expr) == var) {
      return 1;
    }
  }
  for (tmp = T_OPLS(expr); tmp != NULL; tmp = T_NEXT(tmp)) {
    if (varequals_expr(tmp, var)) {
      return 1;
    }
  }
  return 0;
}

/*
 * returns 0 if not invariant
 *         1 if invariant only in innermost loop
 *         2 if invariant only in two innermost loops
 *         k if invariant only in "k" innermost loops
 * note if k is mloop rank, expr is mloop invariant
 */
int mloopinvariant_expr(expr_t* expr)
{
  mloop_t* mloop;
  genlist_t* list;
  symboltable_t* pst;
  int i, k;

  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to mloopinvariant_expr");
  }
  if (T_STMT(expr) == NULL) {
    INT_FATAL(NULL, "expr with no associated stmt in mloopinvariant_expr");
  }
  if (T_IN_MLOOP(T_STMT(expr)) == NULL) {
    INT_FATAL(NULL, "expr with no associated mloop in mloopinvariant_expr");
  }
  mloop = T_MLOOP(T_IN_MLOOP(T_STMT(expr)));
  if (mloopassigned_expr(expr)) {
    return 0;
  }
  for (i = T_MLOOP_RANK(mloop); i > 0; i--) {
    k = T_MLOOP_ORDER(mloop, i);
    for (list = T_MLOOP_VARS(mloop, k); list != NULL; list = G_NEXT(list)) {
      pst = G_IDENT(list);
      if (varequals_expr(expr, pst)) {
	return T_MLOOP_RANK(mloop) - i;
      }
    }
    if (parallelindim_expr(expr, k)) {
      return T_MLOOP_RANK(mloop) - i;
    }
  }
  return T_MLOOP_RANK(mloop);
}

int mloopassigned_expr(expr_t* expr)
{
  set_t* out;

  for (out = T_OUT(T_IN_MLOOP(T_STMT(expr)));
       out != NULL;
       out = SET_NEXT(out)) {
    if (SET_TYPE(out) != VAR) {
      INT_FATAL (NULL, "out set contains non-var");
    }
    if (varequals_expr(expr, SET_VAR(out))) {
      return 1;
    }
  }
  return 0;
}

int expr_contains_uat(expr_t* expr, attype_t attype) {
  expr_t* tmp;

  if (expr == NULL) {
    return 0;
  }
  if (T_TYPE(expr) == BIAT && T_SUBTYPE(expr) == attype) {
    return 1;
  }
  else if (T_TYPE(expr) == VARIABLE &&
	   T_SUBTYPE(expr) == AT_RANDACC &&
	   T_RANDACC(expr) != NULL) {
    return 1;
  }
  else {
    tmp = T_OPLS(expr);
    while (tmp != NULL) {
      if (expr_contains_uat(tmp, attype)) {
	return 1;
      }
      tmp = T_NEXT(tmp);
    }
  }
  return 0;
}

int stmtls_contains_uat(statement_t* stmt, attype_t attype) {
  int val = 0;
  for (; stmt != NULL; stmt = T_NEXT(stmt)) {
    switch (T_TYPE(stmt)) {
    case S_NULL:
    case S_EXIT:
    case S_HALT:
    case S_CONTINUE:
    case S_COMM:
    case S_END:
      val +=  0;
      break;
    case S_MLOOP:
      val +=  stmtls_contains_uat(T_BODY(T_MLOOP(stmt)), attype);
      break;
    case S_NLOOP:
      val +=  stmtls_contains_uat(T_NLOOP_BODY(T_NLOOP(stmt)), attype);
      break;
    case S_REGION_SCOPE:
      val +=  stmtls_contains_uat(T_BODY(T_REGION(stmt)), attype);
      break;
    case S_WRAP:
    case S_REFLECT:
      val +=  expr_contains_uat(T_OPLS(T_WRAP(stmt)), attype);
      break;
    case S_MSCAN:
    case S_COMPOUND:
      val +=  stmtls_contains_uat(T_CMPD_STLS(stmt), attype);
      break;
    case S_EXPR:
      val +=  expr_contains_uat(T_EXPR(stmt), attype);
      break;
    case S_IF:
      val +=  expr_contains_uat(T_IFCOND(T_IF(stmt)), attype) +
	stmtls_contains_uat(T_THEN(T_IF(stmt)), attype) +
	stmtls_contains_uat(T_ELSE(T_IF(stmt)), attype);
      break;
    case S_LOOP:
      switch (T_TYPE(T_LOOP(stmt))) {
      case L_DO:
	val +=  expr_contains_uat(T_IVAR(T_LOOP(stmt)), attype) +
	  expr_contains_uat(T_START(T_LOOP(stmt)), attype) +
	  expr_contains_uat(T_STOP(T_LOOP(stmt)), attype) +
	  expr_contains_uat(T_STEP(T_LOOP(stmt)), attype) +
	  stmtls_contains_uat(T_BODY(T_LOOP(stmt)), attype);
	break;
      case L_WHILE_DO: 
      case L_REPEAT_UNTIL:
	val +=  expr_contains_uat(T_LOOPCOND(T_LOOP(stmt)), attype) +
	  stmtls_contains_uat(T_BODY(T_LOOP(stmt)), attype);
	break;
      default:
	INT_FATAL(NULL, "Bad looptype (%d) in replaceall_stmt()",T_TYPE(T_LOOP(stmt)));
      }
      break;
    case S_RETURN:
      val += expr_contains_uat(T_RETURN(stmt), attype);
      break;
    case S_IO:
      val += expr_contains_uat(IO_EXPR1(T_IO(stmt)), attype) +
	expr_contains_uat(IO_FILE(T_IO(stmt)), attype) +
	expr_contains_uat(IO_CONTROL(T_IO(stmt)), attype);
      break;
    default:
      INT_FATAL(stmt, "Bad stmnttype (%d) in replaceall_stmt()",T_TYPE(stmt));
    }
  }
  if (val > 0) {
    return 1;
  }
  return 0;
}

int stmtls_contains_zmacro(statement_t* stmt) {
  for (; stmt != NULL; stmt = T_NEXT(stmt)) {
    if (T_TYPE(stmt) == S_EXPR) {
      if (T_TYPE(T_EXPR(stmt)) == FUNCTION) {
	if (strncmp(S_IDENT(T_IDENT(T_OPLS(T_EXPR(stmt)))), "_ZMAC_", 6) == 0) {
	  return 1;
	}
      }
    }
  }
  return 0;
}


static expr_t* expr_find_ens_expr(expr_t* expr) {
  expr_t* e1;
  expr_t* e2;
    
  if (expr == NULL) {
    return NULL;
  }
  if (expr_is_atom(expr) &&
      !expr_is_indexi(expr) &&
      /*      !expr_is_promoted_indarr(expr) &&  -- SJD trying this */
      expr_find_ens_pst(expr)) {
    return expr;
  }
  e1 = T_OPLS(expr);
  while (e1 != NULL) {
    e2 = expr_find_ens_expr(e1);
    if (e2 != NULL) {
      return e2;
    }
    e1 = T_NEXT(e1);
  }
  return NULL;
}


expr_t* stmtls_find_ens_expr(statement_t* stmt) {
  expr_t* expr;
  for (; stmt != NULL; stmt = T_NEXT(stmt)) {
    switch (T_TYPE(stmt)) {
    case S_NULL:
    case S_EXIT:
    case S_HALT:
    case S_CONTINUE:
    case S_COMM:
    case S_END:
      break;
    case S_MLOOP:
      expr = stmtls_find_ens_expr(T_BODY(T_MLOOP(stmt)));
      if (expr != NULL) {
	return expr;
      }
      break;
    case S_NLOOP:
      expr = stmtls_find_ens_expr(T_NLOOP_BODY(T_NLOOP(stmt)));
      if (expr != NULL) {
	return expr;
      }
      break;
    case S_REGION_SCOPE: /* we should go into rscopes of different rank */
      break;
    case S_WRAP:
    case S_REFLECT:
      expr = expr_find_ens_expr(T_OPLS(T_WRAP(stmt)));
      if (expr != NULL) {
	return expr;
      }
      break;
    case S_MSCAN:
    case S_COMPOUND:
      expr = stmtls_find_ens_expr(T_CMPD_STLS(stmt));
      if (expr != NULL) {
	return expr;
      }
      break;
    case S_EXPR:
      expr = expr_find_ens_expr(T_EXPR(stmt));
      if (expr != NULL) {
	return expr;
      }
      break;
    case S_IF:
      expr = expr_find_ens_expr(T_IFCOND(T_IF(stmt)));
      if (expr != NULL) {
	return expr;
      }
      expr = stmtls_find_ens_expr(T_THEN(T_IF(stmt)));
      if (expr != NULL) {
	return expr;
      }
      expr = stmtls_find_ens_expr(T_ELSE(T_IF(stmt)));
      if (expr != NULL) {
	return expr;
      }
      break;
    case S_LOOP:
      switch (T_TYPE(T_LOOP(stmt))) {
      case L_DO:
	 expr = expr_find_ens_expr(T_IVAR(T_LOOP(stmt)));
	 if (expr != NULL) {
	   return expr;
	 }
	 expr = expr_find_ens_expr(T_START(T_LOOP(stmt)));
	 if (expr != NULL) {
	   return expr;
	 }
	 expr = expr_find_ens_expr(T_STOP(T_LOOP(stmt)));
	 if (expr != NULL) {
	   return expr;
	 }
	 expr = expr_find_ens_expr(T_STEP(T_LOOP(stmt)));
	 if (expr != NULL) {
	   return expr;
	 }
	 expr = stmtls_find_ens_expr(T_BODY(T_LOOP(stmt)));
	 if (expr != NULL) {
	   return expr;
	 }
	break;
      case L_WHILE_DO: 
      case L_REPEAT_UNTIL:
	 expr = expr_find_ens_expr(T_LOOPCOND(T_LOOP(stmt)));
	 if (expr != NULL) {
	   return expr;
	 }
	 expr = stmtls_find_ens_expr(T_BODY(T_LOOP(stmt)));
	 if (expr != NULL) {
	   return expr;
	 }
	break;
      default:
	INT_FATAL(NULL, "Bad looptype (%d) in stmtls_find_ens_expr()",T_TYPE(T_LOOP(stmt)));
      }
      break;
    case S_RETURN:
      expr = expr_find_ens_expr(T_RETURN(stmt));
      if (expr != NULL) {
	return expr;
      }
      break;
    case S_IO:
      expr = expr_find_ens_expr(IO_EXPR1(T_IO(stmt)));
      if (expr != NULL) {
	return expr;
      }
      expr = expr_find_ens_expr(IO_FILE(T_IO(stmt)));
      if (expr != NULL) {
	return expr;
      }
      expr = expr_find_ens_expr(IO_CONTROL(T_IO(stmt)));
      if (expr != NULL) {
	return expr;
      }
      break;
    default:
      INT_FATAL(stmt, "Bad stmnttype (%d) in stmtls_find_ens_expr()",T_TYPE(stmt));
    }
  }
  return NULL;
}


