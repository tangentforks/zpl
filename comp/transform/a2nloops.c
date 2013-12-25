/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: a2nloops.c - nloop insertion code (3rd edition)
 * DATE: 28 March 1997
 * CREATOR: E Christopher Lewis (echris)
 * DESCRIPTION: This code inserts nloops around statements that contain
 *              NULL array refs.
 *
 * HISTORY (Big Picture)
 *   28 March 1997 - A completely reworked version of this code; it now
 *                   fills in NULL array refs, so codegen doesn't have to
 *                   treat them in a special way.
 *   15 Dec 1995 - A reworked version of the first version.
 *   8 Sept 1994 - Original version; this code was hacky and it interacted
 *                 in awkward ways with codegen.
 *
 * ASSUMPTIONS
 *  - typechecking has already happened
 *  - certain factorization has already happened (i.e., scan, reduce and
 *      flood expressions only appear in statements of the form
 *      l-value := scan/reduce/flood expressions; this assumption is
 *      important in the BIASSIGN case in function do_expr())
 *
 * NLOOP INSERTION RULES
 * ....
 */

#include <stdio.h>
#include <string.h>
#include "../include/buildzplstmt.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/passes.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"


/* definitions */

#define POSTORDER 1


/* globals defined in global.h */

int ran_a2nloops=0;


/* globals for this file */

static statement_t *current_stmt;
static int in_wrapscancomm;


/* FN: compatible_nloop_location - return 0 if the two argument
 *                                 nloop depths are "incompatible".  They
 *                                 are compatible so long as one is
 *                                 not NLOOP_INSIDE and the other
 *                                 NLOOP_OUTSIDE.  If they are compatible,
 *                                 return the nloop location desired by them.
 * echris - 4-1-97
 * brad   - 7-27-99 modified to choose a location rather than a boolean
 */

static int compatible_nloop_location(int nl1, int nl2) {
  if (nl1 == NLOOP_ABSENT) {
    return nl2;
  } else if (nl2 == NLOOP_ABSENT) {
    return nl1;
  } else if (nl1 == nl2) {
    return nl1;
  } else {
    return 0;
  }
}


/* FN: create_new_index - allocates a new variable for nloop indexing
 *                        and puts it in an expression which is returned.
 *                        The symboltable structure is not actually
 *                        placed in the symboltable.  It just floats
 *                        around, so redundant variables may be 
 *                        created.
 * echris - 3-31-97
 */

static expr_t *create_new_index(int index,int depth) {
  expr_t *tmp_expr;

  tmp_expr = alloc_expr(VARIABLE);
  T_IDENT(tmp_expr) = get_blank_arrayref_index(index,depth);

  return tmp_expr;
}


/* FN: create_new_index_list - creates an expression list for use
 *                             as an array index expression list;
 *                             depth is the loop nest depth of the
 *                             nloop associated with this index expr.
 *                             Note that we may unnecessarily allocate
 *                             the same variable many times (there is
 *                             a memory efficiency issue).
 * echris - 3-31-97
 */

static expr_t *create_new_index_list(int depth, datatype_t *pdt)
{
  int rank, i;
  expr_t *ret_expr, *tmp_expr;;

  INT_COND_FATAL((D_CLASS(pdt)==DT_ARRAY), NULL,
		 "Non DT_ARRAY class (%d) in create_new_index_list", 
		 S_CLASS(pdt));

  rank = D_ARR_NUM(pdt);

  ret_expr = NULL;
  for (i = rank-1; i>=0; i--) {
    tmp_expr = create_new_index(depth+1, i+1);
    T_NEXT(tmp_expr) = ret_expr;
    ret_expr = tmp_expr;
  }
  
  return (ret_expr);
}


/* FN: is_special_expr - return true if expression is of type scan, reduce,
 *                       or flood; these expressions are "special" in that
 *                       null array refs inside the ensemble do not require
 *                       nloops
 * echris - 3-28-97
 */

static int is_special_expr(expr_t *expr)
{
  switch (T_TYPE(expr)) {
  case REDUCE:
  case SCAN:
  case FLOOD:
  case PERMUTE:
    return (TRUE);
  case FUNCTION:
    /* functions returning indexed arrays should not get nloops
       because return_ens will convert the return type into a final
       var parameter */
    if (D_CLASS(T_TYPEINFO(expr)) == DT_ARRAY) {
      return TRUE;
    } else {
      return FALSE;
    }
  default:
    return (FALSE);
  }
}



