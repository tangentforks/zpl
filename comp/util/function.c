/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "../include/function.h"
#include "../include/parsetree.h"
#include "../include/symmac.h"
#include "../include/treemac.h"

int function_internal(function_t *fn) {
  if (T_STLS(fn) == NULL  || S_STD_CONTEXT(T_FCN(fn))) {
    return 0;
  } else {
    return 1;
  }
}
