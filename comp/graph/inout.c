/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: depend.c - code to calculate in and out sets for stmts
 */

/* In/Out sets are calculated for each statement, indicating what
 * expressions that statement reads and/or writes.  For compound
 * statements such as loops/mloops/nloops/conditionals, etc.  the
 * in/out information is a summary of what its body does (perhaps
 * a conservative one).  This is about all I understand about this
 * code at this time. -BLC 06/29/00
 */


#include <stdio.h>
#include <string.h>
#include "../include/Cgen.h"
#include "../include/allocstmt.h"
#include "../include/buildstmt.h"
#include "../include/callgraph.h"
#include "../include/datatype.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/function.h"
#include "../include/genlist.h"
#include "../include/inout.h"
#include "../include/macros.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/statement.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"

#define IN 0
#define OUT 1

typedef enum summarylisttype {
  SUM_MOD=0,
  SUM_USE,
  SUM_BOTH
} summarylisttype;

struct summarylist_struct {
  summarylisttype type;
  symboltable_t *var;
  genlist_t *regions;
  summarylist_t *next;
};

#define SUM_TYPE(x)		((x)->type)
#define SUM_VAR(x)		((x)->var)
#define SUM_REGIONS(x)		((x)->regions)
#define SUM_NEXT(x)		((x)->next)


static function_t* currentF;
static int InitDepend = FALSE;
static set_t* inout;
static int ioro;
static summarylisttype currentT;
static int changed;
#if (!RELEASE_VER)
static int iterations;
#endif

static set_t *getinout(expr_t *, int);
static void dep_stmtls(statement_t*);


/* FUNCTION: clean_dir_depend - 
 * echris - 2/4/98 */

static void clean_dir_depend(dir_depend dir) {
  genlist_t *ident, *list;

  list = DEP_DIR_IDENT(dir);

  while (list != NULL) {
    ident = list;
    list = G_NEXT(list);
    free_gen(ident);
  }
}


/* FUNCTION: clean_set -
 * echris - 2/4/98
 */

static void clean_set(statement_t *stmt) {
  set_t *set, *f_set;

  /*** free the in and out sets ***/
  set = T_IN(stmt);
  while (set != NULL) {
    f_set = set;
    set = SET_NEXT(set);
    clean_dir_depend(SET_DIR(f_set));
    free_set(f_set);
  }
  set = T_OUT(stmt);
  while (set != NULL) {
    f_set = set;
    set = SET_NEXT(set);
    clean_dir_depend(SET_DIR(f_set));
    free_set(f_set);
  }

  T_IN(stmt) = NULL;
  T_OUT(stmt) = NULL;

}



/* FUNCTION: clean_dependences - 
 * Only called by traverse_stmtls_g().
 * echris - 2/4/98
 */

static void clean_dependences(statement_t *stmt) {
  clean_set(stmt);
}


/* FN: prune_set - remove all references to var_expr in set; return new set
 * echris - 4-14-97
 */

set_t *prune_set(set_t *set, expr_t *var_expr)
{
  set_t *head, *tmp, *tmp2, *last;

  head = set;    /* always points to first non-deleted node in set */
  tmp = set;     /* we will iterate over this */
  last = NULL;   /* always points to the last non-del node from prev iter */

  while (tmp) {                      /* for each node in set */
    if (expr_equal(var_expr, G_EXP(SET_EXPRS(tmp)))) { /* this node will be removed */
      if (head == tmp) {               /* are we removing the head node? */
	head = SET_NEXT(tmp);             /* bump head pointer */
      }
      tmp2 = tmp;                      /* get a tmp handle on this node */
      tmp = SET_NEXT(tmp);

      /* free node */
      clean_dir_depend(SET_DIR(tmp2));
      free_set(tmp2);

    } else {             /* this node will node be removed */
      if (last) {        
	SET_NEXT(last) = tmp;   /* patch up previous node's next pointer */
      }
      last = tmp;
      tmp = SET_NEXT(tmp);
    }
  }

  if (last) {
    SET_NEXT(last) = NULL;       /* patch up previous node's next pointer */
  }

  return (head);
}



/* FUNCTION: add_null_statement -
 * echris - 2/4/98
 */

static statement_t *add_null_statement(statement_t *s, int before) {
  statement_t *nullstmt;

  nullstmt = alloc_statement(S_NULL, T_LINENO(s), T_FILENAME(s));

  /*** the insertion routines take care of flow graph and tree pointers ***/
  if (before)				/*** insert nullstmt BEFORE s ***/
    insertbefore_stmt(nullstmt, s);
  else					/*** insert nullstmt AFTER s ***/
    insertafter_stmt(nullstmt, s);

  return(nullstmt);
}


/* FUNCTION: add_fake_null -
 * echris - 2/4/98
 */

static void add_fake_null(symboltable_t *pst, statement_t *s, int def) {
  expr_t *expr;
  statement_t *nullstmt;

  /*** We use "fake" null statement as follows: ***/
  /*** At the beginning of every function, a fake def of every global ***/
  /*** variable is inserted to ensure proper communication of globals. ***/
  /*** Better insert a use at the end -- SJD 6 Aug 04 ***/

  nullstmt = add_null_statement(s, def); /* def before first, use after last */
    
  /*** create an expression for the variable***/
  expr = alloc_expr(VARIABLE);
  T_IDENT(expr) = pst;
  T_STMT(expr) = nullstmt;

  /*** use flag1 to indicate a def (case 1) or a use (case 2) ***/
  T_FLAG1(expr) = def;

  /*** insert it into the NULL statement ***/
  T_EXPR(nullstmt) = expr;

}


/* FUNCTION: add_fake_defs - 
 * echris - 2/4/98
 */

static void add_fake_defs(symboltable_t *stp) {
  /*** Insert a NULL statement at the beginning of each function ***/
  /*** All global variables and var ensemble parameters will be ***/
  /***  assumed to be defined at this point ***/
  function_t *f;
  genlist_t *glp;
  statement_t *first;
  statement_t *last;
  summarylist_t *uses;

  f = S_FUN_BODY(stp);
  first = T_STLS(f);
  last = T_STLS(f);
  if (T_TYPE(first) == S_COMPOUND || T_TYPE(first) == S_MSCAN) { /* BLC: OK? */
    first = T_STLS(T_CMPD(first));
    last = T_STLS(T_CMPD(last));
    while (T_NEXT(last)) last = T_NEXT(last);
  }

  glp = T_PARAMLS(f);
  while (glp != NULL) {
    if ((S_CLASS(G_IDENT(glp)) == S_PARAMETER) &&
	(D_CLASS(S_DTYPE(G_IDENT(glp))) == DT_ENSEMBLE)) {
      /*** ensemble parameters ***/

      DB1(1, "Creating fake definition for var parameter %s.\n", S_IDENT(G_IDENT(glp)));
      DB1(1, "Creating fake use for var parameter %s.\n", S_IDENT(G_IDENT(glp)));
      if (S_SUBCLASS(G_IDENT(glp)) != SC_OUT) {
	add_fake_null(G_IDENT(glp), first, TRUE); /* def */
      }
      if (S_SUBCLASS(G_IDENT(glp)) != SC_IN) {
	add_fake_null(G_IDENT(glp), last, FALSE); /* use */
      }
    }
    glp = G_NEXT(glp);
  }


  for (uses = T_G_USES(f); uses != NULL; uses = SUM_NEXT(uses)) {
    DB1(1, "Creating fake definition for global variable %s.\n", S_IDENT(SUM_VAR(uses)));
    DB1(1, "Creating fake use for global variable %s.\n", S_IDENT(SUM_VAR(uses)));
    add_fake_null(SUM_VAR(uses), first, TRUE); /* def */
    add_fake_null(SUM_VAR(uses), last, FALSE); /* use */
  }

}


