/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/



#include <stdio.h>
#include "../include/Cgen.h"
#include "../include/Fgen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/cmplx_ens.h"
#include "../include/db.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/struct.h"
#include "../include/symmac.h"
#include "../include/treemac.h"


static int SetupDestTemp(FILE *outfile,expr_t *dstens,expr_t *dstreg,
			 expr_t *srcreg,int numdims,
			 datatype_t *dataptr) {
  symboltable_t *pst;

  pst = expr_is_var(dstens);
  if (!pst || T_IDENT(D_ENS_REG(S_DTYPE(pst))) != pst_mfreg) {
    return 0;
  }
  fprintf(outfile,"{\n");
  fprintf(outfile,"_reg_info _FDRstuff;\n");
  fprintf(outfile,"_region_fnc _FDR=&(_FDRstuff);\n\n");
  fprintf(outfile,"_CreateFloodRegion(_FDR,");
  priv_reg_access(outfile, dstreg);
  fprintf(outfile,",");
  priv_reg_access(outfile, srcreg);
  fprintf(outfile,",%d);\n", numdims);
  fprintf(outfile,"_SetupTempFloodEnsemble(%s,_FDR,",S_IDENT(pst));
  find_element_size(dataptr,outfile);
  fprintf(outfile,");\n");
  return 1;
}


static void DestroyDestTemp(FILE *outfile) {
  fprintf(outfile,"}\n");
}


/* the assumption here is that all statements coming in here will be
   of the form  ens1 := >>[reg] ens2; */

void gen_flood(FILE *outfile,expr_t *expr) {
  expr_t *floodexpr;
  int numdims;
  datatype_t *dataptr;
  expr_t *srcreg;
  expr_t *srcens;
  expr_t *dstens;
  int commid;
  int dsttmp;

  dstens = T_OPLS(expr);
  floodexpr = T_NEXT(T_OPLS(expr));
  srcreg = T_REGION_SYM(T_REGMASK(floodexpr));
  srcens = T_OPLS(floodexpr);
  numdims = D_REG_NUM(T_TYPEINFO(srcreg));
  dataptr = T_TYPEINFO(dstens);
  commid = T_IRON_COMMID(T_STMT(expr));

  OpenFloodReduceDynamicRegion(outfile, srcreg);
  dsttmp=SetupDestTemp(outfile, dstens, buildqregexpr(numdims), srcreg, 
		       numdims, dataptr);

  TestFloodForComplexEns(outfile, expr);
  dstens = T_OPLS(expr);
  srcens = T_OPLS(floodexpr);

  if (!dsttmp) {
    fprintf(outfile, "{\n");
    fprintf(outfile, "int flooderror;\n");
    fprintf(outfile, "flooderror = ");
  }
  fprintf(outfile,"_Flood");
  if (dsttmp) {
    fprintf(outfile,"Guts");
  }
  fprintf(outfile,"(");
  if (dsttmp) {
    fprintf(outfile,"_FDR");
  } else {
    genStackReg(outfile);
  }
  fprintf(outfile,",");
  priv_reg_access(outfile, srcreg);
  fprintf(outfile,",");
  brad_no_access++;
  gen_expr(outfile, dstens);
  fprintf(outfile,",");
  gen_expr(outfile, srcens);
  brad_no_access--;
  fprintf(outfile,",");
  find_element_size(dataptr,outfile);
  fprintf(outfile,",%d)",commid);
  if (dsttmp) {
    fprintf(outfile,";\n");
  } else {
    fprintf(outfile, ";\n");
    fprintf(outfile, "if (flooderror) {\n");
    fprintf(outfile, 
	    "_RT_ALL_FATAL1(\"Ranges in source/destination regions of flood do not match\\n\"\n"
	    "\"(dim %%d, line %d)\\n\", flooderror);\n", T_LINENO(T_STMT(expr)));
    fprintf(outfile, "}\n");
    fprintf(outfile, "}\n");
  }

  FinishFloodForComplexEns(outfile, expr);
  if (dsttmp) {
    DestroyDestTemp(outfile);
  }
  CloseFloodReduceDynamicRegion(outfile, srcreg);
}

