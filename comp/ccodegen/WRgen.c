/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "../include/Cgen.h"
#include "../include/Privgen.h"
#include "../include/WRgen.h"
#include "../include/buildzplstmt.h"
#include "../include/cmplx_ens.h"
#include "../include/comm.h"
#include "../include/db.h"
#include "../include/generic_stack.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/treemac.h"


void gen_wrap(FILE* outfile, statement_t * stmt) {
  expr_t * exprlist;      /* pointer to the variable symbols */
  expr_t * exprptr;
  expr_t * elemexpr;
  int numensembles;
  wrap_t * wrap;
  int commid;
  expr_t *reg;

  wrap = T_WRAP(stmt);

  commid = T_IRON_COMMID(stmt);

  reg = RMSCurrentRegion();

  if (T_WRAP_SEND(wrap)) {
    TestWRForComplexEns(outfile, wrap);
    exprlist = T_OPLS(wrap);

    fprintf(outfile,"{\n");
    fprintf(outfile,"  _array_pnc _ensvect[");

    exprptr = exprlist;
    numensembles = 0;
    while (exprptr != NULL) {
      numensembles++;
      exprptr = T_NEXT(exprptr);
    }
    fprintf(outfile,"%d];\n",numensembles+1);
    
    fprintf(outfile,"  unsigned int _sizevect[%d] = {",numensembles);
    
    exprptr = exprlist;
    while (exprptr != NULL) {
      elemexpr = exprptr;
      /* strip off any null arrayrefs (indexed array element types) */
      while (T_TYPE(elemexpr) == ARRAY_REF && nthoperand(elemexpr,2) == NULL) {
	elemexpr = T_OPLS(elemexpr);
      }
      find_element_size(T_TYPEINFO(elemexpr),outfile);
      exprptr = T_NEXT(exprptr);
      if (exprptr != NULL) {
	fprintf(outfile,",");
      }
    }
    fprintf(outfile,"};\n");
    
    exprptr = exprlist;
    numensembles = 0;
    while (exprptr != NULL) {
      fprintf(outfile,"_ensvect[%d] = ",numensembles);
      brad_no_access++;
      gen_expr(outfile, exprptr);
      brad_no_access--;
      numensembles++;
      fprintf(outfile,";\n");
      exprptr = T_NEXT(exprptr);
    }
    fprintf(outfile,"_ensvect[%d] = NULL;\n",numensembles);
    
    if (numensembles > max_numensembles[commid]) {
      max_numensembles[commid] = numensembles;
    }
    
    fprintf(outfile,"_WrapI(");
    priv_reg_access(outfile, reg);
    fprintf(outfile, ",%d,_ensvect,_sizevect,%d);\n",numensembles,commid);
    fprintf(outfile,"}\n");
    
    FinishWRForComplexEns(outfile, wrap);
  } else {
    fprintf(outfile,"_WrapII(");
    priv_reg_access(outfile, reg);
    fprintf(outfile,",%d);\n",commid);
  }
}


void gen_reflect(FILE* outfile, statement_t * stmt) {
  expr_t * exprlist;      /* pointer to the variable symbols */
  expr_t * exprptr;
  expr_t * elemexpr;
  int numensembles;
  wrap_t * reflect;
  expr_t *reg;
  int commid;
  
  reflect = T_WRAP(stmt);

  commid = T_IRON_COMMID(stmt);

  TestWRForComplexEns(outfile, reflect);
  exprlist = T_OPLS(reflect);

  reg = RMSCurrentRegion();

  fprintf(outfile,"{\n");
  fprintf(outfile,"  _array_pnc _ensvect[");
  
  exprptr = exprlist;
  numensembles = 0;
  while (exprptr != NULL) {
    numensembles++;
    exprptr = T_NEXT(exprptr);
  }
  fprintf(outfile,"%d];\n",numensembles+1);
  
  fprintf(outfile,"  unsigned int _sizevect[%d] = {",numensembles);

  exprptr = exprlist;
  while (exprptr != NULL) {
    elemexpr = exprptr;
    /* strip off any null arrayrefs (indexed array element types) */
    while (T_TYPE(elemexpr) == ARRAY_REF && nthoperand(elemexpr,2) == NULL) {
      elemexpr = T_OPLS(elemexpr);
    }
    find_element_size(T_TYPEINFO(elemexpr),outfile);
    exprptr = T_NEXT(exprptr);
    if (exprptr != NULL) {
      fprintf(outfile,",");
    }
  }
  fprintf(outfile,"};\n");
  
  exprptr = exprlist;
  numensembles = 0;
  while (exprptr != NULL) {
    fprintf(outfile,"_ensvect[%d] = ",numensembles);
    brad_no_access++;
    gen_expr(outfile, exprptr);
    brad_no_access--;
    numensembles++;
    fprintf(outfile,";\n");
    exprptr = T_NEXT(exprptr);
  }
  fprintf(outfile,"_ensvect[%d] = NULL;\n",numensembles);

  fprintf(outfile,"_ReflectI(");
  priv_reg_access(outfile, reg);
  fprintf(outfile, ",_ensvect,_sizevect,%d);\n",commid);

  fprintf(outfile,"}\n");

  FinishWRForComplexEns(outfile, reflect);
}
