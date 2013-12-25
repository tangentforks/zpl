/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: contraction.c - array contraction code
 * DATE: 25 September 1996
 * CREATOR: echris
 *
 * SUMMARY
 */

#include <stdio.h>
#include <limits.h> 
#include <string.h>
#include "../include/bb_traverse.h"
#include "../include/contraction.h"
#include "../include/createvar.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/main.h"
#include "../include/passes.h"
#include "../include/runtime.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/symboltable.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/buildzplstmt.h"
#include "../include/dtype.h"
#include "../include/dimension.h"

#define REFLIST_VAR(x) ((x)->var)
#define REFLIST_EXPR(x) ((x)->expr_p)
#define REFLIST_LIST(x) ((x)->reflist)
#define REFLIST_NEXT(x) ((x)->next)

#define IN 0
#define OUT 1

#define VISITED_TRUE 2
#define VISITED_FALSE 1

/* a structure definition */

struct reflist_struct {
  symboltable_t *var;   
  expr_t *expr_p;       /* expr containing ref to var */
  glist reflist;        /* list of statements that reference expr_p */
  struct reflist_struct *next;
};


/* here are some globals - of them I am not proud */
static expr_t *e_var;
static symboltable_t *s_var;    

static statement_t *mloop_statement;
static dvlist_t constraints;
static reflist_t *refs;
static glist instmts;

static int CdaFuse = 0;
static int CdaContract = 0;
static int CFuse = 0;
static int LFuse = 0;
static int XFuse = 0;
static int Contract = 0 ;

static int ArraysConsidered = 0;
static int UserArraysContracted = 0;
static int CdaArraysContracted = 0;


/* Some temporary notes from e...

What are the catagories of functions in this file?

- lsd calculation - to find loop structure definition given constraints
	hilow, prune, lsd

- contraction/fusion traversal and core code
	bb_process, 
	contraction, 
	profitable, noedges, dvs_due_stmts(used in legal also)
	legal,
	sort_mloop_body, sort_mloop_body2, no_ins, statement_in_stmtls,
	fuse, update_reflist,
	xfuse,
	topsort_stmts, topsort_stmtls, get_first, stmt_in_list, instmts_pre,
	closure, closure_set, closure_subset, 
	arrays_to_scalar, arrays_to_scalar_stmt, a_to_s_expr, 
		arrays_to_scalar_expr

- misc
	eq,
	build_ref_list, reflist_add, reflist_find, print_reflist,
	reflist_sort, reflist_length,
	glist_remove2, glist_remove_duplicates,
	glist_contains_nonmloops
	set_mloop_pre, set_mloop_post, set_mloop,
	build_constraints, constraints_in_mloop, set_constraints_pre, 
		set_contraints, set_constraints_stmtls,
	set_last_next (used by fuse and traversal, moved to bb_traverse.c),
	cda_var, cda_expr, get_var
	total_arrays_local, total_arrays_global
	inmscan, mscan_dep
 */


static int eq(void *a, void *b)
{
  return (a == b);
}

/* FUNCTION: glist_remove
 * Remove element from glist
 * RETURN: list minus element
 */
static glist glist_remove2(glist list, void *data)
{
  if (!list) return (NULL);

  if (GLIST_DATA(list) == data) {
    list = glist_remove2(GLIST_NEXT(list), data);
  } else {
    GLIST_NEXT(list) = glist_remove2(GLIST_NEXT(list), data);
  }

  return (list);
}

/* FUNCTION: glist_remove_duplicates - remove duplicates from list and
 *           return resulting list
 */

static glist glist_remove_duplicates(glist list)
{
  if (!list)
    return (NULL);
  
  GLIST_NEXT(list) = glist_remove2(GLIST_NEXT(list), GLIST_DATA(list));

  return (list);
}

static int glist_contains_nonmloops(glist list)
{
  statement_t *stmt;

  while (list) {
    stmt = (statement_t *) GLIST_DATA(list);
    if (T_TYPE(stmt) != S_MLOOP)
      return (TRUE);
    list = GLIST_NEXT(list);
  }

  return (FALSE);
}

/* BLC: a routine that gets rid of a contracted variable's setup/destroy
   signal in a statement's T_PRE/T_POST list */

static void contract_pre_post(statement_t *stmt,expr_t *var_expr,
			      symboltable_t *newvar) {
  symboltable_t *var_name;
  genlist_t *gptr;

  var_name = expr_find_root_pst(var_expr);
  if (var_name != NULL) {
    gptr = T_PRE(stmt);
    while (gptr != NULL) {
      if (G_IDENT(gptr) == var_name) {
	G_IDENT(gptr) = newvar;
      }
      gptr = G_NEXT(gptr);
    }

    gptr = T_POST(stmt);
    while (gptr != NULL) {
      if (G_IDENT(gptr) == var_name) {
	G_IDENT(gptr) = newvar;
      }
      gptr = G_NEXT(gptr);
    }
  }
}


/*
   CdaFuse     (-e)
   CdaContract (-t)
   CFuse       (-b)
   LFuse       (-f)
   Contract    (-c)
   All         (-a)
   XFuse       (-x)
 */
   

/* FUNCTION: total_arrays_local - return the total numer of arrays in module
 */

static int total_arrays_local(module_t *mod)
{
  function_t *ftp;
  symboltable_t *locals;
  int ret;

  ret = 0;
  while (mod) {
    ftp = T_FCNLS(mod);
    INT_COND_FATAL((ftp!=NULL), NULL, 
		   "No functions in mod in total_arrays_local()");
    while (ftp) {
      locals = T_CMPD_DECL(T_STLS(ftp));

      /* skip passed formal parameters */
      while (locals && S_FG_PARAM(locals)) {
	locals = S_SIBLING(locals);
      } 

      while (locals) {
	if (S_DTYPE(locals) && D_IS_ENSEMBLE(S_DTYPE(locals)) &&
	    S_IS_USED(locals))
	  ret++;

	/* skip passed formal parameters */
	do {
	  locals = S_SIBLING(locals);
	} while (locals && S_FG_PARAM(locals));
      }
      ftp = T_NEXT(ftp);
    }
    mod = T_NEXT(mod);
  }

  return (ret);
}

/* FUNCTION: total_arrays_global - return the total numer of global arrays
 *                                 (ignoring RMS and Indexi "vars")
 */

static int total_arrays_global(void)
{
  symboltable_t *globals;
  int ret;

  ret = 0;
  globals = maplvl[0];
  
  while(globals) {
    if ((S_DTYPE(globals) && D_IS_ENSEMBLE(S_DTYPE(globals))) &&
	!symtab_is_qmask(globals) && !symtab_is_indexi(globals)) {
      ret++;
    }
    globals = S_SIBPREV(globals);
  }

  return (ret);
}


static void build_constraints(statement_t *stmt) {
  outdep_t *deps;
  statement_t *headstmt;

  deps = T_OUTDEP(stmt);
  while(deps) {
    if (!OUTDEP_BACK(deps)) {
      headstmt = OUTDEP_HEAD_STMT(deps);
      if (T_IN_MLOOP(headstmt) == T_IN_MLOOP(stmt)) {
	constraints = dvlist_add(constraints, *OUTDEP_DIST(deps));
      }
    }
    deps = OUTDEP_NEXT(deps);
  }
}


static dvlist_t constraints_in_mloop(mloop_t *mloop) {
  constraints = NULL;
  traverse_stmtls_g(T_MLOOP_BODY(mloop), build_constraints, 
		    NULL, NULL, NULL);
  return (constraints);
}


static void set_constraints_pre(statement_t *stmt) {
  if (T_TYPE(stmt) == S_MLOOP) {
    T_MLOOP_CONSTR(T_MLOOP(stmt)) = constraints_in_mloop(T_MLOOP(stmt));
    IFDB(40) {
      dbg_gen_stmt(zstdout,stmt);
      dvlist_print(T_MLOOP_CONSTR(T_MLOOP(stmt)));
    }
  }
}


static void set_constraints_stmtls(statement_t *stmtls) {
  traverse_stmtls_g(stmtls, set_constraints_pre, NULL, NULL, NULL);
}


static void set_constraints(module_t *mod) {
  function_t *ftp;

  while (mod) {
    ftp = T_FCNLS(mod);
    INT_COND_FATAL((ftp!=NULL), NULL,
		   "No functions in mod in set_constraints()");
    while (ftp) {
      set_constraints_stmtls(T_STLS(ftp));
      ftp = T_NEXT(ftp);
    }
    mod = T_NEXT(mod);
  }
}


/* FUNCTION: reflist_add - add reference to variable expr var_expr in 
 *           statement s to reflist rl; return resulting reflist_t *
 */

