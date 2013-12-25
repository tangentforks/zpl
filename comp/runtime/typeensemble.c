/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <string.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/db.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/glist.h"
#include "../include/glistmac.h"
#include "../include/global.h"
#include "../include/runtime.h"
#include "../include/symboltable.h"
#include "../include/datatype.h"

static void GenUpDownFluff(FILE *outfile,char* arrname,int strided,
			   int fluffval,int mfluff,int wrapfluff,
			   int dim, int lohi) {
  char regname[1024];
  
  sprintf(regname, "_ARR_DECL_REG(%s)", arrname);
  gen_fluffval(outfile,fluffval,mfluff,dim, regname);
}

static void DetermineRegionForEnsemble(symboltable_t* pst,
				       datatype_t*pdt,
				       expr_t** reg) {
  *reg = symtab_find_reg(pst);
  if (*reg == NULL) {
    *reg = buildqregexpr(D_ENS_NUM(pdt));
  }
  if (datatype_reg_dynamicdim(T_TYPEINFO(*reg))) {
    *reg = buildqregexpr(D_REG_NUM(T_TYPEINFO(*reg)));
  }
}


void GenAllocEnsemble(FILE* outfile, symboltable_t* pst, char* arrayname, 
		      datatype_t* pdt, int isrealloc, int hoistable) {
  expr_t* reg;
  int numdims;
  int strided;
  int wrap;
  int arrtype;
  char* accname;
  int i;

  DetermineRegionForEnsemble(pst, pdt, &reg);
  numdims = D_REG_NUM(T_TYPEINFO(reg));

  strided = expr_is_strided_reg(reg, 1);
  wrap = 0;
  for (i = 0; i < numdims; i++) {
    wrap = wrap || S_WRAPFLUFF_LO(pst,i) || S_WRAPFLUFF_HI(pst,i);
  }
  arrtype = ARR_NORMAL;
  if (expr_is_strided_reg(reg,1)) {
    arrtype = ARR_STRIDED;
  }
  if (!expr_is_dense_reg(reg)) {
    arrtype = ARR_SPARSE;
  }
  accname = gen_accessors(pst, reg, numdims, arrtype, pdt);

  /* fluff stuff */
  fprintf(outfile, "_ARR_DECL_REG(%s) = ", arrayname);
  priv_reg_access(outfile,reg);
  fprintf(outfile, ";\n");
  for (i=0; i < numdims; i++) {
    fgenf(outfile, "_ARR_FLUFF_DOWN(%s, %d) = ", arrayname, i);
    GenUpDownFluff(outfile, arrayname, strided, S_FLUFF_LO(pst, i),
		   S_UNK_FLUFF_LO(pst,i), S_WRAPFLUFF_LO(pst,i), i, LO);
    fgenf(outfile, ";\n");
  }
  for (i=0; i < numdims; i++) {
    fgenf(outfile, "_ARR_FLUFF_UP(%s, %d) = ", arrayname, i);
    GenUpDownFluff(outfile, arrayname, strided, S_FLUFF_HI(pst, i),
		   S_UNK_FLUFF_HI(pst,i), S_WRAPFLUFF_HI(pst,i), i, HI);
    fgenf(outfile, ";\n");
  }
  fgenf(outfile, "_ARR_FLUFF_WRAP(%s) = %d;\n", arrayname, wrap);

  fgenf(outfile, "_SetupEnsemble(");
  fgenf(outfile, "%s, ", arrayname);
  priv_reg_access(outfile, reg);
  fprintf(outfile, ", ");
  if (D_CLASS(D_ENS_TYPE(pdt)) == DT_ARRAY) {
    dimension_t* pdTemp = D_ARR_DIM(D_ENS_TYPE(pdt));
    if (!expr_computable_const(DIM_LO(pdTemp)) ||
	!expr_computable_const(DIM_HI(pdTemp))) {
      fgenf(outfile, "sizeof(%D) * ((%E)-(%E)+1), ", D_ARR_TYPE(D_ENS_TYPE(pdt)), PDT_SIZE,
	    DIM_HI(pdTemp), DIM_LO(pdTemp));
    }
    else {
      fgenf(outfile, "sizeof(%D), ", D_ENS_TYPE(pdt), PDT_SIZE);
    }
  }
  else {
    fgenf(outfile, "sizeof(%D), ", D_ENS_TYPE(pdt), PDT_SIZE);
  }
  fgenf(outfile, "%s, ", accname);
  if (!D_ENS_COPY(pdt)) {
    fgenf(outfile, "NULL");
  }
  else {
    fgenf(outfile, "%s", S_IDENT(D_ENS_COPY(pdt)));
  }
  fgenf(outfile, ", \"%s\", %d, %d);\n", arrayname, isrealloc, D_ENS_DNR(pdt));
  fgenf(outfile, "_InitEnsemble(%s);\n", arrayname);
  if (hoist_access_ptrs == HOIST_PTRS_TO_SUBPROG && 
      hoistable) {  /* we can't handle indarrays or structures here yet */
    dimension_t* arrdim;

    fprintf(outfile, "_INIT_HOISTED_PTR(");
    arrdim = gen_pdt(outfile, D_ENS_TYPE(pdt), PDT_PCST);
    if (arrdim) {
      fprintf(outfile, "*");
    }
    fprintf(outfile, ", %s", arrayname);
    fprintf(outfile, ");\n");
  }
}


void GenFreeEnsemble(FILE* outfile, symboltable_t* pst, char* arrayname, 
		     datatype_t* pdt, int isrealloc, int hoistable) {
  fgenf(outfile, "_DestroyEnsemble(%s,%d);\n", arrayname, D_ENS_DNR(pdt));
}