static set_t* get_inout_pst(symboltable_t* pst) {
  set_t* set;
  genlist_t* new_g;

  if (pst == NULL) {
    return NULL;
  }

  set = alloc_set(VAR);
  SET_VAR(set) = pst;
  SET_NEXT(set) = NULL;
  new_g = alloc_gen();
  G_EXP(new_g) = build_0ary_op(VARIABLE, pst);
  SET_EXPRS(set) = new_g;

  return set;
}

/* FN: getinoutname - returns set of in or out variables for expr.
 *                    returns base variable and calls 
 *                    expr must have lvalue (i.e., be comprised of only
 *                    ARRAY_REF, VARIABLE, BIDOT or BIAT)
 *                    getinout() on expressions encountered along the way
 */

static set_t *getinoutname(expr_t *expr, int inout) {
  set_t *set, *node;
  expr_t *sub_expr;
  expr_t *lvalue_expr;  /* the lvalue expr that represents this reference */
  genlist_t *new_g;
  expr_t *subscripts;

  if (expr == NULL) {               /* null expr? return null set */
    return (NULL);
  }

  set = (set_t *) NULL;             /* initialize return set to null */
  node = alloc_set(VAR);

  IFDB(30) {
    DBS0(30, "getinoutname() called\n");
    fprintf(zstdout, "ensemble expression is ");
    DBS0(30, "\n");
  }
  
  sub_expr = expr;
  lvalue_expr = expr;

  /* this loop finds the variable in sub_expr */
  /* it also sets lvalue_expr to be the largest lvalue expression in expr */

  while (sub_expr && (T_TYPE(sub_expr) != VARIABLE)) {
    switch (T_TYPE(sub_expr)) {
    case ARRAY_REF:                         /* call getinout() on subscripts */
      if (inout == IN) {
	subscripts = nthoperand(sub_expr, 2);
	while (subscripts) {
	  set = s_union(set, getinout(subscripts, inout));
	  subscripts = T_NEXT(subscripts);
	}
      }
      break;

    case BIAT:
      set_dir(sub_expr, &(SET_DIR(node)));

      if (T_SUBTYPE(expr) == AT_RANDACC && inout == IN) {
	expr_t* map;

	for (map = T_RANDACC(expr); map != NULL; map = T_NEXT(map)) {
	  set = s_union(set, getinout(map, inout));
	}
      }
      break;
    case BIDOT:
      break;
    default:
      lvalue_expr = T_OPLS(sub_expr);
      break;
    }

    sub_expr = T_OPLS(sub_expr);
  }

  if (sub_expr && (T_TYPE(sub_expr) == VARIABLE)) {
    SET_VAR(node) = T_IDENT(sub_expr);
    SET_NEXT(node) = set;
    new_g = alloc_gen();
    G_EXP(new_g) = lvalue_expr;
    SET_EXPRS(node) = new_g;
    
  } else {                              /* no variable found, free node */
    /* possible that gelists allocateded in set_dir() unfreed? */
    PFREE(node, sizeof(set_t));
    node = set;
  }

  return (node);
}


/* FN: loc_getinlhs - return all vars in expr but the base variable 
 */

static set_t *loc_getinlhs(expr_t *expr) {
  set_t *set;
  expr_t *sub_expr;
  
  set = (set_t *) NULL;

  if (expr == NULL)                        /* null expr? */
    return(set);                           /* return null set */
  
  DB0(30, "loc_getinlhs()\n");
  IFDB(30) {
    DB0(30, "expression is ");
    dbg_gen_expr(zstdout, expr);
    DBS0(30, "\n");
  }

  sub_expr = expr;
  while (sub_expr) {
    if (T_TYPE(sub_expr) == ARRAY_REF) {
      expr_t *subscript_expr;

      DBS0(100, "\nType is array ref\n");
      DBS1(100, "Type is %d", T_TYPE(sub_expr));

      subscript_expr = nthoperand(sub_expr, 2);
      while (subscript_expr) {
	set = s_union(set, getinout(subscript_expr, IN));
	subscript_expr = T_NEXT(subscript_expr);
      }
    }
    if (T_TYPE(sub_expr) == BIAT &&
	T_SUBTYPE(sub_expr) == AT_RANDACC) {
      expr_t* map;
      
      for (map = T_RANDACC(sub_expr); map != NULL; map = T_NEXT(map)) {
	set = s_union(set, getinout(map, IN));
      }
    }
    
    sub_expr = T_OPLS(sub_expr);
  }

  IFDB(30) {
    DB0(30, "set is ");
    p_set(zstdout, set);
  }
  DB0(30, "loc_getinlhs() exiting\n");

  return(set);
}


/***
 *** FUNCTION: get_external_univ()
 ***           Return the set that includes fake dependence on ExternalDep
 *** sungeun - 21 May 1998
 ***/

static set_t* get_external_univ(void) {
  set_t *new;
  new = alloc_set(VAR);
  SET_VAR(new) = ExternalDep;
  SET_IMOD(new) = 1; /** why am i doing this? ***/

  /* allocate dummy expression for variable - never freed */
  SET_EXPRS(new) = alloc_gen();
  G_EXP(SET_EXPRS(new)) = alloc_expr(VARIABLE);
  T_IDENT(G_EXP(SET_EXPRS(new))) = SET_VAR(new);

  SET_NEXT(new) = NULL;

  return new;
}


/* FN: get_params - returns in/out set for params based on inout flag
 *                  in - return all variables in params except out params
 *                  out - return out and inout params
 */

static set_t *get_params(expr_t *params, function_t *f, int inout) {
  genlist_t *glp;
  set_t *set;

  set = (set_t *) NULL;

  glp = T_PARAMLS(f);
  while (glp) {
    if (inout == IN) { /* inset? */
      if ((S_CLASS(G_IDENT(glp)) == S_PARAMETER) &&   /* out parameter not added to in */
	  (S_SUBCLASS(G_IDENT(glp)) != SC_OUT)) {
        set = s_union(set, getinout(params, inout));
      }
    } else {
      if (((S_CLASS(G_IDENT(glp)) == S_PARAMETER) &&
	   (S_SUBCLASS(G_IDENT(glp)) == SC_INOUT)) ||
	  ((S_CLASS(G_IDENT(glp)) == S_PARAMETER) &&
	   (S_SUBCLASS(G_IDENT(glp)) == SC_OUT))) {
	set = s_union(set, getinoutname(params, inout)); 
      }
    }
    glp = G_NEXT(glp);
    params = T_NEXT(params);
  }

  return(set);
}


