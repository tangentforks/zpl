/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "../include/symmac.h"
#include "../include/init.h"
#include "../include/expr.h"

int init_c_legal_init(initial_t *init) {
  int retval;

  if (IN_BRA(init) == FALSE) {
    retval = expr_c_legal_init(IN_VAL(init));
  } else {
    init = IN_LIS(init);
    retval = 1;
    while (init != NULL) {
      retval &= init_c_legal_init(init);
      init = S_NEXT(init);
    }
  }
  return retval;
}