/* FN: insert_nloop - replace statement with nloop containing statement
 *                    as body; return the statement that was passed in,
 *                    NOT the nloop statement that was created.
 * echris - 12-15-95
 */

static statement_t *insert_nloop(statement_t *stmt, datatype_t *pdt, 
				 int depth, int mloop_proximity)
{
  statement_t *nloop;

  INT_COND_FATAL((stmt!=NULL), NULL, "null stmt in insert_nloop()");

/*  copy = copy_stmt(stmt); */
  nloop = build_nloop_statement(D_ARR_NUM(pdt), D_ARR_DIM(pdt), stmt, depth,
				mloop_proximity,
				T_LINENO(stmt), T_FILENAME(stmt));
  insertbefore_stmt(nloop, stmt);
  remove_stmt(stmt);
  T_NEXT(stmt) = NULL; T_NEXT_LEX(stmt) = NULL;
  T_PREV(stmt) = NULL; T_PREV_LEX(stmt) = NULL;
  T_PARENT(stmt) = nloop;
  T_PRE(nloop) = T_PRE(stmt);
  T_PRE(stmt) = NULL;
  T_POST(nloop) = T_POST(stmt);
  T_POST(stmt) = NULL;
  /* free_stmt(stmt); */

  /* need to update other pointers here? */

  return (stmt);
}


/* FN: compatible_array_dt - return TRUE if the argument datatypes
 *                           are "compatible" according to nloop
 *                           rules.  Typecheck should have already
 *                           checked this.
 * echris - 3-31-97
 */

static int compatible_array_dt(datatype_t *dt1, datatype_t *dt2)
{
  INT_COND_FATAL(dt1 != NULL, NULL, "NULL pointer in compatible_array_dt()");
  INT_COND_FATAL(dt2 != NULL, NULL, "NULL pointer in compatible_array_dt()");

  if ((D_CLASS(dt1) != DT_ARRAY) || (D_CLASS(dt2) != DT_ARRAY) ||
      (D_ARR_NUM(dt1) != D_ARR_NUM(dt2)) 
      /* || !compatible_dim_list(D_ARR_DIM(dt1),D_ARR_DIM(dt2)) */)
    return (FALSE);
  else
    return (TRUE);
}


/* This is very similar to do_expr below.  Except that due to the
   logic commented at the callsite, we know that the expression is
   going to be an atom, so only have to deal with a few cases.
   
   What we do here is walk over the formal datatype and the actual to
   see how they match up, and which dimensions need nloops based on
   the formal datatype.  To do this, we traverse the actual in the
   opposite order as in do_expr, since we have to walk the datatype in
   the reverse order, and datatypes are one-way data structures. */

static datatype_t *do_param(expr_t *actual, datatype_t* formaldt,int depth, 
			    int *nloop_location) {
  if (actual == NULL || T_PARENT(actual) == NULL) {
    return (NULL);
  }
  actual = T_PARENT(actual);

  if (T_TYPE(actual) == FUNCTION) {
    return NULL;
  }

  if (expr_at_ensemble_root(actual) && T_TYPE(actual) != ARRAY_REF) {
    datatype_t* tmp_pdt;

    if (D_CLASS(formaldt) == DT_ENSEMBLE) {
      tmp_pdt = do_param(actual, D_ENS_TYPE(formaldt), depth, 
			 nloop_location);
    } else {
      tmp_pdt = do_param(actual, formaldt, depth, nloop_location);
    }
    if (tmp_pdt) {
      *nloop_location = NLOOP_INSIDE;
    }
    return tmp_pdt;
  }

  switch (T_TYPE(actual)) {
  case ARRAY_REF:
    {
      expr_t *index_list, *array;
      datatype_t *pdt, *tmp_pdt;

      array      = nthoperand(actual,1);
      index_list = nthoperand(actual,2);
      if (D_CLASS(formaldt) != DT_ARRAY && index_list == NULL && 
	  !in_wrapscancomm) {
	/* BLC -- changed this due to typeinfo now calling x[] an array */
	pdt = T_TYPEINFO(actual);
	/* create a new index list */
	if (D_CLASS(pdt) == DT_ARRAY) {
	  index_list = create_new_index_list(depth, pdt);
	  T_NEXT(array) = index_list;
	  *nloop_location = NLOOP_OUTSIDE;
	  return (pdt);
	} else {
	  /* BLC -- flag bad indexing here */
	  USR_FATAL_CONT(T_STMT(actual),
			 "Indexing into a scalar or non-indexable array");
	  return NULL;
	}
      } else {
	if (D_CLASS(formaldt) == DT_ARRAY) {
	  tmp_pdt = do_param(actual, D_ARR_TYPE(formaldt),depth,nloop_location);
	} else {
	  tmp_pdt = do_param(actual, formaldt, depth, nloop_location);
	}
	return tmp_pdt;
      }
    }
    
  case BIDOT:
    return do_param(actual, formaldt, depth,nloop_location);

  case VARIABLE:
  case CONSTANT:
    return (NULL);

  default:
    INT_FATAL(current_stmt, "Unexpected type %d in do_param()", T_TYPE(actual));
  }

  INT_FATAL(NULL, "Should never reach this point!");
  return NULL; /* compiler doesn't know that this is unreachable */
}