static set_t* add_hierarchy_inout(set_t* set, symboltable_t* pst, symboltable_t* regpst,
				  symboltable_t* distpst, symboltable_t* gridpst,
				  int preserve, int inout, int debug, int* first) {
  datatype_t* pdt;
  symboltable_t* pstreg = NULL;
  symboltable_t* pstdist = NULL;
  symboltable_t* pstgrid = NULL;

  /** conservative: destructive assignment may add to out set only **/

  if (!(S_CLASS(pst) == S_VARIABLE || S_CLASS(pst) == S_PARAMETER) ||
      S_IS_CONSTANT(pst)) {
    return set;
  }

  pdt = datatype_find_ensemble(S_DTYPE(pst));
  if (pdt) {
    if (D_ENS_REG(pdt)) {
      pstreg = expr_find_root_pst(D_ENS_REG(pdt));
      if (pstreg) {
	pstdist = D_REG_DIST(S_DTYPE(pstreg));
	if (pstdist) {
	  pstgrid = D_DIST_GRID(S_DTYPE(pstdist));
	}
      }
    }
    if (gridpst && pstgrid && S_CLASS(pstgrid) != S_PARAMETER && S_CLASS(gridpst) != S_PARAMETER && pstgrid != gridpst) {
      return set;
    }
    if (distpst && pstdist && S_CLASS(pstdist) != S_PARAMETER && S_CLASS(distpst) != S_PARAMETER && pstdist != distpst) {
      return set;
    }
    if (regpst && pstreg && S_CLASS(pstreg) != S_PARAMETER && S_CLASS(regpst) != S_PARAMETER && pstreg != regpst) {
      return set;
    }
    if (inout == IN && !preserve) {
      if (pstreg == regpst || pstdist == distpst || pstgrid == gridpst) {
	return set;
      }
    }
    set = s_union(set, get_inout_pst(pst));
    if (debug) {
      if (*first) { printf("  Adding"); *first = 0; }
      fgenf(stdout, " %s", S_IDENT(pst));
    }
    return set;
  }

  if (gridpst || distpst) {
    pdt = datatype_find_region(S_DTYPE(pst));
    if (pdt) {
      pstdist = D_REG_DIST(pdt);
      if (pstdist) {
	pstgrid = D_DIST_GRID(S_DTYPE(pstdist));
      }
      if (gridpst && pstgrid && S_CLASS(pstgrid) != S_PARAMETER && S_CLASS(gridpst) != S_PARAMETER && pstgrid != gridpst) {
	return set;
      }
      if (distpst && pstdist && S_CLASS(pstdist) != S_PARAMETER && S_CLASS(distpst) != S_PARAMETER && pstdist != distpst) {
	return set;
      }
      set = s_union(set, get_inout_pst(pst));
      if (debug) {
	if (*first) { printf("  Adding"); *first = 0; }
	fgenf(stdout, " %s", S_IDENT(pst));
      }
      return set;
    }
  }

  if (gridpst) {
    pdt = datatype_find_distribution(S_DTYPE(pst));
    if (pdt) {
      pstgrid = D_DIST_GRID(pdt);
      if (pstgrid && S_CLASS(pstgrid) != S_PARAMETER && S_CLASS(gridpst) != S_PARAMETER && pstgrid != gridpst) {
	return set;
      }
      set = s_union(set, get_inout_pst(pst));
      if (debug) {
	if (*first) { printf("  Adding"); *first = 0; }
	fgenf(stdout, " %s", S_IDENT(pst));
      }
      return set;
    }
  }

  return set;
}

/* FN: getinout - returns in or out set for expr based on inout flag
 *                (IN or OUT).  
 */

static set_t *getinout(expr_t *expr, int inout) {
  set_t *set;
  
  set = (set_t *) NULL;                  /* initialize return set to null */

  if (expr == NULL)                      /* null expr? return null set */
    return(set);
  
  IFDB(30) {
    DBS0(30, "getinout() called\n");
    DBS0(30, "expression is ");
    dbg_gen_expr(zstdout, expr);
    DBS0(30, "\n");
  }
  
  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case CONSTANT:
    break;

  case VARIABLE:
  case ARRAY_REF:
  case BIDOT:
  case BIAT:
    if (inout == IN)
      set = getinoutname(expr, inout);

    break;

  case BIOP_GETS:
  case BIASSIGNMENT:
    {
      expr_t *rhs_expr;

      if ((inout == IN) && (T_TYPE(expr) == BIASSIGNMENT))
	set = loc_getinlhs(nthoperand(expr,1));
      else
	set = getinoutname(nthoperand(expr,1), inout);

      rhs_expr = nthoperand(expr,2);
      while (rhs_expr) {
	set = s_union(set, getinout(rhs_expr, inout));
	rhs_expr = T_NEXT(rhs_expr);
      }
    }
    break;

  case REDUCE:
    set = getinoutname(T_OPLS(expr),inout);
    break;

  case FUNCTION:
    {
      expr_t *fun_expr;
      symboltable_t *fun_pst;
      function_t *fn;

      fun_expr = nthoperand(expr, 1);
      fun_pst = T_IDENT(fun_expr);
      fn = S_FUN_BODY(fun_pst);

      /* This is a ZPL function */
      if (function_internal(fn)) {
	if (T_PARALLEL(fn)) {
	  /* parallel functions assumed to def/use everything? */
	  set = get_external_univ();
	}
      } else { /* external or std context */
	if (inout == OUT) {
	  if (S_STD_CONTEXT(fun_pst)) {
	    if (symtab_is_fixed(fun_pst)) {
	      /* certain standard context functions can't be reordered */
	      set = get_external_univ();
	    }
	  } else {
	    /* external parallel user calls can't be reordered */
	    if (!T_PROMOTED_FUNC(fun_expr)) {
	      set = get_external_univ();
	    }
	  }
	}
      }

      set = s_union(set, get_params(nthoperand(expr, 2),
				    S_FUN_BODY(T_IDENT(fun_expr)), inout));

      if ((!strcmp(S_IDENT(fun_pst), "_UPDATE_GRID")) ||
	  (!strcmp(S_IDENT(fun_pst), "_UPDATE_DISTRIBUTION")) ||
	  (!strcmp(S_IDENT(fun_pst), "_UPDATE_REGION"))) {
	summarylist_t* uses;
	symboltable_t* pst;
	symboltable_t* gridpst = NULL;
	symboltable_t* distpst = NULL;
	symboltable_t* regpst = NULL;
	int preserve = expr_intval(nthoperand(expr, 4));
	int debug = 0; /* to debug inout hierarchy building, turn to 1 */
	int first; /* for debugging */

	if (!strcmp(S_IDENT(fun_pst), "_UPDATE_GRID")) {
	  gridpst = expr_find_root_pst(nthoperand(expr, 2));
	}
	if (!strcmp(S_IDENT(fun_pst), "_UPDATE_DISTRIBUTION")) {
	  distpst = expr_find_root_pst(nthoperand(expr, 2));
	}
	if (!strcmp(S_IDENT(fun_pst), "_UPDATE_REGION")) {
	  regpst = expr_find_root_pst(nthoperand(expr, 2));
	}

	if (debug) {
	  fgenf(stdout, "%s: %E (%s:%d)\n", inout == IN ? "IN " : "OUT", expr,
		T_FILENAME(T_STMT(expr)), T_LINENO(T_STMT(expr)));
	  first = 1;
	}
	for (uses = T_G_USES(T_PARFCN(T_STMT(expr))); uses != NULL; uses = SUM_NEXT(uses)) {
	  set = add_hierarchy_inout(set, SUM_VAR(uses), regpst, distpst, gridpst, preserve, inout, debug, &first);
	}
	for (pst = T_DECL(T_PARFCN(T_STMT(expr))); pst != NULL; pst = S_SIBLING(pst)) {
	  set = add_hierarchy_inout(set, pst, regpst, distpst, gridpst, preserve, inout, debug, &first);
	}
	if (debug && !first) printf("\n");
      }
    }
    break;
  case PERMUTE:
    {
      expr_t *map;

      /*** get the in/out set for the expression ***/
      set = s_union(set, getinout(T_OPLS(expr), inout));

      for (map = T_MAP(expr); map != NULL; map = T_NEXT(map)) {
	set = s_union(set, getinout(map, inout));
      }
    }
    break;
  default:
    {
      expr_t *op_expr;

      op_expr = T_OPLS(expr);
      while (op_expr) {
	set = s_union (set, getinout(op_expr, inout));
	op_expr = T_NEXT(op_expr);
      }
    }
    break;
  }
  IFDB(30) {
    DBS0(30, "set is ");
    p_set(zstdout, set);
    DBS0(30, "getinout exiting\n");
  }

  return(set);
}