static reflist_t *reflist_add(expr_t *var_expr, statement_t *s, reflist_t *rl)
{
  if (T_TYPE(var_expr) == BIAT) var_expr = T_OPLS(var_expr);

  if (T_TYPEINFO_REG(var_expr) && s)  {
    if (rl) {
      if (dependent_exprs(REFLIST_EXPR(rl),var_expr,MAY)) {
	REFLIST_LIST(rl) = glist_prepend_new(REFLIST_LIST(rl), s, 
					     GLIST_NODE_SIZE, eq);
      } else {
	REFLIST_NEXT(rl) = reflist_add(var_expr, s, REFLIST_NEXT(rl));
      }
    } else {
      rl = (reflist_t *)PMALLOC(sizeof(reflist_t));
      REFLIST_EXPR(rl) = var_expr;
      REFLIST_LIST(rl) = glist_prepend(NULL, s, GLIST_NODE_SIZE);
      REFLIST_NEXT(rl) = NULL;
    }
  }
  return (rl);
}


/* FUNCTION: build_ref_list - build a reference list for all variables
 *           that appear in stmtls
 */

static reflist_t *build_ref_list(statement_t *stmtls, reflist_t *old) {
  reflist_t *result;
  set_t *inset, *outset;
  genlist_t *exprs;

  result = old;

  while (stmtls) {
    if (T_TYPE(stmtls) == S_MLOOP) {
      result = build_ref_list(T_MLOOP_BODY(T_MLOOP(stmtls)), result);
      /* no need to look deeper, because shards are the only compound */
      /* statement type in mloops */
    }

    inset = T_IN(stmtls);
    while (inset) {
      exprs = SET_EXPRS(inset);
      while (exprs) {
	result = reflist_add(G_EXP(exprs), T_IN_MLOOP(stmtls), result);
	exprs = G_NEXT(exprs);
      }
      inset = SET_NEXT(inset);
    }
    outset = T_OUT(stmtls);
    while (outset) {
      exprs = SET_EXPRS(outset);
      while (exprs) {
	result = reflist_add(G_EXP(exprs), T_IN_MLOOP(stmtls), result);
	exprs = G_NEXT(exprs);
      }
      outset = SET_NEXT(outset);
    }
    stmtls = T_NEXT(stmtls);
  }
  return (result);
}


/* FUNCTION: print_reflist - display a printed representation of a reflist_t
 */

static void print_reflist(reflist_t *refs)
{
  while (refs) {
    printf("REFLIST_VAR = ");
    dbg_gen_expr(zstdout, REFLIST_EXPR(refs));
    printf("/%d\n", glist_length(REFLIST_LIST(refs)));
    refs = REFLIST_NEXT(refs);
  }
}


/* FUNCTION: reflist_length - return the number of variables in a reflist
 */

static int reflist_length(reflist_t *rl) {
  int count = 0;

  while (rl) {
    count++;
    rl = REFLIST_NEXT(rl);
  }

  return (count);
}


/* FUNCTION: reflist_sort - return a sorted version of a reflist_t;
 *           sorts by decreasing variable count; i.e. the most used
 *           variable is first in the returned reflist
 */

static reflist_t *reflist_sort(reflist_t *rl) {
  int length, i, j, tmp;
  reflist_t **nodes;
  int *nodes_size;
  reflist_t *tmprl;

  if (rl == NULL) 
    return (NULL);

  length = reflist_length(rl);
  nodes = (reflist_t **) PMALLOC(sizeof(reflist_t *) * length);
  nodes_size = (int *) PMALLOC(sizeof(int) * length);

  tmprl = rl;
  i = 0;
  while (tmprl) {
    nodes[i] = tmprl;
    nodes_size[i] = glist_length(REFLIST_LIST(tmprl));
    tmprl = REFLIST_NEXT(tmprl);
    i++;
  }

  /* sort nodes[] (decreasing) keying off nodes_size[] */
  /* bubble sort, I know, I know */
  for(i=0; i<length-1; i++) {
    for(j=0; j<length-1-i; j++) {
      if (nodes_size[i]>nodes_size[i+1]) {
	tmp = nodes_size[i];
	nodes_size[i] = nodes_size[i+1];
	nodes_size[i+1] = tmp;
	tmprl = nodes[i];
	nodes[i] = nodes[i+1];
	nodes[i+1] = tmprl;
      }
    }
  }

  for (i=0; i<length-1; i++) {
    REFLIST_NEXT(nodes[i]) = nodes[i+1];
  }
  REFLIST_NEXT(nodes[length-1]) = NULL;

  rl = nodes[0];

  return (rl);
}


/* FUNCTION: cda_var - returns true if var is a cda inserted array
 */

static int cda_var(symboltable_t *var) {
  return (strncmp(S_IDENT(var), "_cda_temp", 9) == 0);
}


/* FUNCTION: get_var - returns the base variable in an expr
 *           e.g., A.x[3] => A
 * echris - 2/13/98
 */

static symboltable_t *get_var(expr_t *expr) {
  while (expr && (T_TYPE(expr) != VARIABLE)) {
    expr = T_OPLS(expr);
  }

  return (expr?T_IDENT(expr):NULL);
}


/* FUNCTION: cda_expr - returns true if var in expr is a cda temp array
 * echris - 2/13/98
 */

static int cda_expr(expr_t *expr) {
  return (cda_var(get_var(expr)));
}


/* FUNCTION: closure_subset - add every statement between s1 and stmts
 *           in stmtls to list; cycles in dep graph will result in
 *           infinite recursion.  Returns FALSE iff there are no 
 *           path from s1 to stmts.
 * sungeun:  added recursion depth flag..  don't go deeper than 50
 */

static int closure_subset(statement_t *stmtls, statement_t *s1, glist stmts, 
		   glist *list, int recdepth)
{
  int flag;
  outdep_t *outdeps;
  statement_t *stmp, *stmp2, *s2, *s2b;
  glist tmpstmts;

  if (!s1 || !stmts) return (FALSE);

  /* return false if s1 is outside stmtls */
  stmp = T_IN_MLOOP(s1)?T_IN_MLOOP(s1):s1;
  stmp2 = stmtls;
  flag = FALSE;
  while (stmp2) {
    if (stmp2 == stmp) {
      flag = TRUE;
      break; /*** sungeun ***/
    }
    stmp2 = T_NEXT(stmp2);
  }
  if (flag == FALSE)
    return (FALSE);

  /* return if s1 has already been visited */
  if (T_VISITED(s1)) {
    if (T_VISITED(s1) == VISITED_TRUE) {
      return (TRUE);   /* visited and there exists a path to stmts from s1 */
    } else {   /* VISITED_FALSE */
      return (FALSE);   /* otherwise */
    }
  }

  /* recurse */
  flag = FALSE;
  if (recdepth < 50) {
    outdeps = T_OUTDEP(s1);
    while (outdeps) {
      if (!OUTDEP_BACK(outdeps)) {
	stmp = OUTDEP_HEAD_STMT(outdeps);
	if (closure_subset(stmtls, stmp, stmts, list, recdepth+1)) {
	  flag = TRUE;
	  stmp = T_IN_MLOOP(stmp)?T_IN_MLOOP(stmp):stmp;
	  *list = glist_prepend_new(*list, stmp, GLIST_NODE_SIZE, eq);
	}
      }
      outdeps = OUTDEP_NEXT(outdeps);
    }
  }
  if (flag) {
    T_VISITED(s1) = VISITED_TRUE;
    return (TRUE);
  } else {

    /* is s1 in stmts?  if yes, return TRUE */
    tmpstmts = stmts;
    while (tmpstmts) {
      s2 = (statement_t *)GLIST_DATA(tmpstmts);
      s2b = (T_TYPE(s2)==S_MLOOP)?T_MLOOP_BODY(T_MLOOP(s2)):NULL;
      while (s2b) {
	if (s1 == s2b) {
	  T_VISITED(s1) = VISITED_TRUE;
	  return (TRUE);
	}
	s2b = T_NEXT(s2b);
      }
      tmpstmts = GLIST_NEXT(tmpstmts);
    }
  }

  T_VISITED(s1) = VISITED_FALSE;
  return (FALSE);

}


static glist closure_set(statement_t *stmtls, glist stmts) {
  glist list;
  glist stmp1a;
  statement_t *stmp1b, *stmp1;

  list = NULL;

  /* mark all stmts (including statements in mloop) as unvisited */
  stmp1 = stmtls;
  while (stmp1) {
    T_VISITED(stmp1) = FALSE;
    stmp1b = (T_TYPE(stmp1)==S_MLOOP)?T_MLOOP_BODY(T_MLOOP(stmp1)):NULL;
    while (stmp1b) {
      T_VISITED(stmp1b) = FALSE;
      stmp1b = T_NEXT(stmp1b);
    }
    stmp1 = T_NEXT(stmp1);
  }

  /* for each statement in stmts (including statements in mloops) */
  stmp1a = stmts;
  while (stmp1a) {
    stmp1 = GLIST_DATA(stmp1a);
    stmp1b = (T_TYPE(stmp1)==S_MLOOP)?T_MLOOP_BODY(T_MLOOP(stmp1)):NULL;
    while (stmp1b) {
      closure_subset(stmtls, stmp1b, stmts, &list, 0);
      stmp1b = T_NEXT(stmp1b);
    } 
    stmp1a = GLIST_NEXT(stmp1a);
  }

  return (list);
}


