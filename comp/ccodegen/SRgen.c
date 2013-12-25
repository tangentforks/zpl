/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <string.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/SRgen.h"
#include "../include/datatype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/red2mloops.h"
#include "../include/symmac.h"
#include "../include/treemac.h"


static void gen_op(FILE *outfile,binop_t op) {
  switch(op) {
  case MIN:
    fprintf(outfile,"MIN");
    break;
  case MAX:
    fprintf(outfile,"MAX");
    break;
  case PLUS:
    fprintf(outfile,"ADD");
    break;
  case TIMES:
    fprintf(outfile,"MULT");
    break;
  case AND:
    fprintf(outfile,"AND");
    break;
  case OR:
    fprintf(outfile,"OR");
    break;
  case BAND:
    fprintf(outfile,"BAND");
    break;
  case BOR:
    fprintf(outfile,"BOR");
    break;
  case BXOR:
    fprintf(outfile,"XOR");
    break;
  case USER:
    fprintf(outfile,"USER");
    break;
  default:
    INT_FATAL(NULL, "Unexpected type in gen_op(): %d",op);
  }
}


void gen_sr_pdt(FILE *outfile,datatype_t *pdt,int type) {
  datatype_t* basepdt;

  basepdt = ensure_good_scanred_type(pdt);
  while (D_CLASS(basepdt) == DT_ARRAY) {
    basepdt = D_ARR_TYPE(basepdt);
  }
  gen_pdt(outfile,ensure_good_scanred_type(basepdt),type);
}


void gen_sr_tot_array_size(FILE *outfile,datatype_t *resdt) {
  dimension_t *dimptr;
  int bracket=0;
  int dim;
  expr_t *lodim;
  expr_t *hidim;
  int numelems=1;

  while (D_CLASS(resdt) == DT_ARRAY) {
    dimptr = D_ARR_DIM(resdt);
    dim = 0;
    while (dimptr != NULL) {
      lodim = DIM_LO(dimptr);
      hidim = DIM_HI(dimptr);
      if (expr_computable_const(lodim) && expr_computable_const(hidim)) {
	numelems*=(expr_intval(hidim)-expr_intval(lodim)+1);
      } else {
	if (DIM_NEXT(dimptr) == NULL && D_CLASS(D_ARR_TYPE(resdt)) != DT_ARRAY) {
	  fprintf(outfile, "((");
	  gen_expr(outfile, hidim);
	  fprintf(outfile, ")-(");
	  gen_expr(outfile, lodim);
	  fprintf(outfile, ")+1)");
	  return;
	}
	INT_FATAL(NULL,"non-const multi-dimensional arrays used in Redgen.c");
      }

      dimptr = DIM_NEXT(dimptr);
      dim++;
    }
    resdt = D_ARR_TYPE(resdt);
    bracket++;
  }
  fprintf(outfile,"%d",numelems);
}


void gen_sr_nloop_index(FILE *outfile,int bracket,int dim) {
  fprintf(outfile,"_j%d_%d",bracket,dim);
}


void gen_sr_nloop_decls(FILE *outfile,datatype_t *resdt) {
  dimension_t *dimptr;
  int bracket=0;
  int dim;
  int first=1;

  if (D_CLASS(resdt) != DT_ARRAY) {
    return;  /* suppress printing of "int" and ";\n" */
  }
  fprintf(outfile,"int ");
  while (D_CLASS(resdt) == DT_ARRAY) {
    dimptr = D_ARR_DIM(resdt);
    dim = 0;
    while (dimptr != NULL) {
      if (first) {
	first=0;
      } else {
	fprintf(outfile,",");
      }
      gen_sr_nloop_index(outfile,bracket,dim);
      dimptr = DIM_NEXT(dimptr);
      dim++;
    }
    resdt = D_ARR_TYPE(resdt);
    bracket++;
  }
  fprintf(outfile,";\n");
}