/* FUNCTION: inout_stmtls_fn - 
 * Only called by traverse_stmtls_g().
 * echris - 2/4/98
 */

static void inout_stmtls_fn(statement_t *stmt) {
  if (ioro == IN) {
    inout = s_union(inout, s_copy(T_IN(stmt)));
  } else {
    inout = s_union(inout, s_copy(T_OUT(stmt)));
  }
}


/* FUNCTION: inout_stmtls - 
 * echris - 2/4/98
 */

static set_t *inout_stmtls(statement_t *stmtls, int io) {
  inout = NULL;
  ioro = io;

  traverse_stmtls_g(stmtls, NULL, inout_stmtls_fn, NULL, NULL);

  return (inout);
}


/* FUNCTION: dep_if -
 * echris - 2/4/98
 */

static void dep_if(statement_t *stmt) {
  if_t	*ifstmt;
  
  DB0(30, "dep if called\n");
  if (stmt == NULL) {
    INT_FATAL(NULL, "dep_if - stmt is null");
  }
  ifstmt = T_IF(stmt);
  if (ifstmt == NULL) {
    INT_FATAL(stmt, "dep_if - ifstmt is null"); 
  }
  
  T_IN(stmt) = getinout(T_IFCOND(ifstmt), IN);
  T_OUT(stmt) = getinout(T_IFCOND(ifstmt), OUT);
  dep_stmtls(T_THEN(ifstmt));
  dep_stmtls(T_ELSE(ifstmt));

  /* put body in/outs on statement if this if is a shard */
  if (stmt_is_shard(stmt)) {
    T_IN(stmt) = s_union(T_IN(stmt), inout_stmtls(T_THEN(ifstmt), IN));
    T_IN(stmt) = s_union(T_IN(stmt), inout_stmtls(T_ELSE(ifstmt), IN));
    T_OUT(stmt) = s_union(T_OUT(stmt), inout_stmtls(T_THEN(ifstmt), OUT));
    T_OUT(stmt) = s_union(T_OUT(stmt), inout_stmtls(T_ELSE(ifstmt), OUT));
    
  }
  DB0(30, "dep_if exiting\n");
}


/* FUNCTION: dep_loop - 
 * echris - 2/4/98
 */

static void dep_loop(statement_t *stmt) {
  loop_t *loop;
  
  if (stmt == NULL) {
    INT_FATAL(NULL, "dep_loop - stmt is null");
  }
  
  loop = T_LOOP(stmt);
  
  if (loop == NULL) {
    INT_FATAL(stmt, "dep_loop - loop is null");
  }
  
  switch (T_TYPE(loop)) {
  case L_WHILE_DO:
  case L_REPEAT_UNTIL:
    T_IN(stmt) = getinout(T_LOOPCOND(loop), IN);
    T_OUT(stmt) = getinout(T_LOOPCOND(loop), OUT);
    dep_stmtls(T_BODY(loop));

    break;
  case L_DO:
    /* BLC -- iterator is never in */
    T_IN(stmt) = getinout(T_START(loop),IN);
    T_IN(stmt) = s_union(T_IN(stmt), getinout(T_STOP(loop), IN));
    T_IN(stmt) = s_union(T_IN(stmt), getinout(T_STEP(loop), IN));

    T_OUT(stmt) = getinout(T_IVAR(loop),OUT);
    T_OUT(stmt) = s_union(T_OUT(stmt), getinout(T_START(loop), OUT));
    T_OUT(stmt) = s_union(T_OUT(stmt), getinout(T_STOP(loop), OUT));
    T_OUT(stmt) = s_union(T_OUT(stmt), getinout(T_STEP(loop), OUT));
    dep_stmtls(T_BODY(loop));
    break;
  default:
    INT_FATAL(NULL, "dep_loop - Unknown loop, type is %d",T_TYPE(loop));
    break;
  }

  /* put body in/outs on statement if this if is a shard */
  if (stmt_is_shard(stmt)) {
    T_IN(stmt) = s_union(T_IN(stmt), inout_stmtls(T_BODY(loop), IN));
    T_OUT(stmt) = s_union(T_OUT(stmt), inout_stmtls(T_BODY(loop), OUT));
  }
}


/* FUNCTION: dep_region - 
 * echris - 2/4/98
 */

static void dep_region(statement_t *stmt) {
  region_t	*regionstmt;
  
  DB0(30, "dep_region()\n");
  
  if (stmt == NULL) {
    INT_FATAL(NULL, "dep_region - stmt is null");
  }

  regionstmt = T_REGION(stmt);

  if (T_MASK_EXPR(regionstmt) != NULL)
    T_IN(stmt) = getinout(T_MASK_EXPR(regionstmt), IN);

  if (T_BODY(regionstmt) != NULL) {
    dep_stmtls(T_BODY(regionstmt));
  }

}


/* FUNCTION: dep_mloop - 
 * echris - 2/4/98
 */