/* FUNCTION: closure - return the statements in thetransative closure
 *           of all the statements in stmts (by the data deps).  This
 *           function does not consider statements outside stmtls because
 *           it assumes that such "loops" can never occur (echris: think
 *           about this again?)
 *           return NULL if not all stmts in the closure have type S_MLOOP
 */

static glist closure(statement_t *stmtls, glist stmts) {
  glist newlist;

  newlist = glist_append_list(stmts, closure_set(stmtls, stmts));

  /* remove duplicates from newlist */
  newlist = glist_remove_duplicates(newlist);

  if (glist_contains_nonmloops(newlist))
    return (NULL);
  else
    return (newlist);
}


/* FUNCTION: contractible_expr - return TRUE if expr is a contractible
 *           expr (ie lvalue with no array_refs)
 * echris - 2/17/98
 */

static int contractible_expr(expr_t *expr) {
  while (expr) {
    switch (T_TYPE(expr)) {
    case VARIABLE:
      return TRUE;
    case BIAT:
      expr = T_OPLS(expr);
      break;
    case BIDOT:
      expr = T_OPLS(expr);
      break;
    case ARRAY_REF:
      if (expr_find_ensemble_root(T_OPLS(expr))) {
	expr = T_OPLS(expr);
      } else {
	return FALSE;
      }
      break;
    default:
      return FALSE;
    }
  }
  return FALSE;
}


/* FUNCTION: inmscan - return the S_MSCAN statement that contains the
 *                     argument statement, s; if s does not 
 *                     appear in a MSCAN block, NULL is returned.
 */

static statement_t *inmscan(statement_t *s) {
  if (s == NULL) {
    return (NULL);
  }

  if (T_TYPE(s) == S_MLOOP) {
    return (MLOOP_IS_MSCAN(s)?s:NULL);
  }

  return (inmscan(T_PARENT(s)));
}


/* FUNCTION: mscan_dep - return TRUE if both the head and the tail of the 
 *                       argument indep appear in the same mscan block.
 */

static int mscan_dep(indep_t *dep) {
  statement_t *s1, *s2;

  s1 = inmscan(INDEP_STMT(dep));
  s2 = inmscan(INDEP_TAIL_STMT(dep));
	       
  return (s1 && (s1 == s2));
}


/* FUNCTION: dvs_due_stmts - return a list of dvs that represents all the
 *           constraints on an mloop that contains all the statements in
 *           stmts; ignore_mscan is used to ignore deps that have both
 *           their head and tail in a scan block
 * NOTES:    stmtls is not used
 *           a duplicate-free list is returned
 */

static dvlist_t dvs_due_stmts(statement_t *stmtls, glist stmts, 
			      depedgetype_t t, expr_t *var_expr, 
			      int ignore_mscan) {
  glist tmp;
  statement_t *stmt;
  indep_t *ins;
  dvlist_t dvs;
  expr_t *expr;

  dvs = NULL;

  tmp = stmts;
  /* for each statement in stmts */
  while (tmp) {                       
    stmt = (statement_t *)GLIST_DATA(tmp);
    if (T_TYPE(stmt) ==  S_MLOOP) {
      stmt = T_MLOOP_BODY(T_MLOOP(stmt));
      /* for each statement in mloop body of stmt */
      while (stmt) {               
	ins = T_INDEP(stmt);
        /* for each indep in stmt */
	while (ins) {
	  if (!INDEP_BACK(ins)) {
	    expr = INDEP_EXPR(ins);
	    /* is this an intra mloop dependence? */
	    if (glist_find(stmts, T_IN_MLOOP(INDEP_TAIL_STMT(ins)))) {
	      if (((t == XANY) || (t == INDEP_TYPE(ins))) &&
		  ((var_expr == NULL) || 
		   (dependent_exprs(var_expr,expr,MAY))) &&
		  (!mscan_dep(ins) || !ignore_mscan)) {
		dvs = dvlist_add(dvs, *INDEP_DIST(ins));
	      }
	    }
	  }
	  ins = INDEP_NEXT(ins);
	}
	stmt = T_NEXT(stmt);
      }
    }
    tmp = GLIST_NEXT(tmp);
  }

  return (dvlist_rm_dups(dvs));
}


/* FN: mk_ginls - build a genlist_t node containing indep_t d
 */

static genlist_t *mk_ginls(genlist_t *ls, indep_t *d)
{
  genlist_t *node = (genlist_t *) PMALLOC(sizeof(genlist_t));

  G_INDEP(node) = d;
  G_NEXT(node) = ls;

  return(node);
}

/* FN: mk_goutls - build a genlist_t node containing outdep_t d
 */

static genlist_t *mk_goutls(genlist_t *ls, outdep_t *d)
{
  genlist_t *node = (genlist_t *) PMALLOC(sizeof(genlist_t));

  G_OUTDEP(node) = d;
  G_NEXT(node) = ls;

  return(node);
}

/* FUNCTION: noedges - returns a genlist_t (of outdep_t or indep_t) of
 *           IN/OUT edges to/from any statement in stmts due to var_expr
 *           dir = IN (0) or OUT (1)
 * NOTES     stmtls is not used
 */

static genlist_t *noedges(statement_t *stmtls, glist stmts, expr_t *var_expr,
			  int dir) {
  glist tmp;
  statement_t *stmt, *nstmt;
  outdep_t *outs;
  indep_t *ins;
  int innloop;
  genlist_t *ls = NULL;
  
  tmp = stmts;
  /* for each statement in stmts */
  while (tmp) {
    stmt = (statement_t *)GLIST_DATA(tmp);
    if (T_TYPE(stmt) ==  S_MLOOP) {
      stmt = T_MLOOP_BODY(T_MLOOP(stmt));
      /* for each statement in mloop body of stmt */
      while (stmt) {
	innloop = (T_TYPE(stmt) == S_NLOOP);
	nstmt = innloop?T_NLOOP_BODY(T_NLOOP(stmt)):stmt;
	do {
	  outs = (dir==OUT)?T_OUTDEP(nstmt):NULL;
	  ins = (dir==IN)?T_INDEP(nstmt):NULL;
	  /* at this point either outs or ins must be NULL */
	  /* for each outdep in nstmt */
	  while (outs) {
	    if (dependent_exprs(OUTDEP_EXPR(outs), var_expr, MAY)) {
	      if (!glist_find(stmts, T_IN_MLOOP(OUTDEP_HEAD_STMT(outs)))) {
		ls = mk_goutls(ls, outs);
	      }
	    }
	    outs = OUTDEP_NEXT(outs);
	  }
	  /* for each indep in nstmt */
	  while (ins) {
	    if (dependent_exprs(INDEP_EXPR(ins), var_expr, MAY)) {
	      if (!glist_find(stmts, T_IN_MLOOP(INDEP_TAIL_STMT(ins)))) {
		ls = mk_ginls(ls, ins);
	      }
	    }
	    ins = INDEP_NEXT(ins);
	  }
	  /* only move on to next statement if in an nloop */
	  nstmt = innloop?T_NEXT(nstmt):nstmt;
        } while (innloop && nstmt);
	stmt = T_NEXT(stmt);
      }
    }
    tmp = GLIST_NEXT(tmp);
  }

  return (ls);
}


/* FN: legal_partial - this function compares two regions to determine 
 *                     whether one, sother, is a valid input/output
 *                     for a scan block computing over the other region,
 *                     sbase; rclass may be, for example, IN_REGION or
 *                     OF_REGION; dim is the dimension to be contracted
 *                     and val is the val of the '@ direction in that
 *                     dimension
 */

