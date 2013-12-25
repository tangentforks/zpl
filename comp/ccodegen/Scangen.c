/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/Cgen.h"
#include "../include/Mgen.h"
#include "../include/Privgen.h"
#include "../include/SRgen.h"
#include "../include/Scangen.h"
#include "../include/Wgen.h"
#include "../include/comm.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/parsetree.h"
#include "../include/rmstack.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"

static elist gelist=NULL;

static void gen_size(FILE *outfile,int num) {
  fprintf(outfile,"_size[%d]",num);
}


static void gen_size_decl(FILE *outfile,int num,datatype_t *pdt) {
  int i;

  fprintf(outfile,"int ");
  gen_size(outfile,num);
  fprintf(outfile," = {");
  for (i=0;i<num;i++) {
    if (i) {
      fprintf(outfile,",");
    }
    gen_sr_tot_array_size(outfile,pdt);
  }
  fprintf(outfile,"};\n");
}


static void gen_block(FILE * outfile,int num,int dim) {
  fprintf(outfile,"_block[%d][%d]",num,dim);
}


static void gen_scan_res(FILE *outfile,int num) {
  fprintf(outfile,"_scan_result[%d]",num);
}


static void gen_scan_tal(FILE *outfile,int num) {
  fprintf(outfile,"_scan_tally[%d]",num);
}


static void gen_scan_access(FILE *outfile,int numdims,datatype_t *pdt) {
  fprintf(outfile,"_S_ACCESS_%dD",numdims);
  if (D_CLASS(pdt) == DT_ARRAY) {
    fprintf(outfile,"_OFF");
  }
}


static void gen_dim_size(FILE *outfile,dimension_t *dimptr) {
  expr_t *lodim;
  expr_t *hidim;
  int numelems=1;

  while (dimptr!=NULL) {
    lodim = DIM_LO(dimptr);
    hidim = DIM_HI(dimptr);
    if (expr_computable_const(lodim) && expr_computable_const(hidim)) {
      numelems*=(expr_intval(hidim)-expr_intval(lodim)+1);
    } else {
      INT_FATAL(NULL,"non-const arrays used in Scangen.c");
    }
    
    dimptr = DIM_NEXT(dimptr);
  }
  fprintf(outfile,"%d",numelems);
}


static void gen_access_off(FILE *outfile,datatype_t *resdt) {
  dimension_t *dimptr;
  int bracket=0;
  int dim;

  if (D_CLASS(resdt) != DT_ARRAY) {
    return;
  }
  fprintf(outfile,",");
  while (D_CLASS(resdt) == DT_ARRAY) {
    dimptr = D_ARR_DIM(resdt);
    dim = 0;
    if (bracket) {
      fprintf(outfile,"+");
    }
    fprintf(outfile,"(");
    while (dimptr != NULL) {
      if (dim) {
	fprintf(outfile,"+");
      }
      fprintf(outfile,"((");
      gen_sr_nloop_index(outfile,bracket,dim);
      fprintf(outfile,"-");
      gen_expr(outfile, DIM_LO(dimptr));
      fprintf(outfile,")*");
      dimptr = DIM_NEXT(dimptr);
      dim++;

      gen_dim_size(outfile,dimptr);
      fprintf(outfile,")");
    }
    fprintf(outfile,")*");

    resdt = D_ARR_TYPE(resdt);
    bracket++;

    gen_sr_tot_array_size(outfile,resdt);
  }
}


static void gen_scan_tal_access(FILE *outfile,int num,int numdims,
				expr_t *reg,datatype_t *pdt) {
  gen_scan_access(outfile,numdims,pdt);
  fprintf(outfile,"(%d,",num);
  gen_scan_tal(outfile,num);
  fprintf(outfile,",");
  priv_reg_access(outfile, reg);
  gen_access_off(outfile,pdt);
  fprintf(outfile,")");
}


static void gen_scan_res_access(FILE *outfile,int num,int numdims,
				expr_t *reg,datatype_t *pdt) {
  gen_scan_access(outfile,numdims,pdt);
  fprintf(outfile,"(%d,",num);
  gen_scan_res(outfile,num);
  fprintf(outfile,",");
  priv_reg_access(outfile, reg);
  gen_access_off(outfile,pdt);
  fprintf(outfile,")");
}


