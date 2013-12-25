/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "../include/error.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"
#include "../include/bb_traverse.h"
#include "../include/rmstack.h"

void set_last_next(statement_t *stmtls, statement_t *stmt)
{
  INT_COND_FATAL((stmtls!=NULL), NULL, "NULL stmtls in set_last_next()");

  while(T_NEXT(stmtls)) {
    stmtls = T_NEXT(stmtls);
  }

  T_NEXT(stmtls) = stmt;
}

/* The following never break basic blocks:
   S_MLOOP, S_EXPR, S_NULL, S_ZPROF

   The follwoing always break basic blocks:
   S_EXIT, S_END, S_HALT, S_CONTINUE, S_RETURN (no action required)
   S_NLOOP, S_REGION_SCOPE, S_COMPOUND, S_IF, S_LOOP

   By default, the following don't break basic blocks:
   S_IO, S_COMM, S_WRAP, S_REFLECT
   The last argument is a bit mask which tells the traversal routines
   which of the above break basic blocks (see bb_traverse.h for values)
 */
statement_t *bb_traversal_stmtls(statement_t *stmtls, 
				 statement_t *(*bb_fn)(statement_t *),
				 int trmask)
{
  statement_t *last, *head, *ret, *prev;
  int mymask = BBTR_NONE;

  if (stmtls == NULL) return (NULL);

  switch (T_TYPE(stmtls)) {
  case S_EXIT:        /*** these statements always break basic blocks ***/
  case S_END:
  case S_HALT:
  case S_CONTINUE:
  case S_RETURN:
    T_NEXT(stmtls) = bb_traversal_stmtls(T_NEXT(stmtls), bb_fn, trmask);
    return (stmtls);
    
  case S_NLOOP:
    T_NLOOP_BODY(T_NLOOP(stmtls)) = 
      bb_traversal_stmtls(T_NLOOP_BODY(T_NLOOP(stmtls)), bb_fn, trmask);
    T_NEXT(stmtls) = bb_traversal_stmtls(T_NEXT(stmtls), bb_fn, trmask);
    return (stmtls);

  case S_REGION_SCOPE:
    RMSPushScope(T_REGION(stmtls));
    T_BODY(T_REGION(stmtls)) = bb_traversal_stmtls(T_BODY(T_REGION(stmtls)),
						   bb_fn, trmask);
    RMSPopScope(T_REGION(stmtls));
    T_NEXT(stmtls) = bb_traversal_stmtls(T_NEXT(stmtls), bb_fn, trmask);
    return (stmtls);

  case S_MSCAN: /* BLC -- should this be treated the same? */
  case S_COMPOUND:
    T_CMPD_STLS(stmtls) = bb_traversal_stmtls(T_CMPD_STLS(stmtls),
					      bb_fn, trmask);
    T_NEXT(stmtls) = bb_traversal_stmtls(T_NEXT(stmtls), bb_fn, trmask);
    return (stmtls);

  case S_IF:
    T_THEN(T_IF(stmtls)) = bb_traversal_stmtls(T_THEN(T_IF(stmtls)),
					       bb_fn, trmask);
    T_ELSE(T_IF(stmtls)) = bb_traversal_stmtls(T_ELSE(T_IF(stmtls)),
					       bb_fn, trmask);
    T_NEXT(stmtls) = bb_traversal_stmtls(T_NEXT(stmtls), bb_fn, trmask);
    return (stmtls);

  case S_LOOP:
    T_BODY(T_LOOP(stmtls)) = bb_traversal_stmtls(T_BODY(T_LOOP(stmtls)), 
						 bb_fn, trmask);
    T_NEXT(stmtls) = bb_traversal_stmtls(T_NEXT(stmtls), bb_fn, trmask);
    return (stmtls);

  case S_IO:
    mymask |= BBTR_IO;
  case S_COMM:
    mymask |= BBTR_NN_COMM;
  case S_WRAP:
    mymask |= BBTR_WRAP;
  case S_REFLECT:
    mymask |= BBTR_REFLECT;

    if (trmask & mymask) {
      /*** break the basic block ***/
      T_NEXT(stmtls) = bb_traversal_stmtls(T_NEXT(stmtls), bb_fn, trmask);
      return(stmtls);
    }
    else {
      /*** do the default *** no break necessary ***/
    }
  default: /* otherwise statement type does not break basic block */

    last = NULL;
    head = stmtls;

    /* loop until bb is broken */
    while (stmtls && 
	   ((T_TYPE(stmtls) == S_NULL) ||    /*** these never break bbs ***/
	    (T_TYPE(stmtls) == S_EXPR) || 
	    (T_TYPE(stmtls) == S_MLOOP) ||
	    (T_TYPE(stmtls) == S_ZPROF) ||
                                             /*** these may break bbs ***/
	    ((T_TYPE(stmtls) == S_IO) && !(BBTR_IO & trmask)) ||
	    ((T_TYPE(stmtls) == S_COMM) && !(BBTR_NN_COMM & trmask)) ||
	    ((T_TYPE(stmtls) == S_WRAP) && !(BBTR_WRAP & trmask)) ||
	    ((T_TYPE(stmtls) == S_REFLECT) && !(BBTR_REFLECT & trmask)))) {
      last = stmtls;
      stmtls = T_NEXT(stmtls);
    }

    INT_COND_FATAL((last!=NULL), NULL,
		   "Disjunction problem in bb_traversal_stmtls()");

    prev = T_PREV(head);
    T_PREV(head) = NULL;
    T_NEXT(last) = NULL;
    ret = bb_fn(head);
    stmtls = bb_traversal_stmtls(stmtls, bb_fn, trmask);
    T_PREV(ret) = prev;
    set_last_next(ret, stmtls);

    return (ret);
  }
}

/* FUNCTION: bb_traversal - This function will call the caller supplied
 *           function bb_fn() on each "basic block" in mod.  The basic
 *           blocks are passed to bb_fn() as statement lists (statement_t *),
 *           and bb_fn() is expected to return the same type, which is
 *           reinserted into the AST.
 *
 * See comments for bb_traversal_stmtls() for more info
 *
 */

void bb_traversal(module_t *mod,
		  statement_t *(*bb_fn)(statement_t *),
		  int trmask)
{
  function_t *ftp;

  while (mod) {
    ftp = T_FCNLS(mod);
    INT_COND_FATAL((ftp!=NULL), NULL, "No functions in mod in bb_traversal()");
    while (ftp) {
      T_STLS(ftp) = bb_traversal_stmtls(T_STLS(ftp), bb_fn, trmask);
      ftp = T_NEXT(ftp);
    }
    mod = T_NEXT(mod);
  }
}