static int legal_partial(expr_t *sbase, expr_t *sother,
			 regionclass rclass, int dim, int val)
{
  expr_t *dir;
  int rank, i;
  long dirval;
  int success;

  INT_COND_FATAL((sbase!=NULL), NULL, "NULL arg1 in legal_partial_base()");
  INT_COND_FATAL((sother!=NULL), NULL, "NULL arg2 in legal_partial_base()");
  INT_COND_FATAL((D_CLASS(T_TYPEINFO(sbase))==DT_REGION), NULL, 
		 "Bad arg1 type legal_partial_base()");
  INT_COND_FATAL((D_CLASS(T_TYPEINFO(sother))==DT_REGION), NULL, 
		 "Bad arg2 type legal_partial_base()");
  if (T_SUBTYPE(sother) != rclass) return(FALSE);
  if (T_OPLS(sother) != sbase) return(FALSE);

  dir = T_NEXT(T_OPLS(sother));
  rank = D_DIR_NUM(T_TYPEINFO(dir));
  for (i=0; i<rank; i++) {
    /* BC+SD: This only works for literal directions */
    dirval = dir_comp_to_int(T_NEXT(T_OPLS(sother)), i, &success);
    if (!success || dirval != ((i!=dim)?0:val)) {
      return (FALSE);
    }
  }
  
  return (TRUE);
}


/* FUNCTION: profitable - returns the dim (1-based) of variable expr
 *           var_expr that may be profitably contracted if the statements
 *           in stmts are fused 
 *           INT_MAX => full contraction
 *
 * conditions under which it is profitable (contraction potential)
 *  - expr is contractible (i.e., of supported datatype)
 *  - candidate is local var
 *  - full contraction
 *    . all udvs due to var are zero
 *    . values do not flow in or out of stmtls due to var
 *  - partial contraction 
 *    . only 1 (non-null, cardinal) udv due to var
 *    . limited flow due to var in and out of scan block
 * NOTE: Maybe flow into scan block doesn't matter?
 */

static int profitable(statement_t *stmtls, glist stmts, expr_t *var_expr) {
  dvlist_t dvs, tmp;
  symboltable_t *sym;
  int allnull = TRUE;
  int len;
  int rank, i, dim, val;
  statement_t *scope, *otherscope, *s;
  genlist_t *ls;
  outdep_t *outs;
  indep_t *ins;

  if (!stmts) return (FALSE);

  scope = T_SCOPE((statement_t*)GLIST_DATA(stmts));

  /* test that candidate variable expr is contractible expression */
  if (!contractible_expr(var_expr)) {
    return (FALSE);
  }

  /* test that candidate variable expr is local */
  sym = get_var(var_expr);
  if (symtab_is_global(sym) || symtab_is_param(sym)) {
    return (FALSE);
  }

  /* test that all dvs due to expr var are zero */
  dvs = dvs_due_stmts(stmtls, stmts, XANY, var_expr, FALSE);
  len = glist_length((glist)dvs);
  tmp = dvs;
  while(tmp) {
    if (!dv_zero(DVLIST_DV(tmp)))
      allnull = FALSE;
    tmp = DVLIST_NEXT(tmp);
  }
  /* free dvs? */

  /* full contraction case (all udvs are null */
  if (allnull) {
    /* test that first ref is def */
    /* be conservative; test that there are no indeps due to var */
    ls = noedges(stmtls, stmts, var_expr, OUT);
    while (ls) {
      outs = G_OUTDEP(ls);
      if (dependent_exprs(OUTDEP_EXPR(outs), var_expr, MAY)) {
	if (OUTDEP_TYPE(outs) == XFLOW) return (FALSE);
      }
      ls = G_NEXT(ls);
    }

    /* test that last ref is dead */
    ls = noedges(stmtls, stmts, var_expr, IN);
    while (ls) {
      ins = G_INDEP(ls);
      if (dependent_exprs(INDEP_EXPR(ins), var_expr, MAY)) {
	if (INDEP_TYPE(ins) == XFLOW) return (FALSE);
      }
      ls = G_NEXT(ls);
    }

    return (INT_MAX);
  }

  /* partial contraction case (only 1 (non-null) udv */
  if (len == 1) {
    rank = (DVLIST_DV(dvs))[MAXRANK];
    dim = -1;
    /* only cardinal directions now */
    for(i=0; i<rank; i++) {
      if ((DVLIST_DV(dvs))[i] != 0) {
	if (dim != -1) return (FALSE);
	else {
	  dim = i;
	  val = (DVLIST_DV(dvs))[i];
	}
      }
    }

    /* check in edges */
    ls = noedges(stmtls, stmts, var_expr, IN);
    while (ls) {
      s = T_IN_MLOOP(INDEP_TAIL_STMT(G_INDEP(ls)))?
	  T_IN_MLOOP(INDEP_TAIL_STMT(G_INDEP(ls))):
          INDEP_TAIL_STMT(G_INDEP(ls));
      otherscope = T_SCOPE(s);
      if ((scope == NULL) || (otherscope == NULL) ||
	  !legal_partial(T_REGION_SYM(T_REGION(scope)),
			 T_REGION_SYM(T_REGION(otherscope)),
			 OF_REGION, dim, val)) {
	return (FALSE);
      }
      ls = G_NEXT(ls);
    }
    /* free ls */

    /* check out edges */
    ls = noedges(stmtls, stmts, var_expr, OUT);
    while (ls) {
      s = T_IN_MLOOP(OUTDEP_HEAD_STMT(G_OUTDEP(ls)))?
          T_IN_MLOOP(OUTDEP_HEAD_STMT(G_OUTDEP(ls))):
          OUTDEP_HEAD_STMT(G_OUTDEP(ls));
      otherscope = T_SCOPE(s);
      if ((scope == NULL) || (otherscope == NULL) ||
	  !legal_partial(T_REGION_SYM(T_REGION(scope)),
			 T_REGION_SYM(T_REGION(otherscope)),
			 IN_REGION, dim, -val)) {
	return (FALSE);
      }
      ls = G_NEXT(ls);
    }
    /* free ls */

    return (dim+1);

  }

  return (0);
}

static int randacc_uats_in_exprls(expr_t* super) {
  if (super == NULL) {
    return 0;
  }
  if (T_TYPE(super) == BIAT && T_SUBTYPE(super) == AT_RANDACC) {
    return 1;
  }
  return randacc_uats_in_exprls(T_NEXT(super)) || randacc_uats_in_exprls(T_OPLS(super));
}

/* return TRUE if any BIATS in stmt are randomly accessed */
static int randacc_uats_in_stmt(statement_t* stmt) {
  int result;
  if (stmt == NULL) {
    return 0;
  }
  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_HALT:
  case S_CONTINUE:
  case S_COMM:
  case S_END:
    result = 0;
    break;
  case S_MLOOP:
    result = randacc_uats_in_stmt(T_BODY(T_MLOOP(stmt)));
    break;
  case S_NLOOP:
    result = randacc_uats_in_stmt(T_NLOOP_BODY(T_NLOOP(stmt)));
    break;
  case S_REGION_SCOPE:
    result = randacc_uats_in_stmt(T_BODY(T_REGION(stmt)));
    break;
  case S_WRAP:
  case S_REFLECT:
    result = randacc_uats_in_exprls(T_OPLS(T_WRAP(stmt)));
    break;
  case S_MSCAN:
  case S_COMPOUND:
    result = randacc_uats_in_stmt(T_CMPD_STLS(stmt));
    break;
  case S_EXPR:
    result = randacc_uats_in_exprls(T_EXPR(stmt));
    break;
  case S_IF:
    result = randacc_uats_in_exprls(T_IFCOND(T_IF(stmt))) ||
      randacc_uats_in_stmt(T_THEN(T_IF(stmt))) ||
      randacc_uats_in_stmt(T_ELSE(T_IF(stmt)));
    break;
  case S_LOOP:
    switch (T_TYPE(T_LOOP(stmt))) {
    case L_DO:
      result = randacc_uats_in_exprls(T_IVAR(T_LOOP(stmt))) ||
	randacc_uats_in_exprls(T_START(T_LOOP(stmt))) ||
	randacc_uats_in_exprls(T_STOP(T_LOOP(stmt))) ||
	randacc_uats_in_exprls(T_STEP(T_LOOP(stmt))) ||
	randacc_uats_in_stmt(T_BODY(T_LOOP(stmt)));
      break;
    case L_WHILE_DO: 
    case L_REPEAT_UNTIL:
      result = randacc_uats_in_exprls(T_LOOPCOND(T_LOOP(stmt))) ||
	randacc_uats_in_stmt(T_BODY(T_LOOP(stmt)));
      break;
    default:
      INT_FATAL(NULL, "Bad looptype (%d) in replaceall_stmt()",T_TYPE(T_LOOP(stmt)));
    }
    break;
  case S_RETURN:
    result = randacc_uats_in_exprls(T_RETURN(stmt));
    break;
  case S_IO:
    result = randacc_uats_in_exprls(IO_EXPR1(T_IO(stmt))) ||
      randacc_uats_in_exprls(IO_FILE(T_IO(stmt))) ||
      randacc_uats_in_exprls(IO_CONTROL(T_IO(stmt)));
    break;
  default:
    INT_FATAL(stmt, "Bad stmnttype (%d) in replaceall_stmt()",T_TYPE(stmt));
  }
  return result;
}

