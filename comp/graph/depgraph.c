/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: depgraph.c - dependance graph construction code
 * DATE: 15 September 1996
 * CREATOR: echris
 *
 * SUMMARY
 *  This code creates the (array and scalar level) dependence graph for
 *  a module (see function 'depgraph').  
 *
 * WHAT IS THE STRUCTURE OF THE DEPDENCE GRAPH?
 *  The dependence graph is implemented as two fields in each statement,
 *  accessed via the macros 'T_OUTDEP' and 'T_INDEP'.  The fields are
 *  pointers to chains of 'outdep_t' and 'indep_t' structures, 
 *  respectively.  Both these structures point to the statement that
 *  contains them (macros 'OUTDEP/INDEP_STMT') and the next statement in the
 *  chain (macros 'OUTDEP_NEXT' and 'INDEP_NEXT').  In addition, each 
 *  'outdep_t' structure points to the corresponding 'indep_t' structure
 *  and vice versa (macros 'OUTDEP_HEAD' and 'INDEP_TAIL', respectively).
 *  Furthermode, structure 'outdep_t' points to the expression
 *  that induces the dependence and the associate unconstrained distance
 *  vector (macros 'OUTDEP_EXPR' and 'OUTDEP_DIST').
 *
 * Implication: any tranformations on the AST after the depgraph is 
 *  constructed may break the depgraph.
 *
 * BLC: 6/29/00 -- My understanding is that dependence arcs should be
 * between sibling statements (statements connect via a series of
 * T_NEXT()/T_PREV() hops).  Any nesting of statements (due to loop,
 * conditional, or scope type statements) don't need dependences going
 * from parent to child or vice-versa due to the fact that the scope
 * itself maintains the dependence.  Merging of two scopes would require
 * checking to make sure that their statements were legal to be merged.
 * The one possible exception to this (and I don't understand it myself)
 * is MLOOPs, which may currently only have arcs going from statements
 * in their bodies to statements outside of their bodies or in other
 * MLOOP bodies.  My impression is that this was done to make fusion
 * easier (or at least was the assumption that fusion made).  Rewriting
 * this from scratch, I would probably keep the same rules as above, and
 * force fusion to look deeper into the bodies when it wanted to fuse
 * things.
 *
 * This current check-in prevents arcs from being added between a
 * statement and its ancestor scope, and fixes a problem that existed
 * in test/misc/brad/blktri.z due to the fact that meaningless arcs
 * existed between the NLOOP statement and its body.  My understanding
 * is that the reason this hasn't been a problem in other scoped nodes
 * is due to the fact that other scopes break basic blocks.
 *
 * BLC: 2/28/00 -- I took out the scope field of the dependence info
 * structure, not convinced that it was doing any good (and because it
 * was a bit overly cautious when it came to scalars).  The
 * implications are that any array access is going to access an entire
 * array and that any definition of an array is going to write to
 * every location in the array.  This is good enough for the present.
 * For example in the following code:
 *
 *       [R] A := 0;
 *       [i,i] A := 1;
 *       [R] B := A;
 *
 * there will be a flow dependence from stmt 2 to stmt 3, but not from
 * stmt 1 to stmt 3.  If you use this, think: is it good enough for
 * you?  If not, you'll want to associate region symbols with array
 * references (and not scalars) to disambiguate them
 * */

#include <stdio.h>
#include "../include/Dgen.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/depgraph.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/idfa_traverse.h"
#include "../include/macros.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/statement.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"


#define DEP_INFO_VAR(x) ((x)->var_p)
#define DEP_INFO_DIR(x) ((x)->dir_p)
#define DEP_INFO_EXPR(x) ((x)->expr_p)
#define DEP_INFO_SCOPE(x) ((x)->scope_p)
#define DEP_INFO_STMT(x) ((x)->stmt_p)
#define DEP_DEFS(x) ((x)->defs)
#define DEP_USES(x) ((x)->uses)

typedef	enum depedgetype {
  FLOW,
  OUTPUT,
  ANTI
} depedgetype;

struct dep_info_struct {
  symboltable_t *var_p;
  expr_t   *dir_p;
  expr_t *scope_p;
  expr_t	*expr_p;
  statement_t   *stmt_p;
};

struct dep_struct {
  glist defs;
  glist uses;
};


/* FUNCTION: dep_info_new - create, init to null and return new dep_t struct
 * echris - 2/3/98
 */

static dep_t *dep_info_new(void) {
  dep_t *new;

  new = (dep_t *)PMALLOC(sizeof(dep_t));
  DEP_DEFS(new) = NULL;
  DEP_USES(new) = NULL;
  return (new);
}


/* FUNCTION: dep_info_equal - test equality of dependence info
 * echris - 2/3/98
 */

/*
static int dep_info_equal(void *one, void *two) {
  dep_info_t *di1, *di2;
  di1 = (dep_info_t *)one;
  di2 = (dep_info_t *)two;

  return (dependent_exprs(DEP_INFO_EXPR(di1),DEP_INFO_EXPR(di2),MUST) &&
	  (DEP_INFO_DIR(di1) == DEP_INFO_DIR(di2)) &&
	  expr_equal(DEP_INFO_SCOPE(di1),DEP_INFO_SCOPE(di2)));
}
*/


/* FUNCTION: dep_info_equal_exact - test exact equality of dependence info
 * echris - 2/3/98
 */

static int dep_info_equal_exact(void *one, void *two) {
  dep_info_t *di1, *di2;
  di1 = (dep_info_t *)one;
  di2 = (dep_info_t *)two;

  return ((DEP_INFO_DIR(di1) == DEP_INFO_DIR(di2)) &&
	  expr_equal(DEP_INFO_SCOPE(di1),DEP_INFO_SCOPE(di2)) &&
	  (DEP_INFO_STMT(di1) == DEP_INFO_STMT(di2)) &&
	  expr_equal(DEP_INFO_EXPR(di1), DEP_INFO_EXPR(di2)));
}


static void* dep_info_copy(void* in) {
  dep_info_t* new = NULL;
  dep_info_t* old = (dep_info_t*)in;

  if (old) {
    new = (dep_info_t*)PMALLOC(sizeof(dep_info_t));
    DEP_INFO_VAR(new) = DEP_INFO_VAR(old);
    DEP_INFO_EXPR(new) = DEP_INFO_EXPR(old);
    DEP_INFO_DIR(new) = DEP_INFO_DIR(old);
    DEP_INFO_STMT(new) = DEP_INFO_STMT(old);
    DEP_INFO_SCOPE(new) = DEP_INFO_SCOPE(old);
  } else {
    INT_FATAL(NULL,"BLC: should never get here");
  }
  return new;
}


/* FUNCTION: dep_copy - create a copy of a dep_t struct
 * echris - 2/3/98
 */

static void* dep_copy(void* d) {
  dep_t *new=NULL;
  dep_t *old = (dep_t*)d;
  if (old) {
    new = (dep_t *)PMALLOC(sizeof(dep_t));
    DEP_DEFS(new) = glist_copy(DEP_DEFS(old),GLIST_NODE_SIZE,dep_info_copy);
    DEP_USES(new) = glist_copy(DEP_USES(old),GLIST_NODE_SIZE,dep_info_copy);
  }
  return new;
}


static void print_def_glist(glist list) {
  dep_info_t* data;
  int counter=0;

  while (list != NULL) {
    data = (dep_info_t*)(GLIST_DATA(list));
    IFDB(17) {
      printf("\n");
      printf("  Node %d:\n",counter);
      printf("    var: %s\n",S_IDENT(data->var_p));
      printf("    dir: ");
      if (data->dir_p) {
	printf("%s",S_IDENT(expr_find_root_pst(data->dir_p)));
      } else {
	printf("NULL");
      }
      printf("\n");
      printf("    expr: ");
      dbg_gen_expr(stdout,data->expr_p);
      printf("\n");
      printf("    scope: ");
      gen_name(stdout, data->scope_p);
      printf("    stmt_p: %d\n",S_LINENO(data->stmt_p));
    }
    
    list = GLIST_NEXT(list);
    counter++;
  }
  printf("%d nodes\n",counter);
}


static void print_dep_list(dep_t* list) {
  printf("DEFS: ");
  print_def_glist(DEP_DEFS(list));
  printf("USES: ");
  print_def_glist(DEP_USES(list));
}



/* BC+SD: This only works for literal directions */
static void direction_to_intvect(int rank, expr_t* dir, 
				 int vect[MAXRANK]) {
  int i;
  int success;

  for (i=0; i<rank; i++) {
    if (dir == NULL) {
      vect[i] = 0;
    } else {
      vect[i] = dir_comp_to_int(dir, i, &success);
      if (!success) {
	switch (dir_get_sign(dir, i)) {
	case SIGN_ZERO:
	  vect[i] = 0;
	  break;
	case SIGN_POS:
	  vect[i] = 777;
	  break;
	case SIGN_NEG:
	  vect[i] = -777;
	  break;
	default:
	  INT_FATAL(T_STMT(dir), "Direction confusing in depgraph.c");
	}
      }
    }
  }
}
  


/* FUNCTION: calc_dv - calculate un. dist. vect. given two directions
 * echris - 2/3/98
 */

static distvect_t *calc_dv(expr_t *hd, expr_t *tl, int rank,
			   attype_t attype, depedgetype_t deptype) {
  int hdv[MAXRANK];
  int tlv[MAXRANK];
  int i;
  distvect_t *dv;
  int multiplier;

  if (rank == 0) {
    return NULL;
  }

  multiplier = ((attype==AT_PRIME)&&(deptype==XANTI))?-1:1;

  direction_to_intvect(rank, hd, hdv);
  direction_to_intvect(rank, tl, tlv);

  dv = (distvect_t *)PMALLOC(sizeof(distvect_t));

  for (i=0; i<rank; i++) {
    (*dv)[i] = (hdv[i]-tlv[i])*multiplier;
  }
  (*dv)[MAXRANK] = rank;

  return (dv);
}


/* FUNCTION: dep_info_equal_var - test equality of var of dependence
 * echris - 2/3/98
 */

static int dep_info_equal_var(void *one, void *two) {
  dep_info_t *di1, *di2;

  di1 = (dep_info_t *)one;
  di2 = (dep_info_t *)two;

  return (dependent_exprs(DEP_INFO_EXPR(di1), DEP_INFO_EXPR(di2), MAY));
}


/* FUNCTION: dep_alloc_out - allocate a new outdep_t struct for a statement
 * echris - 2/3/98
 */

static outdep_t *dep_alloc_out(statement_t *s) {
  outdep_t *outdep; /*** , *tmpoutdep; ***/

  outdep = (outdep_t *) PMALLOC(sizeof(outdep_t));
  OUTDEP_TYPE(outdep) = (depedgetype_t)0;
  OUTDEP_HEAD(outdep) = NULL;
  OUTDEP_STMT(outdep) = s;
  OUTDEP_BACK(outdep) = FALSE;
  /*** sungeun *** OUTDEP_NEXT(outdep) = NULL; ***/

  OUTDEP_NEXT(outdep) = T_OUTDEP(s);
  T_OUTDEP(s) = outdep;

  /*** sungeun
  tmpoutdep = T_OUTDEP(s);
  if (!tmpoutdep) {
    T_OUTDEP(s) = outdep;
  } else {
    while (OUTDEP_NEXT(tmpoutdep)) {
      tmpoutdep = OUTDEP_NEXT(tmpoutdep);
    }
    OUTDEP_NEXT(tmpoutdep) = outdep;
  }
  ***/

  return (outdep);
}


/* FUNCTION: dep_alloc_in - allocate a new indep_t struct for a statement
 * echris - 2/3/98
 */

static indep_t *dep_alloc_in(statement_t *s) {
  indep_t *indep; /*** , *tmpindep; ***/

  indep = (indep_t *) PMALLOC(sizeof(indep_t));
  INDEP_TAIL(indep) = NULL;
  INDEP_STMT(indep) = s;
  /*** sungeun *** INDEP_NEXT(indep) = NULL; ***/

  INDEP_NEXT(indep) = T_INDEP(s);
  T_INDEP(s) = indep;

  /*** sungeun
  tmpindep = T_INDEP(s);
  if (!tmpindep) {
    T_INDEP(s) = indep;
  } else {
    while (INDEP_NEXT(tmpindep)) {
      tmpindep = INDEP_NEXT(tmpindep);
    }
    INDEP_NEXT(tmpindep) = indep;
  }
  ***/

  return indep;
}


/* FUNCTION: alloc_distvect - create and copy a distvect_t struct
 * echris - 2/3/98
 */

static distvect_t *alloc_distvect(distvect_t *dv) {
  distvect_t *newdv;
  int i;

  if (!dv)
    return (NULL);

  newdv = (distvect_t *) PMALLOC(sizeof(distvect_t));
  for (i = 0; i < MAXRANK+1; i++) {
    (*newdv)[i] = (*dv)[i];
  }

  return (newdv);
}


static int stmt_follow(statement_t *s1, statement_t *s2) {
  if (s1 == s2) return (TRUE);

  if (!s1 || !s2) return (FALSE);

  return(stmt_follow(T_NEXT_LEX(s1),s2));
}


/* FUNCTION: dep_back - return true if dep is a back edge
 * echris - 9/21/00
 */

static int dep_back(outdep_t *outdep) {
  statement_t *s1, *s2;

  s1 = OUTDEP_STMT(outdep);
  s2 = INDEP_STMT(OUTDEP_HEAD(outdep));

  return (stmt_follow(T_NEXT_LEX(s2),s1));
}


/* FUNCTION: dep_add - create a dependence from one stmt to another
 * echris - 2/3/98
 */

static outdep_t *dep_add(statement_t *tail,statement_t *head,symboltable_t *var,
			 expr_t *expr, depedgetype_t type, distvect_t *dv) {
  outdep_t *outdep;
  indep_t *indep;

  if (stmts_are_ancestors(head,tail)) {
    return NULL;
  }

  outdep = dep_alloc_out(tail);
  indep  = dep_alloc_in(head);
  OUTDEP_TYPE(outdep) = type;
  OUTDEP_HEAD(outdep) = indep;
  OUTDEP_EXPR(outdep) = expr;
  OUTDEP_DIST(outdep) = alloc_distvect(dv);
  INDEP_TAIL(indep) = outdep;
  OUTDEP_BACK(outdep) = dep_back(outdep);

  return (outdep);
}

/* FUNCTION: dep_check - check for and add deps (that's not clear!)
 * echris - 2/3/98
 */

static void dep_check(statement_t *stmt, set_t *inoutset, glist defs_uses, 
	       depedgetype_t deptype) {
  dep_info_t tmpdi, *di;
  expr_t *headdir, *taildir;
  distvect_t *dv;
  glist tmpdus;
  genlist_t *lvalue_exprs;
  expr_t *uat_expr;
  set_t* setptr;
  int rank;
  
  tmpdus = defs_uses;
  /* for each def/use in defs_uses */
  while (tmpdus) {
    di  = (dep_info_t *) GLIST_DATA(tmpdus);
    headdir = DEP_INFO_DIR(di);
    setptr = inoutset;
    /* for each element in in/out set */
    while (setptr) {
      lvalue_exprs = SET_EXPRS(setptr);
      INT_COND_FATAL((lvalue_exprs!=NULL), stmt, 
		     "NULL SET_EXPRS field in depgraph_fun");    
      /* for each direction used with this element */
      while(lvalue_exprs) {
	DEP_INFO_VAR(&tmpdi) = SET_VAR(setptr);
	DEP_INFO_SCOPE(&tmpdi) = NULL;
	DEP_INFO_DIR(&tmpdi) = NULL;
	DEP_INFO_EXPR(&tmpdi) = G_EXP(lvalue_exprs);
	taildir = (uat_expr=expr_find_at(G_EXP(lvalue_exprs)))?
	  T_NEXT(T_OPLS(uat_expr)):NULL;
	
	rank = expr_rank(G_EXP(lvalue_exprs));

	if (dep_info_equal_var(&tmpdi, di)) {
	  dv = calc_dv(headdir, taildir, rank,
		       (uat_expr?((attype_t)T_SUBTYPE(uat_expr)):AT_NORMAL), 
		       deptype);
	  dep_add(stmt, DEP_INFO_STMT(di), SET_VAR(setptr), 
		  ((T_TYPE(DEP_INFO_EXPR(di))==BIAT)?
		   T_OPLS(DEP_INFO_EXPR(di)):DEP_INFO_EXPR(di)), 
		  deptype, dv);
	  if (dv != NULL) {
	    PFREE(dv,sizeof(distvect_t));
	  }
	}
	
	lvalue_exprs = G_NEXT(lvalue_exprs);
      }
      setptr = SET_NEXT(setptr);
    }
    tmpdus = GLIST_NEXT(tmpdus);
  }
}


/* FUNCTION: dep_stmt_fn - main flow function for depgraph construction
 * echris - 2/3/98
 */

static void *dep_stmt_fn(statement_t *stmt, void *in, int final) {
  dep_t *depin;
  dep_info_t* newdi;
  set_t *inset, *outset, *tmpinset, *tmpoutset;
  glist defs, uses;
  glist new_defs, new_uses;
  expr_t *uat_expr;
  genlist_t *lvalue_exprs;
  glist dead;


  DBS1(15,"   in dep_stmt_fn for stmt %d...\n",T_LINENO(stmt));
  depin = (dep_t *)in;
  /* WAS:  depin = dep_copy((dep_t *)in); */
  IFDB(16) {
    print_dep_list(in);
  }
  defs = DEP_DEFS(depin);
  uses = DEP_USES(depin);
  inset = T_IN(stmt);
  outset = T_OUT(stmt);

  if (final) {
    dep_check(stmt, outset, uses, XFLOW);
    dep_check(stmt, inset, defs, XANTI);
    dep_check(stmt, outset, defs, XOUTPUT);
  }

  /* compute new uses from this statement */
  new_uses = NULL;
  tmpinset = inset;
  while (tmpinset) {
    lvalue_exprs = SET_EXPRS(tmpinset);
    INT_COND_FATAL((lvalue_exprs!=NULL), stmt, 
		   "NULL SET_EXPRS field in depgraph_fun");    
    while (lvalue_exprs) {
      newdi = (dep_info_t *)PMALLOC(sizeof(dep_info_t));
      DEP_INFO_VAR(newdi) = SET_VAR(tmpinset);
      DEP_INFO_DIR(newdi) = (uat_expr=expr_find_at(G_EXP(lvalue_exprs)))?
                            T_NEXT(T_OPLS(uat_expr)):NULL;
      DEP_INFO_EXPR(newdi) = G_EXP(lvalue_exprs);
      if (T_TYPEINFO_REG(G_EXP(lvalue_exprs))) {
	DEP_INFO_SCOPE(newdi) = RMSCurrentRegion();
      } else {
	DEP_INFO_SCOPE(newdi) = NULL;
      }
      DEP_INFO_STMT(newdi) = stmt;
      new_uses = glist_prepend_new(new_uses, newdi, GLIST_NODE_SIZE, 
				   dep_info_equal_exact);
      lvalue_exprs = G_NEXT(lvalue_exprs);
    }
    tmpinset = SET_NEXT(tmpinset);
  }

  /* compute new defs from this statement */
  new_defs = NULL;
  tmpoutset = outset;
  while (tmpoutset) {
    lvalue_exprs = SET_EXPRS(tmpoutset);
    INT_COND_FATAL((lvalue_exprs!=NULL), stmt, 
		   "NULL SET_EXPRS field in depgraph_fun");    
    while (lvalue_exprs) {
      newdi = (dep_info_t *)PMALLOC(sizeof(dep_info_t));
      DEP_INFO_VAR(newdi) = SET_VAR(tmpoutset);
      DEP_INFO_DIR(newdi) = (uat_expr=expr_find_at(G_EXP(lvalue_exprs)))?
                            T_NEXT(T_OPLS(uat_expr)):NULL;
	DEP_INFO_EXPR(newdi) = G_EXP(lvalue_exprs);
      if (T_TYPEINFO_REG(G_EXP(lvalue_exprs))) {
	DEP_INFO_SCOPE(newdi) = RMSCurrentRegion();
      } else {
	DEP_INFO_SCOPE(newdi) = NULL;
      }
      DEP_INFO_STMT(newdi) = stmt;
      new_defs = glist_prepend_new(new_defs, newdi, GLIST_NODE_SIZE,
				   dep_info_equal_exact);
      lvalue_exprs = G_NEXT(lvalue_exprs);
    }
    tmpoutset = SET_NEXT(tmpoutset);
  }

  /* prune uses and defs */
  dead = NULL;
  /* SJD: in the following line, changed new_defs to new_uses */
  uses = glist_difference(uses, new_uses, dep_info_equal_exact, &dead);
  glist_kill(dead,GLIST_NODE_SIZE,sizeof(dep_info_t));

  dead = NULL;
  defs = glist_difference(defs, new_defs, dep_info_equal_exact, &dead);
  glist_kill(dead,GLIST_NODE_SIZE,sizeof(dep_info_t));

  /* add uses and defs from this stmt */
  uses = glist_append_list(uses, new_uses);
  defs = glist_append_list(defs, new_defs);
  DEP_USES(depin) = uses;
  DEP_DEFS(depin) = defs;

  DBS0(15,"    ...and out\n");
  return (depin);
}


/* FUNCTION: dep_merge_fn - perform join op on dependence data
 * echris - 2/3/98
 */

static void *dep_merge_fn(void *one, void *two) {
  dep_t *d1, *d2, *new;
  glist deflist1, deflist2, deflistnew, uselist1, uselist2, uselistnew;
  glist dead;
  glist diff;

  d1 = (dep_t *)one;
  d2 = (dep_t *)two;
  if (!d1)
    return (d2);
  else if (!d2)
    return (d1);

  new = (dep_t *)PMALLOC(sizeof(dep_t));

  deflist1 = DEP_DEFS(d1);
  deflist2 = DEP_DEFS(d2);
  uselist1 = DEP_USES(d1);
  uselist2 = DEP_USES(d2);

  /* take union of deflist1 and deflist2, minimizing copying */
  dead = NULL;
  diff = glist_difference(deflist2, deflist1, dep_info_equal_exact,&dead);
  deflistnew = glist_append_list(diff, deflist1);
  glist_kill(dead,GLIST_NODE_SIZE,sizeof(dep_info_t));

  /* same for uselist */
  dead = NULL;
  diff = glist_difference(uselist2, uselist1, dep_info_equal_exact,&dead);
  uselistnew = glist_append_list(diff, uselist1);
  glist_kill(dead,GLIST_NODE_SIZE,sizeof(dep_info_t));
  
  DEP_DEFS(new) = deflistnew;
  DEP_USES(new) = uselistnew;

  return (new);
}


/* FUNCTION: print_edge_type - generate printed rep of dep edge type
 * echris - 2/3/98
 */

static void print_edge_type(depedgetype_t x) {
  if (x == FLOW)
    printf("flow");
  if (x == ANTI)
    printf("anti");
  if (x == OUTPUT)
    printf("output");
}


/* FUNCTION: print_dv - generate printed rep of a distance vector
 * echris - 2/3/98
 */

static void print_dv(distvect_t *dv) {
  int rank, i;

  if (!dv) {
    printf("ZEROV");
  } else {
    rank = (*dv)[MAXRANK];
    for (i=0; i<rank; i++) {
      printf("%d ", (*dv)[i]);
    }
  }
}


/* FUNCTION: dep_print_edge - generate printed rep of a dependence
 * echris - 2/3/98
 */

static void dep_print_edge(outdep_t *outdep) {
  /*  printf("%s ", S_IDENT(OUTDEP_VAR(outdep))); */
  dbg_gen_expr(zstdout,OUTDEP_EXPR(outdep));
  printf(" ");
  print_edge_type(OUTDEP_TYPE(outdep));
  printf("(");
  print_dv(OUTDEP_DIST(outdep));
  printf(")\n");
}


/* FUNCTION: dep_print_stmt - generate printed rep of deps on one stmt
 * echris - 2/3/98
 */

static void dep_print_stmt(statement_t *stmt) {
  outdep_t *outdeps;

  outdeps = T_OUTDEP(stmt);
  if (outdeps) {
    printf("(line %d):", T_LINENO(stmt));
    dbg_gen_stmt(zstdout,stmt);
    while (outdeps) {
      printf("   %s arc to (line %d) ", OUTDEP_BACK(outdeps)?"B":"F",
	     T_LINENO(OUTDEP_HEAD_STMT(outdeps)));
      dbg_gen_stmt(zstdout,OUTDEP_HEAD_STMT(outdeps));
      printf("       ");
      dep_print_edge(outdeps);
      outdeps = OUTDEP_NEXT(outdeps);
    }
  }
}


/* FUNCTION: dep_print - generate printed rep of deps in statement list
 * echris - 2/3/98
 */

static void dep_print(statement_t *stmtls) {
  traverse_stmtls_g(stmtls, dep_print_stmt, NULL, NULL, NULL);  
}


/* FUNCTION: dep_equal_fn - test equality of dependence info
 * echris - 2/3/98
 */

static int dep_equal_fn(void *one, void *two) {
  dep_t *d1, *d2;
  int val;

  if (one == two) {
    return 1;
  }

  if (!one || !two) {
    return 0;
  }
  d1 = (dep_t *)one;
  d2 = (dep_t *)two;

  val = (glist_equal(DEP_USES(d1), DEP_USES(d2), dep_info_equal_exact) &&
	 glist_equal(DEP_DEFS(d1), DEP_DEFS(d2), dep_info_equal_exact));
  return val;
}

/***
 *** sungeun
 ***
 *** clean_outdep(), clean_indep(), clean_depgraph()
 ***
 ***/
static void
clean_outdep(outdep_t *d) {
  outdep_t *td, *ttd;

  for (td = d; td != NULL; td = ttd) {
    ttd = OUTDEP_NEXT(td);
    if (OUTDEP_DIST(td) != NULL) {
      PFREE(OUTDEP_DIST(td), sizeof(distvect_t));
    }
    PFREE(td, sizeof(outdep_t));
  }
}

static void
clean_indep(indep_t *d) {
  indep_t *td, *ttd;

  for (td = d; td != NULL; td = ttd) {
    ttd = INDEP_NEXT(td);
    PFREE(td, sizeof(indep_t));
  }
}

static void
clean_depgraph(statement_t *s) {
  if (T_OUTDEP(s) != NULL) {
    clean_outdep(T_OUTDEP(s));
    T_OUTDEP(s) = NULL;
  }
  if (T_INDEP(s) != NULL) {
    clean_indep(T_INDEP(s));
    T_INDEP(s) = NULL;
  }
  return;
}

/* FUNCTION: depgraph_fun - overlay depgraph on statements in function
 * depgraph_fun() calls idfa_stmtls() on the stmts in the function
 * echris - 2/3/98
 */

static void depgraph_fun(function_t *fcn) {
  statement_t *stmtls;
  dep_t *indata;
  idfa_t idfa_dat;

  stmtls = T_STLS(fcn);
  traverse_stmtls_g(stmtls, clean_depgraph, NULL, NULL, NULL); /*** sungeun ***/

  indata = dep_info_new();

  INT_COND_FATAL((stmtls!=NULL), NULL, "NULL stmtls in depgraph_fun()");
  IDFA_STMT_FN(&idfa_dat) = dep_stmt_fn;
  IDFA_MERGE_FN(&idfa_dat) = dep_merge_fn;
  IDFA_EQUAL_FN(&idfa_dat) = dep_equal_fn;
  IDFA_COPY_FN(&idfa_dat) = dep_copy;
  IDFA_DIR(&idfa_dat) = REVERSE;

  no_iterate = FALSE;     /* don't iterate to fixed point */
  idfa_stmtls(stmtls, (void *)indata, &idfa_dat, 1);
  IFDB(10) {
    dep_print(stmtls);
  }
}


/* FUNCTION: dependent_exprs2 - helper function that does most of the work
 *           for dependent_exprs 
 * echris - 2/18/98
 */

static int dependent_exprs2(expr_t *e1,expr_t *e2,expr_t *estop1,expr_t *estop2,
			    maymust_t may) {
  int recurse1, recurse2;

  INT_COND_FATAL((e1!=NULL),T_STMT(estop1),"NULL arg1 in dependent_exprs2()");
  INT_COND_FATAL((e2!=NULL),T_STMT(estop1),"NULL arg2 in dependent_exprs2()");
  INT_COND_FATAL((estop1!=NULL), T_STMT(estop1), "NULL arg3 in dependent_exprs2()");
  INT_COND_FATAL((estop2!=NULL), T_STMT(estop1), "NULL arg4 in dependent_exprs2()");
  
  recurse1 = !(e1 == estop1);
  recurse2 = !(e2 == estop2);

  /* first verify that e1 and e2 match */
  /* first ignore @s */
  if (T_TYPE(e1) != T_TYPE(e2)) {
    if ((T_TYPE(e1) == BIAT) && recurse1) {
      return (dependent_exprs2(T_PARENT(e1),e2,estop1,estop2,may));
    } else if ((T_TYPE(e2) == BIAT) && recurse2) {
      return (dependent_exprs2(e1,T_PARENT(e2),estop1,estop2,may));
    } else {
      return (may==MAY);
    }
  } else {

    switch(T_TYPE(e1)) {
    case VARIABLE:
      if (T_IDENT(e1) != T_IDENT(e2)) return (FALSE);
      break;
    case ARRAY_REF:
    case BIAT:
      break;
    case BIDOT:
      if (T_IDENT(e1) != T_IDENT(e2)) return (FALSE);
      break;
    default:
      INT_FATAL(T_STMT(e1), "Bad expression type in dependent_exprs2()");
      return (FALSE);
    }
  }

  /* recurse (if nec)*/
  if (!recurse1 && !recurse2) {
    return (TRUE);
  }

  return (dependent_exprs2(recurse1?T_PARENT(e1):e1,recurse2?T_PARENT(e2):e2,
			   estop1,estop2,may));
}


/* FUNCTION: dependent_exprs - test whether two exprs could depend on each 
 *           other
 * echris - 2/11/98
 */

int dependent_exprs(expr_t *e1, expr_t *e2, maymust_t may) {
  expr_t *te1, *te2;
  /* false if either ptr is null */
  if (!e1 || !e2) {
    return (FALSE);
  }

  /* false if either expr does not have lvalue */
  if (!expr_is_lvalue(e1) || !expr_is_lvalue(e2)) {
    return (FALSE);
  }

  /* true if they are exactly the same expr */
  if (e1 == e2) {
    return (TRUE);
  }

  /* find leaf variable expression */
  te1 = e1;
  while (T_TYPE(te1) != VARIABLE) {
    T_PARENT(T_OPLS(te1)) = te1;        /* make sure parent ptrs are good */
    te1 = T_OPLS(te1);
    INT_COND_FATAL((te1!=NULL), T_STMT(e1), "NULL ptr in dependent_expr()");
  }

  te2 = e2;
  while (T_TYPE(te2) != VARIABLE) {
    T_PARENT(T_OPLS(te2)) = te2;        /* make sure parent ptrs are good */
    te2 = T_OPLS(te2);
    INT_COND_FATAL((te2!=NULL), T_STMT(e2), "NULL ptr in dependent_expr()");
  }

  return (dependent_exprs2(te1,te2,e1,e2,may));
}


/* FUNCTION: depgraph - overlay depgraph on statements in a module
 * depgraph() calls depgraph_fun() for each function in the module.
 * echris - 2/3/98
 */

static int depgraph(module_t *mod, char *s) {
  function_t *ftp;

  RMSInit();
  while (mod) {
    ftp = T_FCNLS(mod);
    INT_COND_FATAL((ftp!=NULL), NULL, "No functions in mod in dead()");
    while (ftp) {
      depgraph_fun(ftp);
      ftp = T_NEXT(ftp);
    }
    mod = T_NEXT(mod);
  }
  RMSFinalize();

  return (0);
}


/* function: call_dep - build dependence graph
 * date: 15 September 1996
 * creator: echris
 */

int call_dep(module_t *mod,char *s) {
  return (depgraph(mod, s));
}