static void dep_mloop(statement_t *stmt) {
  mloop_t	*mloopstmt;
  statement_t *s;
  int i;
  
  DB0(30, "dep_mloop()\n");
  
  if (stmt == NULL) {
    INT_FATAL(NULL, "dep_mloop - stmt is null");
  }

  mloopstmt = T_MLOOP(stmt);

  if (T_BODY(mloopstmt) != NULL) {
    dep_stmtls(T_BODY(mloopstmt));
  }

  for (i = 0; i < T_MLOOP_RANK(mloopstmt); i++) {
    if (T_MLOOP_PREPRE(mloopstmt,i) != NULL) {
      dep_stmtls(T_MLOOP_PREPRE(mloopstmt,i));
    }
    if (T_MLOOP_PRE(mloopstmt,i) != NULL) {
      dep_stmtls(T_MLOOP_PRE(mloopstmt,i));
    }
    if (T_MLOOP_POST(mloopstmt,i) != NULL) {
      dep_stmtls(T_MLOOP_POST(mloopstmt,i));
    }
    if (T_MLOOP_POSTPOST(mloopstmt,i) != NULL) {
      dep_stmtls(T_MLOOP_POSTPOST(mloopstmt,i));
    }
  }

  T_IN(stmt) = NULL;
  T_OUT(stmt) = NULL;
  for (s = T_BODY(mloopstmt); s != NULL; s = T_NEXT(s)) {
    T_IN(stmt) = s_union(T_IN(stmt), s_copy(T_IN(s)));
    T_OUT(stmt) = s_union(T_OUT(stmt), s_copy(T_OUT(s)));
  }
  for (i = 0; i < T_MLOOP_RANK(mloopstmt); i++) {
    for (s = T_MLOOP_PREPRE(mloopstmt,i); s != NULL; s = T_NEXT(s)) {
      T_IN(stmt) = s_union(T_IN(stmt), s_copy(T_IN(s)));
      T_OUT(stmt) = s_union(T_OUT(stmt), s_copy(T_OUT(s)));
    }
    for (s = T_MLOOP_PRE(mloopstmt,i); s != NULL; s = T_NEXT(s)) {
      T_IN(stmt) = s_union(T_IN(stmt), s_copy(T_IN(s)));
      T_OUT(stmt) = s_union(T_OUT(stmt), s_copy(T_OUT(s)));
    }
    for (s = T_MLOOP_POST(mloopstmt,i); s != NULL; s = T_NEXT(s)) {
      T_IN(stmt) = s_union(T_IN(stmt), s_copy(T_IN(s)));
      T_OUT(stmt) = s_union(T_OUT(stmt), s_copy(T_OUT(s)));
    }
    for (s = T_MLOOP_POSTPOST(mloopstmt,i); s != NULL; s = T_NEXT(s)) {
      T_IN(stmt) = s_union(T_IN(stmt), s_copy(T_IN(s)));
      T_OUT(stmt) = s_union(T_OUT(stmt), s_copy(T_OUT(s)));
    }
  }
}


/* FUNCTION: dep_nloop - 
 * echris - 2/4/98
 */

static void dep_nloop(statement_t *stmt) {
  nloop_t* nloop;
  statement_t* body;
  int dim, depth;
  statement_t* s;
  
  DB0(30, "dep_nloop()\n");
  
  if (stmt == NULL) {
    INT_FATAL(NULL, "dep_nloop - stmt is null");
  }

  nloop = T_NLOOP(stmt);
  body = T_NLOOP_BODY(nloop);

  if (body != NULL) {
    dep_stmtls(body);
  }

  T_IN(stmt) = NULL;
  T_OUT(stmt) = NULL;
  
  depth = T_NLOOP_DEPTH(nloop);
  for (dim=0; dim < T_NLOOP_DIMS(nloop); dim++) {
    expr_t* tmp_expr;

    tmp_expr = alloc_expr(VARIABLE);
    T_IDENT(tmp_expr) = get_blank_arrayref_index(dim+1,depth);
    T_OUT(stmt) = s_union(T_OUT(stmt), getinoutname(tmp_expr,OUT));
  }
  for (s = body; s != NULL; s = T_NEXT(s)) {
    T_IN(stmt) = s_union(T_IN(stmt), s_copy(T_IN(s)));
    T_OUT(stmt) = s_union(T_OUT(stmt), s_copy(T_OUT(s)));
  }
}


/* FUNCTION: dep_stmt - 
 * echris - 2/4/98
 */

static void dep_stmt(statement_t *stmt) {
  expr_t *expr;
  
  if (stmt == NULL)
    return;
  IFDB(30) {
    DBS1(30, "dep_stmt called on lineno %d\n",T_LINENO(stmt));
    DBS0(30, "dep_stmt called\n");
    dbg_gen_stmt(zstdout, stmt);
    DBS0(30, "\n");
  }
  T_IN(stmt) = NULL;
  T_OUT(stmt) = NULL;
  
  switch (T_TYPE(stmt)) {
  case S_NULL:
    /*** Assumes that ALL S_NULL statements other than the fake ***/
    /*** S_NULL nodes inserted at the beginning of the pass have ***/
    /*** nothing in the T_EXPR() fields. ***/
    T_IN(stmt) = NULL;
    T_OUT(stmt) = NULL;
    if ((expr = T_EXPR(stmt)) != NULL) {
      for ( ; expr != NULL; expr = T_OPLS(expr)) {
	if (T_FLAG1(expr))
	  T_OUT(stmt) = s_union(T_OUT(stmt), getinoutname(expr, OUT));
	else
	  T_IN(stmt) = s_union(T_IN(stmt), getinoutname(expr, IN));
      }
    }
    break;
  case S_IO:
    if (T_TYPE(T_IO(stmt)) == IO_READ) {
      T_IN(stmt) = NULL;
      T_OUT(stmt) = getinoutname(IO_EXPR1(T_IO(stmt)), OUT);
    } else {
      T_IN(stmt) = getinout(IO_EXPR1(T_IO(stmt)), IN);
      T_OUT(stmt) = NULL;
    }
    /* BLC: I/O statements may not be reordered */
    T_OUT(stmt) = s_union(T_OUT(stmt),get_external_univ());
    break;
  case S_MSCAN: /* BLC -- OK to do the same thing? */
  case S_COMPOUND: 
    dep_stmtls(T_CMPD_STLS(stmt));
    break;
  case S_WRAP:
  case S_REFLECT:
    for (expr = T_OPLS(T_WRAP(stmt)); expr; expr = T_NEXT(expr)) {
      T_IN(stmt) = s_union(T_IN(stmt), getinout(expr, IN));
      T_OUT(stmt) = s_union(T_OUT(stmt), getinoutname(expr, OUT));
    }
    break;
  case S_COMM:
    /*** inserted by the compiler, so we can ignore, hopefully ***/
    break;
  case S_EXPR:
    T_IN(stmt) = getinout(T_EXPR(stmt), IN);
    T_OUT(stmt) = getinout(T_EXPR(stmt), OUT);
    break;
  case S_IF:
    dep_if(stmt);
    break;
  case S_LOOP:
    dep_loop(stmt);
    break;
  case S_REGION_SCOPE:
    dep_region(stmt);
    break;
  case S_MLOOP:
    dep_mloop(stmt);
    break;
  case S_NLOOP:
    dep_nloop(stmt);
    break;
  case S_HALT:
    /* BLC: Halt statements may not be reordered */
  case S_EXIT:
  case S_CONTINUE:
    /*** sungeun *** nor may exit or continue ***/
    T_OUT(stmt) = s_union(T_OUT(stmt),get_external_univ());
    break;
  case S_END:
    break;
  case S_RETURN:
    T_IN(stmt) = getinout(T_RETURN(stmt), IN);
    T_OUT(stmt) = getinoutname(T_RETURN(stmt), OUT);
    /*** sungeun *** return statements may not be reordered, either ***/
    T_OUT(stmt) = s_union(T_OUT(stmt),get_external_univ());
    break;
  default:
    INT_FATAL(stmt, "dep_stmt - unknown stmt, type is %d",T_TYPE(stmt));
    break;
  }
  /* BLC: make all statements use the magic external symbol to prevent
     illegal reorderings;  shouldn't prevent them from getting reordered
     relative to one another -- only in terms of statements that define
     this magic symbol */
  T_IN(stmt) = s_union(T_IN(stmt),get_external_univ());
  IFDB(5) {
    printf("%d: ",T_LINENO(stmt));
    dbg_gen_stmt(zstdout, stmt);
    DBS0(5, "In set is ");
    p_set(zstdout, T_IN(stmt));
    DBS0(5, "\nOut set is ");
    p_set(zstdout, T_OUT(stmt));
    DBS0(5, "\n\n");
  }
  DB0(30, "Calling addinset\n");
  addinset(zstdout, T_IN(stmt), stmt);
  DB0(30, "Passed addinset, calling addoutset\n");
  addoutset(zstdout, T_OUT(stmt),stmt);
  DB0(30, "Passed addoutset\n");
}


