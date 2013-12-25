/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "../include/allocstmt.h"
#include "../include/callgraph.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/traverse.h"
#include "../include/treemac.h"


static int depth = 0;
static callgraph_t *global_cg;


static void mark_reachability(symboltable_t *pst) {
  function_t *pfunc;
  callgraph_t *cgnode;
  calledge_t *edge;

  if (pst==NULL) {
    return;
  }

  pfunc = S_FUN_BODY(pst);
  if (T_REACHABLE(pfunc)) {
    return;
  } else {
    T_REACHABLE(pfunc) = TRUE;
    cgnode = CG_NODE(pst);
    if (cgnode) {
      edge = CG_EDGE(cgnode);
      while (edge) {
	mark_reachability(CG_SINK(edge));

	edge = T_NEXT(edge);
      }
    }
  }
}



static void indent(void) {
  int	i;
  
  for (i = 0; i < depth; i++) {
    putc('\t',zstdout);
  }
}


static void print_callgraph(symboltable_t *f) {
  callgraph_t	*c;
  calledge_t	*tmp;

  DB0(20,"print_callgraph()\n");

  if (f == NULL) {
    INT_FATAL(NULL, "f == NULL in print_callgraph");
    return;
  }

  DB1(20,"print_callgraph(%s)\n",S_IDENT(f));

  c = CG_NODE(f);

  if (c == NULL) {
    IFDB(1) {
      indent();
      DBS1(1, "%s :\n",S_IDENT(f));
      indent();
      DBS1(1, ": %s\n",S_IDENT(f));
    }
  } else if (CG_FLAG(c) == 0) {
    IFDB(1) {
      CG_FLAG(c) = 1;
      indent();
      DBS1(1, "%s :\n",S_IDENT(f));
      depth++;
      tmp = CG_EDGE(c);
      for ( ; tmp != NULL; tmp = T_NEXT(tmp)) {
	indent();
	DBS0(1, "\n");
	print_callgraph(CG_SINK(tmp));
	if (CG_SINK(tmp) == NULL) {
	  dbg_gen_expr(stderr,CG_CALL(tmp));
	  DBS0(1, "\n");
	}
      }
      depth--;
      indent();
      DBS1(1, ": %s\n",S_IDENT(CG_FCN(c)));
      CG_FLAG(c) = 0;
    }
  }
}


static void print_callgraph_fcn(function_t *f) {
  DB0(10,"print_callgraph_fcn()\n");

  if ((f == NULL) || (T_FCN(f) == NULL)) {
    return;
  }

  print_callgraph(T_FCN(f));
}


static callgraph_t *alloc_callgraph(symboltable_t *fp) {
  callgraph_t *new;
  
  new = (callgraph_t *)PMALLOC(sizeof(*new));
  CG_FLAG(new) = 0;
  CG_FCN(new) = fp;
  CG_EDGE(new) = NULL;
  CG_IN_EDGE(new) = NULL;
  return new;
}


static calledge_t *alloc_calledge(expr_t *e,symboltable_t *cgp) {
  calledge_t *new;

  new = (calledge_t *)PMALLOC(sizeof(*new));
  CG_CALL(new) = e;
  CG_SINK(new) = cgp;
  T_NEXT(new) = NULL;

  return new;
}


static void add_call(expr_t *e,symboltable_t *f,callgraph_t *cg) {
  callgraph_t     *sink;
  calledge_t      *in;
  calledge_t	*g;
  genlist_t       *glp;
  calledge_t	*prev;
  calledge_t	*next;

  DB0(40,"add_call(,,)\n");

  if ((f == NULL) || (cg == NULL)) {
    INT_FATALX(S_LINENO(f), S_FILENAME(f), 
	       "null pointer in add_call() in callgraph.c");
  }

  g = alloc_calledge(e,f);
  in = alloc_calledge(e,CG_FCN(cg));

  glp = alloc_gen();
  G_STATEMENT(glp) = T_STMT(e);

  if (S_DTYPE(f) == NULL) 	/*** sungeun ***  assume external ***/
    return;			/*** for now, ignore external calls ***/

  G_NEXT(glp) = T_INVOCATIONS(S_FUN_BODY(f));
  T_INVOCATIONS(S_FUN_BODY(f)) = glp;

  if (T_CGNODE(S_FUN_BODY(f)) == NULL) {
    T_CGNODE(S_FUN_BODY(f)) = alloc_callgraph(f);
  }
  sink = T_CGNODE(S_FUN_BODY(f));

  if (CG_IN_EDGE(sink) == NULL) {
    CG_IN_EDGE(sink) = in;
  } else {
    prev = CG_IN_EDGE(sink);
    next = T_NEXT(prev);
    while (next != NULL) {
      prev = next;
      next = T_NEXT(next);
    }
    T_NEXT(prev) = in;
  }
   	
  if (CG_EDGE(cg) == NULL) {
    CG_EDGE(cg) = g;
  } else {
    prev = CG_EDGE(cg);
    next = T_NEXT(prev);
    while (next != NULL) {
      prev = next;
      next = T_NEXT(next);
    }
    T_NEXT(prev) = g;
  }

  CG_EDGE_SAME(g) = in;  /* MDD: pointer to respective in edge  */
  CG_EDGE_SAME(in) = g;  /* MDD: pointer to respective out edge */
}


