/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/runtime.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/treemac.h"


/**
 **  Note that the copts pass saves and restores the region scope
 **  using compound statments.  This was added for checkpointing.
 **/


/**
 ** Push new region and mask information on region stack
 **/
void push_region_scope(FILE* outfile, region_t *region_p) {
  if (region_p != NULL) {
    expr_t *reg = T_REGION_SYM(region_p);
    expr_t *maskexpr = T_MASK_EXPR(region_p);
    int maskbit = T_MASK_BIT(region_p);
    symboltable_t *maskvar=NULL;

    if (maskexpr!=NULL) {
      maskvar = expr_find_root_pst(maskexpr);
    }

    /* Create a new scope */
    fprintf(outfile, "{  /* Push region scope */\n");
  
    /* set up any dynamic regions as necessary */
    if (!expr_is_qreg(reg) && T_IDENT(reg) && !S_SETUP(T_IDENT(reg))) {
      force_pst_init(outfile, T_IDENT(reg));
      force_pst_finalize(outfile, T_IDENT(reg));
    }
  
    /* push the new values on the stack */

    fprintf(outfile,"%s.%s = ",RMS,RMS_REG);
    gen_name(outfile, reg);
    fprintf(outfile, ";\n");


    if (maskvar == NULL) {
      fprintf(outfile,"%s.%s = NULL;\n",RMS,RMS_MASK);
    } else {
      if (!expr_is_qmask(maskexpr)) {
	fprintf(outfile,"%s.%s = ",RMS,RMS_MASK);
	
	mask_no_access++;
	gen_expr(outfile, maskexpr);
	mask_no_access--;
      
	fprintf(outfile,";\n");
      }
      if (maskbit != -1) {
	fprintf(outfile,"%s.%s = %d;\n",RMS,RMS_WITH,maskbit);
      }
    }
    fprintf(outfile,"\n"); 
  }
}


/**
 ** Pop region and mask information off region stack
 **/
void pop_region_scope(FILE* outfile, region_t *region_p) {
  if (region_p != NULL) {
    expr_t * reg = T_REGION_SYM(region_p);

    /* set up any dynamic regions as necessary */
    if (!expr_is_qreg(reg) && T_IDENT(reg) && !S_SETUP(T_IDENT(reg))) {
      force_pst_done(outfile, T_IDENT(reg));
    }

    fprintf(outfile, "} /* Pop region scope */ \n");
  }
}

/***
 ***
 *** FUNCTION: genStackReg()
 ***           Prints out the "current" region or mask.
 ***
 *** AUTHOR: sungeun (5 February 1998)
 ***
 ***/
void genStackReg(FILE* f) {
  priv_reg_access(f, buildqregexpr(0));
}


/**
 **  Dynamic flood regions seem to be special-cased.
 **/
void OpenFloodReduceDynamicRegion(FILE* outfile, expr_t *reg) {
  fprintf(outfile,"{\n");
  if (!expr_is_qreg(reg) && T_IDENT(reg) && !S_SETUP(T_IDENT(reg))) {
    force_pst_init(outfile, T_IDENT(reg));
    force_pst_finalize(outfile, T_IDENT(reg));
  }
}

void CloseFloodReduceDynamicRegion(FILE* outfile, expr_t *reg) {
  if (!expr_is_qreg(reg) && T_IDENT(reg) && !S_SETUP(T_IDENT(reg))) {
    force_pst_done(outfile, T_IDENT(reg));
  }
  fprintf(outfile,"}\n");
}
