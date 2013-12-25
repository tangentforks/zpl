/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/*
 * FILE: nudge.c - nudge optimization
 * DATE: 9 May 2000
 * CREATOR: deitz
 *
 * SUMMARY: A nudge expression structure is always the immediate
 * parent of a BIAT. Such a structure acts like a direction where the
 * magnitude in a given dimension is always equal to a multiple of the
 * bumper in that dimension. The nudge optimization seeks to alleviate
 * register pressure by eliminating the offset inherent in the
 * direction and replacing this by a bumper. For example, in the
 * standard Jacobi computation, the east and west offsets are equal to
 * the bumper in the east-west dimension and the north and west
 * offsets are equal to the bumper in the north-south dimension.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/createvar.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/passes.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"
#include "../include/buildsym.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/Repgen.h"
#include "../include/scale.h"
#include "../include/rmstack.h"
#include "../include/nudge.h"

int expr_nudged (expr_t*);
int set_nudge (expr_t*, int, int); /* sets a nudge of a given dimension */
int get_nudge (expr_t*, int);

/*
 * returns 1 if expr is nudged at all in any dimension
 */
int expr_nudged (expr_t* expr)
{
  int i;

  if (expr == NULL) {
    return 0;
  }

  if (T_TYPE(expr) != BIAT && T_TYPE(expr) != NUDGE) {
    return 0;
  }

  for (i = 0; i < expr_rank (expr); i++) {
    if (T_GET_NUDGE (expr, i) != 0) {
      return 1;
    }
  }
  return 0;
}

int set_nudge (expr_t* expr, int d, int n)
{
  expr_t* nudge;

  INT_COND_FATAL (expr != NULL, NULL, "setting nudge in null expr");

  if (T_TYPE (T_PARENT (expr)) == NUDGE) {
    expr = T_PARENT (expr);
  } else if (T_TYPE(expr) != NUDGE) {
    nudge = alloc_expr (NUDGE);
    T_OPLS (nudge) = expr;
    T_PARENT (nudge) = T_PARENT (expr);
    T_NEXT (nudge) = T_NEXT (expr);
    T_STMT (nudge) = T_STMT (expr);
    if (T_PARENT (expr) != NULL) {
      if (T_OPLS (T_PARENT (expr)) == expr) {
	T_OPLS (T_PARENT (expr)) = nudge;
      }
      else {
	T_NEXT (T_OPLS (T_PARENT (expr))) = nudge;
      }
    }
    T_PARENT (expr) = nudge;
    T_NEXT (expr) = NULL;
    expr = nudge;
  }

  INT_COND_FATAL (T_TYPE (expr) == NUDGE, NULL, "setting nudge of non-nudge");

  T_NUDGE (expr)[d] += n;

  return 0;
}

int get_nudge (expr_t* expr, int d)
{
  INT_COND_FATAL (expr != NULL, NULL, "inquiry of nudge in null expr");

  if (T_TYPE (expr) == BIAT) {
    return T_PARENT (expr) != NULL && T_TYPE (T_PARENT (expr)) == NUDGE
      ? get_nudge (T_PARENT (expr), d)
      : 0;
  }

  if (T_TYPE(expr) != NUDGE) {
    INT_FATAL(NULL, "bad inquiry of nudge");
  }

  return T_NUDGE(expr) [d];
}