/* FUNCTION: legal - returns TRUE if it is legal to fuse all the statements
 *           in stmts
 *
 * conditions under which it is legal
 *  - all stmts in stmts are also in stmtls (x)
 *  - all over same region (x)
 *  - legal lsd (x)
 *  - deps preserved - no cycles (don't need to do this, because of growing)
 *  - don't fuse statements that require intervening comm (x)
 *  - no BIATS in statements are of subtype AT_RANDACC
 */

static int legal(statement_t *stmtls, glist stmts) {
  dvlist_t dvs;
  distvect_t res;
  glist tmp;
  statement_t *tmp2, *region_scope;
  int flag;

  if (!stmts)
    return (FALSE);

  /* test that all statements in stmts are also in stmtls */
  tmp = stmts;
  while (tmp) {
    tmp2 = stmtls;
    flag = FALSE;
    while (tmp2) {
      if (GLIST_DATA(tmp) == tmp2)
	flag = TRUE;
      tmp2 = T_NEXT(tmp2);
    }
    if (!flag)
      return (FALSE);
    tmp = GLIST_NEXT(tmp);
  }

  /* test that all stmts are over the same region */
  tmp = stmts;
  region_scope = T_SCOPE((statement_t *)GLIST_DATA(tmp));
  tmp = GLIST_NEXT(tmp);
  while (tmp) {
    if (T_SCOPE((statement_t *)GLIST_DATA(tmp)) != region_scope)
      return (FALSE);
    tmp = GLIST_NEXT(tmp);
  }

  /* test that none of the BIATs in the stmts are randomly accessed */
  tmp = stmts;
  while (tmp) {
    if (randacc_uats_in_stmt((statement_t*)GLIST_DATA(tmp))) {
      return FALSE;
    }
    tmp = GLIST_NEXT(tmp);
  }

  /* test that there exists a legal lsd */
  dvs = dvs_due_stmts(stmtls, stmts, XANY, NULL, FALSE);
  if (dvs && !lsd(dvs, res))
    return (FALSE);
  /* free dvs */

  IFDB(40) {
    printf("dvs: ");
    dvlist_print(dvs);
    printf("res: ");
    dv_print(res);
  }

  /* test that no intervening comm is required */
  dvs = dvs_due_stmts(stmtls, stmts, XFLOW, NULL, TRUE);
  while (dvs) {
    if (!dv_zero(DVLIST_DV(dvs)))
      return (FALSE);
    dvs = DVLIST_NEXT(dvs);
  }
  /* free dvs */

  /* if all the tests have passed, return true */
  return (TRUE);
}


/* FUNCTION: update_reflist - change all references to statement old in
 *           reflist rl to references to new; this is required because
 *           fusion moves statements from one mloop to another
 */

static reflist_t *update_reflist(reflist_t *rl,statement_t *old,
				 statement_t *new) {
  reflist_t *tmp;
  glist l;

  tmp = rl;
  while(tmp) {
    l = REFLIST_LIST(tmp);
    while (l) {
      if (GLIST_DATA(l) == old) {
	GLIST_DATA(l) = new;
      }
      l = GLIST_NEXT(l);
    }
    tmp = REFLIST_NEXT(tmp);
  }
  return (rl);
}


/* set the T_IN_MLOOP field of each statement */

static void set_mloop_pre(statement_t *stmt)
{
  if (T_TYPE(stmt) == S_MLOOP) {
    mloop_statement = stmt;
    T_IN_MLOOP(stmt) = NULL;
  } else {
    T_IN_MLOOP(stmt) = mloop_statement;
  }
}


static void set_mloop_post(statement_t *stmt)
{
  if (T_TYPE(stmt) == S_MLOOP) {
    mloop_statement = NULL;
  }
}


static void set_mloop_stmtls(statement_t *stmtls) {
  mloop_statement = NULL;

  traverse_stmtls_g(stmtls, set_mloop_pre, set_mloop_post, NULL, NULL);
}


/* FUNCTION: fuse - fuse all statements in glist stmts into a single
 *           mloop, update reflist to reflect this change, and return
 *           the new stmtls that reflects the change; the body of the
 *           fused mloop is returned in thebody
 */

static statement_t *fuse(statement_t *stmtls, glist stmts, reflist_t *reflist, 
			 statement_t **thebody) {
  statement_t *s, *tmp, *body;

  if (!stmtls || !stmts) 
    return (stmtls);

  if (!glist_find(stmts, stmtls)) {
    T_NEXT(stmtls) = fuse(T_NEXT(stmtls),stmts, reflist, thebody);
    return (stmtls);
  }

  INT_COND_FATAL((T_TYPE(stmtls) == S_MLOOP), NULL,
		 "Bad statement type (%d) in fuse()", T_TYPE(stmtls));
  body = T_MLOOP_BODY(T_MLOOP(stmtls));

  while(stmts) {
    s = GLIST_DATA(stmts);
    if (s != stmtls) {
      /* only set fused bit if there are at least 2 statements */
      reflist = update_reflist(reflist, s, stmtls);
      tmp = T_PREV(s);
      if (tmp) 
	T_NEXT(tmp) = T_NEXT(s);
      if (T_NEXT(s)) {
	T_PREV(T_NEXT(s)) = tmp;
      }
      INT_COND_FATAL((T_TYPE(s) == S_MLOOP), NULL,
		     "Bad statement type (%d) in fuse() in loop", T_TYPE(s));
      set_last_next(body, T_MLOOP_BODY(T_MLOOP(s)));
      T_MLOOP_BODY(T_MLOOP(s)) = NULL;
      T_SUBTYPE(stmtls) |= MLOOP_FUSED | T_SUBTYPE(s);

      /* update pre/post properties */
      T_PRE(stmtls) = cat_genlist_ls(T_PRE(stmtls), T_PRE(s));
      T_POST(stmtls) = cat_genlist_ls(T_POST(stmtls), T_POST(s));
    }
    stmts = GLIST_NEXT(stmts);
  }

  set_mloop_stmtls(stmtls);
  set_constraints_stmtls(stmtls);

  *thebody = body;
  return (stmtls);
}


/* FUNCTION: arrays_to_scalar_expr - replace all occurences of ens_var
 *           with scalar_var in expr; if BIAT, BIDOT or VARIABLE expression 
 *           is changed to scalar_var, the new expression is returned;
 *           otherwise, NULL is returned.
 * echris - 2/13/98
 */

static expr_t *arrays_to_scalar_expr(expr_t *expr, expr_t *var_expr, 
				     symboltable_t *scalar_var) {
  expr_t *return_expr, *new_expr, *tmpexpr;

  if (!expr)
    return (NULL);

  /* take care of NEXT expression first */
  return_expr = arrays_to_scalar_expr(T_NEXT(expr), var_expr, scalar_var);
  if (return_expr)
    T_NEXT(expr) = return_expr;

  /* have we found the matching expression? */
  if (expr_equal(var_expr,expr)) {
    /* allocate new expression */
    new_expr = alloc_expr(VARIABLE);
    T_IDENT(new_expr) = scalar_var;
    T_NEXT(new_expr) = return_expr?return_expr:T_NEXT(expr);
    /* fixup will fill in all the right fields? */

    /* deallocate old expression? */

    /* return pointer to new expression */
    return (new_expr);
  }

  /* otherwise, recurse */
  switch(T_TYPE(expr)) {

  case BIAT:
    return_expr = arrays_to_scalar_expr(T_OPLS(expr), var_expr, scalar_var);
    if (return_expr) {
      T_NEXT(return_expr) = T_NEXT(expr);
      expr = return_expr;
    } else {
      expr = NULL;
    }
    /* deallocate this node? */
    break;

  case BIDOT:
    return_expr = arrays_to_scalar_expr(T_OPLS(expr), var_expr, scalar_var);
    if (return_expr) {
      T_NEXT(return_expr) = T_NEXT(expr);
      expr = return_expr;
    } else {
      expr = NULL;
    }
    break;
    
  case ARRAY_REF:
    return_expr = arrays_to_scalar_expr(T_OPLS(expr), var_expr, scalar_var);
    if (return_expr) {
      /* keep array subscripts if they exist */
      tmpexpr = T_OPLS(expr)?T_NEXT(T_OPLS(expr)):NULL;
      T_NEXT(return_expr) = tmpexpr;   /* tack on old array subscripts */
      T_OPLS(expr) = return_expr;
    }
    expr = NULL;
    break;

  case VARIABLE:
    expr = NULL;
    break;

  default:
    return_expr = arrays_to_scalar_expr(T_OPLS(expr), var_expr, scalar_var);
    if (return_expr) {
      /* keep array subscripts if they exist */
      tmpexpr = T_OPLS(expr)?T_NEXT(T_OPLS(expr)):NULL;
      T_NEXT(return_expr) = tmpexpr;   /* tack on old array subscripts */
      T_OPLS(expr) = return_expr;
    }
    expr = NULL;
    break;
  }

  return (expr);
}


