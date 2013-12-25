/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: cganal.c - simple call graph analysis
 *                  label each function (T_CGPOS and T_CGCYCLE fields)
 * DATE: 29 July 1998
 * CREATOR: echris
 */

#include <stdio.h>
#include <limits.h> 
#include <string.h>
#include "../include/callgraph.h"
#include "../include/db.h"
#include "../include/error.h"
#include "../include/function.h"
#include "../include/glist.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/symmac.h"
#include "../include/treemac.h"


/* FUNCTION: clear_visited - clear the visited bit for all function in mod
 * echris - 11-14-98
 */

static void clear_visited(module_t *mod)
{
  module_t *m;
  function_t *fn;

  for (m = mod; m != NULL; m = T_NEXT(m)) {
    fn = T_FCNLS(m);
    while (fn != NULL) {
      T_CLEAR_CG_VISITED(fn);
      fn = T_NEXT(fn);
    }
  }
}


/* FUNCTION: compute_finish_times_visit - this is the recursive component
 *           of compute_finish_times()
 * echris - 11-14-98
 */

static glist compute_finish_times_visit(function_t *fn)
{
  callgraph_t *cginfo;
  calledge_t *callee;
  function_t *f1;
  glist tmp, wlist;

  T_SET_CG_VISITED(fn);
  wlist = NULL;

  /* consider each out edge */
  cginfo = T_CGNODE(fn);
  callee = CG_EDGE(cginfo);
  while (callee) {
    f1 = S_FUN_BODY(CG_SINK(callee));
    if (!T_CG_VISITED(f1)) {
      tmp = compute_finish_times_visit(f1);
      wlist = glist_append_list(tmp, wlist);
    }
    callee = CG_NEXT(callee);
  }
  
  wlist = glist_prepend(wlist, (void *)fn, GLIST_NODE_SIZE);

  return (wlist);
}


/* FUNCTION: compute_finish_times - returns a list of functions in mod
 *           ordered by decrease finish time according to DFS
 *           (see DFS discussion in Cormen, p 478)
 * echris - 11-14-98
 */

static glist compute_finish_times(module_t *mod)
{
  module_t *m;
  function_t *fn;
  glist tmp, wlist;

  wlist = NULL;

  clear_visited(mod);

  for (m = mod; m != NULL; m = T_NEXT(m)) {
    fn = T_FCNLS(m);
    while (fn != NULL) {
      if (!T_CG_VISITED(fn)) {
	tmp = compute_finish_times_visit(fn);
	wlist = glist_append_list(tmp,wlist);
      }
      fn = T_NEXT(fn);
    }
  }

  return (wlist);
}


/* FUNCTION: label_cycle_visit - this is the recursive component of
 *           label_cycle
 * echris - 11-14-98
 */

static glist label_cycle_visit(function_t *fn)
{
  glist tmp, wlist;
  callgraph_t *cginfo;
  calledge_t *caller;
  function_t *f1;

  wlist = NULL;

  T_SET_CG_VISITED(fn);
  
  /* consider each out edge */
  cginfo = T_CGNODE(fn);
  caller = CG_IN_EDGE(cginfo);
  while (caller) {
    f1 = S_FUN_BODY(CG_SINK(caller));
    if (f1 == fn) {
      T_SET_CG_CYCLE(f1);
    }
    if (!T_CG_VISITED(f1)) {
      tmp = label_cycle_visit(f1);
      wlist = glist_append_list(tmp, wlist);
    }
    caller = CG_NEXT(caller);
  }
  
  wlist = glist_prepend(wlist, (void *)fn, GLIST_NODE_SIZE);

  return (wlist);
}


/* FUNCTION: label_cycle - this is a modified version of DFS;
 *           visits functions in the order they are repsented in wlist;
 *           finds strongly connected components (Cormen p 488);
 *           if scc is > 1 node, label all nodes in scc as in a cycle
 * echris - 11-14-98
 */

