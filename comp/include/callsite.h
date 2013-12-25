/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _CALLSITE_H_
#define _CALLSITE_H_

#include "const.h"
#include "genlist.h"
#include "struct.h"


typedef struct _cover_1D_t {
  expr_t *reg;
  expr_t *mask;
  int with;
} cover_t;

#define COV_REG(x) ((x).reg)
#define COV_MSK(x) ((x).mask)
#define COV_WTH(x) ((x).with)

struct callsite_struct {
  cover_t cover;
  genlist_t *actuals;
  struct callsite_struct *next;
};

#define CALL_COVER(x)   ((x)->cover)
#define CALL_ACTUALS(x) ((x)->actuals)
#define CALL_NEXT(x)    ((x)->next)

callsite_t *new_callsite(cover_t,expr_t **,int);
int match_callsite(callsite_t *,cover_t,expr_t **,int);
void print_callsite(function_t *,callsite_t *);

#endif