/* FUNCTION: create_name_from_expr - create a legal identifier  based
 *           on an lvalue expression
 * echris - 2/13/98
 */

static char *create_name_from_expr(expr_t *expr)
{
  int size;
  char *str, *sub_str;

  switch(T_TYPE(expr)) {
  case VARIABLE:
    size = 1+strlen(S_IDENT(T_IDENT(expr)));
    str = PMALLOC(size);
    strcpy(str,S_IDENT(T_IDENT(expr)));
    return (str);

  case BIAT:
    sub_str = create_name_from_expr(T_OPLS(expr));
    size = 1 + 4 + strlen(sub_str) + strlen(S_IDENT(expr_find_root_pst(T_NEXT(T_OPLS(expr)))));
    str = PMALLOC(size);
    sprintf(str,"%s_at_%s", sub_str, S_IDENT(expr_find_root_pst(T_NEXT(T_OPLS(expr)))));
    /* deallocate sub_str */
    PFREE(sub_str,1+strlen(sub_str));
    return (str);

  case BIDOT:
    sub_str = create_name_from_expr(T_OPLS(expr));
    size = 1 + 5 + strlen(sub_str) + strlen(S_IDENT(T_IDENT(expr)));
    str = PMALLOC(size);
    sprintf(str,"%s_dot_%s", sub_str, S_IDENT(T_IDENT(expr)));
    /* deallocate sub_str */
    PFREE(sub_str,1+strlen(sub_str));
    return (str);

  case ARRAY_REF:
    sub_str = create_name_from_expr(T_OPLS(expr));
    return sub_str;

  default:
    INT_FATAL(T_STMT(expr), "Bad T_TYPE in create_name_from_expr()");
    return (NULL);   /* put this here to eliminate warnings */
  }
}


/* FUNCTION: a_to_s_stmt - replace all occurences of e_var with
 *           s_var in noncompound part of stmt (e.g. T_START part
 *           of L_DO loop).  The body of compound stmts are handled
 *           elsewhere (a_to_s_expr).
 * echris - 6/5/98
 */

static void a_to_s_stmt(statement_t *stmt) {
  expr_t *ret;

  contract_pre_post(stmt,e_var,s_var);

  switch(T_TYPE(stmt)) {
  case S_IF:
    ret = arrays_to_scalar_expr(T_IFCOND(T_IF(stmt)), e_var, s_var);
    if (ret) { T_IFCOND(T_IF(stmt)) = ret; }
    break;
  case S_LOOP:
    switch(T_TYPE(T_LOOP(stmt))) {
    case L_DO:
      ret = arrays_to_scalar_expr(T_IVAR(T_LOOP(stmt)), e_var, s_var);
      if (ret) { T_IVAR(T_LOOP(stmt)) = ret; }
      ret = arrays_to_scalar_expr(T_START(T_LOOP(stmt)), e_var, s_var);
      if (ret) { T_START(T_LOOP(stmt)) = ret; }
      ret = arrays_to_scalar_expr(T_STOP(T_LOOP(stmt)), e_var, s_var);
      if (ret) { T_STOP(T_LOOP(stmt)) = ret; }
      ret = arrays_to_scalar_expr(T_STEP(T_LOOP(stmt)), e_var, s_var);
      if (ret) { T_STEP(T_LOOP(stmt)) = ret; }
      break;
    case L_WHILE_DO:
    case L_REPEAT_UNTIL:
      ret = arrays_to_scalar_expr(T_LOOPCOND(T_LOOP(stmt)), e_var, s_var);
      if (ret) { T_LOOPCOND(T_LOOP(stmt)) = ret; }
      break;
    }
    break;
  default:
    break;
  }
}


/* FUNCTION: a_to_s_expr - replace all occurences of e_var with s_var
 *           in expr
 * echris - 2/13/98
 */

static void a_to_s_expr(expr_t *expr) {
  /* don't need to look at return value, because arrays_to_scalar_expr()
   * will never be called on a single VARIABLE expression */
  arrays_to_scalar_expr(expr, e_var, s_var);
}


/* FUNCTION: arrays_to_scalar_stmt - replace all occurences of ens_var
 *           with scalar_var in stmt
 * echris - 2/13/98
 */

static void arrays_to_scalar_stmt(statement_t *stmt, expr_t *var_expr, 
				  symboltable_t *scalar_var) {
  T_IN(stmt)  = prune_set(T_IN(stmt), var_expr);
  T_OUT(stmt) = prune_set(T_OUT(stmt), var_expr);
  s_var = scalar_var;
  e_var = var_expr;
  
  traverse_stmt_g(stmt, a_to_s_stmt, NULL, a_to_s_expr, NULL);
}


/* FUNCTION: arrays_to_scalar - create a new local scalar, and replace all
 *           occurences of var_expr in stmts with this new scalar
 */

static void arrays_to_scalar(statement_t *stmts, expr_t *var_expr, 
			     statement_t *stmtls) {
  datatype_t    *dt;
  function_t    *fn;
  char          *var_name;
  char          *old_name;
  symboltable_t *new_var;
  symboltable_t *locals;

  /* the root is really what we're looking for for array refs.
     is it different for bidots and uats?  --BLC */
  while (T_TYPE(var_expr) == ARRAY_REF) {
    var_expr = T_OPLS(var_expr);
  }

  fn       = T_PARFCN(stmts);
  dt       = T_TYPEINFO(var_expr);
  old_name = create_name_from_expr(var_expr);

  var_name = (char *) PMALLOC(strlen(old_name)+9);
  sprintf(var_name, "_scalar_%s", old_name);

  /* has this var already been allocated? */
  locals = T_CMPD_DECL(T_STLS(fn));
  while (locals) {
    if (!strcmp(var_name, S_IDENT(locals))) {
      new_var = locals;
      break;
    }
    locals = S_SIBLING(locals);
  }

  if (locals == NULL) {  /* if var_name does not already exist */
    new_var = create_named_local_var(dt, fn, var_name);
  }

  /* used previous allocated var if it already exists */
  /* this is a problem if it was declared by the user; but there should */
  /* be no conflicts with compiler variables */

  /* take care of the containing statement MLOOP */
  while (stmtls) {
    contract_pre_post(stmtls,var_expr,new_var);
    stmtls = T_NEXT(stmtls);
  }
  while (stmts) {
    arrays_to_scalar_stmt(stmts, var_expr, new_var);
    stmts = T_NEXT(stmts);
  }
}


/* FUNCTION: containsmscan - returns TRUE if stmts contains an mscan mloop
 */

static int containsmscans(glist stmts)
{
  while (stmts) {
    if (MLOOP_IS_MSCAN(((statement_t *)GLIST_DATA(stmts)))) {
      return (TRUE);
    }
    stmts = GLIST_NEXT(stmts);
  }

  return (FALSE);
}


static void instmts_pre(statement_t *stmt) {
  indep_t *ins;

  ins = T_INDEP(stmt);
  while (ins) {
    if (!INDEP_BACK(ins)) {
      instmts = glist_prepend_new(instmts, INDEP_TAIL_STMT(ins), 
				  GLIST_NODE_SIZE, eq);
    }
    ins = INDEP_NEXT(ins);
  }
}


/* FUNCTION: get_instmts - return a glist of all the statements that have
 *           dep arcs into stmt (which may be an S_MLOOP that contains 
 *           other statements)
 */

static glist get_instmts(statement_t *stmt) {
  instmts = NULL;
  traverse_stmt_g(stmt, instmts_pre, NULL, NULL, NULL);  
  return (instmts);
}


static int stmt_in_list(statement_t *stmtls, statement_t *stmt) {
  if (!stmtls || !stmt)
    return (FALSE);
  else if (stmtls == stmt)
    return (TRUE);
  else 
    return (stmt_in_list(T_NEXT(stmtls), stmt));
}


/* FUNCTION: get_first - return a statement in stmtls that has no dep in
 *           edges from other statements in stmtls; if no such statement
 *           exists, return NULL.
 */

static statement_t *get_first(statement_t *stmtls) {
  statement_t *tmp, *tailstmt;
  glist tmplist;
  int flag;

  tmp = stmtls;
  while (tmp) {			/* for each statement in stmtls */
    if (!T_VISITED(tmp)) {
      tmplist = get_instmts(tmp);
      flag = TRUE;
      while(tmplist) {		/* for each statement in in dep set */
	tailstmt = GLIST_DATA(tmplist);
	if ((tailstmt != tmp) && (T_IN_MLOOP(tailstmt) != tmp)) {
	  if ((stmt_in_list(stmtls, tailstmt) && !T_VISITED(tailstmt)) ||
	      (stmt_in_list(stmtls, T_IN_MLOOP(tailstmt)) &&
	       !T_VISITED(T_IN_MLOOP(tailstmt)))) {
	    flag = FALSE;
	  }
	}
	tmplist = GLIST_NEXT(tmplist);
      }
      /* free tmplist */
      if (flag) {
	return (tmp);
      }
    }
    tmp = T_NEXT(tmp);
  }
  return (NULL);
}