/* FUNCTION: dep_pst - 
 * echris - 2/4/98
 */

static void dep_pst(symboltable_t *stp) {
  statement_t *pstmt;

  if (stp == NULL)
    return;

  if (S_STD_CONTEXT(stp) == TRUE)
    return;

  switch(S_CLASS(stp)) {
  case S_SUBPROGRAM: 
    currentF = S_FUN_BODY(stp);
    if (T_PARALLEL(currentF)) {
      pstmt = T_STLS(currentF);
      if (pstmt == NULL) {
	/*** body of function is null or this is a prototype ***/
	return;
      }

      DBS1(1, "In function %s...\n", S_IDENT(stp));

      if (InitDepend == FALSE) {                 /*** not initialized ***/
	if (strcmp(entry_name, S_IDENT(stp))) {  /*** not the entry point ***/
	  add_fake_defs(stp);
	}
      }

      while (pstmt != NULL) {
	DBS1(30, "statement num %d \n",T_LINENO(pstmt));

	dep_stmt(pstmt);
	DB0(30, "dep_stmt returns\n");
	pstmt = T_NEXT(pstmt);
      }
    }
  default:
    break;
  }
}

/* FUNCTION: dep_stmtls - 
 * echris - 2/4/98
 */

static void dep_stmtls(statement_t *stmtls) {
  statement_t *temp;

  temp = stmtls;

  for (; temp != NULL; temp = T_NEXT(temp) ) {
    dep_stmt(temp);
  }
}

/*** exported routines ***/
set_t* getin(expr_t *expr) {
  return getinout(expr, IN);
}

set_t* getout(expr_t *expr) {
  return getinout(expr, OUT);
}


/*********************/
/*** summary stuff ***/
/*********************/

/* FUNCTION: reset_summarized - reset all T_IS_SUMMARIZED bits on each fun
 * echris - 2/4/98
 */

static void reset_summarized(module_t *mod) {
  function_t *f;

  for (f = T_FCNLS(mod) ; f != NULL; f = T_NEXT(f))
    T_IS_SUMMARIZED(f) = FALSE;

}


/* FUNCTION: varOsummarylist - 
 * echris - 2/4/98
 */

static summarylist_t *varOsummarylist(summarylist_t *list,
				      summarylisttype type,
				      symboltable_t *var) {
  /*** returns the summarylist element which has ***/
  /***	var of type (ignores region) ***/

  if (list == NULL) {
    return(NULL);
  }
  else if ((var == SUM_VAR(list)) && (type == SUM_TYPE(list))) {
    return(list);
  }
  else {
    return(varOsummarylist(SUM_NEXT(list), type, var));
  }
}


/* FUNCTION: memberOsummarylist - 
 * echris - 2/4/98
 */

static summarylist_t *memberOsummarylist(summarylist_t *list,
					 summarylisttype type,
					 symboltable_t *var,
					 expr_t *region) {
  /*** returns the summarylist element which has ***/
  /***	var of type and region ***/
  genlist_t *glp;
  summarylist_t *l;

  if ((l = varOsummarylist(list, type, var)) != NULL) {
    glp = SUM_REGIONS(l);
    while (glp != NULL) {
      if (expr_equal(G_EXPR(glp), region))
	return(l);
      glp = G_NEXT(glp);
    }
  }

  return(NULL);
}


/* FUNCTION: add_region - add region to a genlist (if it is not there already)
 * return ptr to node containing region
 * echris - 2/4/98
 */

static genlist_t *add_region(summarylist_t *element, expr_t* region) {
  genlist_t *glp;

  if (element == NULL) {
    INT_FATAL(NULL, "List element is NULL.");
  }

  glp = SUM_REGIONS(element);

  while ((glp != NULL) && (G_EXPR(glp) != region)) {
    glp = G_NEXT(glp);
  }

  if (glp == NULL) {
    glp = alloc_gen();
    G_EXPR(glp) = region;
    G_NEXT(glp) = SUM_REGIONS(element);
    SUM_REGIONS(element) = glp;
  }

  return(glp);

}


/* FUNCTION: new_summarylist - allocate a new summarylist_t struct
 * echris - 2/4/98
 */

static summarylist_t *new_summarylist(summarylisttype type,
				      symboltable_t *var,
				      expr_t* region) {
  summarylist_t *new;

  new = (summarylist_t *) PMALLOC(sizeof(summarylist_t));

  SUM_TYPE(new) = type;
  SUM_VAR(new) = var;
  SUM_REGIONS(new) = NULL;
  SUM_NEXT(new) = NULL;

  if (region != NULL) {
    add_region(new, region);
  }

  return(new);
}


/* FUNCTION: prepend2summarylist - 
 * echris - 2/4/98
 */

static summarylist_t *prepend2summarylist(summarylist_t *list,
					  summarylisttype type,
					  symboltable_t *var,
					  expr_t* region) {
  summarylist_t *new;

  if (!(new = varOsummarylist(list, type, var))) {
    new = new_summarylist(type, var, region);
    SUM_NEXT(new) = list;
  }
  else {
    add_region(new, region);
    new = list;
  }

  /*** return head of list ***/
  return(new);
}


/* FUNCTION: find_globals_in_expr - ???
 * Only called from traverse_exprls_g().
 * echris - 2/4/98
 */


