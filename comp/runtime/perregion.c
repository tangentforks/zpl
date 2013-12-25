/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/***************************************************************\
* perregion.c is used to generate code on a per-region basis.   *
* It is chiefly responsible for dividing the regions amongst    *
* processors at runtime.                                        *
\***************************************************************/

#include <stdlib.h>
#include <string.h>
#include "../include/Mgen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/buildstmt.h"
#include "../include/buildsymutil.h"
#include "../include/buildzplstmt.h"
#include "../include/createvar.h"
#include "../include/datatype.h"
#include "../include/db.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/symbol.h"
#include "../include/symboltable.h"
#include "../include/typeinfo.h"


/* globals used to minimize the amount of parameter-passing necessary */
/* most of these store frequently-used symbol table values.           */

static void gen_glob_decls(FILE *outfile,symboltable_t* reg) {
  priv_tid_decl(outfile);
  /*** tid: required for any routine that accesses global data ***/
  priv_reg_alloc(outfile, reg);
  fprintf(outfile,"\n");
}


static void gen_loc_decls(FILE *outfile, char* regname, datatype_t* regdt) {
  if (!datatype_is_dense_reg(regdt)) {
    gen_sps_loc_decls(outfile,regname, regdt);
  }
    
  priv_tid_decl(outfile);
  /*** tid: required for any routine that accesses global data ***/
  fprintf(outfile,"\n");
}


static void gen_adjust(FILE* outfile, char* regname, 
		       expr_t* basereg, expr_t* dir, expr_t* initexpr) {
  fprintf(outfile,"_AdjustRegionWith");
  switch (T_SUBTYPE(initexpr)) {
  case IN_REGION:
    fprintf(outfile,"In");
    break;
  case OF_REGION:
    fprintf(outfile,"Of");
    break;
  case AT_REGION:
    fprintf(outfile,"At");
    break;
  case BY_REGION:
    fprintf(outfile,"By");
    break;
  default:
    INT_FATAL(NULL,"Unknown region type in gen_glob_prep_reg_info");
  }
  fprintf(outfile, "(");

  fprintf(outfile, regname);
  fprintf(outfile,",");
  gen_name(outfile, basereg);
  fprintf(outfile,",");
  gen_expr(outfile, dir);
  fprintf(outfile,");\n");
}


static void gen_glob_prep_reg_info(FILE *outfile, char* regname, 
				   expr_t* initexpr) {
  expr_t* basereg;
  expr_t* dir;
  symboltable_t* dirpst;

  basereg = T_OPLS(initexpr);

  dir = T_NEXT(T_OPLS(initexpr));
  typeinfo_expr(dir);
  dirpst = T_IDENT(dir);
  if (dirpst && (S_SETUP(dirpst) != 1)) {
    force_pst_init(outfile, dirpst);
  }
  gen_adjust(outfile, regname, basereg, dir, initexpr);
}


static void gen_reg_bound(FILE *outfile,expr_t *bound,int dim) {
  if (bound == NULL) {
    fprintf(outfile,"?");
  } else {
    gen_expr(outfile, bound);
  }
}


static void gen_lo_bound(FILE *outfile,dimension_t *dimptr,int dim) {
  expr_t *dimexpr;
  
  if (dimptr == NULL) {
    dimexpr = NULL;
  } else {
    dimexpr = DIM_LO(dimptr);
  }
  typeinfo_expr(dimexpr);
  gen_reg_bound(outfile,dimexpr,dim);
}


static void gen_hi_bound(FILE *outfile,dimension_t *dimptr,int dim) {
  expr_t *dimexpr;
  
  if (dimptr == NULL) {
    dimexpr = NULL;
  } else {
    dimexpr = DIM_HI(dimptr);
  }
  typeinfo_expr(dimexpr);
  gen_reg_bound(outfile,dimexpr,dim);
}


/* This routine generates code to setup a region's global information */

