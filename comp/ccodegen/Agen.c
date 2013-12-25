/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/Agen.h"
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Mgen.h"
#include "../include/Wgen.h"
#include "../include/coverage.h"
#include "../include/datatype.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/nudge.h"


static datatype_t *FindBaseDT(expr_t *expr) {
  expr_t *reg;

  reg = T_TYPEINFO_REG(expr);
  while (reg == NULL && T_PARENT(expr) != NULL) {
    expr = T_PARENT(expr);
    reg = T_TYPEINFO_REG(expr);
  }
  return T_TYPEINFO(expr);
}


static int ShouldWeHoist(expr_t* expr) {
  int hoist;

  if (T_RANDACC(expr) != NULL) {
    return 0;
  }
  if (T_TYPE(expr) == ARRAY_REF || T_TYPE(expr) == BIDOT) {
    /* can't currently hoist array refs or bidots */
    return 0;
  }
 
  /* if the caller has requested hoisting, make sure we really want
     to do it.  The check against current_dimnum is to make sure
     we're within the MLOOP (while this could work outside the
     MLOOP, it currently doesn't.  sweep.z in particular uses an
     access in a mloop variable, and we currently don't check mloop
     variable initializations for walker/bumper/hoisting.  This
     probably ought to be done, but this fix was easier. 
     
     The second test is the generic test: should we be using access
     macros and are any of the hoisting options turned on.  */
  if ((current_dimnum != 0) && 
      !new_access && (hoist_access_ptrs || hoist_access_mults)) {
    return 1;
  } else {
    return 0;
  }
  return hoist;
}


static void gen_access_string(FILE *outfile, expr_t *expr, int hoist) {
  expr_t *reg;
  int fastaccess=1;

  reg = T_TYPEINFO_REG(expr);
  if (expr_is_ever_strided_ens(expr)) {
    fastaccess = 0;
  }
  if (fastaccess) {
    fprintf(outfile,"_F");
  }
  if (hoist) {
    fprintf(outfile,"_HOISTED");
  }
  fprintf(outfile,"_ACCESS_%dD",D_REG_NUM(T_TYPEINFO(reg)));
}


static void gen_open_paren(FILE* outfile, expr_t* expr, int hoisted) {
  symboltable_t* rootpst;
  expr_t* reg;
  datatype_t* regdt;
  int somethingflat;
  int flat[MAXRANK];
  int numdims;
  static region_t regscope = {NULL,NULL,0,NULL};
  int i;

  rootpst = expr_find_root_pst(expr);
  reg = T_TYPEINFO_REG(expr);

  if (reg == NULL || rootpst == NULL) {
    INT_FATAL(T_STMT(expr),"BLC: Mis-assumption in gen_open_paren()\n");
  }
  if (S_CLASS(rootpst) == S_PARAMETER) {
    /* for parameters we really should do some alias checking to see
       if the actual is flat, but we currently don't */
    fprintf(outfile,"(");
    return;
  }
  regdt = T_TYPEINFO(reg);
  if (expr_is_qreg(reg) || datatype_reg_inherits(regdt)) {
    fprintf(outfile,"(");
    return;
  }

  numdims = D_REG_NUM(regdt);

  regscope.region = reg;
  

  RMSPushScope(&regscope);
  somethingflat = 0;
  for (i=0; i<numdims; i++) {
    if (RMSRegDimDegenerate(i,0)) {
      flat[i] = 1;
      somethingflat = 1;
    } else {
      flat[i] = 0;
    }
  }
  RMSPopScope(&regscope);

  /* we don't have _OPT versions of hoisted access macros yet, so
     this disables them */
  if (somethingflat && hoisted) {
    somethingflat = 0;
  }

  if (somethingflat) {
    fprintf(outfile,"_OPT");
  }
  fprintf(outfile,"(");
  if (somethingflat) {
    for (i=0; i<numdims; i++) {
      if (flat[i]) {
	fprintf(outfile,"F,");
      } else {
	fprintf(outfile,"N,");
      }
    }
  }
}


static void gen_access_expr(FILE *outfile,expr_t *expr) {
  brad_no_access++;
  gen_expr(outfile, expr);
  brad_no_access--;
}


static void gen_index(FILE *outfile,int dim) {
  fprintf(outfile,"_i%d",dim);
}


static void gen_lo_indexbound(FILE *outfile,int dim,int up) {
  gen_index(outfile,dim);
  if (up == 1) {
    fprintf(outfile,"Lo");
  } else {
    fprintf(outfile,"Hi");
  }
}


