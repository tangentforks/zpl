/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdlib.h>
#include "../include/Cgen.h"
#include "../include/buildzplstmt.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/runtime.h"
#include "../include/stmtutil.h"
#include "../include/symbol.h"
#include "../include/symboltable.h"
#include "../include/typeinfo.h"


void gen_dir_name_as_ident(FILE *outfile,expr_t *dir) {
  fprintf(outfile,"%s",S_IDENT(expr_find_root_pst(dir)));
}


static expr_t* dir_findval(expr_t* direxpr) {
  if (T_IDENT(direxpr) != NULL) {
    return IN_VAL(S_VAR_INIT(T_IDENT(direxpr)));
  } else {
    return NULL;
  }
}


void direxpr_gen_comp(FILE* outfile, expr_t* dir, int dim) {
  symboltable_t* dirpst;
  char* name;
  expr_t* value;

  switch (T_TYPE(dir)) {
  case CONSTANT:
    dirpst = T_IDENT(dir);
    name = S_IDENT(dirpst);
    value = IN_VAL(S_VAR_INIT(dirpst));
    if (name != NULL) {
      fprintf(outfile, "%s[%d]", name, dim);
    } else if (value != NULL) {
      gen_expr(outfile, expr_direction_value(value, dim));
    } else {
      INT_FATAL(NULL, "unexpected case in direxpr_gen_comp()");
    }
    break;
  default:
    typeinfo_expr(dir);
    gen_expr(outfile, dir);
    fprintf(outfile, "[%d]", dim);
  }
}


void dir_gen_comp(FILE* outfile, expr_t* dir, int dim) {
  expr_t* dirval = dir_findval(dir);

  if (dirval != NULL) {
    gen_noncomplex_expr(outfile, expr_direction_value(dirval,dim));
  } else {
    gen_expr(outfile,dir);
    fprintf(outfile, "[%d]", dim);
  }
}


void dir_gen_comp_offset(FILE* outfile, expr_t* dir, int dim) {
  datatype_t* dirpdt = T_TYPEINFO(dir);
  expr_t* dirval = dir_findval(dir);

  if (dirval != NULL) {
    switch (D_DIR_SIGN(dirpdt,dim)) {
    case SIGN_ZERO:
      break;
    case SIGN_POS:
    case SIGN_POSONE:
    case SIGN_UNKNOWN:
      fprintf(outfile,"+");
      /* fall through */
    case SIGN_NEG:
    case SIGN_NEGONE:
      dir_gen_comp(outfile, dir, dim);
    }
  } else {
    fprintf(outfile,"+");
    dir_gen_comp(outfile, dir, dim);
  }
}


long dir_comp_to_int(expr_t* dir, int dim, int* succeeded) {
  symboltable_t* dirpst;
  expr_t* dirval;

  *succeeded = 0;  /* be pessimistic by default */

  dirpst = T_IDENT(dir);
  if (dirpst != NULL) {
    dirval = IN_VAL(S_VAR_INIT(dirpst));
    if (dim < D_DIR_NUM(T_TYPEINFO(dir)) && dirval &&
	expr_computable_const(expr_direction_value(dirval, dim))) {
      *succeeded = 1;
      return expr_intval(expr_direction_value(dirval, dim));
    }
  }

  return 0;
}


signclass dir_get_sign(expr_t* dir, int dim) {
  datatype_t* dirdt;

  dirdt = T_TYPEINFO(dir);
  if (dirdt == NULL) {
    USR_WARN(T_STMT(dir), "typeinfo wasn't run on direction");
    typeinfo_expr(dir);
    dirdt = T_TYPEINFO(dir);
  }

  return D_DIR_SIGN(dirdt,dim);
}