static void gen_scan_flat_bloop_start(FILE *outfile,int num) {
  fprintf(outfile,"for (_i=0;_i<");
  gen_size(outfile,num);
  fprintf(outfile,";_i++) {\n");
}


static void gen_scan_flat_bloop_stop(FILE *outfile) {
  fprintf(outfile,"}\n");
}


static void gen_broadcast_slice(FILE *outfile,int numdims,int current_dim,
				int *dimlist,expr_t *reg) {
  int i;
  int slicenum;
  int bit;

  slicenum = 0;
  bit = 0x1;
  for (i=0; i<numdims; i++) {
    slicenum |= bit;
    bit = bit << 1;
  }

  for (i=0;i<current_dim;i++) {
    slicenum -= (0x1 << dimlist[i]);
  }
  fprintf(outfile,"%d",slicenum);
}


static void gen_bcast_root(FILE *outfile,int numdims,int current_dim,
			   int *dimlist,expr_t *reg) {
  fprintf(outfile,"_MaxProcInSlice(_DefaultGrid,");
  gen_broadcast_slice(outfile,numdims,current_dim,dimlist,reg);
  fprintf(outfile,")");
}


static void gen_involved_proc(FILE *outfile,int current_dim,int *dimlist,
			      expr_t *reg) {
  int i;
  int print = 0;

  for (i=0; i<current_dim; i++) {
    if (print) {
      fprintf(outfile,"&&");
    }
    fprintf(outfile,"_T_ATEND(");
    priv_reg_access(outfile, reg);
    fprintf(outfile,",%d)",dimlist[i]);
    print=1;
  }
  if (!print) {
    fprintf(outfile,"1");
  }
}


static void gen_filter_start(FILE *outfile,int num,int *scandims,
			     expr_t *reg) {
  fprintf(outfile,"if (");
  gen_involved_proc(outfile,num,scandims,reg);
  fprintf(outfile,") {\n");
}


static void gen_filter_stop(FILE *outfile) {
  fprintf(outfile,"}\n");
}


static void gen_scan_mloop_start(FILE *outfile,expr_t *reg,expr_t *mask,
				 int maskbit,int numdims) {
  gen_mloop_rmo_start(outfile,reg,mask,maskbit,numdims,gelist,0);
}


static void gen_scan_mloop_stop(FILE *outfile,expr_t *reg,expr_t *mask,
				int maskbit,int numdims) {
  gen_mloop_rmo_stop(outfile,reg,mask,maskbit,numdims,gelist,0);
}



static void GenStartScan(FILE *outfile,statement_t *stmt,int numdims,
			 datatype_t *resdt,int scandims,int *dimlist,
			 expr_t *reg) {
  int i;
  int j;
  int k;
  int flattened_dim;

  fprintf(outfile,"{ /* begin Scan (line %d) */\n",T_LINENO(stmt));

  fprintf(outfile," int _i;\n");

  gen_size_decl(outfile,scandims,resdt);

  fprintf(outfile,"int ");
  gen_block(outfile,scandims,numdims);
  fprintf(outfile,";\n");

  gen_sr_pdt(outfile,resdt,PDT_LOCL);
  fprintf(outfile,"*");
  gen_scan_res(outfile,scandims);
  fprintf(outfile,";\n");

  gen_sr_pdt(outfile,resdt,PDT_LOCL);
  fprintf(outfile,"*");
  gen_scan_tal(outfile,scandims);
  fprintf(outfile,";\n");
  
  gen_sr_pdt(outfile,resdt,PDT_LOCL);
  fprintf(outfile,"_temp;\n");

  gen_sr_nloop_decls(outfile,resdt);

  fprintf(outfile,"\n");

  for (i=0;i<scandims;i++) {
    for (j=numdims-1;j>=0;j--) {
      flattened_dim=0;
      for (k=0;k<=i;k++) {
	if (j==dimlist[k]) {
	  gen_block(outfile,i,j);
	  fprintf(outfile,"=0;\n");
	  flattened_dim=1;
	}
      }
      if (!flattened_dim) {
	gen_block(outfile,i,j);
	fprintf(outfile,"=");
	gen_size(outfile,i);
	fprintf(outfile,";\n");

	gen_size(outfile,i);
	fprintf(outfile,"*=_REG_NUMELMS(");
	priv_reg_access(outfile, reg);
	fprintf(outfile,",%d);\n",j);
      }
    }
  }

  for (i=0;i<scandims;i++) {
    gen_scan_res(outfile,i);
    fprintf(outfile,"=(");
    gen_sr_pdt(outfile,resdt,PDT_PCST);
    fprintf(outfile,"*)_zmalloc(");
    gen_size(outfile,i);
    fprintf(outfile,"*sizeof(");
    gen_sr_pdt(outfile,resdt,PDT_SIZE);
    fprintf(outfile,"),\"scan result buffer\");\n");
  
    gen_scan_tal(outfile,i);
    fprintf(outfile,"=(");
    gen_sr_pdt(outfile,resdt,PDT_PCST);
    fprintf(outfile,"*)_zmalloc(");
    gen_size(outfile,i);
    fprintf(outfile,"*sizeof(");
    gen_sr_pdt(outfile,resdt,PDT_SIZE);
    fprintf(outfile,"),\"scan tally buffer\");\n");
  }
}
 