static int gen_glob_norm_reg_info(FILE *outfile, symboltable_t* reg,
				  char* regname, datatype_t* regdt, 
				  expr_t* init, int first) {
  int i;
  int flooddim;
  int rgriddim;
  int numdims = D_REG_NUM(regdt);
  int retval = 1;
  dimension_t* dimptr = NULL;

  if (init && T_TYPE(init) == SBE) {
    dimptr = T_SBE(init);
  }
  if (first) {
    if (use_default_distribution) {
      fprintf(outfile, "_REG_DIST(");
      fprintf(outfile, regname);
      fprintf(outfile, ") = _DefaultDistribution;\n");
    }
    else {
      if (D_REG_DIST(regdt) == NULL) {
	fprintf(outfile, "_REG_DIST(%s) = _DefaultDistribution;\n", regname);
      } else {
	if (D_CLASS(S_DTYPE(D_REG_DIST(regdt))) == DT_ENSEMBLE) {
	  fprintf(outfile, "_REG_DIST(%s) = _ARR_DIST(%s);\n", regname, S_IDENT(D_REG_DIST(regdt)));
	}
	else {
	  fprintf(outfile, "_REG_DIST(%s) = %s;\n", regname, S_IDENT(D_REG_DIST(regdt)));
	}
      }
    }
    fprintf(outfile,"_NUMDIMS(");
    fprintf(outfile, regname);
    fprintf(outfile,") = %d;\n",numdims);
  }

  if (init && T_TYPE(init) == SBE && dimptr == NULL) {
    fprintf(outfile, "/* NULL dimptr */\n");
    return 0;
  }

  if (init) {
    for (i=0;i<numdims;i++) {
      if (!dimptr) {
	fprintf(outfile, "_INHERIT_GLOB_BOUNDS(");
	fprintf(outfile, regname);
	fgenf(outfile, ",%E,%d);\n", init, i);
      } else {
	if (DIM_TYPE(dimptr) == DIM_INHERIT) {
	  fprintf(outfile, "_INHERIT_GLOB_BOUNDS(");
	  fprintf(outfile, regname);
	  fprintf(outfile, ",%s.%s,%d);\n", RMS, RMS_REG, i);
	} else if (DIM_TYPE(dimptr) == DIM_DYNAMIC) {
	  fprintf(outfile, "/* dimension %d dynamic */\n", i);
	  retval = 0;
	} else {
	  rgriddim = dimension_rgrid(dimptr);
	  flooddim = dimension_floodable(dimptr) || rgriddim;
	  
	  fprintf(outfile,"_SET_GLOB_BOUNDS");
	  if (flooddim) {
	    fprintf(outfile,"_FLOOD");
	  }
	  fprintf(outfile,"(");
	  fprintf(outfile, regname);
	  fprintf(outfile,",%d,",i);
	  if (flooddim) {
	    fprintf(outfile,"0,0,_DIM_");
	    if (rgriddim) {
	      fprintf(outfile,"RGRID");
	    } else {
	      fprintf(outfile,"FLOODED");
	    }
	  } else {
	    gen_lo_bound(outfile,dimptr,i);
	    fprintf(outfile,",");
	    gen_hi_bound(outfile,dimptr,i);
	    fprintf(outfile,",1");
	  }
	  fprintf(outfile,");\n");
	}
	dimptr=DIM_NEXT(dimptr);
      }
    }
  }
  return retval;
}


static int gen_glob_reg_info(FILE *outfile, char* regname, 
			      symboltable_t* reg, datatype_t* regdt, 
			      expr_t* initexpr, int first) {
  int retval = 0;

  if (!initexpr) {
    gen_glob_norm_reg_info(outfile, reg, regname, regdt, NULL, first);
    retval = 0;
  }
  else switch (T_TYPE(initexpr)) {
  case CONSTANT:
  case VARIABLE:
  case SBE:
    if (T_TYPE(initexpr) == SBE && 
	DIM_TYPE(T_SBE(initexpr)) == DIM_FLAT &&
	T_TYPE(DIM_LO(T_SBE(initexpr))) == SBE) {
      retval = gen_glob_reg_info(outfile, regname, reg, regdt, DIM_LO(T_SBE(initexpr)), first);
    }
    else {
      retval = gen_glob_norm_reg_info(outfile, reg, regname, regdt, initexpr, first);
    }
    break;
  case BIPREP:
    gen_glob_prep_reg_info(outfile, regname, initexpr);
    retval = 1;
    break;
  case BIWITH:
    typeinfo_expr(T_OPLS(initexpr));
    retval = gen_glob_reg_info(outfile, regname, reg, 
			       T_TYPEINFO(T_OPLS(initexpr)),
			       T_OPLS(initexpr), first);
    if (!datatype_is_dense_reg(regdt)) {
      fprintf(outfile," _SPS_MEMREQ(%s) = _%s_spsmemreqs;\n", regname, 
	      S_IDENT(reg));
    }
    break;
  default:
    INT_FATAL(NULL, "Unexpected case in gen_glob_reg_info");
  }

  return retval;
}


