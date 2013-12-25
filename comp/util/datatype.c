/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "../include/datatype.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/symmac.h"


datatype_t* datatype_find_dtclass(datatype_t* pdt, typeclass class) {
  symboltable_t* pst;

  if (pdt == NULL) {
    return NULL;
  }
  if (D_CLASS(pdt) == class) {
    return pdt;
  }

  switch (D_CLASS(pdt)) {
  case DT_ENSEMBLE:
    return datatype_find_dtclass(D_ENS_TYPE(pdt), class);
  case DT_ARRAY:
    return datatype_find_dtclass(D_ARR_TYPE(pdt), class);
  case DT_STRUCTURE:
    for (pst = D_STRUCT(pdt); pst != NULL; pst = S_SIBLING(pst)) {
      datatype_t* retdt;
      retdt = datatype_find_dtclass(S_DTYPE(pst), class);
      if (retdt) {
	return retdt;
      }
    }
    return NULL;
  case DT_SUBPROGRAM:
    return datatype_find_dtclass(D_FUN_TYPE(pdt), class);
  default:
    return NULL;
  }
}

datatype_t* datatype_find_ensemble(datatype_t* pdt) {
  return datatype_find_dtclass(pdt, DT_ENSEMBLE);
}


datatype_t* datatype_find_region(datatype_t* pdt) {
  return datatype_find_dtclass(pdt, DT_REGION);
}


datatype_t* datatype_find_distribution(datatype_t* pdt) {
  return datatype_find_dtclass(pdt, DT_DISTRIBUTION);
}


datatype_t* datatype_find_grid(datatype_t* pdt) {
  return datatype_find_dtclass(pdt, DT_GRID);
}


datatype_t* datatype_find_direction(datatype_t* pdt) {
  return datatype_find_dtclass(pdt, DT_DIRECTION);
}


datatype_t* datatype_base(datatype_t* pdt) {
  switch (D_CLASS(pdt)) {
  case DT_ENSEMBLE:
    return datatype_base(D_ENS_TYPE(pdt));
  case DT_ARRAY:
    return datatype_base(D_ARR_TYPE(pdt));
  default:
    return pdt;
  }
}


int datatype_rank(datatype_t* pdt) {
  pdt = datatype_find_ensemble(pdt);
  if (pdt) {
    return D_ENS_NUM(pdt);
  }
  return 0;
}


int datatype_scalar(datatype_t* pdt) {
  if (pdt == NULL) {
    return 0;
  }
  switch (D_CLASS(pdt)) {
  case DT_ARRAY:
  case DT_FILE:
  case DT_STRUCTURE:
  case DT_SUBPROGRAM:
  case DT_GRID:
  case DT_DISTRIBUTION:
  case DT_REGION:
  case DT_ENSEMBLE:
  case DT_DIRECTION:
  case DT_GENERIC:
  case DT_GENERIC_ENSEMBLE:
  case DT_PROCEDURE:
  case DT_OPAQUE:
    return 0;
  default:
    return 1;
  }
}


int datatype_complex(datatype_t* pdt) {
  if (pdt == NULL) {
    return 0;
  }
  switch (D_CLASS(pdt)) {
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
    return 1;
  default:
    return 0;
  }
}
 

int datatype_float(datatype_t* pdt) {
  if (pdt == NULL) {
    return 0;
  }
  switch (D_CLASS(pdt)) {
  case DT_REAL:
  case DT_DOUBLE:
  case DT_QUAD:
    return 1;
  default:
    return 0;
  }
}


int datatype_int(datatype_t* pdt) {
  if (pdt == NULL) {
    return 0;
  }
  switch (D_CLASS(pdt)) {
  case DT_INTEGER:
  case DT_SHORT:
  case DT_LONG:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_LONG:
  case DT_SIGNED_BYTE:
  case DT_UNSIGNED_BYTE:
    return 1;
  default:
    return 0;
  }
}


int datatype_dyn_array(datatype_t* pdt) {
  if (S_CLASS(pdt) != DT_ARRAY) {
    return 0;
  }
  if (dimlist_const(D_ARR_DIM(pdt))) {
    return 0;
  }
  return 1;
}


static int datatype_reg_find_any_dimclass(datatype_t* pdt, dimtypeclass dc) {
  int i;

  if (D_CLASS(pdt) != DT_REGION) {
    INT_FATAL(NULL, "bad call to datatype_reg_find_any_dimclass");
  }
  for (i=0; i<D_REG_NUM(pdt); i++) {
    if (D_REG_DIM_TYPE(pdt, i) == dc) {
      return 1;
    }
  }
  return 0;
}

static int datatype_reg_find_all_dimclass(datatype_t* pdt, dimtypeclass dc) {
  int i;

  if (D_CLASS(pdt) != DT_REGION) {
    INT_FATAL(NULL, "bad call to datatype_reg_find_all_dimclass");
  }
  for (i=0; i<D_REG_NUM(pdt); i++) {
    if (D_REG_DIM_TYPE(pdt, i) != dc) {
      return 0;
    }
  }
  return 1;
}


 int datatype_is_dense_reg(datatype_t* pdt) {
   return !(datatype_reg_find_any_dimclass(pdt, DIM_SPARSE));
 }


int datatype_reg_inherits(datatype_t* pdt) {
  return datatype_reg_find_any_dimclass(pdt, DIM_INHERIT);
}


int datatype_reg_inherits_all(datatype_t* pdt) {
  return datatype_reg_find_all_dimclass(pdt, DIM_INHERIT);
}


int datatype_reg_dynamicdim(datatype_t* pdt) {
  return datatype_reg_find_any_dimclass(pdt, DIM_DYNAMIC);
}