static void find_globals_in_expr(expr_t *e) {
  expr_t *reg;

  if (e == NULL)
    return;

  switch(T_TYPE(e)) {
  case VARIABLE:
    if (S_DTYPE(T_IDENT(e)) == NULL) {
      INT_FATAL(NULL, "Symbol '%s' undefined", S_IDENT(T_IDENT(e)));
    } else if ((S_LEVEL(T_IDENT(e)) == 0) &&
	       /*** (D_CLASS(S_DTYPE(T_IDENT(e))) == DT_ENSEMBLE) && ***/
	       (S_STD_CONTEXT(T_IDENT(e)) == FALSE)) {
      reg = RMSCurrentRegion();
      switch (currentT) {
      case SUM_MOD:
	if (!memberOsummarylist(T_G_MODS(currentF),SUM_MOD, T_IDENT(e),reg)) {
	  T_G_MODS(currentF) = prepend2summarylist(T_G_MODS(currentF),SUM_MOD,
						   T_IDENT(e),reg);
	  changed = TRUE;
	}
	break;
      case SUM_USE:
	if (!memberOsummarylist(T_G_USES(currentF),SUM_USE,T_IDENT(e),reg)) {
	  T_G_USES(currentF) = prepend2summarylist(T_G_USES(currentF),SUM_USE,
						   T_IDENT(e),reg);
	  changed = TRUE;
	}
	break;
      case SUM_BOTH:
	if (!memberOsummarylist(T_G_MODS(currentF),SUM_MOD,T_IDENT(e),reg)) {
	  T_G_MODS(currentF) = prepend2summarylist(T_G_MODS(currentF),SUM_MOD,
						   T_IDENT(e),reg);
	  changed = TRUE;
	}
	if (!memberOsummarylist(T_G_USES(currentF),SUM_USE,T_IDENT(e),reg)) {
	  T_G_USES(currentF) = prepend2summarylist(T_G_USES(currentF),SUM_USE,
						   T_IDENT(e),reg);
	  changed = TRUE;
	}
	break;
      default:
	INT_FATAL(NULL, "No such summarylisttype %d", currentT);
	break;
      }
    }
    break;
  default:
    break;
  }
}



/* FUNCTION: find_globals_in_stmt - ???
 * Only called by traverse_stmtls_g().
 * echris - 2/4/98
 */

static void find_globals_in_stmt(statement_t *s) {
  expr_t* expr;

  if (s == NULL)
    return;

  switch (T_TYPE(s)) {
  case S_EXPR:
    expr = T_EXPR(s);
    if (T_IS_ASSIGNOP(T_TYPE(expr))) {
      currentT = SUM_MOD;
      traverse_expr_g(left_expr(expr), find_globals_in_expr, NULL);
      currentT = SUM_USE;
      if (T_TYPE(expr) != BIASSIGNMENT)
	traverse_expr_g(left_expr(expr), find_globals_in_expr, NULL);
      traverse_exprls_g(right_expr(expr), find_globals_in_expr, NULL);
    } else if (T_TYPE(expr) == REDUCE) {
      currentT = SUM_MOD;
      traverse_exprls_g(T_OPLS(expr), find_globals_in_expr, NULL);
      currentT = SUM_USE;
      traverse_exprls_g(T_OPLS(expr), find_globals_in_expr, NULL);
    } else {
      if (T_TYPE(expr) != FUNCTION) {
	INT_FATAL(s, "Only function calls should be here.");
      }
      currentT = SUM_USE;
      traverse_exprls_g(nthoperand(expr, 2), find_globals_in_expr, NULL);
    }
    break;
  case S_IF:
    currentT = SUM_USE;
    traverse_exprls_g(T_IFCOND(T_IF(s)), find_globals_in_expr, NULL);
    break;
  case S_LOOP:
    currentT = SUM_USE;
    switch (T_TYPE(T_LOOP(s))) {
    case L_DO:
      traverse_exprls_g(T_IVAR(T_LOOP(s)), find_globals_in_expr, NULL);
      traverse_exprls_g(T_START(T_LOOP(s)), find_globals_in_expr, NULL);
      traverse_exprls_g(T_STOP(T_LOOP(s)), find_globals_in_expr, NULL);
      traverse_exprls_g(T_STEP(T_LOOP(s)), find_globals_in_expr, NULL);
      break;
    case L_WHILE_DO:
    case L_REPEAT_UNTIL:
      traverse_exprls_g(T_LOOPCOND(T_LOOP(s)), find_globals_in_expr, NULL);
      break;
    default:
      INT_FATAL(s, "Invalid loop type!");
      break;
    }
    break;
  case S_RETURN:
    currentT = SUM_USE;
    traverse_exprls_g(T_RETURN(s), find_globals_in_expr, NULL);
    break;
  case S_REGION_SCOPE:
    if (T_MASK_EXPR(T_REGION(s)) != NULL) {
      currentT = SUM_USE;
      traverse_exprls_g(T_MASK_EXPR(T_REGION((s))), find_globals_in_expr, NULL);
    }
    break;
  case S_MLOOP:
  case S_NLOOP:
    break;
  case S_IO:
    if (IO_TYPE(T_IO(s)) == IO_READ)
      currentT = SUM_MOD;
    else
      currentT = SUM_USE;
    traverse_exprls_g(IO_EXPR1(T_IO(s)), find_globals_in_expr, NULL);
    break;
  case S_WRAP:
  case S_REFLECT:
    currentT = SUM_BOTH;
    traverse_exprls_g(T_OPLS(T_WRAP(s)), find_globals_in_expr, NULL);
    break;
  case S_NULL:
  case S_EXIT:
  case S_HALT:
  case S_CONTINUE:
  case S_MSCAN:
  case S_COMPOUND:
  case S_END:
  case S_COMM:
    break;
  default:
    INT_FATAL(s, "Unrecognized statement type %d", T_TYPE(s));
    break;
  }
}


static summarylist_t *copy_summarylist(summarylist_t *sl)
{
  summarylist_t *new, *last, *l, *nl;
  genlist_t *reg;

  new = NULL;
  for (l = sl; l != NULL; l = SUM_NEXT(l)) {
    nl = new_summarylist(SUM_TYPE(l), SUM_VAR(l), NULL);
    for (reg = SUM_REGIONS(l); reg != NULL; reg = G_NEXT(reg)) {
      add_region(nl, G_EXPR(reg));
    }
    if (new == NULL) {
      new = last = nl;
    }
    else {
      SUM_NEXT(last) = nl;
    }
  }

  return(new);
}


/* FUNCTION: merge2summarylists - merge list1 with list2..
 *                                list2 remains in tact, list1 is modified
 * echris - 2/4/98
 */

static summarylist_t *merge2summarylists(summarylist_t *list1,
					 summarylist_t *list2) {
  summarylist_t *l;
  genlist_t *glp;

  if (list1 == NULL) {
    return(copy_summarylist(list2));
  }
  else if (list2 == NULL) {
    return(list1);
  }
  else {
    for (l = list2; l != NULL; l = SUM_NEXT(l)) {
      for (glp = SUM_REGIONS(l); glp != NULL; glp = G_NEXT(glp)) {
	if (!memberOsummarylist(list1, SUM_TYPE(l), SUM_VAR(l), G_EXPR(glp))) {
	  list1 = prepend2summarylist(list1, SUM_TYPE(l),
				      SUM_VAR(l), G_EXPR(glp));
	  changed = TRUE;
	}
      }
    }
    return(list1);
  }
}



/* FUNCTION: print_summarylist - produce text form of summarylist_t struct
 * echris - 2/4/98
 */