/* FN: do_expr - This is the work horse for the nloop insertion code
 *               (though this function does not actually insert any
 *               nloops).  It recursively traverses the AST from expr 
 *               and labels the deepest NULL ARRAY_REF with index depth
 *               and returns the datatype associated with that ARRAY_REF.
 *               One of the following is returned in nloop_location:
 *                 NLOOP_INSIDE, NLOOP_OUTSIDE, NLOOP_ABSENT,
 *               indicating whether the nloop is inside an ensemble,
 *               outside an ensemble, or there was no ensemble.
 *               Notice that the codes for binary ops and function calls
 *               are a little bit tricky.
 * echris - 3-31-97
 */

static datatype_t *do_expr(expr_t *expr, int depth, int *nloop_location) 
{
  *nloop_location = NLOOP_ABSENT;

  if (expr == NULL) {
    return (NULL);
  }

  /* if we're the beginning of the ensemble subexpression, we need to
     set nloop_location; we don't do this for ARRAY_REFs because
     they're complex enough that we do it inside the rule itself */
  if (expr_at_ensemble_root(expr) && T_TYPE(expr) != ARRAY_REF) {
    datatype_t* tmp_pdt;
    
    tmp_pdt = do_expr(T_OPLS(expr), depth, nloop_location);
    if (tmp_pdt) {
      *nloop_location = NLOOP_OUTSIDE;
    } else {
      *nloop_location = NLOOP_INSIDE;
    }
    return tmp_pdt;
  }

  switch (T_TYPE(expr)) {
  case ARRAY_REF: {
    expr_t *index_list, *array;
    datatype_t *pdt, *tmp_pdt;
    datatype_t* retval = NULL;

    array      = nthoperand(expr,1);
    index_list = nthoperand(expr,2);
    tmp_pdt = do_expr(array, depth, nloop_location);
    if (tmp_pdt) {
      retval = tmp_pdt;
    } else if (index_list == NULL) {           /* is this a null ARRAY_REF? */
      /* BLC -- changed this due to typeinfo now calling x[] an array */
      pdt = T_TYPEINFO(expr);
      /* create a new index list */
      if ((*nloop_location == NLOOP_INSIDE) && in_wrapscancomm) {
	retval = NULL;
      } else {
	if (D_CLASS(pdt) != DT_ARRAY) {
	  /* BLC -- flag bad indexing here */
	  USR_FATAL_CONT(T_STMT(expr),
			 "Indexing into a scalar or non-indexable array");
	  retval = NULL;
	} else {
	  index_list = create_new_index_list(depth, pdt);
	  T_NEXT(array) = index_list;
	  retval = pdt;
	}
      }
    } else { /* not a null ARRAY_REF and no null refs deeper */
    }
    if (expr_at_ensemble_root(expr)) {
      if (retval) {
	*nloop_location = NLOOP_OUTSIDE;
      } else {
	*nloop_location = NLOOP_INSIDE;
      }
    }
    return retval;
  }
  
  case VARIABLE:
  case SIZE:
  case DISTRIBUTION:
  case GRID:
  case SBE:
  case CONSTANT: {
    return (NULL);
  }

  case UCOMPLEMENT:
  case UNEGATIVE: 
  case UPOSITIVE: {
  case BIAT:
    return (do_expr(T_OPLS(expr), depth, nloop_location));
  }
  
  case REDUCE:
    return NULL;

  case SCAN:
  case PERMUTE:
  case FLOOD: {
    datatype_t *tmp_pdt;

    in_wrapscancomm = TRUE;
    tmp_pdt = do_expr(T_OPLS(expr), depth, nloop_location);
    in_wrapscancomm = FALSE;
    return (tmp_pdt);
  }

  case FUNCTION: {
    expr_t *actuals;
    datatype_t *tmp_pdt, *tmp_pdt_old;
    int nl, nl_old;
    symboltable_t* formals;

    if (strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_CALL_PERM_DS") == 0 ||
	strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_CALL_PERM_DD") == 0 ||
	strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_CALL_PERM_IN") == 0) {
      return NULL;
    }

    INT_COND_FATAL((T_OPLS(expr)!=NULL), T_STMT(expr),
		   "NULL T_OPLS() in do_expr()");
    actuals = T_NEXT(T_OPLS(expr));
    formals = T_DECL(D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))));

    tmp_pdt_old = NULL;
    nl_old = FALSE;


    /* BLC -- we really want this to be "if the parameter in question is
       not promoting the function, which is different;  should work more
       on this */

    /* 
       For blank array references in function parameter lists, we often want
       to leave them as-is, since the whole array is being passed to the
       function.  This is handled using the following logic:

       if param is by-var:  must be a fairly simple expression;
                              => can match against formal using do_param
       if param is by-val:  it will have been pulled out into a temp by ensparam
                              => do nothing
                            or it's promoted by the blank array ref
                              => handle as normal

       -BLC  12/13/2000
    */

    /* skip past any non-parameters;  sadly, when parameters are added
       by the compiler, they come after variables */
    while (formals && !S_FG_PARAM(formals)) {
      formals = S_SIBLING(formals);
    }
    while (actuals && formals) {
      if (S_SUBCLASS(formals) == SC_INOUT) { /** IN AND OUT **/
	nl = NLOOP_ABSENT;
	tmp_pdt = do_param(expr_find_root_expr(actuals),S_DTYPE(formals),depth,
			   &nl);
      } else {
	tmp_pdt = do_expr(actuals, depth, &nl);
      }
      if (tmp_pdt_old) {
	if (tmp_pdt != NULL) {
	  if (!compatible_array_dt(tmp_pdt, tmp_pdt_old) ||
	      ((nl != NLOOP_ABSENT) && (nl_old != NLOOP_ABSENT) &&
	       (nl != nl_old))) {
	    USR_FATAL(current_stmt,
		      "Incompatible blank array references in function");
	  }
	}
	if (nl_old == NLOOP_ABSENT) { /* "upgrade" if old nloop not in ens */
	  nl_old = nl;
	  *nloop_location = nl;
	}
      } else {
	tmp_pdt_old = tmp_pdt;
	nl_old = nl;
	*nloop_location = nl;
      }

      /* advance to next parameter */
      actuals = T_NEXT(actuals);
      do {  /* skip past non params */
	formals = S_SIBLING(formals);
      } while (formals && !S_FG_PARAM(formals));
    }

    return (tmp_pdt_old);
  }

  case BIOP_GETS:
  case BIASSIGNMENT: {
    datatype_t *leftdt, *rightdt;
    int nl1, nl2;

    rightdt = do_expr(nthoperand(expr,2), depth, &nl2);
    if ((rightdt == NULL) && is_special_expr(nthoperand(expr,2))) {
      return (NULL);
    }
    leftdt = do_expr(nthoperand(expr,1), depth, &nl1);
    if (rightdt == NULL) {
      *nloop_location = nl1;
      return (leftdt);
    } else if ((leftdt != NULL) && (rightdt != NULL) &&
	       (compatible_array_dt(leftdt, rightdt)) &&
	       (compatible_nloop_location(nl1, nl2))) {
      *nloop_location = nl1;
      return (leftdt);
    } else {
      USR_FATAL(current_stmt, "Incompatible blank array references");
    }
    break;
  }

  case BILOG_AND: /* binary ops */
  case BILOG_OR:
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BIPREP:
  case BIDOT: {
    datatype_t *leftdt, *rightdt;
    int nl1, nl2, nlcomp;

    leftdt = do_expr(nthoperand(expr,1), depth, &nl1);
    rightdt = do_expr(nthoperand(expr,2), depth, &nl2);
    if ((leftdt == NULL) && (rightdt == NULL)) {
      return (NULL);
    } else if (leftdt == NULL) {
      *nloop_location = nl2;
      return (rightdt);
    } else if (rightdt == NULL) {
      *nloop_location = nl1;
      return (leftdt);
    } else if (compatible_array_dt(leftdt, rightdt)) {
      nlcomp = compatible_nloop_location(nl1,nl2);
      if (nlcomp) {
	*nloop_location = nlcomp;
	return (leftdt);
      } else {
	USR_FATAL(current_stmt, "Incompatible blank array references");
      }
    } else {
      USR_FATAL(current_stmt, "Incompatible blank array references");
    }
    break;
  }

    
  
  default: {
    INT_FATAL(current_stmt, "Unknown T_TYPE %d in do_expr()", T_TYPE(expr));
  }
  }

  INT_FATAL(NULL, "Should never reach this point!");
  return NULL; /* compiler doesn't know that this is unreachable */
}


