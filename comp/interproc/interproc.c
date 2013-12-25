/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: interproc.c - interprocedural framework code
 * DATE: 29 July 1998
 * CREATOR: echris
 *
 * SUMMARY
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h> 
#include <string.h>
#include "../include/callgraph.h"
#include "../include/error.h"
#include "../include/depgraph.h"
#include "../include/struct.h"
#include "../include/const.h"
#include "../include/db.h"
#include "../include/parsetree.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/generic_stack.h"
#include "../include/macros.h"
#include "../include/stmtutil.h"
#include "../include/glist.h"
#include "../include/glistmac.h"
#include "../include/idfa_traverse.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/dbg_code_gen.h"
#include "../include/symboltable.h"
#include "../include/bb_traverse.h"
#include "../include/expr.h"
#include "../include/interproc.h"

typedef struct wl_node_struct {
  function_t *fn;
  glist funls_p;
  struct wl_node_struct *next_p;
} wl_node_t, wl_t;

/* globals */

static wl_t *wl;

/* macros */

#define WL_FN(x) ((x)->fn)
#define WL_FNLS(x) ((x)->funls_p)
#define WL_NEXT(x) ((x)->next_p)



void add_callees(function_t *fn) {
  callgraph_t *cginfo;
  calledge_t *callee;

  cginfo = T_CGNODE(fn);
  callee = CG_IN_EDGE(cginfo);
  while (callee != NULL) {
    ip_wl_append(T_PARFCN(T_STMT(CG_CALL(callee))),fn);
    callee = CG_NEXT(callee);
  }
}


void add_callers(function_t *fn) {
  callgraph_t *cginfo;
  calledge_t *caller;

  cginfo = T_CGNODE(fn);
  caller = CG_EDGE(cginfo);
  while (caller != NULL) {
    ip_wl_append(S_FUN_BODY(CG_SINK(caller)),fn);
    caller = CG_NEXT(caller);
  }
}


static void add_leaves(module_t *mod) {
  module_t *m;
  function_t *fn;
  
  for (m = mod; m != NULL; m = T_NEXT(m)) {
    fn = T_FCNLS(m);
    while (fn != NULL) {
      if (T_CG_CYCLE(fn) || T_CG_LEAF(fn)) {
	ip_wl_append(fn, NULL);
      }
      fn = T_NEXT(fn);
    }
  }
}


static void add_roots(module_t *mod) {
  module_t *m;
  function_t *fn;
  
  for (m = mod; m != NULL; m = T_NEXT(m)) {
    ip_wl_append(S_FUN_BODY(pstMAIN),NULL);
    fn = T_FCNLS(m);
    while (fn != NULL) {
      if (!T_REACHABLE(fn) &&
	  (T_CG_CYCLE(fn) || (T_CG_ROOT(fn)))) {
	ip_wl_append(fn, NULL);
      }
      fn = T_NEXT(fn);
    }
  }
}


/* FUNCTION: wl_head
 */

static wl_t *wl_head(wl_t *w)
{
  return (w);
}


/* FUNCTION: wl_pop
 */

static wl_t *wl_pop(wl_t *w)
{
  return (WL_NEXT(w));
}


/* FUNCTION: ip_analyze 
 */

int ip_analyze(module_t *mod, 
	       int dir, 
	       void (*usr_init_wl)(module_t *mod),
	       int (*ip_fn)(function_t *fn, glist funls_p),
	       void (*wl_fn)(function_t *fn))
{
  function_t *fn;
  glist funls;
  wl_node_t *wl_node;
  int ret;

  if ((dir != IPDN) && (dir != IPUP)) {
    INT_FATAL(NULL, "Bad direction in init_wl()");
  }

  /* initialize the work list */
  if (usr_init_wl) {
    usr_init_wl(mod);
  } else {
    if (dir == IPUP) {
      add_leaves(mod);
    } else {
      add_roots(mod);
    }
  }

  /* while worklist is not empty */
  while (wl) {
    wl_node = wl_head(wl);
    wl = wl_pop(wl);
    fn = WL_FN(wl_node);
    funls = WL_FNLS(wl_node);
    ret = ip_fn(fn, funls);
    if (ret) {
      if (wl_fn) {
	wl_fn(fn);
      } else {
	if (dir == IPUP) {
	  add_callees(fn);
	} else {
	  add_callers(fn);
	}
      }
    }
  }

  return (FALSE);
}


/* FUNCTION: ip_wl_append
 * RETURNS: false, if fn is already in list; true, otherwise
 */

int ip_wl_append(function_t *fn, function_t *force_fn)
{
  wl_t *wl_tmp, *wl_last_node;

  wl_tmp = wl;
  wl_last_node = NULL;

  /* look through worklist to see if it is here already */
  while (wl_tmp) {
    if (WL_FN(wl_tmp) == fn) {  /* already in worklist? */
      /* if force_fn not on wl_tmp's list already, add it */
      if (!glist_find(WL_FNLS(wl_tmp), force_fn)) {
	if (WL_FNLS(wl_tmp)) {
	  WL_FNLS(wl_tmp) = glist_append(WL_FNLS(wl_tmp), force_fn, 
					 GLIST_NODE_SIZE);
	} else {
	  WL_FNLS(wl_tmp) = glist_create(force_fn, GLIST_NODE_SIZE);
	}
      }
      return (0);
    }
    wl_last_node = wl_tmp;
    wl_tmp = WL_NEXT(wl_tmp);
  }

  /* not found in work list, add to end */
  wl_tmp = (wl_t *) malloc(sizeof(wl_t));
  if (wl_last_node) 
    WL_NEXT(wl_last_node) = wl_tmp;
  else
    wl = wl_tmp;
  WL_NEXT(wl_tmp) = NULL;
  WL_FN(wl_tmp) = fn;
  WL_FNLS(wl_tmp) = glist_create(force_fn, GLIST_NODE_SIZE);

  return (1);
}
