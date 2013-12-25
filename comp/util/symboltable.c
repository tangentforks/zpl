/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <string.h>
#include "../include/datatype.h"
#include "../include/dimension.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/rmstack.h"
#include "../include/struct.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"


int symtab_is_global(symboltable_t *pst) {
  return (S_LEVEL(pst) == 0);
}


int symtab_is_static(symboltable_t* pst) {
  return S_STYPE(pst) & SC_STATIC;
}


int symtab_is_param(symboltable_t* pst) {
  return (S_CLASS(pst) == S_PARAMETER);
}


int symtab_is_qreg(symboltable_t *pst) {
  int i;

  for (i=1;i<=MAXRANK;i++) {
    if (pst == pst_qreg[i]) {
      return i;
    }
  }
  if (pst == pst_qreg[0]) {
    return -1;
  }
  return 0;
}


int symtab_is_grid_scalar(symboltable_t* pst) {
  int i;

  for (i=1;i<=MAXRANK;i++) {
    if (pst == pstGRID_SCALAR[i]) {
      return i;
    }
  }
  return 0;
}


int symtab_is_qmask(symboltable_t *pst) {
  int i;
  
  for (i=1;i<=MAXRANK;i++) {
    if (pst == T_IDENT(pexpr_qmask[i])) {
      return i;
    }
  }
  return 0;
}


int symtab_is_indexi(symboltable_t *pst) {
  int i;

  for (i=1;i<=MAXRANK;i++) {
    if (pst == pstINDEX[i]) {
      return i;
    }
  }
  return 0;
}


int symtab_is_ia_index(symboltable_t* pst) {
  int i;
  symboltable_t* ipst;

  for (i=1;i<=MAXRANK;i++) {
    ipst = pstindex[i];
    while (ipst != NULL) {
      if (ipst == pst) {
	return i;
      }
      ipst = S_NEXT(ipst);
    }
  }
  return 0;
}


int symtab_is_fixed(symboltable_t *pst) {
  if (pst == lu_pst("open") ||
      pst == lu_pst("close") ||
      pst == lu_pst("ResetTimer") ||
      pst == lu_pst("CheckTimer") ||
      pst == lu_pst("UncResetTimer") ||
      pst == lu_pst("UncCheckTimer") ||
      pst == lu_pst("StartTimer") ||
      pst == lu_pst("StopTimer") ||
      pst == lu_pst("ReadTimer") ||
      pst == lu_pst("bind_write_func") ||
      pst == lu_pst("bind_read_func") ||
      pst == lu_pst("unbind_write_func") ||
      pst == lu_pst("unbind_read_func") ||
      pst == lu_pst("checkpoint")) {
    return 1;
  }
  else {
    return 0;
  }
}


expr_t *symtab_find_reg(symboltable_t *pst) {
  datatype_t *ensdt;

  ensdt = datatype_find_ensemble(S_DTYPE(pst));
  if (ensdt) {
    return D_ENS_REG(ensdt);
  } else {
    return NULL;
  }
}


int symtab_rank(symboltable_t *pst) {
  if (symtab_is_indexi(pst)) {
    return 0;
  } else {
    return datatype_rank(S_DTYPE(pst));
  }
}


int symtab_var_has_lvalue(symboltable_t *pst) {
  datatype_t *pdt;

  if (S_CLASS(pst) != S_VARIABLE || S_IS_CONSTANT(pst)) {
    return 0;
  }

  pdt = S_DTYPE(pst);

  if (pdt != NULL) {
    /*** some of the fake stuff that gets put in here doesn't have pdt ***/
    switch (D_CLASS(pdt)) {
    case DT_INTEGER:
    case DT_REAL:
    case DT_CHAR:
    case DT_ARRAY:
    case DT_ENSEMBLE:
    case DT_FILE:
    case DT_ENUM:
    case DT_STRING:
    case DT_STRUCTURE:
    case DT_SHORT:
    case DT_LONG:
    case DT_DOUBLE:
    case DT_QUAD:
    case DT_UNSIGNED_INT:
    case DT_UNSIGNED_SHORT:
    case DT_UNSIGNED_LONG:
    case DT_SIGNED_BYTE:
    case DT_UNSIGNED_BYTE:
    case DT_BOOLEAN:
    case DT_GENERIC:
    case DT_GENERIC_ENSEMBLE:
    case DT_REGION:
      return 1;

    case DT_VOID:
    case DT_SUBPROGRAM:
    case DT_PROCEDURE:
      return 0;
    default:
      return 0;
    }
  }

  return 0;

}


int symtab_dir_c_legal_init(symboltable_t* dir) {
  expr_t* val = IN_VAL(S_VAR_INIT(dir));
  int numdims = D_DIR_NUM(S_DTYPE(dir));
  int i;

  if (val == NULL) {
    return 0;
  } else {
    for (i=0; i<numdims; i++) {
      if (expr_c_legal_init(expr_direction_value(val, i)) == 0) {
	return 0;
      }
    }
    return 1;
  }
}


int symtab_is_sparse_reg(symboltable_t* reg) {
  if (S_CLASS(S_DTYPE(reg)) == DT_REGION &&
      S_INIT(reg) &&
      T_TYPE(IN_VAL(S_INIT(reg))) == BIWITH) {
    return 1;
  }
  return 0;
}


