/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* FILE: depgraph.h 
 * DATE: 13 September 1996
 * CREATOR: echris
 */

#ifndef __DEPGRAPH_H_
#define __DEPGRAPH_H_

#include "const.h"

#define OUTDEP_TYPE(x)      ((x)->type)
#define OUTDEP_HEAD(x)      ((x)->head_p)
#define OUTDEP_VAR(x)       ((x)->var_p)
#define OUTDEP_EXPR(x)      ((x)->expr_p)
#define OUTDEP_DIST(x)      ((x)->distance_p)
#define OUTDEP_STMT(x)      ((x)->stmt_p)
#define OUTDEP_BACK(x)      ((x)->back)
#define OUTDEP_NEXT(x)      ((x)->next_p)
#define OUTDEP_HEAD_STMT(x) (INDEP_STMT(OUTDEP_HEAD(x)))

#define INDEP_TAIL(x)       ((x)->tail_p)
#define INDEP_STMT(x)       ((x)->stmt_p)
#define INDEP_NEXT(x)       ((x)->next_p)
#define INDEP_TYPE(x)       (OUTDEP_TYPE(INDEP_TAIL(x)))
#define INDEP_VAR(x)        (OUTDEP_VAR(INDEP_TAIL(x)))
#define INDEP_EXPR(x)       (OUTDEP_EXPR(INDEP_TAIL(x)))
#define INDEP_DIST(x)       (OUTDEP_DIST(INDEP_TAIL(x)))
#define INDEP_TAIL_STMT(x)  (OUTDEP_STMT(INDEP_TAIL(x)))
#define INDEP_BACK(x)       (OUTDEP_BACK(INDEP_TAIL(x)))

typedef int distvect_t[MAXRANK+1];

typedef enum _mm {
  MUST,
  MAY
} maymust_t;

typedef	enum depedgetype_struct {
  XFLOW,
  XOUTPUT,
  XANTI,
  XINPUT,
  XANY
} depedgetype_t;

struct outdep_struct {
  depedgetype_t type;
  indep_t       *head_p;
  symboltable_t *var_p;
  expr_t	*expr_p;
  distvect_t    *distance_p;
  statement_t   *stmt_p;
  int		back;
  outdep_t      *next_p;
};

struct indep_struct {
  outdep_t     *tail_p;
  statement_t  *stmt_p;
  indep_t      *next_p;
};


/* prototypes */

int dependent_exprs(expr_t *e1, expr_t *e2, maymust_t may);

#endif