static void label_cycle(glist wlist)
{
  glist tmp, tmp2, tmp3;
  function_t *fn;
  
  tmp = wlist;
  while (tmp) {
    T_CLEAR_CG_VISITED((function_t *)GLIST_DATA(tmp));
    tmp = GLIST_NEXT(tmp);
  }

  tmp = wlist;
  while (tmp) {
    fn = (function_t *)GLIST_DATA(tmp);
    if (!T_CG_VISITED(fn)) {
      tmp2 = label_cycle_visit(fn);
      if (glist_length(tmp2) > 1) {
	tmp3 = tmp2;
	while (tmp3) {
	  T_SET_CG_CYCLE((function_t *)GLIST_DATA(tmp3));
	  tmp3 = GLIST_NEXT(tmp3);
	}
      }
      glist_destroy(tmp2, GLIST_NODE_SIZE);
    }
    tmp = GLIST_NEXT(tmp);
  }
}


/* FUNCTION: numoutedges - return the number of functions that fun fn calls
 *                         (excluding all but user supplied functions)
 * echris - 8-10-98
 */

static int numoutedges(function_t *fn) 
{
  callgraph_t *cginfo;
  calledge_t *called;
  int count;

  count = 0;

  /* consider each out edge */
  cginfo = T_CGNODE(fn);
  called = CG_EDGE(cginfo);
  while (called) {
    if (function_internal(S_FUN_BODY(CG_SINK(called)))) {
      count++;
    }
    called = CG_NEXT(called);
  }

  return (count);
}


/* FUNCTION: numinedges - return number of functions that call function fn
 * echris - 8-10-98
 */

static int numinedges(function_t *fn) 
{
  callgraph_t *cginfo;
  calledge_t *caller;
  int count;

  count = 0;

  /* consider each in edge */
  cginfo = T_CGNODE(fn);
  caller = CG_IN_EDGE(cginfo);
  while (caller) {
    count++;
    caller = CG_NEXT(caller);
  }

  return (count);
}


/* FUNCTION: label_leaf_root - label all leaf and root functions in mod;
 *           simply look at the number of in and out edges in the call graph
 * echris - 11-14-98
 */

static void label_leaf_root(module_t *mod)
{
  module_t *m;
  function_t *fn;

  for (m = mod; m != NULL; m = T_NEXT(m)) {
    fn = T_FCNLS(m);
    while (fn != NULL) {
      if (numoutedges(fn) == 0) {
	T_SET_CG_LEAF(fn);
      }
      if (numinedges(fn) == 0) {
	T_SET_CG_ROOT(fn);
      }
      fn = T_NEXT(fn);
    }
  }
}


/* FUNCTION: cganal - fills in the T_CGPROPS (properties) field of all 
 *                    functions in mod
 * T_CGPROPS(f) = some or-ing of
 *                  CG_ROOT, CG_LEAF, CG_CYCLE, CG_VISIT
 *                use the following macros to get at the bits
 *                  T_CG_ROOT(), T_CG_LEAF(), T_CG_CYCLE(), and T_CG_VISITED()
 * echris - 11-14-98
 */

static int cganal(module_t *mod, char *s)
{
  module_t *m;
  function_t *fn;
  glist ls;

  /* label nodes on cycles */
  ls = compute_finish_times(mod);
  label_cycle(ls);
  glist_destroy(ls, GLIST_NODE_SIZE);

  /* label nodes as leaves and roots */
  label_leaf_root(mod);

  /* clear the visited bit, just for kicks */
  clear_visited(mod);

  /* Summarize results */
  IFDB(20) {
    for (m = mod; m != NULL; m = T_NEXT(m)) {
      fn = T_FCNLS(m);
      if (fn == NULL) {
	INT_FATAL(NULL, "No functions in module in cganal()");
      }
      while (fn != NULL) {
	printf("Function: %s T_CGPROPS:%02x\n", S_IDENT(T_FCN(fn)), 
	       T_CGPROPS(fn));
	fn = T_NEXT(fn);
      }
    }
  }
  return (0);
}


int call_cganal(module_t *mod, char *s) {
  return (cganal(mod, s));
}