/* FN: a2nloops_expr - discover if nloops are required for this
 *                     expression and insert nloops (using the global
 *                     current_stmt)
 * echris - 12-15-95
 */

static void a2nloops_expr(expr_t *expr)
{
  datatype_t *pdt;
  int depth, nloop_location;

  depth = 0;

  /* repeatedly look for NULL ARRAY_REFs until they have all been filled in; */
  /* note that there are more efficient ways to do this (w/o retraversing  */
  /* the same AST); let's not work about this efficiency detail now */
  do {
    pdt = do_expr(expr, depth, &nloop_location);
    if (pdt) {                                   /* NULL ARRAY_REF found? */ 
     /* insert nloops */
      INT_COND_FATAL((current_stmt!=NULL), T_STMT(expr),
		     "null current_stmt in a2nloops_expr()");
      /* BLC -- added this to get typeinfo updated for the new indexed expr */
      current_stmt = insert_nloop(current_stmt, pdt, depth, nloop_location);
      typeinfo_stmt(current_stmt);
      depth++;
    }
  } while (pdt);

}


/* FN: do_stmt_a2nloop - insert nloops in a statement; a2nloops_expr() does the
 *               actual insertion of nloop nodes; notice that nloops
 *               are NOT generated for NULL ARRAY_REFs in control flow.
 * echris - 12-15-95
 */