static void print_summarylist(FILE * fp, summarylist_t *list) {
  summarylist_t *l;
  genlist_t *glp;

  for (l = list; l != NULL; l = SUM_NEXT(l)) {
    fprintf(fp, "'%s'", S_IDENT(SUM_VAR(l)));
    switch (SUM_TYPE(l)) {
    case SUM_USE:
      fprintf(fp, "\t         USED ");
      break;
    case SUM_MOD:
      fprintf(fp, "\t     MODIFIED ");
      break;
    case SUM_BOTH:
      fprintf(fp, "\tUSED/MODIFIED ");
      break;
    default:
      INT_FATAL(NULL, "No such summarylisttype %d", currentT);
      break;
    }
    fprintf(fp, "over regions [");
    for (glp = SUM_REGIONS(l); glp != NULL; glp = G_NEXT(glp)) {
      if (T_IDENT(G_EXPR(glp)) != NULL)
	fprintf(fp, "%s", S_IDENT(T_IDENT(G_EXPR(glp))));
      else
	fprintf(fp, "RUNTIME REGION");
      if (G_NEXT(glp) != NULL)
	fprintf(fp, ", ");
    }
    fprintf(fp, "]\n");
  }
}


/* FUNCTION: summarize_globals -
 * echris - 2/4/98
 */

static void summarize_globals(function_t *f) {
  callgraph_t *cg;
  calledge_t *edge;

  if (T_PARALLEL(f) && (T_IS_SUMMARIZED(f) == FALSE)) {

    cg = T_CGNODE(f);

    T_IS_SUMMARIZED(f) = TRUE;

    for (edge = CG_EDGE(cg); edge != NULL; edge = CG_NEXT(edge)) {
      if ((T_PARALLEL(S_FUN_BODY(CG_SINK(edge)))) &&
	  (T_IS_SUMMARIZED(S_FUN_BODY(CG_SINK(edge))) == FALSE)) {
	DBS1(1, "Summarizing %s\n", S_IDENT(T_FCN(S_FUN_BODY(CG_SINK(edge)))));
	summarize_globals(S_FUN_BODY(CG_SINK(edge)));
      }
    }

    currentF = f;
    traverse_stmtls_g(T_STLS(f), find_globals_in_stmt, NULL, NULL, NULL);

    for (edge = CG_EDGE(cg); edge != NULL; edge = CG_NEXT(edge)) {
      if ((f != S_FUN_BODY(CG_SINK(edge))) &&
	  (T_PARALLEL(S_FUN_BODY(CG_SINK(edge))))) {
	/*** skip if sink is self (i.e. recursive) ***/
	T_G_MODS(f) = merge2summarylists(T_G_MODS(f),
					 T_G_MODS(S_FUN_BODY(CG_SINK(edge))));;
	T_G_USES(f) = merge2summarylists(T_G_USES(f),
					 T_G_USES(S_FUN_BODY(CG_SINK(edge))));;
      }
    }
    DBS1(1, "Summarized globals for procedure %s..\n", S_IDENT(T_FCN(f)));
    IFDB(1) {
      print_summarylist(zstdout, T_G_MODS(f));
      print_summarylist(zstdout, T_G_USES(f));
    }
  }
}


/* misc stuff */

/* FUNCTION: delete_summarylist - deallocate a summarylist_t struct
 * echris - 2/4/98
 */

static void delete_summarylist(summarylist_t *list) {
  summarylist_t *l;
  genlist_t *glp, *g;

  while (list != NULL) {
    glp = SUM_REGIONS(list);
    while (glp != NULL) {
      g = glp;
      glp = G_NEXT(glp);
      free_gen(g);
    }

    l = list;
    list = SUM_NEXT(list);
    PFREE(l, sizeof(summarylist_t));
  }

}

/* FUNCTION: clean_symtab - ???
 * echris - 2/4/98
 */

static void clean_symtab(void) {
  symboltable_t *pst;
  int i;

  for (i = 0; i < MAXLEVEL; i++) {
    pst = maplvl[i];
    while (pst && (S_LEVEL(pst) == i)) {

      free_stmtlist(S_INLIST(pst));
      S_INLIST(pst) = NULL;
      free_stmtlist(S_OUTLIST(pst));
      S_OUTLIST(pst) = NULL;

      pst = S_SIBPREV(pst);
    }
  }
}


/* FUNCTION: dep_anal - add in/out info to each statement in module
 * echris - 2/4/98
 */

static void dep_anal(module_t *module) {
  symboltable_t *stp;
  function_t *f, *entry;

  /* free and null old summary list data (from previous run of this pass) */
  DBS0(1, "Cleaning summary lists...\n");
  for (f = T_FCNLS(module); f != NULL; f = T_NEXT(f)) {
    if (T_PARALLEL(f)) {
      /*** clean up old graph ***/
      traverse_stmtls_g(T_STLS(f), clean_dependences, NULL, NULL, NULL);

      /*** cleanup summary info ***/
      delete_summarylist(T_G_MODS(f));
      T_G_MODS(f) = NULL;
      delete_summarylist(T_G_USES(f));
      T_G_USES(f) = NULL;
    }
  }

  /*** clean up S_INLIST and S_OUTLIST for all variables ***/
  DBS0(1, "Cleaning symboltable...\n");
  clean_symtab();

  if (pstMAIN == NULL) {  /* find entry procedure if pstMAIN is unset */
    f = T_FCNLS(module);
    entry = NULL;
    while ((entry == NULL) && (f != NULL)) {
      if (!strcmp(S_IDENT(T_FCN(f)), entry_name))
	entry = f;
      else
	f = T_NEXT(f);
    }

    if (entry == NULL) {   /* entry procedure could not be found */
      INT_FATAL(NULL, "Entry procedure not found.");
    }

  } else {  /* entry procedure already known */
    entry = S_FUN_BODY(pstMAIN);
  }

#if (!RELEASE_VER)
  IFDB(1) { iterations = 0; }
#endif

  do {
    DBS1(1, "*** iteration %d ***\n", ++iterations);
    changed = FALSE;
    reset_summarized(module);
    
    /*** do an downward traversal of the callgraph ***/
    DBS0(1, "Summarizing...\n");
    summarize_globals(entry);
  } while (changed);

  for (stp = T_DECL(module); stp != NULL; stp = S_SIBLING(stp)) {
    DB0(30, "decl enter\n");
    dep_pst(stp);
    DB0(30, "decl exit\n");
  }

  if (InitDepend == FALSE) {
    InitDepend = TRUE;    /*** don't initialize again ***/
  }

}


/* FUNCTION: inout - add in/out info to stmts in each module in mod list
 * echris - 2/4/98
 */

static void commdepend(module_t *mod) {
  module_t *temp;

  DB0(30, "commdepend()\n");

  RMSInit();
  for (temp = mod; temp != NULL; temp = T_NEXT(temp) ) {
    DB0(30, "module enter\n");
    dep_anal(temp);
    DB0(30, "module exit\n");
  }
  RMSFinalize();

  DB0(30, "commdepend exiting\n");
}


int call_commdep(module_t *mod,char *s) {
  module_t *modls;

  modls = mod;
  setbuf((FILE *) zstdout, (char *) NULL);
  while (modls != NULL) {
    commdepend(modls);
    modls = T_NEXT(modls);
  }
  if (zstdout != stdout)
    fclose(zstdout);
  return 0;
}