static statement_t *topsort_stmtls(statement_t *stmtls) {
  statement_t *first;

  if (!stmtls) return (NULL);

  first = get_first(stmtls);

  if (first) {
    T_VISITED(first) = TRUE;
    T_NEXT(first) = topsort_stmtls(stmtls);
    if (T_NEXT(first)) {
      T_PREV(T_NEXT(first)) = first;
    }
  }

  return (first);
}


/* FUNCTION statement_in_stmtls - return TRUE if s appears in stmtls
 *          and s has not "been visited"
 */

static int statement_in_stmtls(statement_t *stmtls, statement_t *s) {
  if (!stmtls)
    return (FALSE);
  else if (!T_VISITED(stmtls)) {
    if ((stmtls == s) || 
	((T_TYPE(stmtls) == S_NLOOP) && 
	 (statement_in_stmtls(T_NLOOP_BODY(T_NLOOP(stmtls)),s))))
      return (TRUE);
  } 
  return (statement_in_stmtls(T_NEXT(stmtls), s));
}


/* FUNCTION: return a statement in stmtls that has no unvisited in deps 
 *           (indeps are ignored if they are from outside stmtls)
 */

static statement_t *no_ins(statement_t *stmtls) {
  statement_t *tmp;
  indep_t *indeps;
  int flag;

  tmp = stmtls;
  while(tmp) {
    if (!T_VISITED(tmp)) {
      indeps = T_INDEP(tmp);
      flag = TRUE;
      while(indeps) {
	if (!INDEP_BACK(indeps)) {
	  if (statement_in_stmtls(stmtls, INDEP_TAIL_STMT(indeps))) {
	    flag = FALSE;
	  }
	}
	indeps = INDEP_NEXT(indeps);
      }
      if (flag)
	return (tmp);
    }
    tmp = T_NEXT(tmp);
  }
  return (NULL);
}


/* FUNCTION: sort_mloop_body2 - used by sort_mloop_body
 */

static statement_t *sort_mloop_body2(statement_t *stmtls) {
  statement_t *tmp;

  if (!stmtls)
    return (NULL);

  tmp = no_ins(stmtls);
  if (tmp) {
    T_VISITED(tmp) = TRUE;
    T_NEXT(tmp) = sort_mloop_body2(stmtls);
  }

  return (tmp);
}


/* FUNCTION: sort_mloop_body - return a topsorted version of the statements
 *           in stmtls; this gaurantees that the deps are correct
 */
static statement_t *sort_mloop_body(statement_t *stmtls) {
  statement_t *tmp;

  /* mark stmts unvisited */
  tmp = stmtls;
  while(tmp) {
    T_VISITED(tmp) = FALSE;
    tmp = T_NEXT(tmp);
  }

  return (sort_mloop_body2(stmtls));
}


static statement_t *topsort_stmts(statement_t *stmtls) {
  statement_t *tmp;

  /* mark all stmts unvisited */
  tmp = stmtls;
  while (tmp) {
    T_VISITED(tmp) = FALSE;
    tmp = T_NEXT(tmp);
  }

  stmtls = topsort_stmtls(stmtls);

  INT_COND_FATAL((stmtls!=NULL), NULL, "Cycle in topsort_stmts()");

  /* call sort_mloop_body on each body */
  tmp = stmtls;
  while (tmp) {
    if (T_TYPE(tmp) == S_MLOOP) {
      T_MLOOP_BODY(T_MLOOP(tmp)) = sort_mloop_body(T_MLOOP_BODY(T_MLOOP(tmp)));
    }
    tmp = T_NEXT(tmp);
  }
  return (stmtls);
}


/* FUNCTION: xfuse -
 */

static statement_t *xfuse(statement_t *stmtls) {
  statement_t *last = NULL;
  statement_t *tmp, *thebody;
  glist l1, l2;

  l1 = (glist)PMALLOC(GLIST_NODE_SIZE);
  l2 = (glist)PMALLOC(GLIST_NODE_SIZE);
  GLIST_NEXT(l1) = l2;
  GLIST_NEXT(l2) = NULL;


  tmp = stmtls;
  while (tmp) {
    if (T_TYPE(tmp) == S_MLOOP) {
      if (last) {
	GLIST_DATA(l1) = last;
	GLIST_DATA(l2) = tmp;
	if (legal(stmtls, l1)) {
	  DB2(20, "Fusing for X(%d,%d)\n", T_LINENO(last), T_LINENO(tmp));
	  stmtls = fuse(stmtls, l1, NULL, &thebody);
	} else {
	  DB2(20, "Not fusing for X(%d,%d)\n", T_LINENO(last), T_LINENO(tmp));
	  last = tmp;
	}
      } else {
	last = tmp;
      }
    } else {
      last = NULL;
    }
    tmp =  T_NEXT(tmp);
  }

  /* free nodes */
  return (stmtls);
}


/* FUNCTION: bb_process - this is a core function; it takes a statement 
 *           list, transforms it (fusion/contraction) and returns the 
 *           resulting statement list
 */

static statement_t *bb_process(statement_t *stmtls) {
  statement_t *thebody;
  reflist_t *ordered_refs, *tmp;
  glist stmts;
  expr_t *expr;
  int prof, leg;

  IFDB(40) {
    printf("----------here is a bb-----------\n");
    dbg_gen_stmtls(zstdout, stmtls);
    printf("----------here is end -----------\n");
  }

  refs = build_ref_list(stmtls, NULL);

  IFDB(40) {
    print_reflist(refs);
    printf("----------            -----------\n");
  }

  IFDB(29) {
    DB0(29, "before everything++++++++++\n");  
    dbg_gen_stmtls(zstdout, stmtls);
  }

  ordered_refs = reflist_sort(refs);

  /* fusion for contraction */
  tmp = ordered_refs;
  if (CdaFuse || CFuse) {
    while(ordered_refs) {
      expr = REFLIST_EXPR(ordered_refs);
      if (CFuse || (CdaFuse && cda_expr(expr))) {
	ArraysConsidered++;
	IFDB(25) {
	  DB0(25, "Considering (contraction) variable ");
	  dbg_gen_expr(zstdout,expr);
	  printf("\n");
	}
	stmts = REFLIST_LIST(ordered_refs);
	stmts = closure(stmtls, stmts);
	prof = profitable(stmtls, stmts, expr);

	/* full contraction case */
	leg = 0;
	if (stmts) {
	  leg = legal(stmtls, stmts);
	}
	if ((prof == INT_MAX) && leg) {
	  IFDB(20) {
	    DB0(20, "Fusing to contract variable ");
	    dbg_gen_expr(zstdout, expr);
	    printf("\n");
	  }
	  thebody = NULL;
	  stmtls = fuse(stmtls, stmts, tmp, &thebody);
	  /* contract here */
	  if (Contract || (CdaContract && cda_expr(expr))) {
	    if (cda_expr(expr))
	      CdaArraysContracted++;
	    else
	      UserArraysContracted++;
	    IFDB(20) {
	      DB0(20, "Contracting variable ");
	      dbg_gen_expr(zstdout, expr);
	      printf("\n");
	    }
	    arrays_to_scalar(thebody, expr, stmtls);
	  }
	
	  /* update reflist */
	} else {
	  /* partial contraction case */
	  if ((prof != 0) && (prof != INT_MAX) && leg)  { 
	    expr_t *reg;
	    symboltable_t* pst;
	    dimension_t *old_dim, *new_dim;
	    int i;
	    char *s, *reg_name;

	    /* BLC -- This is ridiculous, but it was uninitialized before */
	    old_dim = NULL;
	    
	    pst = expr_find_root_pst(expr);

	    /* don't contract rgrid dimensions */
	    if (D_REG_DIM_TYPE(T_TYPEINFO(D_ENS_REG(S_DTYPE(pst))), prof-1) 
		!= DIM_GRID) {

	      s = create_name_from_expr(expr);
	      reg_name = (char *) PMALLOC(strlen(s)+strlen(S_IDENT(T_IDENT(D_ENS_REG(S_DTYPE(pst)))))+3);
	      sprintf(reg_name, "_%s_%s", 
		      S_IDENT(T_IDENT(D_ENS_REG(S_DTYPE(pst)))), 
		      s);
	      reg = copy_expr(D_ENS_REG(S_DTYPE(pst)));
	      INT_FATAL(NULL, "Trying to copy a region's dimension list");
	      new_dim = NULL;
	      i = 1;
	      while (old_dim) {
		if (i == prof) {
		  new_dim = build_rgrid_dim_list(new_dim);
		} else {
		  new_dim = build_dim_list(new_dim,DIM_LO(old_dim),
					   DIM_HI(old_dim));
		}
		i++;
		old_dim = DIM_NEXT(old_dim);
	      }
	      S_DTYPE(pst) = copy_dt(S_DTYPE(pst));
	      D_ENS_REG(S_DTYPE(pst)) = reg;
	    }
	  }
	}
      }
      ordered_refs = REFLIST_NEXT(ordered_refs);
    }
  }

  /* fusion for locality */
  ordered_refs = tmp;
  if (LFuse) {
    while(ordered_refs) {
      expr = REFLIST_EXPR(ordered_refs);
      IFDB(25) {
	DB0(25, "Considering (locality) variable ");
	dbg_gen_expr(zstdout,expr);
	printf("\n");
      }
      stmts = REFLIST_LIST(ordered_refs);
      stmts = closure(stmtls, stmts);
      if (stmts && legal(stmtls, stmts) && !containsmscans(stmts)) {
	IFDB(20) {
	  DB0(20, "Fusing for locality variable expression ");
	  dbg_gen_expr(zstdout, expr);
	  printf("\n");
	}
	thebody = NULL;
	stmtls = fuse(stmtls, stmts, tmp, &thebody);
	/* update reflist */
      }
      ordered_refs = REFLIST_NEXT(ordered_refs);
    }
  }
  IFDB(29) {
    DB0(29, "before topsort++++++++++\n");  
    dbg_gen_stmtls(zstdout, stmtls);
  }

  stmtls = topsort_stmts(stmtls);
  if (XFuse) {
    stmtls = xfuse(stmtls);
  }

  IFDB(29) {
    DB0(29, "after topsort++++++++++\n");
    dbg_gen_stmtls(zstdout, stmtls);
    DB0(29, "done topsort++++++++++\n");
  }

  return(stmtls);
}