void gen_access_begin(FILE *outfile,expr_t *expr,int deref) {
  datatype_t *basedt;
  int hoist;

  basedt = FindBaseDT(expr);
  hoist = ShouldWeHoist(expr);

  fprintf(outfile,"(");
  if (deref) {
    fprintf(outfile,"*");
  }
  fprintf(outfile,"(");
  if (deref && (!hoist || use_charstar_pointers)) {
    fprintf(outfile,"(");
    gen_pdt(outfile,basedt,PDT_PCST);
    fprintf(outfile," *)");
  }

  gen_access_string(outfile, expr, hoist);
  gen_open_paren(outfile,expr, hoist);
  if (hoist) {
    gen_walker_name_from_expr(outfile,expr);
    fprintf(outfile, ",");
  }
  gen_access_expr(outfile,expr);
}


void gen_access_end(FILE *outfile,expr_t *expr,expr_t *dir) {
  int i;
  expr_t *reg;
  expr_t* nudgexpr;

  if (brad_no_access) {
    return;
  }

  reg = T_TYPEINFO_REG(expr);

  if (expr_nudged(expr)) {
    nudgexpr = expr;
  } else if (expr_nudged(T_PARENT(expr))) {
    nudgexpr = T_PARENT(expr);
  } else {
    nudgexpr = NULL;
  }

  for (i=0;i<D_REG_NUM(T_TYPEINFO(reg));i++) {
    fprintf(outfile,",");
    if (in_mloop & mloop_dim[i]) { /* if we're already within loop i */
      gen_index(outfile,i);
    } else {
      if (current_mloop == NULL) {
	INT_FATAL(T_STMT(expr),"Trying to access array outside of MLOOP");
      }
      gen_lo_indexbound(outfile,i,T_MLOOP_DIRECTION(current_mloop,i));
    }
    if (dir != NULL) {
      dir_gen_comp_offset(outfile, dir, i);
    }
    if (nudgexpr != NULL) {
      int nudgeval = T_GET_NUDGE(nudgexpr, i);
      if (nudgeval) {
	fprintf(outfile, " + (%d * _REG_STRIDE(", nudgeval);
	gen_expr(outfile, T_MLOOP_REG(current_mloop));
	fprintf(outfile, ", %d))", i);
      }
    }
  }
  fprintf(outfile,")))");
}


void gen_randaccess_end(FILE *outfile,expr_t *expr) {
  expr_t *reg;
  expr_t* map;
  int savecodegen_imaginary;
  int i;

  if (brad_no_access) {
    return;
  }

  reg = T_TYPEINFO_REG(expr);

  map = T_RANDACC(expr);
  if (map == (expr_t*)0x1) {
    map = NULL;
  }

  if (map != NULL) {
    while (map != NULL) {
      fprintf(outfile,",");
      savecodegen_imaginary = codegen_imaginary;
      codegen_imaginary = 0;
      gen_expr(outfile, map);
      codegen_imaginary = savecodegen_imaginary;
      map = T_NEXT(map);
    }
  }
  else {
    for (i = 0; i < T_MAPSIZE(expr); i++) {
      fgenf(outfile, ", _tind%d", i+1);
    }
  }
  fprintf(outfile,")))");
}


void gen_cached_access_stuff(FILE* outfile, mloop_t* mloop) {
  elist wblist = T_MLOOP_WBLIST(mloop);
  int numdims = T_MLOOP_RANK(mloop);
  elist node;
  expr_t* expr;
  dimension_t* arrdim;

  if (new_access || (!hoist_access_ptrs && !hoist_access_mults)) {
    return;
  }

  number_elist(wblist);
  node = wblist;
  while (node) {
    expr = ELIST_DATA(node);
    if (ELIST_ARR_COUNTS(node) && ELIST_DENSITY(node) == EXPR_DENSE) {
      if (hoist_access_ptrs == HOIST_PTRS_TO_MLOOP) {
	fprintf(outfile, "_DECL_INIT_HOISTED_PTRS(");
	arrdim = gen_pdt(outfile, T_TYPEINFO(ELIST_DATA(node)), PDT_LOCL);
	if (arrdim) {
	  fprintf(outfile, "*");
	}
	fprintf(outfile, ",");
	gen_walker_name(outfile, node);
	fprintf(outfile, ",");
	gen_access_expr(outfile, expr);
	fprintf(outfile, ");\n");
      }
      if (hoist_access_mults) {
	fprintf(outfile, "_DECL_INIT_HOISTED_MULTS_%dD(", numdims);
	arrdim = gen_pdt(outfile, T_TYPEINFO(ELIST_DATA(node)), PDT_LOCL);
	/* BLC: Not sure what to do here with arrdim... */
	fprintf(outfile, ",");
	gen_walker_name(outfile, node);
	fprintf(outfile, ",");
	gen_access_expr(outfile, expr);
	fprintf(outfile, ");\n");
      }
    }

    node = ELIST_NEXT(node);
  }
}