static void visit_expr(expr_t *e) {
  DB0(30,"visit_expr()\n");

  assert(e);
  if (T_TYPE(e) ==  FUNCTION) {
    INT_COND_FATAL((T_OPLS(e)!=NULL), NULL,
		   "No function name in function call");
    
    DB1(30,"adding function %s to call graph\n",S_IDENT(T_IDENT(T_OPLS(e))));
    add_call(e,T_IDENT(T_OPLS(e)),global_cg);
  }
}


static void tmpexpr(expr_t *e) {
  DB0(30,"tmpexpr()\n");

  traverse_exprls(e,0,visit_expr);
}


static void build_stmt_calls(callgraph_t *cg,statement_t *ls) {
  DB0(30,"build_stmt_calls(,)\n");

  global_cg = cg;
  
  traverse_stmtls(ls,0,NULL,tmpexpr,NULL);
}


static void build_fcn_calls(symboltable_t *f) {
  DB0(20,"{build_fcn_calls()\n");

  if (f == NULL) {
    INT_FATAL(NULL, "f == NULL in build_fcn_calls() in callgraph.c");
    DB0(20,"}build_fcn_calls() : returning\n");
    return;
  }

  DB1(50,"S_IDENT(f) == %s\n",S_IDENT(f));
  
  if (((S_FUN_BODY(f) != NULL) && (CG_NODE(f) == NULL)) ||
      (CG_FLAG(CG_NODE(f)) == 0)) {
    callgraph_t	*cg;
    
    DB1(10,"building callgraph for %s\n",S_IDENT(f));
    
    if (CG_NODE(f) != NULL) {
      cg = CG_NODE(f);
    } else {
      cg = alloc_callgraph(f);
      T_CGNODE(S_FUN_BODY(f)) = cg;
    }
    CG_FLAG(cg) = 1;
    build_stmt_calls(cg,T_STLS(S_FUN_BODY(f)));
  } else {

    DBS1(1, "not building callgraph for %s\n",S_IDENT(f));

  }

  DB0(20,"}build_fcn_calls() : returning\n");
}


static void build_callgraph(function_t *f) {
  DB0(10,"build_callgraph()\n");

  build_fcn_calls(T_FCN(f));

  DB0(10,"CALLGRAPH DONE\n");
}


static void clean_callgraph(function_t *f) { /* MDD */
  T_CGNODE(f) = NULL;
}


static void clear_cg_flag(function_t *f) {
  if ((f != NULL) && (T_FCN(f) != NULL) && (CG_NODE(T_FCN(f)) != NULL)) {
    CG_FLAG(CG_NODE(T_FCN(f))) = 0;
  }
}


static void mark_special(function_t *f) {
  genlist_t *glp;

  switch (T_SEMANTICS(f)) {
    case SEM_LOCK:
    case SEM_UNLOCK:
    case SEM_BARRIER:
    case SEM_FLOW_SINK:
      for (glp = T_INVOCATIONS(f); glp != NULL; glp = G_NEXT(glp)) 
	T_SEMANTICS(G_STATEMENT(glp)) = T_SEMANTICS(f);
      break;
    default:
      break;
  }
}


int call_callgraph(module_t *mod, char *s) {
  DB1(10,"{call_callgraph(,%s)\n", s);
  
  if (CG_NODE(pstMAIN) != NULL) { /* MDD */
    traverse_modules(mod,FALSE,NULL, clean_callgraph);
  }

  traverse_modules(mod,FALSE,NULL,build_callgraph);

  traverse_modules(mod,FALSE,NULL,clear_cg_flag);

  if (pstMAIN == NULL) {
    traverse_modules(mod,FALSE,NULL,print_callgraph_fcn);
  } else {
    print_callgraph(pstMAIN);
  }

  traverse_modules(mod,FALSE,NULL,mark_special);

  mark_reachability(pstMAIN);

  DB1(10,"}returning from call_callgraph(,%s)\n", s);
  return 0;
}

