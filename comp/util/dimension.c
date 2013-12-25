/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <string.h>
#include "../include/dimension.h"
#include "../include/expr.h"
#include "../include/symmac.h"
#include "../include/treemac.h"



int dimension_const(dimension_t *dim) {
  if (dim == NULL) {
    return 0;
  }
  return (expr_computable_const(DIM_LO(dim)) && 
	  expr_computable_const(DIM_HI(dim)));
}


int dimension_rt_const(dimension_t *dim) {
  if (dim == NULL) {
    return 0;
  }
  if (dimension_floodable(dim) || dimension_rgrid(dim)) {
    return 1;
  }
  if (dimension_flat(dim)) {
    return expr_rt_const(DIM_LO(dim));
  }
  return (expr_rt_const(DIM_LO(dim)) && expr_rt_const(DIM_HI(dim)));
}


int dimension_flat(dimension_t *dim) {
  if (dim == NULL) {
    return 0;
  }
  if ((DIM_TYPE(dim) == DIM_FLAT) || (DIM_LO(dim) == DIM_HI(dim))) {
    return 1;
  }
  return 0;
}


int dimension_floodable(dimension_t *dim) {
  if (dim == NULL) {
    return 0;
  }
  if (DIM_TYPE(dim) == DIM_FLOOD) {
    return 1;
  }
  return 0;
}


int dimension_rgrid(dimension_t *dim) {
  if (dim == NULL) {
    return 0;
  }
  if (DIM_TYPE(dim) == DIM_GRID) {
    return 1;
  }
  return 0;
}

int dimension_dynamic(dimension_t* dim) {
  if ((dim == NULL) || (DIM_LO(dim) == NULL)) {
    return 0;
  }
  if (T_TYPE(DIM_LO(dim)) == CONSTANT &&
      !strcmp(S_IDENT(T_IDENT(DIM_LO(dim))),"?")) {
    return 1;
  } else {
    return 0;
  }
}


int dimension_inherit(dimension_t *dim) {
  if (dim == NULL) {
    return 0;
  }
  if (DIM_TYPE(dim) == DIM_INHERIT) {
    return 1;
  }
  return 0;
}


int dimlist_const(dimension_t *dim) {
  int retval = 1;

  while (dim != NULL) {
    retval = retval && dimension_const(dim);
    dim = DIM_NEXT(dim);
  }
  return retval;
}


int dimlist_rt_const(dimension_t *dim) {
  int retval = 1;

  if (dim == NULL) {
    retval = 0;     /* I believe this only happens for [... " ...] regions... */
  }
  while (dim != NULL) {
    retval = retval && dimension_rt_const(dim);
    dim = DIM_NEXT(dim);
  }
  return retval;
}


int dimlist_inherits(dimension_t* dim) {
  while (dim != NULL) {
    if (dimension_inherit(dim)) {
      return 1;
    }
    
    dim = DIM_NEXT(dim);
  }
  return 0;
}


int dimlist_is_qreg(dimension_t* dim) {
  int numdims=0;

  while (dim != NULL) {
    numdims++;
    if (!dimension_inherit(dim)) {
      return 0;
    }
    
    dim = DIM_NEXT(dim);
  }

  return numdims;
}
