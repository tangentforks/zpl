/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include "../include/expr.h"
#include "../include/mloop.h"
#include "../include/treemac.h"

/* this is an oversimplified implementation that assumes all quote regions
   are dense.  My intention is to develop it more later, but to start a
   central place where this decision is made to prevent several copies from
   existing. */

int mloop_is_sparse(mloop_t* mloop) {
  expr_t* reg;

  reg = T_MLOOP_REG(mloop);
  if (!expr_is_dense_reg(reg)) {
    return 1;
  }
  return 0;
}
