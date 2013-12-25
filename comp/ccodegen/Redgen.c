/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "../include/Cgen.h"
#include "../include/Privgen.h"
#include "../include/Redgen.h"
#include "../include/SRgen.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/rmstack.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/stmtutil.h"
#include "../include/buildzplstmt.h"
#include "../include/error.h"
#include "../include/expr.h"

/* gen the "0th" element of a potentially multidimensional array */

static void gen_array_origin_index(FILE* outfile, datatype_t* resdt) {
  dimension_t* pdim;

  if (D_CLASS(resdt) == DT_ARRAY) {
    pdim = D_ARR_DIM(resdt);
    if (pdim != NULL) {
      fprintf(outfile, "[0]");
    }
    gen_array_origin_index(outfile, D_ARR_TYPE(resdt));
  }
}


static void GenGlobalReduce(FILE *outfile,int complete,datatype_t *resdt,
			    expr_t *srcreg,exprtype red_op,
			    expr_t *dstreg,expr_t* arg_expr,
			    symboltable_t *userfn) {
  if (red_op != USER) {
    fprintf(outfile,"_Reduce_");
    gen_sr_pdt(outfile,resdt,PDT_SRED);
  }
  else {
    fprintf(outfile,"_Reduce_user");
  }
  fprintf(outfile,"(");

  if (complete) {
    /* while one might expect "&a" and "a" to work here for scalars and
       vectors respectively, it turns out that the T3E doesn't get the
       cast correct automatically in the second case for 2D vectors.  
       Thus, we explicitly emit &(a) and &(a[0][0]) for these two cases. */
    fprintf(outfile, "&(");
    gen_noncomplex_expr(outfile, arg_expr);
    gen_array_origin_index(outfile, resdt);
    fprintf(outfile, ")");
  } else {
    fprintf(outfile,"(");
    gen_sr_pdt(outfile,resdt,PDT_SRED);
    fprintf(outfile,"*)");
    fprintf(outfile,"_ARR_DATA(");
    brad_no_access++;
    gen_noncomplex_expr(outfile,arg_expr);
    brad_no_access--;
    fprintf(outfile,")");
  }
  fprintf(outfile,",");

  if (complete) {
    gen_sr_tot_array_size(outfile,resdt);
  } else {
    fprintf(outfile,"_ARR_DATA_SIZE(");
    brad_no_access++;
    gen_noncomplex_expr(outfile,arg_expr);
    brad_no_access--;
    fprintf(outfile,")/sizeof(");
    gen_sr_pdt(outfile,resdt,PDT_SRED);
    fprintf(outfile,")");
  }
  if (userfn != NULL) {
    fprintf(outfile,"/");
    gen_sr_tot_array_size(outfile,T_TYPE(T_DECL(S_FUN_BODY(userfn))));
  }
  fprintf(outfile,",");

  if (!dstreg) { 
    fprintf(outfile,"NULL");
  } else {    
    priv_reg_access(outfile, dstreg);
  }
  fprintf(outfile,",");
  if (!srcreg) {
    fprintf(outfile,"NULL");
  } else {    
    priv_reg_access(outfile,srcreg);
  }
  fprintf(outfile,",");
  if (red_op != USER) {
    gen_op_op(outfile,red_op);
  }
  else {
    fprintf(outfile,S_IDENT(userfn));
    fprintf(outfile,",sizeof(");
    gen_pdt(outfile,T_TYPEINFO(arg_expr),PDT_PARM);
    fprintf(outfile,")");
  }
  fprintf(outfile,");\n");
}


void gen_reduce(FILE *outfile,expr_t *red_expr) {
  datatype_t *resdt;     /* the result type */
  expr_t *arg_expr;      /* the expression being reduced */
  exprtype red_op;       /* the reduction operation */
  expr_t *srcreg; /* the source region */
  expr_t *dstreg; /* the destination region (if applicable) */
  int complete;          /* complete reduction? (or partial) */
  symboltable_t *userfn; /* user supplied overloaded function for user
                            defined reduction */

  arg_expr = T_OPLS(red_expr);
  resdt = T_TYPEINFO(red_expr);
  red_op = T_SUBTYPE(red_expr);

  if (T_REGMASK2(red_expr) == NULL) {
    INT_FATAL(T_STMT(red_expr), "destination region was not set up properly");
  }
  RMSPushScope(T_REGMASK2(red_expr));
  dstreg = RMSCurrentRegion();
  RMSPopScope(T_REGMASK2(red_expr));
  
  complete = (T_REGMASK(red_expr) == NULL);
  if (complete) {
    if (T_RED_RANK(red_expr) == 0) {
      srcreg = NULL;
      dstreg = NULL;
    }
    else {
      srcreg = dstreg;
      dstreg = NULL;
    }
  } else {
    RMSPushScope(T_REGMASK(red_expr));
    srcreg = RMSCurrentRegion();
    RMSPopScope(T_REGMASK(red_expr));
  }
  userfn = T_IDENT(red_expr);

  GenGlobalReduce(outfile,complete,resdt,srcreg,red_op,dstreg,arg_expr,userfn);
}
