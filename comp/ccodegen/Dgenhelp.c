/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Privgen.h"
#include "../include/dimension.h"
#include "../include/dtype.h"
#include "../include/datatype.h"
#include "../include/error.h"
#include "../include/runtime.h"
#include "../include/struct.h"
#include "../include/symboltable.h"

static void gen_dual_ensemble_symlist(FILE *,symboltable_t *);
dimension_t *gen_dual_ensemble_pdt(FILE *,datatype_t *,int);

void gen_dual_ensemble_pst(FILE* outfile, symboltable_t *pst,int flag) {
  int level;
  dimension_t * pdTemp;

  if (pst == NULL) {
    return;
  }
  level = S_LEVEL(pst);
  while (--level > 0) {
    fprintf(outfile,"\t");
  }
  if (S_FG_2(pst)==1) {
    fprintf(outfile, "(");
  }
  switch (S_CLASS(pst)) {
  case S_COMPONENT:
    pdTemp = gen_dual_ensemble_pdt(outfile,S_COM_TYPE(pst),PDT_COMP);
    fprintf(outfile, "%s", S_IDENT(pst));
    gen_array_sizes(outfile,pdTemp,S_DTYPE(pst));
    if (S_COM_BITLHS(pst) != NULL) {
      fprintf(outfile, " : %d", S_COM_BIT(pst));
    }
    fprintf(outfile, ";\n");
    break;
  case S_VARIABLE:
     if (S_LEVEL(pst)==0) {  /* ensure that all user globals are private */
      priv_decl_begin(outfile);
    }
    gen_psc(outfile, S_STYPE(pst));
    if (S_LEVEL(pst)==0) {  /* ensure that all user globals are private */
      fprintf(outfile, ",");
    }
    if (datatype_find_ensemble(S_DTYPE(pst)) &&
	D_CLASS(S_DTYPE(pst))==DT_ENSEMBLE) {
      pdTemp = gen_dual_ensemble_pdt(outfile,S_DTYPE(pst), PDT_EGLB);
    } else {
      pdTemp = gen_dual_ensemble_pdt(outfile,S_DTYPE(pst), PDT_GLOB);
    }
    if (S_LEVEL(pst)==0) {  /* ensure that all user globals are private */
      fprintf(outfile, ",");
    }
    fprintf(outfile, "_E_%s", S_IDENT(pst));
    gen_array_sizes(outfile,pdTemp,S_DTYPE(pst));   
    if (S_LEVEL(pst)==0) {  /* ensure that all user globals are private */
      priv_decl_end(outfile);
    }
    else if (S_FG_TYPE(pst) == 0) {
      fprintf(outfile, ";\n");
    }
    break;
  case S_TYPE:
    fprintf(outfile,"typedef ");
    pdTemp = gen_dual_ensemble_pdt(outfile,S_DTYPE(pst),PDT_TYPE);
    fprintf(outfile,"_dual_%s",S_IDENT(pst));
    gen_array_sizes(outfile,pdTemp,S_DTYPE(pst));
    fprintf(outfile,";\n\n");
    break;
  default:
    fprintf(outfile,"/* <UNKNOWN SYMBOLCLASS IN gen_dual_ensemble_pst() */");
    break;
  }
  if (S_FG_2(pst) == 1) {
    fprintf(outfile, ")");
  }
}


static void gen_dual_ensemble_symlist(FILE *outfile,symboltable_t *pst) {
  symboltable_t *pstTemp;
  int level, i;
  
  if (pst == NULL) {
    return;
  }
  fprintf(outfile, "{\n");
  pstTemp = pst;
  level = S_LEVEL(pstTemp);
  while (pstTemp != NULL) {
    fprintf(outfile,"\t");
    if (datatype_find_ensemble(S_DTYPE(pstTemp))) {
      gen_dual_ensemble_pst(outfile, pstTemp,0);
    }
    pstTemp = S_SIBLING(pstTemp);
  }
  for (i=1;i<level;i++) {
    fprintf(outfile,"\t");
  }
  fprintf(outfile,"} ");
}


dimension_t *gen_dual_ensemble_pdt(FILE *outfile,datatype_t *pdt,int type) {
  dimension_t *retval = NULL;
  
  if (pdt == NULL) {
    return NULL;
  }

  switch(D_CLASS(pdt)) {
  case DT_INTEGER:
  case DT_REAL:
  case DT_SHORT:
  case DT_LONG:
  case DT_DOUBLE:
  case DT_QUAD:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_LONG:
  case DT_UNSIGNED_BYTE:
  case DT_SIGNED_BYTE:
  case DT_BOOLEAN:
  case DT_CHAR:
  case DT_ENUM:
  case DT_VOID:
  case DT_SUBPROGRAM:
  case DT_FILE:
  case DT_STRING:
  case DT_OPAQUE:
    break;
  case DT_ARRAY:
    if ((type&DTF_SHORT || type&DTF_RECRS) && D_NAME(pdt)!=NULL) {
      fprintf(outfile,"_dual_%s",S_IDENT(D_NAME(pdt)));
    } else {
      gen_dual_ensemble_pdt(outfile,D_ARR_TYPE(pdt),type|DTF_RECRS);
      retval = D_ARR_DIM(pdt);
    }
    break;
  case DT_STRUCTURE:
    if ((type&DTF_SHORT || type&DTF_RECRS) && D_NAME(pdt) != NULL) {
      fprintf(outfile,"_dual_%s",S_IDENT(D_NAME(pdt)));
    } else {
      fprintf(outfile,"struct ");
      gen_dual_ensemble_symlist(outfile,D_STRUCT(pdt));
    }
    break;
  case DT_ENSEMBLE:		/*** sungeun ***/
    if (type&DTF_SHORT && D_NAME(pdt) != NULL) {
      fprintf(outfile,"_dual_%s",S_IDENT(D_NAME(pdt)));
    } else {
      fprintf(outfile,"_arr_info");
    }
    break;
  case DT_REGION:
    if (type&DTF_SHORT && D_NAME(pdt) != NULL && S_IDENT(D_NAME(pdt)) != NULL) {
      fprintf(outfile,"_dual_%s",S_IDENT(D_NAME(pdt)));
    } else {
      fprintf(outfile,"_reg_info");
    }
    break;
  case DT_DISTRIBUTION:
    if (type&DTF_SHORT && D_NAME(pdt) != NULL && S_IDENT(D_NAME(pdt)) != NULL) {
      fprintf(outfile,"_dual_%s",S_IDENT(D_NAME(pdt)));
    } else {
      fprintf(outfile,"_distribution_info");
    }
    break;
  case DT_GRID:
    if (type&DTF_SHORT && D_NAME(pdt) != NULL && S_IDENT(D_NAME(pdt)) != NULL) {
      fprintf(outfile,"_dual_%s",S_IDENT(D_NAME(pdt)));
    } else {
      fprintf(outfile,"_grid_info");
    }
    break;
  default:
    INT_FATAL(NULL, "Unknown typeclass %d in gen_dual_ensemble_pdt()",
	       D_CLASS(pdt));
    break;
  }

  if (type&DTF_SPACE) {
    fprintf(outfile," ");
  }

  return retval;
}