static void GenLocalScan(FILE *outfile,exprtype op,datatype_t *resdt,
			 expr_t *reg,int numdims,expr_t *arg_expr,
			 expr_t *res_expr,expr_t *mask,int maskbit) {
  if (new_access) {
    gelist = (elist)search_downfor_ensemble_exprs(res_expr);
    gelist = (elist)glist_append_list((glist)gelist,
				      search_downfor_ensemble_exprs(arg_expr));
  } else {
    gelist = NULL;
  }
  gen_scan_flat_bloop_start(outfile,0);

  gen_scan_tal(outfile,0);
  gen_flat_bloop_access(outfile);
  fprintf(outfile,"=");
  gen_ident_elem(outfile,op,resdt);
  fprintf(outfile,";\n");

  gen_scan_flat_bloop_stop(outfile);

  gen_scan_mloop_start(outfile,reg,mask,maskbit,numdims);

  gen_sr_nloop_start(outfile,resdt);

  /** tmp **/
  gen_sr_pdt(outfile,resdt,PDT_LOCL);
  fprintf(outfile, " _scanswaptemp;\n");
  fprintf(outfile,"_scanswaptemp =");
  gen_scan_tal_access(outfile,0,numdims,reg,resdt);
  fprintf(outfile,";\n");

  gen_sr_setup_complex_temp(outfile,arg_expr,resdt);
  gen_op_stmt(outfile,op,resdt);
  fprintf(outfile,"(");
  gen_scan_tal_access(outfile,0,numdims,reg,resdt);
  fprintf(outfile,",");
  gen_sr_noncomplex_expr(outfile,arg_expr,resdt);
  gen_sr_nloop_access(outfile,resdt);
  fprintf(outfile,");\n");

  gen_noncomplex_expr(outfile,res_expr);
  gen_sr_nloop_access(outfile,resdt);
  fprintf(outfile,"=");
  fprintf(outfile,"_scanswaptemp");
  fprintf(outfile,";\n");

  gen_sr_nloop_stop(outfile,resdt);

  gen_scan_mloop_stop(outfile,reg,mask,maskbit,numdims);
  glist_destroy((glist)gelist,ELIST_NODE_SIZE);
  gelist = NULL;
}


static void GenLocalSubScan(FILE *outfile,exprtype op,datatype_t *resdt,
			    expr_t *reg,int numdims,expr_t *arg_expr,
			    int num,int *dimlist) {
  int numloops;

  gen_filter_start(outfile,num,dimlist,reg);

  gen_scan_flat_bloop_start(outfile,num);

  gen_scan_tal(outfile,num);
  gen_flat_bloop_access(outfile);
  fprintf(outfile,"=");
  gen_ident_elem(outfile,op,resdt);
  fprintf(outfile,";\n");

  gen_scan_flat_bloop_stop(outfile);

  gen_sr_nloop_start(outfile,resdt);

  numloops=gen_sub_mloop_start(outfile,reg,num,dimlist);

  fprintf(outfile,"_temp=");
  gen_scan_tal_access(outfile,num,numdims,reg,resdt);
  fprintf(outfile,";\n");

  gen_op_stmt(outfile,op,resdt);
  fprintf(outfile,"(");
  gen_scan_tal_access(outfile,num,numdims,reg,resdt);
  fprintf(outfile,",");
  gen_scan_tal_access(outfile,num-1,numdims,reg,resdt);
  fprintf(outfile,");\n");

  gen_scan_tal_access(outfile,num-1,numdims,reg,resdt);
  fprintf(outfile,"=_temp;\n");

  gen_sr_nloop_stop(outfile,resdt);

  gen_sub_mloop_stop(outfile,numloops);
  gen_filter_stop(outfile);
}