static int contraction(module_t *mod, char *s) {
  /*  module_t *tmpmod;*/
#if (!RELEASE_VER)
  int ArraysLocalBefore;
  int ArraysGlobalBefore;
  int ArraysLocalAfter;
  int ArraysGlobalAfter;
#endif

  /*** obey command line parameters for fusion ***/
  switch (fuse_level) {
  case 0:
    LFuse = 0;
    CFuse = 0;
    CdaFuse = 0;
    break;
  case 1:
    LFuse = 1;
    CFuse = 0;
    CdaFuse = 0;
    break;
  case 2:
    LFuse = 1;
    CFuse = 1;
    CdaFuse = 0;
    break;
  case 3:
    CdaFuse = 1;
    LFuse = 1;
    CFuse = 1;
    break;
  default:
    INT_FATAL(NULL, "No such fusion level.");
  }

  /*** obey command line parameters for contraction ***/
  if (contract == TRUE) {
    Contract = 1;
  }
  else {
    Contract = 0;
  }

  /*** allow flags in the passlist file to override the above or defaults ***/
  if (get_flag_arg(s, 'e') == 0) {
    CdaFuse = 1;
  } 

  if (get_flag_arg(s, 't') == 0) {
    CdaFuse = 1;
    CdaContract = 1;
  } 

  if (get_flag_arg(s, 'b') == 0) {
    CFuse = 1;
  } 

  if (get_flag_arg(s, 'f') == 0) {
    LFuse = 1;
  } 

  if (get_flag_arg(s, 'c') == 0) {
    CFuse = 1;
    Contract = 1;
  } 

  if (get_flag_arg(s, 'a') == 0) {
    LFuse = 1;
    CFuse = 1;
    Contract = 1;
  } 
  if (get_flag_arg(s, 'x') == 0) {
    XFuse = 1;
  }

#if (!RELEASE_VER)
  ArraysLocalBefore = total_arrays_local(mod);
  ArraysGlobalBefore = total_arrays_global();
#endif

  /*  tmpmod = mod;*/
/*  set_mloop(mod); */ /* this is done in r2mloops.c */
  set_constraints(mod);

  if (CdaFuse || CFuse || LFuse || XFuse) {
    bb_traversal(mod, bb_process, BBTR_NN_COMM);
  }

  call_uve(mod, NULL);
#if (!RELEASE_VER)
  ArraysLocalAfter = total_arrays_local(mod);
  ArraysGlobalAfter = total_arrays_global();
#endif
  DBS0(2, "\nbefore(l+g)/contracted(c+u)/considered/after(l+g)\n");

  /* BC+SD: UserArraysContracted, ArraysConsidered, ArraysLocalAfter
     differ */

  DBS6(2, "%d(%d+%d)/%d(%d+%d)/",
       ArraysLocalBefore+ArraysGlobalBefore, ArraysLocalBefore,
       ArraysGlobalBefore,
       UserArraysContracted + CdaArraysContracted,
       CdaArraysContracted, UserArraysContracted);

  DBS4(2, "%d/%d(%d+%d)\n",
       ArraysConsidered,
       ArraysLocalAfter+ArraysGlobalAfter, ArraysLocalAfter,
       ArraysGlobalAfter);

/*  set_mloop(tmpmod); */ /**/

  return (0);
}


/* mutates argument list - never need to backtrack */
static dvlist_t prune(dvlist_t dvlist, int index) {
  dvlist_t rest;

  if (!dvlist) return (NULL);
  rest = prune(DVLIST_NEXT(dvlist), index);
  if (DVLIST_DV(dvlist)[index] == 0) {
    DVLIST_NEXT_LHS(dvlist) = (glist)rest;
  } else {
    /* free this node */
    PFREE(dvlist, DVLIST_NODE_SIZE);
    dvlist = rest;
  }

  return (dvlist);
}


void hilow(dvlist_t dvlist, int index, int *hi, int *low) {
  if (!dvlist) {
    *hi = 0;
    *low = 0;
    return;
  }

  *hi = INT_MIN;
  *low = INT_MAX;

  while (dvlist) {
    *hi = max(*hi, DVLIST_DV(dvlist)[index]);
    *low = min(*low, DVLIST_DV(dvlist)[index]);
    dvlist = DVLIST_NEXT(dvlist);
  }
}


/* note that lsd() destructively updates is 1st arg 
   (because it calls prune() */

int lsd(dvlist_t dvlist, distvect_t dv) {
  distvect_t donemask;
  dvlist_t tmp;
  int i, j, ti, hi, low, d, flag, rank;

  /* get first nonzero rank in dvlist*/
  rank = 0;
  tmp = dvlist;
  while (tmp) {
    rank = DVLIST_DV(tmp)[MAXRANK];
    if (rank)
      break;
    tmp = DVLIST_NEXT(tmp);
  }
  (dv)[MAXRANK] = rank;

  if (rank == 0) {
    INT_FATAL(NULL, "0 rank in lsd()");
  }

  /* null dvlist or rank == 0? no constraints ; be very cache friendly */
  if (!dvlist || !rank) {
    for (i=0;i<rank;i++) {
      (dv)[i] = cmo_alloc?(rank-i):(i+1);
    }
    return (TRUE);
  }

  /* initialize donemask */
  donemask[MAXRANK] = rank;
  for(i=0; i<MAXRANK; i++) {
    donemask[i] = FALSE;
  }

  for(j=0; j<rank; j++) {  /* iterate over loops, outter to inner */
    flag = FALSE;
    for(ti=0; ti<rank; ti++) {   /* iterate over dims, distant to near */
      i = cmo_alloc?(rank-ti-1):ti;
      if (!donemask[i]) {
	hilow(dvlist, i, &hi, &low);
	if (hi*low >= 0) {
	  donemask[i] = TRUE;
	  flag = TRUE;
	  if (hi > 0) {
	    d = -(i+1);
	  } else {
	    d = (i+1);
	  }
	  (dv)[j] = d;
	  dvlist = prune(dvlist, i);
	  break;
	}
      }
    }
    if (!flag) {
      return (FALSE);
    }
  }
  return (TRUE);
}


void set_mloop(module_t *mod)
{
  function_t *ftp;

  while (mod) {
    ftp = T_FCNLS(mod);
    INT_COND_FATAL((ftp!=NULL), NULL,
		   "No functions in mod in set_mloop()");
    while (ftp) {
      set_mloop_stmtls(T_STLS(ftp));
      ftp = T_NEXT(ftp);
    }
    mod = T_NEXT(mod);
  }
}


/* function: call_contraction - array contraction code
 * date: 25 September 1996
 * creator: echris
 */

int call_contraction(module_t *mod,char *s) {
  return (contraction(mod, s));
}