static void do_stmt_a2nloop(statement_t *stmt) {
  comm_info_t *c;
  expr_t *e;

  current_stmt = stmt;

  switch (T_TYPE(stmt)) {
  case S_EXPR:
    a2nloops_expr(T_EXPR(stmt));
    break;
  case S_COMM:
    in_wrapscancomm = TRUE;
    for (c = T_COMM_INFO(stmt); c!= NULL; c = T_COMM_INFO_NEXT(c))
      a2nloops_expr(T_COMM_INFO_ENS(c));
    in_wrapscancomm = FALSE;
    break;
  case S_WRAP:
  case S_REFLECT:
    in_wrapscancomm = TRUE;
    for (e = T_OPLS(T_WRAP(stmt)); e != NULL; e = T_NEXT(e)) {
      a2nloops_expr(e);
    }
    in_wrapscancomm = FALSE;
    break;
  case S_IO:
    INT_COND_FATAL((T_IO(stmt)!=NULL), stmt,
		   "NULL T_IO field in stmt in do_stmt_a2nloop()");
    a2nloops_expr(IO_EXPR1(T_IO(stmt)));
    break;
  default:
    break;
  }
}


/* FN: a2nloops_do_module - inserts nloops in a single module
 *                 traverses all the functions in mod
 * echris - 12-15-95
 */

static void a2nloops_do_module(module_t *mod)
{
  function_t *f;

  for (f = T_FCNLS(mod); f != NULL; f = T_NEXT(f)) {
    traverse_stmtls_g(T_STLS(f), do_stmt_a2nloop, NULL, NULL, NULL);
  }
}


/* FN: a2nloops - main procedure for nloop insertion code
 *                traverses all the modules in mod
 * echris - 12-15-95
 */


static int a2nloops(module_t *mod, char *s)
{
  in_wrapscancomm = FALSE;

  FATALCONTINIT();
  traverse_modules(mod, POSTORDER, a2nloops_do_module, NULL);  
  FATALCONTDONE();

  ran_a2nloops = 1;
  return (0);
}


/* function: call_a2nloops - nloop insertion code
 * date: 28 April 1997
 * creator: echris
 */

int call_a2nloops(module_t *mod, char *s) {
  return (a2nloops(mod, s));
}