static void GenGlobalScan(FILE *outfile,int num,int dim,datatype_t *resdt,
			  expr_t *reg,exprtype op) {
  fprintf(outfile,"_Scan");
  gen_sr_pdt(outfile,resdt,PDT_SRED);
  fprintf(outfile,"(");
  gen_scan_tal(outfile,num);
  fprintf(outfile,",");
  gen_scan_res(outfile,num);
  fprintf(outfile,",");
  priv_reg_access(outfile, reg);
  fprintf(outfile, ",%d,",dim);
  gen_size(outfile,num);
  fprintf(outfile,",1,");
  gen_op_op(outfile,op);
  fprintf(outfile,",");
  gen_ident_elem(outfile,op,resdt);
  fprintf(outfile,");\n");
}


static void UpdateSubTally(FILE *outfile,int num,int *dimlist,
			   expr_t *reg,exprtype op,int numdims,
			   datatype_t *resdt) {
  int numloops;

  gen_filter_start(outfile,num,dimlist,reg);
  numloops = gen_sub_mloop_start(outfile,reg,num,dimlist);

  gen_sr_nloop_start(outfile,resdt);
  
  gen_op_stmt(outfile,op,resdt);
  fprintf(outfile,"(");
  gen_scan_tal_access(outfile,num-1,numdims,reg,resdt);
  fprintf(outfile,",");
  gen_scan_res_access(outfile,num,numdims,reg,resdt);
  fprintf(outfile,");\n");

  gen_sr_nloop_stop(outfile,resdt);

  gen_sub_mloop_stop(outfile,numloops);
  gen_filter_stop(outfile);
}


static void GenSubBroadcast(FILE *outfile,int num,int *dimlist,
			    expr_t *reg,int numdims,datatype_t *resdt) {
  int nextdim;
  int cid;

  if (num) {
    nextdim = dimlist[num-1];
    cid = get_cid();
      
    fprintf(outfile,"_BroadcastSimpleToSlice(_DefaultGrid, ");
    gen_scan_tal(outfile,num-1);
    fprintf(outfile,",");
    gen_size(outfile,num-1);
    fprintf(outfile,"*sizeof(");
    gen_sr_pdt(outfile,resdt,PDT_SIZE);
    fprintf(outfile,"),");
    gen_involved_proc(outfile,num-1,dimlist,reg);
    fprintf(outfile,",");
    gen_bcast_root(outfile,numdims,num,dimlist,reg);
    fprintf(outfile,",%d,",cid);
    gen_broadcast_slice(outfile,numdims,num,dimlist,reg);
    /*    fprintf(outfile,"%d",0x1 << nextdim);*/
    fprintf(outfile,");\n");
  }
}


static void UpdateSubResult(FILE *outfile,int num,int *dimlist,
			    expr_t *reg,exprtype op,int numdims,
			    datatype_t *resdt) {
  int numloops;

  gen_filter_start(outfile,num-1,dimlist,reg);

  numloops = gen_sub_mloop_start(outfile,reg,num,dimlist);

  gen_sr_nloop_start(outfile,resdt);

  gen_op_stmt(outfile,op,resdt);
  fprintf(outfile,"(");
  gen_scan_res_access(outfile,num-1,numdims,reg,resdt);
  fprintf(outfile,",");
  gen_scan_tal_access(outfile,num-1,numdims,reg,resdt);
  fprintf(outfile,");\n");

  gen_sr_nloop_stop(outfile,resdt);

  gen_sub_mloop_stop(outfile,numloops);

  gen_filter_stop(outfile);
}