static void gen_expand_bounds(FILE *outfile,char* regname, 
			      datatype_t* regdt) {
  int numdims = D_REG_NUM(regdt);

  fprintf(outfile,"_EXPAND_DIST_BOUNDS(%s, %d);\n",regname, numdims);
}


static void gen_sparse_setup(FILE* outfile, char* regname, 
			     datatype_t* regdt) {
  fprintf(outfile,"_SPS_REGION(");
  fprintf(outfile, regname);
  fprintf(outfile,") = %d;\n",(datatype_is_dense_reg(regdt) == 0));
}


static void gen_basic_setup(FILE* outfile, char* regname) {
  fprintf(outfile,"_InitRegion(");
  fprintf(outfile, regname);
  fprintf(outfile,", 1);\n");
}


void GenGlobalSetup(FILE *outfile, 
		    symboltable_t* reg, datatype_t*regdt,
		    expr_t* initexpr, char* regname, int first) {
  int expand;

  if (datatype_reg_dynamicdim(regdt)) {
    return;
  }

  if (first && initexpr && (T_TYPE(initexpr) == BIPREP || T_TYPE(initexpr) == BIWITH)) {
    expr_t* basereg;
    symboltable_t* baseregpst;
    
    basereg = T_OPLS(initexpr);
    baseregpst = T_IDENT(basereg);
    if (baseregpst && (S_SETUP(baseregpst) == 0)) {
      force_pst_init(outfile, baseregpst);   /** is this necessary;  if so we need some equivalent for force_pst_done */
    }
  }

  fprintf(outfile, "{\n");
  gen_glob_decls(outfile,reg);

  gen_sparse_setup(outfile, regname, regdt);

  /* generate global stuff */
  expand = gen_glob_reg_info(outfile, regname, reg, regdt, initexpr, first);

  if (first && expand && S_LEVEL(reg) == 0) {
    /* expand propagated bounds as necessary */
    gen_expand_bounds(outfile,regname, regdt);
  }

  if (first) fprintf(outfile,"_REG_LABEL(%s) = \"%s\";\n", regname, regname);
  if (first) fgenf(outfile, "_REG_ARRLIST(%s) = NULL;\n", regname);
  if (first) fprintf(outfile, "_DIST_REGISTER(_REG_DIST(%s), %s);\n", regname, regname);

  if (initexpr && T_TYPE(initexpr) != BIPREP) {
    fprintf(outfile,"_REG_PREP(%s) = _NONE;\n", regname);
  }

  fprintf(outfile, "_REG_CLEAR_FLAG(%s);\n", regname);
  if (initexpr) {
    fprintf(outfile, "_REG_SET_SETUP_ON(%s);\n", regname);
  }

  fprintf(outfile,"}\n\n");
}


void GenLocalSetup(FILE *outfile, symboltable_t* reg,
		   datatype_t* regdt, expr_t* initexpr, char* regname, int first) {

  if (datatype_reg_dynamicdim(regdt) || !initexpr) {
    return;
  }
			  
  if (first && (T_TYPE(initexpr) == BIPREP || T_TYPE(initexpr) == BIWITH)) {
    expr_t* basereg;
    symboltable_t* baseregpst;
    
    basereg = T_OPLS(initexpr);
    baseregpst = T_IDENT(basereg);
    if (baseregpst && (S_SETUP(baseregpst) == 0)) {
      force_pst_finalize(outfile, baseregpst);
    }
  }
  fprintf(outfile, "{\n");
  gen_loc_decls(outfile, regname, regdt);

  gen_basic_setup(outfile, regname);

  if (!datatype_is_dense_reg(regdt)) {
    gen_sparse_fluff(outfile, regname, reg, regdt);
    gen_sparse_structure(outfile, reg, regname, regdt, initexpr);
  }

  fprintf(outfile,"}\n\n");
}

