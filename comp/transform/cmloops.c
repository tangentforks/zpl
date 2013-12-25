/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/cmloops.h"
#include "../include/contraction.h"
#include "../include/error.h"
#include "../include/global.h"
#include "../include/rmstack.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"


/* FN: calc_tile_lsd - given mloop, calculate loop structure of tile loops
 *                     return result in res
 *                     res[i] = j => loop i iterates over dim |j| in
 *                                   direction of sign of j
 *                     note that i is 0-based, and j is 1-based
 * 11-24-99 echris
 */

static int calc_tile_lsd(mloop_t *mloop, distvect_t res)
{
  int numdims, i, j, k, hi, lo;
  int v[MAXRANK], m[MAXRANK];

 

  numdims = T_MLOOP_RANK(mloop);

  for (i=0; i<numdims; i++) {
    hilow(T_MLOOP_CONSTR(mloop), i, &hi, &lo);
    if (hi*lo < 0) {
      INT_FATAL(NULL, "Can't handle these deps in MSCAN yet.");
    }
    v[i] = ((hi<0) || (lo<0))?1:(((hi>0) || (lo>0))?-1:0);
    m[i] = FALSE;
  }

  k = 0;
  for (i=0; i<numdims; i++) {
    for (j=0; j<numdims; j++) {
      if (!m[j]) {
        if (v[j] == 0) {
          res[i] = j+1;
          m[j] = TRUE;
          k++;
          break;
        }
      }
    }
  }

  for (i=k; i<numdims; i++) {
    for (j=0; j<numdims; j++) {
      if (!m[j]) {
        res[i] = (j+1) * v[j];
        m[j] = TRUE;
        break;
      }
    }
  }

  return (TRUE);

}




/* move all degenerate dimensions to the outer part of the MLOOP as an
   optimization.  This makes the assumption that degenerate dimensions
   can always be moved around since they can carry no data dependences
   --BLC */

static void optimize_dimension_order(int lineno,int numdims,int order[],
				     int flat[]) {
  int i;
  int j;
  int numflatdims=0;
  int currentdim;

  if (numdims == 1 || mloop_opt == FALSE) {
    return;
  }

  /* I think this was stupid -BLC
  currentdim = order[0];
  if (RMSRegDimDegenerate(currentdim,0)) {
    flat[currentdim] = 1;
    printf("FOUND FLAT DIM: %d\n",currentdim);
  } */
  for (i=0;i<numdims;i++) {
    currentdim = order[i];
    if (RMSRegDimDegenerate(currentdim,0)) {
      flat[currentdim] = 1;
  IFDB(30) {
      printf("FOUND FLAT DIM: %d\n",currentdim);
  }

#if (!RELEASE_VER)
      IFDB(1) {
	expr_t* reg;
	reg = RMSCurrentRegion();
	DBS3(1,"Moving dimension %d down to dimension %d for region %s\n",i,
	     numflatdims,S_IDENT(T_IDENT(reg)));
      }
#endif

      for (j=i; j>numflatdims; j--) {
	order[j] = order[j-1];
      }
      order[numflatdims] = currentdim;  /* given that I've set tile to 1
					   above, is this still what I want,
					   or do I want tile order?  -BLC */
      numflatdims++;
    }
  }
}


/* FN: mscan_mloop - given mloop returns TRUE if it is mscan mloop
 * 11/24/99 echris
 */

int mscan_mloop(mloop_t *mloop)
{
  if ((mloop == NULL) ||
      !T_MLOOP_BODY(mloop) ||
      !T_PARENT(T_MLOOP_BODY(mloop))) {
    return (FALSE);
  } else {
    return (MLOOP_IS_MSCAN(T_PARENT(T_MLOOP_BODY(mloop))));
  }
}


static int mloop_tiled(mloop_t* mloop) {
  int numdims = T_MLOOP_RANK(mloop);
  int i;

  for (i=0; i<numdims; i++) {
    if (T_MLOOP_TILE(mloop,i) != 0) {
      return 1;
    }
  }
  return 0;
}


/* Called on every statement */

static void cmloop(statement_t* stmt) {
  mloop_t *mloop;
  int numdims;
  distvect_t res;
  int i;

  /* only consider mloop statements */
  if ((stmt == NULL) ||
      (T_TYPE(stmt) != S_MLOOP) ||
      !(mloop = T_MLOOP(stmt))) 
    return;

  numdims = T_MLOOP_RANK(mloop);

  if (T_MLOOP_CONSTR(mloop)) {

    /* fill in T_MLOOP_ORDER and T_MLOOP_DIRECTION fields based on */
    /* constraints in T_MLOOP_CONSTR() */

    /* note that lsd() destructively updates its first arg */
    if (!lsd(dvlist_copy(T_MLOOP_CONSTR(mloop)),res)) {
      INT_FATAL(NULL, "No legal lsd for mloop in constrain_mloop()");
    }
    for (i=0; i<numdims; i++) {
      T_MLOOP_DIRECTION(mloop, abs((res)[i])-1) = (((res)[i])>0)?1:-1;
      T_MLOOP_ORDER(mloop, i+1) = abs((res)[i])-1;
    }
    
    
    /* do tile structure if (i) mloop marked as tiled, or (ii) it is
       mscan mloop (which will eventually be tiled) */
    if (mloop_tiled(mloop) || 
	(mscan_mloop(mloop))) {
      
      /* fill in T_MLOOP_TILE_ORDER/DIRECTION fields based on */
      /* contraints in T_MLOOP_CONSTR() */
      if (!calc_tile_lsd(mloop, res)) {
	INT_FATAL(NULL, "No legal lsd for mloop in mscan_stmt()");
      } else {
	for (i=0; i<numdims; i++) {
	  T_MLOOP_TILE_ORDER(mloop, i+1) = abs((res)[i])-1;
	  T_MLOOP_TILE_DIRECTION(mloop,abs((res)[i])-1) = 
	    (((res)[i])>0)?1:-1;
	  /* overwrite T_MLOOP_DIRECTION() with tile directions */
	  T_MLOOP_DIRECTION(mloop,abs((res)[i])-1) = 
	    (((res)[i])>0)?1:-1;
	}
      }
    } else {
      for (i=0; i<numdims; i++) {
	T_MLOOP_TILE_ORDER(mloop, i+1) = T_MLOOP_ORDER(mloop, i+1);
      }
    }
    
  } /* else no constraints, so use the defaults */

  optimize_dimension_order(T_LINENO(stmt),numdims,T_MLOOP_ORDER_V(mloop),
			   T_MLOOP_FLAT_V(mloop));
}



/* Given a module, constrains all of the mloop statements for every function
   in the module */

static void constrainmloops(module_t *mod) {
  symboltable_t* pst;
  statement_t* stmt;

  DB0(20,"constrainmloops()\n");

  for (; mod != NULL; mod = T_NEXT(mod)) {
    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
      if (S_CLASS(pst) == S_SUBPROGRAM) {
	stmt = T_STLS(S_FUN_BODY(pst));
	if (stmt != NULL)
	  traverse_stmtls(T_STLS(T_CMPD(stmt)), FALSE, cmloop, NULL, NULL);
      }
    }
  }
}


/* Entry point */

int call_constrainmloops(module_t *mod, char *s) {
  RMSInit();
  constrainmloops(mod);
  RMSFinalize();
  return 0;
}