static void GenLocalUpdateScan(FILE *outfile,exprtype op,expr_t *res_expr,
			       int numdims,expr_t *reg,expr_t *mask,
			       int maskbit,datatype_t *resdt) {
  if (new_access) {
    gelist = (elist)search_downfor_ensemble_exprs(res_expr);
  } else {
    gelist = NULL;
  }
  gen_scan_mloop_start(outfile,reg,mask,maskbit,numdims);
  gen_sr_nloop_start(outfile,resdt);

  gen_sr_setup_complex_temp(outfile,res_expr,resdt);
  gen_op_stmt(outfile,op,resdt);
  fprintf(outfile,"(");
  gen_sr_noncomplex_expr(outfile,res_expr,resdt);
  gen_sr_nloop_access(outfile,resdt);
  fprintf(outfile,",");
  gen_scan_res_access(outfile,0,numdims,reg,resdt);
  fprintf(outfile,");\n");

  gen_sr_nloop_stop(outfile,resdt);
  gen_scan_mloop_stop(outfile,reg,mask,maskbit,numdims);
  glist_destroy((glist)gelist,ELIST_NODE_SIZE);
  gelist = NULL;
}


static void GenEndScan(FILE *outfile,int scandims,statement_t *stmt) {
  int i;

  for (i=0;i<scandims;i++) {
    fprintf(outfile,"_zfree(");
    gen_scan_res(outfile,i);
    fprintf(outfile,",\"scan result buffer\");\n");
    fprintf(outfile,"_zfree(");
    gen_scan_tal(outfile,i);
    fprintf(outfile,",\"scan tally buffer\");\n");
  }
  fprintf(outfile,"} /* end Scan (line %d) */\n",T_LINENO(stmt));
}


void gen_scan(FILE *outfile,expr_t *expr) {
  statement_t *stmt;
  expr_t *res_expr;
  datatype_t *resdt;
  expr_t *scan_expr;
  expr_t *arg_expr;
  exprtype scan_op;
  int numdims;
  expr_t *reg;
  expr_t *mask;
  int maskbit;
  int scandims;
  int dimlist[MAXRANK];
  int i;

  stmt = T_STMT(expr);
  res_expr = T_OPLS(expr);
  resdt = T_TYPEINFO(res_expr);

  scan_expr = T_NEXT(T_OPLS(expr));
  arg_expr = T_OPLS(scan_expr);
  scan_op = T_SUBTYPE(scan_expr);

  numdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(res_expr))); /* could check arg_expr, but it
							    might be constant */

  reg = RMSCurrentRegion();
  if (T_IDENT(reg) == pst_qreg[0]) {
    T_IDENT(reg) = pst_qreg[numdims];
    typeinfo_expr(reg);
  }
  mask = RMSCurrentMask(&maskbit);

  if (T_DIMS(scan_expr) == NULL) {
    scandims = numdims;
    for (i=0;i<numdims;i++) {
      dimlist[i] = numdims-i-1;
    }
  } else {
    scandims = T_DIMS(scan_expr)[MAXRANK];
    for (i=0;i<scandims;i++) {
      dimlist[i] = T_DIMS(scan_expr)[i]-1;
    }
  }
  
  GenStartScan(outfile,stmt,numdims,resdt,scandims,dimlist,reg);
  GenLocalScan(outfile,scan_op,resdt,reg,numdims,arg_expr,res_expr,mask,maskbit);
  GenGlobalScan(outfile,0,dimlist[0],resdt,reg,scan_op);

  for (i=1;i<scandims;i++) {
    GenLocalSubScan(outfile,scan_op,resdt,reg,numdims,arg_expr,i,dimlist);
    GenGlobalScan(outfile,i,dimlist[i],resdt,reg,scan_op);
  }

  for (i=scandims-1;i>=1;i--) {
    UpdateSubTally(outfile,i,dimlist,reg,scan_op,numdims,resdt);
    GenSubBroadcast(outfile,i,dimlist,reg,numdims,resdt);
    UpdateSubResult(outfile,i,dimlist,reg,scan_op,numdims,resdt);
  }

  GenLocalUpdateScan(outfile,scan_op,res_expr,numdims,reg,mask,maskbit,resdt);
  GenEndScan(outfile,scandims,stmt);
}
