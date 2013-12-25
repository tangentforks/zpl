/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <limits.h>
#include "../include/global.h"
#include "../include/error.h"
#include "../include/struct.h"
#include "../include/passes.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/depgraph.h"
#include "../include/contraction.h"
#include "../include/stmtutil.h"
#include "../include/buildstmt.h"
#include "../include/buildsym.h"
#include "../include/symbol.h"
#include "../include/symmac.h"
#include "../include/buildsym.h"
#include "../include/buildsymutil.h"
#include "../include/cmloops.h"

/* A few comments here (echris@cs)...

   This pass must follow comm, so that comm and pre/post statements are
   properly ordered.  Perhaps I will eventually want to fix this (via
   changes in comm)?

   This pass must follow constrainmloops, because it uses DIRECTION fields.

 */

/* local prototypes */

static void mscan_modules(module_t *mod);
static void mscan_stmt(statement_t *s);
static statement_t *build_mscan_stmt(char *fn, expr_t *elist, int pro);


/* FN: mscan_modules - entry point for mscan pass
 */

static void mscan_modules(module_t *mod) {
    function_t *fn = T_FCNLS(mod);

    while (fn) {
	traverse_stmtls_g(T_STLS(fn), mscan_stmt, NULL, NULL, NULL);
	fn = T_NEXT(fn);
    }
}

int call_mscan(module_t *mod, char *s) {

  if (FALSE) {
    INT_FATAL(NULL, "This is necessary to fake out aggressive warnings!");    
  }

    traverse_modules(mod, TRUE, mscan_modules, NULL);

    return(0);
}


/* FN: mscan_stmt - function called on each stmt (during mscan pass)
 *     This function prepares mscan mloop statements by doing the following:
 *     (i) add decls to mloop
 *     (ii) mark mloop as tiled and set tile size
 *     (iii) insert pre/prepre/postpost statements
 *           . _MSCANINIT (pre)
 *           . _MSCAN_REGINIT_DIM_UP/DN (prepre for each dim)
 *           . _MSCAN_REGINIT (prepre)
 *           . _MSCAN_POST (postpost)
 */

static void mscan_stmt(statement_t *s) {
  int i, numdims;
  statement_t *first_stmt, *last_stmt, *tmp_stmt;
  mloop_t *mloop;
  expr_t *elist;
  int tile;
  char buf[16];

  /* only look at mscan statements */
  if ((T_TYPE(s) != S_MLOOP) || 
      !(mloop = T_MLOOP(s)) || 
      !mscan_mloop(mloop))
    return;
	
  numdims = T_MLOOP_RANK(mloop);

  /* tiling an mloop doesn't make since for 1-dimension, which is what
     this routine was doing before I inserted this short-circuit... -BLC */
  if (numdims == 1) {
    return;
  }
	
  /* add _dcid and _MSCANREG decls to mloop */
  T_ADD_MLOOP_VAR(mloop, pst_dcid, -1);
  T_ADD_MLOOP_VAR(mloop, pst_MSCANREG, -1);

  if (nomscanpeel) T_MLOOP_TILE_NOPEEL(mloop) = TRUE;
  else T_MLOOP_TILE_NOPEEL(mloop) = FALSE;

  /* mark mloop as tiled and set tile size in each dim */
  for (i=0; i<numdims; i++) {
    symboltable_t *pst;

    tile = 4;
    T_MLOOP_TILE(mloop,i) = INT_MAX;
    sprintf(buf, "msts%d", i);
    if ((pst = lu(buf)) != NULL) {
      S_INIT(pst_ms_tile_size[i]) = 
	build_init(build_0ary_op(VARIABLE,pst),NULL);
    }
    sprintf (buffer, "%d", tile); /* for build_int */
    T_MLOOP_TILE_EXPR(mloop,i) = build_0ary_op(CONSTANT,pst_ms_tile_size[i]);
    T_MLOOP_UNROLL(mloop,i) = 0;
    T_ADD_MLOOP_VAR(mloop, pst_ms_tile_size[i], -2);
    sprintf(buf, "msts%d", i);
    if ((pst = lu(buf)) != NULL) {
      S_INIT(pst_ms_tile_size[i]) = 
	build_init(build_0ary_op(VARIABLE,pst),NULL);
    }
  }

  /* build prepre statements to initialize _MSCANREG */
  last_stmt = NULL;
  for (i=0; i < numdims; i++ ) {
    /* build elist (actual args = "mloop_reg, i") */
    elist = copy_expr(T_MLOOP_REG(mloop));
    sprintf (buffer, "%d", i); /* for build_int */
    T_NEXT(elist) = build_0ary_op(CONSTANT,build_int(i));
    T_PREV(T_NEXT(elist)) = elist;
    
    /* note that constrainmloops must run before this pass to set up
       T_MLOOP_TILE_DIRECTION field */
    tmp_stmt = build_mscan_stmt((T_MLOOP_TILE_DIRECTION(mloop,i)>0)?
			      "_MSCAN_REGINIT_DIM_UP":
			      "_MSCAN_REGINIT_DIM_DN", 
			      elist,
			      TILED_OUTER_MLOOP_FLAG);

    if (last_stmt) {
      T_NEXT(last_stmt) = tmp_stmt;
      last_stmt = T_NEXT(last_stmt);
    } else {
      last_stmt = first_stmt = tmp_stmt; /* save ptr to 1st stmt */
    }
  }

  /* build elist (actual args = "_MSCANREG,numdims" ) */
  elist = build_0ary_op(VARIABLE, pst_MSCANREG);
  sprintf (buffer, "%d", numdims); /* for build_int */
  T_NEXT(elist) = build_0ary_op(CONSTANT,build_int(numdims));
  T_PREV(T_NEXT(elist)) = elist;

  /* build stmt (call to _MSCAN_REGINIT) */
  T_NEXT(last_stmt) = build_mscan_stmt("_MSCAN_REGINIT", elist,
				       TILED_OUTER_MLOOP_FLAG);
  last_stmt = T_NEXT(last_stmt);

  /* insert first_stmt list before current T_MLOOP_PREPRE statements */
  /* n.b.: it is important that first_stmt list go BEFORE comm */
  T_NEXT(last_stmt) = T_MLOOP_PREPRE(mloop,0);
  T_MLOOP_PREPRE(mloop,0) = first_stmt;

  /* add postpost statement; this stmt must go AFTER comm */
  T_ADD_MLOOP_POSTPOST(mloop, build_mscan_stmt("_MSCAN_POST", NULL,
					       TILED_OUTER_MLOOP_FLAG), 0);

  /* add a pre statement */
  T_ADD_MLOOP_PRE(mloop, 
		  build_mscan_stmt("_MSCAN_INIT", NULL,
				   UNROLLED_MLOOP_FLAG | 
				   UNRLD_FIXUP_MLOOP_FLAG |
				   TILED_OUTER_MLOOP_FLAG | 
				   TILED_INNER_MLOOP_FLAG | 
				   TILED_FIXUP_MLOOP_FLAG | 
				   UNRLD_TILED_INNER_MLOOP_FLAG), 
		  -1);
}



/* FN: build_mscan_stmt
 *     build statement "fn(elist)", where fn is string naming function
 *     and elist is actual list; also ors in prohibitions before 
 *     returning new statement.
 */

static statement_t *build_mscan_stmt(char *fn, expr_t *elist, int pro) {
  statement_t *s;

  s = build_expr_statement(build_Nary_op(FUNCTION, 
					 build_0ary_op(VARIABLE,check_var(fn)),
					 elist),
			   -1, "");

  T_PROHIBITIONS(s) |= pro;
    
  return (s);
}