void gen_sr_nloop_start(FILE *outfile,datatype_t *resdt) {
  dimension_t *dimptr;
  int bracket=0;
  int dim;

  bracket=0;
  while (D_CLASS(resdt) == DT_ARRAY) {
    dimptr = D_ARR_DIM(resdt);
    dim = 0;
    while (dimptr != NULL) {
      fprintf(outfile,"for (");
      gen_sr_nloop_index(outfile,bracket,dim);
      fprintf(outfile,"=");
      gen_expr(outfile, DIM_LO(dimptr));
      fprintf(outfile,";");
      gen_sr_nloop_index(outfile,bracket,dim);
      fprintf(outfile,"<=");
      gen_expr(outfile, DIM_HI(dimptr));
      fprintf(outfile,";");
      gen_sr_nloop_index(outfile,bracket,dim);
      fprintf(outfile,"++) {\n");

      dimptr = DIM_NEXT(dimptr);
      dim++;
    }
    resdt = D_ARR_TYPE(resdt);
    bracket++;
  }
}


void gen_sr_nloop_access_1d(FILE* outfile,dimension_t* dimptr,int bracket) {
  int dim=0;
  
  fprintf(outfile,"[");
  while (dimptr != NULL) {
    fprintf(outfile,"(");
    gen_sr_nloop_index(outfile,bracket,dim);
    fprintf(outfile,"-(");
    gen_expr(outfile, DIM_LO(dimptr));
    fprintf(outfile, "))");
    if (DIM_NEXT(dimptr)) {
      fprintf(outfile, "*(");
      gen_expr(outfile, build_array_size_multiplier(DIM_NEXT(dimptr)));
      fprintf(outfile, ")+");
    }
    dimptr = DIM_NEXT(dimptr);
    dim++;
  }
  fprintf(outfile,"]");
}


void gen_sr_nloop_access(FILE *outfile,datatype_t *resdt) {
  int bracket=0;

  while (D_CLASS(resdt) == DT_ARRAY) {
    gen_sr_nloop_access_1d(outfile,D_ARR_DIM(resdt),bracket);
    
    resdt = D_ARR_TYPE(resdt);
    bracket++;
  }
}


void gen_sr_nloop_stop(FILE *outfile,datatype_t *resdt) {
  dimension_t *dimptr;
  int bracket=0;
  int dim;

  while (D_CLASS(resdt) == DT_ARRAY) {
    dimptr = D_ARR_DIM(resdt);
    dim = 0;
    while (dimptr != NULL) {
      fprintf(outfile,"}\n");

      dimptr = DIM_NEXT(dimptr);
      dim++;
    }
    resdt = D_ARR_TYPE(resdt);
    bracket++;
  }
}


void gen_op_op(FILE *outfile,exprtype op) {
  fprintf(outfile,"_OP_");
  gen_op(outfile,op);
}


void gen_op_stmt(FILE *outfile,exprtype op,datatype_t *pdt) {
  fprintf(outfile,"_");
  gen_op(outfile,op);
  fprintf(outfile,"_STMT");
  if (datatype_complex(pdt)) {
    fprintf(outfile,"_");
    gen_sr_pdt(outfile,pdt,PDT_SRED);
  }
}


void gen_ident_elem(FILE *outfile,exprtype op,datatype_t *pdt) {
  fprintf(outfile,"_");
  gen_op(outfile,op);
  fprintf(outfile,"_");
  gen_sr_pdt(outfile,pdt,PDT_SRED);
  fprintf(outfile,"_IDENTITY");
}


void gen_flat_bloop_access(FILE *outfile) {
  fprintf(outfile,"[_i]");
}


void gen_sr_setup_complex_temp(FILE *outfile,expr_t *expr,datatype_t *resdt) {
  gen_setup_complex_temp(outfile,expr,resdt,2);
}


void gen_sr_noncomplex_expr(FILE *outfile,expr_t *expr,datatype_t *resdt) {
  if (expr_is_atom(expr) || !datatype_complex(T_TYPEINFO(expr))) {
    gen_noncomplex_expr(outfile,expr);
  } else {
    gen_complex_temp_name(outfile,resdt,2);
  }
}





