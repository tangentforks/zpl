/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#define DGEN_DECL
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Privgen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/datatype.h"
#include "../include/dtype.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/glist.h"
#include "../include/global.h"
#include "../include/init.h"
#include "../include/macros.h"
#include "../include/parsetree.h"
#include "../include/runtime.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"

#define VAR_CONST      0 
#define CONSTVAR_CONST 1
#define MACRO_CONST    2


#define REG_SETUP_GLOBAL 0
#define REG_SETUP_LOCAL 1


/* globals from global.h */

int printing_string = 0;

int retpt_used;


static int GLOBAL_MLOOP = 0;  /* silly hack that allows us to use LINEINFO */


static FILE* initfile = NULL;

void StartDgen() {
  char filename[FILEPATHLEN];

  sprintf(filename, "%s%s_%s", DESTPATH, trunc_in_filename, INITCODEFILE);
  if ((initfile=fopen(filename, "w")) == NULL) {
    USR_FATAL(NULL, "Cannot open file '%s'", filename);
  }
  fprintf(initfile, "static void _initAllGlobals() {\n");
  fprintf(initfile, "_SetupDistribution(_DefaultDistribution, _DefaultGrid, \"Implicit Distribution\");\n");
  fprintf(initfile, "  _MAXMINIMIZE_DIST(_DefaultDistribution);\n");
  fprintf(initfile, "  _DIST_SET_INIT_OFF(_DefaultDistribution);\n");

}


void EndDgen() {
  fprintf(initfile, "}\n\n");
}


void GenDistributionSetup(FILE* outfile, symboltable_t* pst,
			  datatype_t* pdt, char* name, expr_t* init, int first) {
  dimension_t* pdTemp;
  int i;
  expr_t* expr;

  if (first) {
    fprintf(outfile, "_SetupDistribution(%s, %s, \"%s\");\n",
	    name, 
	    D_DIST_GRID(pdt) ? S_IDENT(D_DIST_GRID(pdt)) : "_DefaultGrid",
	    name);
  }
  if (!init) {
    fprintf(outfile, "_DIST_SET_INIT_OFF(%s);\n", name);
    return;
  }
  if (T_TYPE(init) == SBE) {
    pdTemp = T_SBE(init);
    for (i = 0; i < D_DIST_NUM(pdt); i++) {
      if (T_TYPE(DIM_LO(pdTemp)) != FUNCTION) {
	USR_FATAL(NULL, "Malformed Distribution Function");
      }
      expr = T_OPLS(DIM_LO(pdTemp));
      fprintf(outfile, "_InitDistributionDimdist(%s, %d, %s);\n", name, i, S_IDENT(T_IDENT(expr)));
      fprintf(outfile, "_%sSetup(%s, %d", S_IDENT(T_IDENT(expr)), name, i);
      while (T_NEXT(expr)) {
	fgenf(outfile, ", %E", T_NEXT(expr));
	expr = T_NEXT(expr);
      }
      fprintf(outfile, ");\n");
      pdTemp = DIM_NEXT(pdTemp);
    }
    fprintf(outfile, "_InitDistribution(%s, 1);\n", name); 
  }
  else if (T_IDENT(init) && D_CLASS(S_DTYPE(T_IDENT(init))) == DT_DISTRIBUTION) {
    fgenf(outfile, "_CopyDistribution(%s, %E, 1);\n", name, init);
  }
  else {
    USR_FATAL(T_STMT(init), "Malformed Distribution Expression");
  }
}

void GenGridSetup(FILE* outfile, symboltable_t* pst,
		  datatype_t* pdt, char* name, expr_t* init, int first) {
  dimension_t* pdTemp;
  int i;

  if (init) {
    fprintf(outfile, "  {\n");
    fprintf(outfile, "    _DECL_ARR(int, gridinit, %d);\n", D_GRID_NUM(pdt));
    fprintf(outfile, "    _INIT_%d_ELTS(gridinit, ", D_GRID_NUM(pdt));
    if (T_TYPE(init) == SBE) {
      pdTemp = T_SBE(init);
      for (i=0; i<D_GRID_NUM(pdt); i++) {
	if (i) {
	  fprintf(outfile, ", ");
	}
	if (DIM_LO(pdTemp)) {
	  gen_expr(outfile, DIM_LO(pdTemp));
	}
	else {
	  fgenf(outfile, "0");
	}
	pdTemp = DIM_NEXT(pdTemp);
      }
    }
    else if (T_IDENT(init) && D_CLASS(S_DTYPE(T_IDENT(init))) == DT_GRID) {
      for (i=0; i < D_GRID_NUM(pdt); i++) {
	if (i) {
	  fprintf(outfile, ", ");
	}
	fgenf(outfile, "_GRID_SIZE(%E, %d)", init, i);
      }
    }
    else {
      USR_FATAL(T_STMT(init), "Malformed Grid Expression");
    }
    fprintf(outfile, ");\n");
    if (first) {
      fprintf(outfile, "    _SetupGrid(%s, \"%s\", gridinit, %d);\n", name, name, D_GRID_NUM(pdt));
    }
    fprintf(outfile, "    _InitGrid(%s, 1);\n", name);
    fprintf(outfile, "  }\n");
  }
  else {
    if (!first) {
      INT_FATAL(NULL, "No initialization and Not first setup?");
    }
    fprintf(outfile, "_SetupGrid(%s, \"%s\", NULL, %d);\n", name, name, D_GRID_NUM(pdt));
  }
}


static void gen_reg_setup(FILE* outfile, symboltable_t* reg, char* name, 
			  int globloc, datatype_t* regdt, initial_t* init) {
  expr_t* initexpr = init ? IN_VAL(init) : NULL;

  if (S_DTYPE(reg) == NULL || S_SETUP(reg) != 1) {
    return;
  }

  if (globloc == REG_SETUP_GLOBAL) {
    GenGlobalSetup(outfile, reg, regdt, initexpr, name, 1);
  } else {
    GenLocalSetup(outfile, reg, regdt, initexpr, name, 1);
  }
}


void gen_function_header(FILE* outfile, symboltable_t* pst, int is_prototype) {
  genlist_t *glp;
  dimension_t* pdTemp;

  gen_psc(outfile, S_STYPE(pst));
  /*** output subprogram declaration ... ***/
  pdTemp = gen_pdt(outfile,S_FUN_TYPE(pst),PDT_RVAL);
  fprintf(outfile, "%s(", S_IDENT(pst));
  glp = T_PARAMLS(S_FUN_BODY(pst));
  /*** ... and parameter type declarations ***/
  if (glp == NULL) {
    fprintf(outfile, "void");
  }
  else while (glp != NULL) {
    if (S_CLASS(G_IDENT(glp)) == S_PARAMETER) {
      gen_pst(outfile,G_IDENT(glp));
    }
    if ((glp = G_NEXT(glp)) != NULL) {
      fprintf(outfile, ",");
    }
  }
  fprintf(outfile, is_prototype ? ");\n" : ") {\n");
}


static void gen_hoisted_vars(FILE* outfile, symboltable_t* pst) {
  genlist_t *glp;

  if (hoist_access_ptrs == HOIST_PTRS_TO_SUBPROG) {
    glp = T_PARAMLS(S_FUN_BODY(pst));
    while (glp != NULL) {
      symboltable_t* pst = G_IDENT(glp);
      if (S_CLASS(pst) == S_PARAMETER &&
          D_CLASS(S_DTYPE(pst)) == DT_ENSEMBLE) {
	dimension_t* arrdim;

	if (strcmp(S_IDENT(pst),"_LHS_ens") == 0) {
	  fprintf(outfile, "_SAFE");
	}
	fprintf(outfile, "_DECL_INIT_HOISTED_PTRS(");
        arrdim = gen_pdt(outfile, D_ENS_TYPE(S_DTYPE(pst)), PDT_GLOB);
	if (arrdim) {
	  fprintf(outfile, "*");
	}
	fprintf(outfile, ", %s, %s);\n", S_IDENT(pst), S_IDENT(pst));
      }
      glp = G_NEXT(glp);
    }

  }
}


void gen_psc(FILE* outfile, int sc) {
  DB0(20, "enter gen_psc\n");
  if (sc & SC_STATIC)
    fprintf(outfile, "static ");
  if (sc & SC_EXTERN)
    fprintf(outfile, "extern ");
  if (sc & SC_REGISTER)
    fprintf(outfile, "register ");
  DB0(20, "exit gen_psc\n");
}


/* gen_array_size() generates array dimensions for a single array
 (possibly part of a nested multidimensional array...) */

void gen_array_size(FILE* outfile, dimension_t* pdTemp,datatype_t* pdt) {
  dimension_t* tmp;
  int dynamic = 0;
  int size;

  tmp = pdTemp;
  while (tmp != NULL) {
    if ((!expr_computable_const(DIM_LO(tmp)) ||
	 !expr_computable_const(DIM_HI(tmp)))) {
      dynamic = 1;
    }
    tmp = DIM_NEXT(tmp);
  }
  if (dynamic) {
    fprintf(outfile, "[]");
  }
  else {
    tmp = pdTemp;
    size = 1;
    while (tmp != NULL) {
      size *= expr_intval(DIM_HI(tmp)) - expr_intval(DIM_LO(tmp)) + 1;
      tmp = DIM_NEXT(tmp);
    }
    fprintf(outfile, "[%d]", size);
  }
}

/* gen_array_sizes() generates array dimensions for arrays of
   arrays... until it reaches a named type */

void gen_array_sizes(FILE* outfile, dimension_t * pdTemp,datatype_t * pdt) {
  while (pdTemp != NULL) {
    gen_array_size(outfile, pdTemp,pdt);

    if (D_CLASS(D_ARR_TYPE(pdt)) == DT_ARRAY && D_NAME(D_ARR_TYPE(pdt))==NULL) {
      pdt = D_ARR_TYPE(pdt);
      pdTemp = D_ARR_DIM(pdt);
    } else {
      pdTemp = NULL;
    }
  }
}

static void gen_array_size_for_parameter1(FILE* outfile, dimension_t* pdTemp,datatype_t* pdt) {
  dimension_t* tmp;
  int dynamic = 0;

  tmp = pdTemp;
  while (tmp != NULL) {
    if ((!expr_computable_const(DIM_LO(tmp)) ||
	 !expr_computable_const(DIM_HI(tmp)))) {
      dynamic = 1;
    }
    tmp = DIM_NEXT(tmp);
  }
  if (dynamic) {
    fprintf(outfile, " * restrict ");
  }
}

/* gen_array_sizes() generates array dimensions for arrays of
   arrays... until it reaches a named type */

static void gen_array_sizes_for_parameter1(FILE* outfile, dimension_t * pdTemp,datatype_t * pdt) {
  while (pdTemp != NULL) {
    gen_array_size_for_parameter1(outfile, pdTemp,pdt);

    if (D_CLASS(D_ARR_TYPE(pdt)) == DT_ARRAY && D_NAME(D_ARR_TYPE(pdt))==NULL) {
      pdt = D_ARR_TYPE(pdt);
      pdTemp = D_ARR_DIM(pdt);
    } else {
      pdTemp = NULL;
    }
  }
}

static void gen_array_size_for_parameter2(FILE* outfile, dimension_t* pdTemp,datatype_t* pdt) {
  dimension_t* tmp;
  int dynamic = 0;
  int size;

  tmp = pdTemp;
  while (tmp != NULL) {
    if ((!expr_computable_const(DIM_LO(tmp)) ||
	 !expr_computable_const(DIM_HI(tmp)))) {
      dynamic = 1;
    }
    tmp = DIM_NEXT(tmp);
  }
  if (!dynamic) {
    tmp = pdTemp;
    size = 1;
    while (tmp != NULL) {
      size *= expr_intval(DIM_HI(tmp)) - expr_intval(DIM_LO(tmp)) + 1;
      tmp = DIM_NEXT(tmp);
    }
    fprintf(outfile, "[%d]", size);
  }
}

/* gen_array_sizes() generates array dimensions for arrays of
   arrays... until it reaches a named type */

static void gen_array_sizes_for_parameter2(FILE* outfile, dimension_t * pdTemp,datatype_t * pdt) {
  while (pdTemp != NULL) {
    gen_array_size_for_parameter2(outfile, pdTemp,pdt);

    if (D_CLASS(D_ARR_TYPE(pdt)) == DT_ARRAY && D_NAME(D_ARR_TYPE(pdt))==NULL) {
      pdt = D_ARR_TYPE(pdt);
      pdTemp = D_ARR_DIM(pdt);
    } else {
      pdTemp = NULL;
    }
  }
}

static void gen_dynarray_alloc(FILE* outfile, symboltable_t* pst,
			       typeclass doduals) {
  dimension_t* pdTemp = D_ARR_DIM(S_DTYPE(pst));
  datatype_t* pdt = S_DTYPE(pst);

  if (S_LEVEL(pst) == 0) {
    outfile = initfile;
  }
  if (doduals) {
    fprintf(outfile, "  _D_%s", S_IDENT(pst));
  } else {
    if (S_LEVEL(pst) == 0) {
      fprintf(outfile, "  %s", S_IDENT(pst));
    }
  }
  
  fprintf(outfile, " = _zmalloc((");
  gen_expr(outfile, build_array_size_multiplier(pdTemp));
  fprintf(outfile,") * sizeof(");
  if (doduals) {
    switch (D_CLASS(D_ARR_TYPE(pdt))) {
    case DT_GRID:
      fprintf(outfile,"_grid_info");
      break;
    case DT_DISTRIBUTION:
      fprintf(outfile,"_dist_info");
      break;
    case DT_REGION:
      fprintf(outfile, "_reg_info");
      break;
    case DT_ENSEMBLE:
      fprintf(outfile,"_arr_info");
      break;
    default:
      INT_FATAL(NULL, "Unexpected type in gen_dynarray_alloc");
    }
  } else {
    gen_pdt(outfile, D_ARR_TYPE(pdt), PDT_CAST);
  }
  fprintf(outfile,"), \"user dynamic array\")");

  if (outfile == initfile || doduals) {
    fprintf(outfile, ";\n");
  }
  if (doduals) {
    if (S_LEVEL(pst) == 1) {
      fprintf(outfile, "  %s", S_IDENT(pst));
    }
    gen_dynarray_alloc(outfile, pst, 0);
  }
}


static void gen_hoisted_ptr_allocs(FILE* outfile, symboltable_t* pst,
				   typeclass doduals) {
  dimension_t* pdTemp = D_ARR_DIM(S_DTYPE(pst));
  datatype_t* pdt = S_DTYPE(pst);

  fprintf(outfile, "_ACCESS_BASE(%s) = _zmalloc(((", S_IDENT(pst));
  gen_expr(outfile, DIM_HI(pdTemp));
  fprintf(outfile,") - (");
  gen_expr(outfile, DIM_LO(pdTemp));
  fprintf(outfile,") + 1) * sizeof(");
  gen_pdt(outfile, datatype_base(pdt), PDT_CAST);
  fprintf(outfile, "*), \"hoisted pointers\")");
  fprintf(outfile,";\n");

  fprintf(outfile, "_F_ACCESS_BASE(%s) = _zmalloc(((", S_IDENT(pst));
  gen_expr(outfile, DIM_HI(pdTemp));
  fprintf(outfile,") - (");
  gen_expr(outfile, DIM_LO(pdTemp));
  fprintf(outfile,") + 1) * sizeof(");
  gen_pdt(outfile, datatype_base(pdt), PDT_CAST);
  fprintf(outfile, "*), \"hoisted pointers\")");
  fprintf(outfile,";\n");
}


static void gen_constant_val(FILE* outfile, expr_t *init_expr) {
  double dblval;
  long lngval;
  unsigned long ulngval;

  typeinfo_expr(init_expr);
  switch (D_CLASS(T_TYPEINFO(init_expr))) {
  case DT_BOOLEAN:
  case DT_SIGNED_BYTE:
  case DT_SHORT:
  case DT_INTEGER:
  case DT_LONG:
    lngval = expr_intval(init_expr);
    fprintf(outfile,"%ld",lngval);
    break;
  case DT_REAL:
  case DT_DOUBLE:
  case DT_QUAD:
    {
      char ctrl[16];
      char val[128];
      
      dblval = expr_doubleval(init_expr);
      sprintf(ctrl,"%%.%dg",DBL_DIG+6);
      fprintf(outfile,ctrl,dblval);
      sprintf(val,ctrl,dblval);
      if (strchr(val,'.') == NULL && strchr(val,'e') == NULL) {
	fprintf(outfile,".0");
      }
    }
    break;
  case DT_UNSIGNED_BYTE:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_LONG:
    ulngval = expr_uintval(init_expr);
    fprintf(outfile,"%lu",ulngval);
    break;
  default:
    USR_FATAL(NULL,"Unexpected constant type -- try compiling -noconstopt\n"
	      "Please contact zpl-info@cs.washington.edu");
    break;
  }
}


static int expr_worth_optimizing(expr_t* expr) {
  switch (T_TYPE(expr)) {
  case NULLEXPR:
  case VARIABLE:
  case CONSTANT:
    return 0;
  case UNEGATIVE:
  case UPOSITIVE:
    return expr_worth_optimizing(T_OPLS(expr));
  default:
    return 1;
  }
}


void gen_init_list(FILE* outfile, initial_t *p) {
  initial_t *pit;
  expr_t *init_expr;

  pit = p;
  while (pit != NULL) {
    if (IN_BRA(pit)) {
      fprintf(outfile, "{");
      gen_init_list(outfile, IN_LIS(pit));
      fprintf(outfile, "}");
    } else {
      init_expr = IN_VAL(pit);
      if (T_TYPE(init_expr) == CONSTANT) {
	gen_expr(outfile, init_expr);
      } else if (const_opt && expr_computable_const(init_expr) && 
		 expr_worth_optimizing(init_expr)) {
	gen_constant_val(outfile, init_expr);
      } else {
	typeinfo_expr(init_expr);
	gen_expr(outfile, init_expr);
      }
    }
    if (S_NEXT(pit) != NULL)
      fprintf(outfile, ",");
    pit = S_NEXT(pit);
  }
}


static void gen_init_semi(FILE* outfile, int depth) {
  if (depth > 1 || outfile == initfile) {
    fprintf(outfile, ";\n");
  }
}


void gen_name(FILE* outfile, expr_t* expr) {
  symboltable_t* pst;

  pst = T_IDENT(expr);
  if (pst != NULL) {
    if (symtab_is_qreg(pst)) {
      fprintf(outfile, "_CurrentRegion");
    } else {
      fprintf(outfile, "%s", S_IDENT(T_IDENT(expr)));
    }
  } else {
    switch (T_TYPE(expr)) {
    case BIPREP:
      switch (T_SUBTYPE(expr)) {
      case OF_REGION:
      case IN_REGION:
	fprintf(outfile, "_");
	fprintf(outfile, "dir");
	/* This is unsatisfying, see note below
	gen_name(outfile, T_NEXT(T_OPLS(expr)));
	*/
	if (T_SUBTYPE(expr) == OF_REGION) {
	  fprintf(outfile, "_of_");
	} else {
	  fprintf(outfile, "_in_");
	}
	gen_name(outfile, T_OPLS(expr));
	break;
      case BY_REGION:
      case AT_REGION:
	fprintf(outfile, "_");
	gen_name(outfile, T_OPLS(expr));
	if (T_SUBTYPE(expr) == BY_REGION) {
	  fprintf(outfile, "_by_");
	} else {
	  fprintf(outfile, "_at_");
	}
	fprintf(outfile, "dir");
	/* This is unsatisfying, see note below
	gen_name(outfile, T_NEXT(T_OPLS(expr)));
	*/
	break;
      default:
	INT_FATAL(NULL, "Unexpected BIPREP in gen_name");
      }
      break;
    case BIWITH:
      gen_name(outfile, T_OPLS(expr));
      break;
    case ARRAY_REF:
      /* BLC: PLACEHOLDER: Problem: when this is gen_expr, it gens
	 []'s in names for "reg at dir[i]".  When it's name, it
	 gens garbage when used to initialize RMStack.reg. */
      gen_expr(outfile, expr);
      /*
      gen_name(outfile, T_OPLS(expr));
      */
      break;
    default:
      INT_FATAL(NULL, "Unexpected expression type in gen_name");
    }
  }
}


/* This new implementation of initialization generation is set up to
   handle arrays of records, directions, grids, distributions, etc. in a
   recusive manner rather than an iterative manner as the old one
   seems to.  It isn't used everywhere yet, but should be soon, at
   which point gen_pstinit() probably should go away (and the "new"
   shoudl be removed from the following routines names. */

static void gen_pst_init_rec(FILE* outfile, char* name, 
			     symboltable_t* pst, datatype_t* pdt,
			     initial_t* init, int depth) {
  char newname[1024];
  symboltable_t* indpst[MAXRANK];
  int numarrdims = D_ARR_NUM(pdt);
  int i;
  dimension_t* pdTemp;

  if (D_NAME(pdt) && !strcmp(S_IDENT(D_NAME(pdt)), "timer")) {
    fprintf(outfile, "ClearTimer(&%s);\n", name);
  }
  switch (D_CLASS(pdt)) {
  case DT_GRID:
    if (S_VAR_INIT(pst)) {
      GenGridSetup(outfile, pst, pdt, name, IN_VAL(S_VAR_INIT(pst)), 1);
    }
    else {
      GenGridSetup(outfile, pst, pdt, name, NULL, 1);
    }
    break;
  case DT_DISTRIBUTION:
    if (S_VAR_INIT(pst)) {
      GenDistributionSetup(outfile, pst, pdt, name, IN_VAL(S_VAR_INIT(pst)), 1);
    }
    else {
      GenDistributionSetup(outfile, pst, pdt, name, NULL, 1);
    }
    break;
  case DT_DIRECTION:
    for (i=0; i<D_DIR_NUM(pdt); i++) {
      fprintf(outfile, "%s[%d] = ", name, i);
      gen_expr(outfile, expr_direction_value(IN_VAL(init), i));
      fprintf(outfile, ";\n");
    }
    break;
  case DT_REGION:
    if (depth > 1) {
      fprintf(outfile, " %s = &(_D_%s);\n", name, name);
    }
    gen_reg_setup(outfile, pst, name, REG_SETUP_GLOBAL, pdt, init);
    if (datatype_is_dense_reg(pdt)) {
      gen_reg_setup(outfile, pst, name, REG_SETUP_LOCAL, pdt, init);
    }
    break;
  case DT_ENSEMBLE:
    if (depth > 1) {
      fprintf(outfile, " %s = &(_D_%s);\n", name, name);
    }
    if (

	/* still do sparse parallel array init in finalize stage */
	expr_is_dense_reg(D_ENS_REG(pdt)) &&

	/* this makes sure Steve's ?-dimensions in FT don't have their allocations generated prematurely */
	!datatype_reg_dynamicdim(T_TYPEINFO(D_ENS_REG(pdt))) && 

	/* also handle unregistered arrays like temp arrays in finalize stage */
	!D_ENS_DNR(pdt)) {
      GenAllocEnsemble(outfile, pst, name, pdt, 0, (depth == 1));
    }
    break;
  case DT_ARRAY:
    if (!(init ||
	  datatype_find_dtclass(pdt, DT_GRID) ||
	  datatype_find_dtclass(pdt, DT_DIRECTION) ||
	  datatype_find_dtclass(pdt, DT_REGION) ||
	  datatype_find_dtclass(pdt, DT_ENSEMBLE))) {
      break;
    }
    for (i=0; i<numarrdims; i++) {
      indpst[i] = get_blank_arrayref_index(depth, i+1);
    }
    fprintf(outfile, "  {\n");
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    int %s;\n", S_IDENT(indpst[i]));
    }
    pdTemp = D_ARR_DIM(pdt);
    sprintf(newname, "%s[", name);
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    for (%s=", S_IDENT(indpst[i]));
      gen_expr(outfile, DIM_LO(pdTemp));
      fprintf(outfile, "; %s<=", S_IDENT(indpst[i]));
      gen_expr(outfile, DIM_HI(pdTemp));
      fprintf(outfile, "; %s++) {\n", S_IDENT(indpst[i]));
      fprintf(outfile, "      int _lo%d_%d = ", depth, i+1);
      gen_expr(outfile, DIM_LO(pdTemp));
      fprintf(outfile, ";\n");
      sprintf(newname, "%s(%s-_lo%d_%d)", newname, S_IDENT(indpst[i]),depth, i+1);
      if (DIM_NEXT(pdTemp)) {
	sprintf(newname, "%s*(%d)+", newname, get_array_size_multiplier(DIM_NEXT(pdTemp)));
      }
      pdTemp = DIM_NEXT(pdTemp);
    }
    sprintf(newname, "%s]", newname);
    gen_pst_init_rec(outfile, newname, pst, D_ARR_TYPE(pdt), init, depth+1);
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    }\n");
    }
    fprintf(outfile, "  }\n");
    break;
  case DT_STRUCTURE:
    {
      symboltable_t* fieldpst = D_STRUCT(pdt);
      while (fieldpst != NULL) {
	sprintf(newname, "%s.%s", name, S_IDENT(fieldpst));
	gen_pst_init_rec(outfile, newname, fieldpst, S_DTYPE(fieldpst), init, 
			 depth+1);

        fieldpst = S_SIBLING(fieldpst);
      }
    }
    break;
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
    if (init) {
      initial_t* init2;
      if (!IN_BRA(init)) {
	USR_FATAL(NULL,"Complex initializers must be of the form {val,val}");
      }
      init = IN_LIS(init);
      init2 = S_NEXT(init);
      S_NEXT(init) = NULL;
      if (IN_BRA(init2) || S_NEXT(init2)) {
	USR_FATAL(NULL,"Complex initializers must be of the form {val,val}");
      }
      fprintf(outfile, "  %s.re = ", name);
      gen_init_list(outfile, init);
      fprintf(outfile, ";\n");
      fprintf(outfile, "  %s.im = ", name);
      gen_init_list(outfile, init2);
      S_NEXT(init) = init2;
      gen_init_semi(outfile, depth);
    }
    break;
  case DT_STRING:
    if (init != NULL) {
      fprintf(outfile, "  strcpy(%s, ", name);
      gen_init_list(outfile, init);
      fprintf(outfile, ")");
      gen_init_semi(outfile, depth);
    }
    break;
  default:
    if (init != NULL) {
      if (depth > 1 || outfile == initfile) {
	fprintf(outfile, "  %s", name);
      }
      fprintf(outfile, " = ");
      gen_init_list(outfile, init);
      gen_init_semi(outfile, depth);
    }
    break;
  }
}


/* The following routines generate "finalizing" code for symboltable
   pointers using a recusrive strategy similar to
   gen_pst_init_rec().  For distributions, this means calculating the
   distribution; for regions it means calculating local bounds; for
   ensembles it currently means setting the whole thing up.
   Eventually, only distributions should need finalizing, and the rest
   should ripple from its distribution's finalization.  Or perhaps even
   distributions don't need finalizing; rather, they finalize automatically
   when their last region expands its bounding box.  -BLC */
/* Sparse regions will probably need finalizing for a long time. -SJD */
/* Parallel non-sparse arrays no longer use this. 6/5/04. -SJD */
static void gen_pst_final_rec(FILE* outfile, char* name, 
			      symboltable_t* pst, datatype_t* pdt,
			      initial_t* init, int depth) {
  char newname[1024];
  symboltable_t* indpst[MAXRANK];
  int numarrdims = D_ARR_NUM(pdt);
  int i;
  dimension_t* pdTemp;

  switch (D_CLASS(pdt)) {
  case DT_REGION:
    if (!datatype_is_dense_reg(pdt)) {
      gen_reg_setup(outfile, pst, name, REG_SETUP_LOCAL, pdt, init);
    }
    break;
  case DT_ENSEMBLE:
    if (!expr_is_dense_reg(D_ENS_REG(pdt)) ||
	D_ENS_DNR(pdt)) {
      GenAllocEnsemble(outfile, pst, name, pdt, 0, (depth == 1));
    }
    break;
  case DT_ARRAY:
    if (!(datatype_find_dtclass(pdt, DT_GRID) ||
	  datatype_find_dtclass(pdt, DT_DIRECTION) ||
	  datatype_find_dtclass(pdt, DT_REGION) ||
	  datatype_find_dtclass(pdt, DT_ENSEMBLE))) {
      break;
    }
    for (i=0; i<numarrdims; i++) {
      indpst[i] = get_blank_arrayref_index(depth, i+1);
    }
    fprintf(outfile, "  {\n");
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    int %s;\n", S_IDENT(indpst[i]));
    }
    pdTemp = D_ARR_DIM(pdt);
    sprintf(newname, "%s[", name);
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    for (%s=", S_IDENT(indpst[i]));
      gen_expr(outfile, DIM_LO(pdTemp));
      fprintf(outfile, "; %s<=", S_IDENT(indpst[i]));
      gen_expr(outfile, DIM_HI(pdTemp));
      fprintf(outfile, "; %s++) {\n", S_IDENT(indpst[i]));
      fprintf(outfile, "      int _lo%d_%d = ", depth, i+1);
      gen_expr(outfile, DIM_LO(pdTemp));
      fprintf(outfile, ";\n");
      sprintf(newname, "%s(%s-_lo%d_%d)", newname, S_IDENT(indpst[i]),depth, i+1);
      if (DIM_NEXT(pdTemp)) {
	sprintf(newname, "%s*(%d)+", newname, get_array_size_multiplier(DIM_NEXT(pdTemp)));
      }
      pdTemp = DIM_NEXT(pdTemp);
    }
    sprintf(newname, "%s]", newname);
    gen_pst_final_rec(outfile, newname, pst, D_ARR_TYPE(pdt), init, depth+1);
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    }\n");
    }
    fprintf(outfile, "  }\n");
    break;
  case DT_STRUCTURE:
    {
      symboltable_t* fieldpst = D_STRUCT(pdt);
      while (fieldpst != NULL) {
	sprintf(newname, "%s.%s", name, S_IDENT(fieldpst));
	gen_pst_final_rec(outfile, newname, fieldpst, S_DTYPE(fieldpst),
			  init, depth+1);

        fieldpst = S_SIBLING(fieldpst);
      }
    }
    break;
  default:
    break;
  }
}


static void gen_pst_done_rec(FILE* outfile, char* name, 
			     symboltable_t* pst, datatype_t* pdt,
			     initial_t* init, int depth) {
  char newname[1024];
  symboltable_t* indpst[MAXRANK];
  int numarrdims = D_ARR_NUM(pdt);
  int i;
  dimension_t* pdTemp;

  switch (D_CLASS(pdt)) {
  case DT_REGION:
    if (!datatype_reg_dynamicdim(pdt)) {  /** Guard against ZALLOC ? arrays */
      fprintf(outfile, "_DIST_UNREGISTER(_REG_DIST(%s), %s);\n", name, name);
    }
    break;
  case DT_ENSEMBLE:
    if (!datatype_reg_dynamicdim(T_TYPEINFO(D_ENS_REG(pdt)))) {  /** Guard against ZALLOC ? arrays */
      GenFreeEnsemble(outfile, pst, name, pdt, 0, (depth == 1));
    }
    break;
  case DT_ARRAY:
    if (!(datatype_find_dtclass(pdt, DT_GRID) ||
	  datatype_find_dtclass(pdt, DT_DIRECTION) ||
	  datatype_find_dtclass(pdt, DT_REGION) ||
	  datatype_find_dtclass(pdt, DT_ENSEMBLE))) {
      break;
    }
    for (i=0; i<numarrdims; i++) {
      indpst[i] = get_blank_arrayref_index(depth, i+1);
    }
    fprintf(outfile, "  {\n");
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    int %s;\n", S_IDENT(indpst[i]));
    }
    pdTemp = D_ARR_DIM(pdt);
    sprintf(newname, "%s[", name);
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    for (%s=", S_IDENT(indpst[i]));
      gen_expr(outfile, DIM_LO(pdTemp));
      fprintf(outfile, "; %s<=", S_IDENT(indpst[i]));
      gen_expr(outfile, DIM_HI(pdTemp));
      fprintf(outfile, "; %s++) {\n", S_IDENT(indpst[i]));
      fprintf(outfile, "      int _lo%d_%d = ", depth, i+1);
      gen_expr(outfile, DIM_LO(pdTemp));
      fprintf(outfile, ";\n");
      sprintf(newname, "%s(%s-_lo%d_%d)", newname, S_IDENT(indpst[i]),depth, i+1);
      if (DIM_NEXT(pdTemp)) {
	sprintf(newname, "%s*(%d)+", newname, get_array_size_multiplier(DIM_NEXT(pdTemp)));
      }
      pdTemp = DIM_NEXT(pdTemp);
    }
    sprintf(newname, "%s]", newname);
    gen_pst_done_rec(outfile, newname, pst, D_ARR_TYPE(pdt), init, depth+1);
    for (i=0; i<numarrdims; i++) {
      fprintf(outfile, "    }\n");
    }
    fprintf(outfile, "  }\n");
    break;
  case DT_STRUCTURE:
    {
      symboltable_t* fieldpst = D_STRUCT(pdt);
      while (fieldpst != NULL) {
	sprintf(newname, "%s.%s", name, S_IDENT(fieldpst));
	gen_pst_done_rec(outfile, newname, fieldpst, S_DTYPE(fieldpst),
			  init, depth+1);

        fieldpst = S_SIBLING(fieldpst);
      }
    }
    break;
  default:
    break;
  }
}
  

void gen_pst_init(FILE* outfile, symboltable_t* pst) {
  char name[1024];

  if (S_SETUP(pst) != 1) {
    return;
  }

  sprintf(name, "%s", S_IDENT(pst));
  gen_pst_init_rec(outfile, name, pst, S_DTYPE(pst), S_VAR_INIT(pst), 1);
}


void force_pst_init(FILE* outfile, symboltable_t* pst) {
  int save_setup = S_SETUP(pst);

  S_SETUP(pst) = 1;
  gen_pst_init(outfile, pst);
  S_SETUP(pst) = save_setup;
}


void gen_pst_finalize(FILE* outfile, symboltable_t* pst) {
  char name[1024];

  if (S_STD_CONTEXT(pst) == TRUE || S_CLASS(pst) == S_TYPE || 
      !S_IS_USED(pst) || (S_STYPE(pst) & SC_CONFIG) || (S_SETUP(pst) != 1)) {
    return;
  }
  sprintf(name, "%s", S_IDENT(pst));
  gen_pst_final_rec(outfile, name, pst, S_DTYPE(pst), S_VAR_INIT(pst), 1);
}


void force_pst_finalize(FILE* outfile, symboltable_t* pst) {
  int save_setup = S_SETUP(pst);
  
  S_SETUP(pst) = 1;
  gen_pst_finalize(outfile, pst);
  S_SETUP(pst) = save_setup;
}


void gen_pst_done(FILE* outfile, symboltable_t* pst) {
  char name[1024];

  if (S_STD_CONTEXT(pst) == TRUE ||
      S_CLASS(pst) == S_TYPE ||
      !S_IS_USED(pst) ||
      (S_STYPE(pst) & SC_EXTERN) || 
      (S_SETUP(pst) != 1)) {
    return;
  }
  sprintf(name, "%s", S_IDENT(pst));
  gen_pst_done_rec(outfile, name, pst, S_DTYPE(pst), S_VAR_INIT(pst), 1);
}


void force_pst_done(FILE* outfile, symboltable_t* pst) {
  int save_setup = S_SETUP(pst);
  
  S_SETUP(pst) = 1;
  gen_pst_done(outfile, pst);
  S_SETUP(pst) = save_setup;
}


static void decl_macro_const(FILE* outfile, symboltable_t *pst) {
  char *tempptr;

  tempptr=S_IDENT(pst);
  S_IDENT(pst)=(char *)PMALLOC(strlen(tempptr)+8);
  sprintf(S_IDENT(pst),"_CONST_%s",tempptr);

  fprintf(outfile,"#define %s (",S_IDENT(pst));
  gen_pdt(outfile,S_DTYPE(pst),PDT_CAST);
  fprintf(outfile,")(");
}


static void decl_const_var_const(FILE* outfile, symboltable_t *pst) {
  dimension_t* arrdim;

  fprintf(outfile,"_CONST ");
  arrdim = gen_pdt(outfile,S_DTYPE(pst),PDT_GLOB);
  fprintf(outfile,"%s",S_IDENT(pst));
  if (arrdim) {
    fprintf(outfile, "[]");
  }
  fprintf(outfile, " = ");
}


static int decl_const(FILE* outfile, symboltable_t *pst) {
  initial_t *initializer;
  int atomicinit;
  int atomicconst;
  expr_t *init_expr;
  int type;

  if (S_SETUP(pst) != 1) {
    return 0;
  }
  initializer = S_VAR_INIT(pst);
  if (initializer == NULL) {
    if (D_CLASS(S_DTYPE(pst)) == DT_GRID || 
	D_CLASS(S_DTYPE(pst)) == DT_REGION) {
      /* don't ask me why a const grid would ever not have an initializer... */
      return 0;
    } else {
      INT_FATAL(NULL, "found a const without an initializer");
    }
  }
  atomicinit = (IN_BRA(initializer) == FALSE);
  atomicconst = datatype_scalar(S_DTYPE(pst)) && !datatype_complex(S_DTYPE(pst));
  if (atomicconst) {
    init_expr = IN_VAL(initializer);

    if (!debug && T_TYPE(init_expr) == CONSTANT) {
      type = MACRO_CONST;
    } else if (const_opt && expr_computable_const(init_expr)) {
      if (debug) {
	type = CONSTVAR_CONST;
      } else {
	type = MACRO_CONST;
      }
    } else if (expr_c_legal_init(init_expr)) {
      type = CONSTVAR_CONST;
    } else {
      type = VAR_CONST;
    }
  } else {
    if (init_c_legal_init(initializer) && !atomicinit) {
      type = CONSTVAR_CONST;
    } else {
      type = VAR_CONST;
    }
  }

  if (type == VAR_CONST) {
    return 0;
  }
  
  switch (type) {
  case MACRO_CONST:
    decl_macro_const(outfile, pst);
    break;
  case CONSTVAR_CONST:
    decl_const_var_const(outfile, pst);
    break;
  default:
    INT_FATAL(NULL,"Problem in declaring constants! (BLC)");
    break;
  }

  gen_init_list(outfile, initializer);

  switch (type) {
  case MACRO_CONST:
    fprintf(outfile,")\n");
    break;
  case CONSTVAR_CONST:
    fprintf(outfile,";\n");
    break;
  default:
    break;
  }
  return 1;
}


void gen_pst_ls(FILE* outfile, symboltable_t* pst) {
  while (pst != NULL) {
    gen_pst(outfile,pst);
    pst = S_SIBLING(pst);
  }
}

void gen_pst_init_ls(FILE* outfile, symboltable_t* pst) {
  while (pst != NULL) {
    gen_pst_init(outfile,pst);
    pst = S_SIBLING(pst);
  }
}

void gen_pst_finalize_ls(FILE* outfile, symboltable_t* pst) {
  while (pst != NULL) {
    gen_pst_finalize(outfile,pst);
    pst = S_SIBLING(pst);
  }
}

void gen_pst_done_ls(FILE* outfile, symboltable_t* pst) {
  while (pst != NULL) {
    gen_pst_done(outfile,pst);
    pst = S_SIBLING(pst);
  }
}

static void gen_pst_scope(FILE *outfile, symboltable_t *pst) {
  if (pst == NULL) {
    return;
  }
  fprintf(outfile, "{\n");
  gen_pst_ls(outfile, pst);
  fprintf(outfile, "}");
}


static void gen_enumlist(FILE *outfile,symboltable_t *pst) {
  symboltable_t *pstTemp;

  if (pst == NULL) {
    return;
  }
  fprintf(outfile, "{\n\t");
  pstTemp = pst;
  while (pstTemp != NULL) {
    gen_pst(outfile,pstTemp);
    pstTemp = S_SIBLING(pstTemp);
    if (pstTemp != NULL) {
      fprintf(outfile, ",\n\t");
    }  else {
      fprintf(outfile, "\n");
    }
  }
  fprintf(outfile, "} ");
}


static void gen_grid_decl_macro(FILE* outfile, symboltable_t* pst) {
  if (D_CLASS(S_DTYPE(pst)) == DT_GRID) {
    fprintf(outfile, "_GRID_DECL(");
  } else {
    fprintf(outfile, "_GRID_DUAL_DECL(");
  }
}


static void gen_dist_decl_macro(FILE* outfile, symboltable_t* pst) {
  if (D_CLASS(S_DTYPE(pst)) == DT_DISTRIBUTION) {
    fprintf(outfile, "_DIST_DECL(");
  } else {
    fprintf(outfile, "_DIST_DUAL_DECL(");
  }
}


static void gen_reg_decl_macro(FILE* outfile, symboltable_t* pst) {
  if (D_CLASS(S_DTYPE(pst)) == DT_REGION) {
    fprintf(outfile, "_REG_DECL(");
  } else {
    fprintf(outfile, "_REG_DUAL_DECL(");
  }
}


static void gen_ens_decl_macro(FILE* outfile, symboltable_t* pst) {
  if (D_CLASS(S_DTYPE(pst)) == DT_ENSEMBLE) {
    fprintf(outfile, "_ENS_DECL(");
  } else {
    fprintf(outfile, "_ENS_DUAL_DECL(");
  }
}


static int init_matches_type(initial_t* init, datatype_t* pdt, int dynarray) {
  if (dynarray) {
    /* dynamic arrays never match their initializers */
    return 0;
  }
  if (D_CLASS(pdt) == DT_DIRECTION) {
    /* we can initialize directions in place */
    return 1;
  }
  if (IN_BRA(init) == FALSE) {
    if (datatype_scalar(pdt)) {
      /* init is scalar && datatype scalar => things match */
      return 1;
    } else {
      /* init is scalar && datatype not => promoted init; */
      return 0;
    }
  } else {
    return 1;
  }
}


void gen_pst(FILE* outfile,symboltable_t *pst) {
  symboltable_t *tmp;
  statement_t *pstmt;
  dimension_t *pdTemp = NULL;
  int level;
  int gotoreturn = 0;
  static int instance_num=0;
  int dynarray;
  initial_t* init;
  typeclass doduals; /* this refers to data structures like ensembles
			and regions that require two parallel
			structures: one to store data, the second to
			store pointers to the data.  Currently,
			regions are stil handled in the old way, so
			this only handles ensembles.  Also, distributions
			currently don't have a pointer, though perhaps
			they should in which case it should use this
			same case. */
  dimension_t* arrdim;

  if (pst == NULL) {
    return;
  }
  if (S_STD_CONTEXT(pst) == TRUE) {
    /*** don't output declarations for the standard context ***/
    return;
  }

  level = S_LEVEL(pst);
  if (S_FG_2(pst) == 1) {
    fprintf(outfile, "(");
  }
  switch(S_CLASS(pst)) {
  case S_VARIABLE:
    LINEINFO(pst);
    DB0(20, "\nS_VARIABLE\n");

    if (S_IS_CONSTANT(pst)) {
      if (S_STYPE(pst) & SC_EXTERN) {
	break;
      }
      if (!(S_STYPE(pst) & SC_CONFIG)) {
        if (decl_const(outfile, pst)) {
	  break;
	}
      }
    }

    if (!S_IS_USED(pst)) {   /* don't declare if it's not used */
      break;
    }
    if (S_STYPE(pst) & SC_EXTERN) {
      break;
    }

    if (datatype_find_ensemble(S_DTYPE(pst))) {
      doduals = DT_ENSEMBLE;
    } else if (datatype_find_region(S_DTYPE(pst))) {
      doduals = DT_REGION;
    } else if (datatype_find_distribution(S_DTYPE(pst))) {
      doduals = DT_DISTRIBUTION;
    } else if (datatype_find_grid(S_DTYPE(pst))) {
      doduals = DT_GRID;
    } else {
      doduals = 0;
    }

    switch (doduals) {
    case DT_GRID:
      gen_grid_decl_macro(outfile, pst);
      break;
    case DT_DISTRIBUTION:
      gen_dist_decl_macro(outfile, pst);
      break;
    case DT_REGION:
      gen_reg_decl_macro(outfile, pst);
      break;
    case DT_ENSEMBLE:
      gen_ens_decl_macro(outfile, pst);
      break;
    default:
      break;
    }
    
    dynarray = datatype_dyn_array(S_DTYPE(pst));

    /* gen initial tags */    
    gen_psc(outfile, S_STYPE(pst));

    if (doduals) {
      fprintf(outfile, ", ");
      switch (D_CLASS(S_DTYPE(pst))) {
      case DT_ARRAY:
	if (dynarray) {
	  pdTemp = gen_dual_ensemble_pdt(outfile,D_ARR_TYPE(S_DTYPE(pst)), 
					 PDT_GLOB);
	  fprintf(outfile, "*");
	  break;
	}
	/* fall through */
      case DT_STRUCTURE:
      case DT_ENSEMBLE:
      case DT_REGION:
      case DT_DISTRIBUTION:
      case DT_GRID:
	pdTemp = gen_dual_ensemble_pdt(outfile,S_DTYPE(pst),PDT_EGLB);
	break;
      default:
	break;
      }
      fprintf(outfile, ", ");
    }

    switch (D_CLASS(S_DTYPE(pst))) {
    case DT_GRID:
    case DT_DISTRIBUTION:
    case DT_REGION:
    case DT_ENSEMBLE:
      pdTemp = gen_pdt(outfile,S_DTYPE(pst), PDT_EGLB);
      break;
    case DT_ARRAY:
      if (dynarray) {
	pdTemp = gen_pdt(outfile, D_ARR_TYPE(S_DTYPE(pst)), PDT_GLOB);
	fprintf(outfile, "*");
	break;
      }
      /* fall through */
    default:
      pdTemp = gen_pdt(outfile, S_DTYPE(pst), PDT_GLOB);
      break;
    }
    if (doduals) {
      fprintf(outfile, ", ");
    }

    fprintf(outfile, "%s", S_IDENT(pst));

    /* if this is a multi-instance variable, specialize its number */
    if (S_NUM_INSTANCES(pst) > 1) {
      fprintf(outfile,"%d",instance_num);
    }

    gen_array_sizes(outfile,pdTemp,S_DTYPE(pst));
    
    if (doduals) {
      fprintf(outfile, ")");
      if (doduals == DT_ENSEMBLE && 
	  hoist_access_ptrs == HOIST_PTRS_TO_SUBPROG &&
          (D_CLASS(S_DTYPE(pst)) != DT_STRUCTURE && /* we cant hoist records */
	   D_CLASS(S_DTYPE(pst)) != DT_ARRAY)) {    /* or arrays yet */
	fprintf(outfile, ";\n");
	fprintf(outfile, "_DECL_HOISTED_PTRS(");
	switch (D_CLASS(S_DTYPE(pst))) {
	case DT_ENSEMBLE:
	  arrdim = gen_pdt(outfile, D_ENS_TYPE(S_DTYPE(pst)), PDT_GLOB);
	  if (arrdim) {
	    fprintf(outfile, "*");
	  }
	  fprintf(outfile, ", ");
	  break;
	case DT_ARRAY:
	  gen_pdt(outfile, D_ENS_TYPE(datatype_find_ensemble(S_DTYPE(pst))),
		  PDT_GLOB);
	  fprintf(outfile, ", *");
	  break;
	default:
	  gen_pdt(outfile, S_DTYPE(pst), PDT_GLOB);
	  break;
	}
	fprintf(outfile, ", %s)", S_IDENT(pst));
      }
    }

    if (dynarray) {
      if (doduals && S_LEVEL(pst) != 0) {
	fprintf(outfile,";\n");
      }
      gen_dynarray_alloc(outfile, pst, doduals);
      if (doduals==DT_ENSEMBLE && 
	  hoist_access_ptrs == HOIST_PTRS_TO_SUBPROG &&
	  (D_CLASS(S_DTYPE(pst)) != DT_STRUCTURE && 
	   D_CLASS(S_DTYPE(pst)) != DT_ARRAY)) {
	if (S_LEVEL(pst) == 0) {
	  gen_hoisted_ptr_allocs(initfile, pst, doduals);
	} else {
	  gen_hoisted_ptr_allocs(outfile, pst, doduals);
	}
      }
    }


    /* Initialize */
    if (S_STYPE(pst) & SC_CONFIG) {
      DoConfig(initfile, pst);
    }
    init = S_VAR_INIT(pst);
    if (init && init_c_legal_init(init) && 
	init_matches_type(init, S_DTYPE(pst), dynarray) && 
	!S_STYPE(pst)&SC_CONFIG) {
      fprintf(outfile, " = ");
      gen_init_list(outfile, init);
    } else {
      if (S_LEVEL(pst) == 0) {
	gen_pst_init(initfile, pst);
      } else {
	if (datatype_scalar(S_DTYPE(pst))) {
	  gen_pst_init(outfile, pst);
	}
	/* otherwise, this should be handled "later"--like by gen_pstinit() */
      }
    }
    fprintf(outfile, ";\n");

    /* if this is a multi-instance variable, generate other instances
       recursively */
    if (S_NUM_INSTANCES(pst) > 1) {
      instance_num++;
      if (instance_num < S_NUM_INSTANCES(pst)) {
	gen_pst(outfile,pst);
      } else {
	instance_num = 0;
      }
    }

    if (S_STYPE(pst) & SC_CONFIG) {
      fprintf(initfile, "}\n");
    }

    break;
  case S_TYPE:
    LINEINFO(pst);
    DB0(20, "\nS_TYPE\n");
    if (S_STYPE(pst) & SC_EXTERN) {
      break;
    }
    fprintf(outfile, "typedef ");
    pdTemp = gen_pdt(outfile,S_DTYPE(pst),PDT_TYPE);
    fprintf(outfile, "%s", S_IDENT(pst));
    gen_array_sizes(outfile,pdTemp,S_DTYPE(pst));
    fprintf(outfile,";\n");
    if (datatype_find_ensemble(S_DTYPE(pst)) || 
	datatype_find_region(S_DTYPE(pst))) {
      gen_dual_ensemble_pst(outfile, pst,1);
    }
    break;
  case S_LITERAL:
    break;
  case S_SUBPROGRAM:			/*** annotated by sungeun ***/
    LINEINFO(pst);
    DB0(20, "\nS_SUBPROGRAM\n");

    if (T_STLS(S_FUN_BODY(pst)) == NULL) {
      break;
    }
    
    codegen_fn = S_FUN_BODY(pst);
    pstmt = T_STLS(S_FUN_BODY(pst));

    gen_function_header(outfile, pst, 0);

    if (T_PARALLEL(S_FUN_BODY(pst))) {
      gotoreturn = 1;
      retpt_used = 0;
      if (S_FUN_TYPE(pst) != pdtVOID) {
	gen_pdt(outfile,S_FUN_TYPE(pst),PDT_RVAL);
	fprintf(outfile,"_retval;\n");
      }
    }

    /**
     ** Output local variable declarations
     **/
    for (tmp = T_CMPD_DECL(pstmt); tmp != NULL; tmp = S_SIBLING(tmp)) {
      if (S_CLASS(tmp) != S_PARAMETER) {
	gen_pst(outfile,tmp);
      }
    }
    gen_hoisted_vars(outfile, pst);
    fprintf(outfile, "\n");
    
    /**
     ** Output code to initialize local variables
     **/
    for (tmp = T_CMPD_DECL(pstmt); tmp != NULL; tmp = S_SIBLING(tmp)) {
      if (S_CLASS(tmp) != S_PARAMETER && 
	  S_SETUP(tmp) == 1 &&
	  S_STD_CONTEXT(tmp) == FALSE &&
	  S_IS_USED(tmp) &&
	  !(S_STYPE(tmp) & SC_EXTERN) &&
	  !datatype_scalar(S_DTYPE(tmp))) {
	gen_pst_init(outfile,tmp);
	gen_pst_finalize(outfile,tmp);
      }
    }

    /**
     ** Output the procedure's body
     **/
    gen_stmtls(outfile, T_STLS(T_CMPD(pstmt)));
      
    /**
     ** Generate any cleanup code which might exist
     **/
    if (gotoreturn && retpt_used) {
      fprintf(outfile,"_retpt:;\n");
    }
    
    /**
     ** Output code to destroy local variables
     **/
    for (tmp = T_CMPD_DECL(pstmt); tmp != NULL; tmp = S_SIBLING(tmp)) {
      if (S_CLASS(tmp) != S_PARAMETER) {
	gen_pst_done(outfile,tmp);
      }
    }

    /***************************************
      Return return value, if any
    ***************************************/

    {
      symboltable_t * pst;
      
      pst = T_CMPD_DECL(pstmt);
      while (pst != NULL) {
	if (S_CLASS(pst) != S_PARAMETER) {
	  if (D_CLASS(S_DTYPE(pst)) == DT_ARRAY &&
	      (!expr_computable_const(DIM_LO(D_ARR_DIM(S_DTYPE(pst)))) ||
	       !expr_computable_const(DIM_HI(D_ARR_DIM(S_DTYPE(pst)))))) {
	    fprintf(outfile,"_zfree(%s, \"user dynamic array\");\n", S_IDENT(pst));
	  }
	}
	pst = S_SIBLING(pst);
      }
    }
    
    
    if (gotoreturn && S_FUN_TYPE(pst) != pdtVOID) {
      fprintf(outfile,"  return _retval;\n");
    }
    
    fprintf(outfile, "}\n\n\n");
  
    break;
  case S_PARAMETER:			/*** annotated by sungeun ***/
    DB0(20, "\nS_PARAMETER\n");
    /**************************************************************/
    /*** Check Cgen.c where function prototypes are outputed to ***/
    /*** foo_usr.h file *** changes here should be made there   ***/
    /**************************************************************/
    /*** output data type and return dimension info  ***/
    pdTemp = gen_pdt(outfile,S_PAR_TYPE(pst),PDT_PARM);

    /*** var parameters passed via pointer mechanisms ***/
    /*** check gen_function() in Cgen.c for revisions at the call site ***/
    if ((S_SUBCLASS(pst) == SC_INOUT || S_SUBCLASS(pst) == SC_OUT) &&
	/*** ignore grids/distributions/regions/ensembles/arrays/strings ***/
	(D_CLASS(S_DTYPE(pst)) != DT_GRID) &&
	(D_CLASS(S_DTYPE(pst)) != DT_DISTRIBUTION) &&
	(D_CLASS(S_DTYPE(pst)) != DT_REGION) &&
	(D_CLASS(S_DTYPE(pst)) != DT_ENSEMBLE) &&
	(D_CLASS(S_DTYPE(pst)) != DT_ARRAY) &&
	(D_CLASS(S_DTYPE(pst)) != DT_STRING)) {
      fprintf(outfile, "*");		/*** request pointer to variable ***/
    }

    /*** output variable name ***/
    gen_array_sizes_for_parameter1(outfile,pdTemp,S_DTYPE(pst));
    fprintf(outfile, " %s", S_IDENT(pst));
    gen_array_sizes_for_parameter2(outfile,pdTemp,S_DTYPE(pst));
    /**************************************************************/
    break;
  case S_STRUCTURE:
    DB0(20, "\nS_STRUCTURE\n");
    fprintf(outfile, "struct %s ", S_IDENT(pst));
    gen_pst_scope(outfile,D_STRUCT(S_STRUCT(pst)));
    S_FG_1(pst) = 1;
    fprintf(outfile, ";\n");
    break;
  case S_VARIANT:
    DB0(20, "\nS_VARIANT\n");
    fprintf(outfile, "union %s ", S_IDENT(pst));
    gen_pst_scope(outfile,D_STRUCT(S_VARI(pst)));
    S_FG_1(pst) = 1;
    fprintf(outfile, ";\n");
    break;
  case S_ENUMTAG:
    DB0(20, "\nS_ENUMTAG\n");
    fprintf(outfile, "enum %s ", S_IDENT(pst));
    gen_enumlist(outfile,D_STRUCT(S_DTYPE(pst)));
    S_FG_1(pst) = 1;
    fprintf(outfile, ";\n");
    break;
  case S_COMPONENT:
    DB0(20, "\nS_COMPONENT\n");
    pdTemp = gen_pdt(outfile,S_COM_TYPE(pst),PDT_COMP);
    fprintf(outfile, "%s", S_IDENT(pst));
    gen_array_sizes(outfile,pdTemp,S_DTYPE(pst));
    if (S_COM_BITLHS(pst) != NULL) {
      fprintf(outfile, " : %d", S_COM_BIT(pst));
    }
    fprintf(outfile, ";\n");
    break;
  case S_ENUMELEMENT:
    DB0(20, "\nS_ENUMELEMENT\n");
    fprintf(outfile, "%s", S_IDENT(pst));
    if (S_EINIT(pst) == SC_EINIT) {
      fprintf(outfile, "=");
      fprintf(outfile, " %d", S_EVALUE(pst));
    }
    break;
  case S_UNKNOWN:
    fprintf(outfile, "/* UNKNOWN */\n");
    break;
    
  default:
    fprintf(outfile, "/* <UNKNOWN SYMBOLCLASS> */\n");
  }
  if (S_FG_2(pst) == 1)
    fprintf(outfile, ")");
}


dimension_t *gen_pdt(FILE *outfile,datatype_t *pdt,int type) {
  dimension_t *retval = NULL;

  if (pdt == NULL) {
    return NULL;
  }

  switch(D_CLASS(pdt)) {
  case DT_BOOLEAN:
    fprintf(outfile,"boolean");
    break;
  case DT_CHAR:
    fprintf(outfile,"char");
    break;
  case DT_SIGNED_BYTE:
    if (type&DTF_ONEWD) {
      fprintf(outfile,"_schar");
    } else {
      fprintf(outfile,"signed char");
    }
    break;
  case DT_UNSIGNED_BYTE:
    if (type&DTF_ONEWD) {
      fprintf(outfile,"_uchar");
    } else {
      fprintf(outfile,"unsigned char");
    }
    break;
  case DT_SHORT:
    fprintf(outfile,"short");
    break;
  case DT_UNSIGNED_SHORT:
    if (type&DTF_ONEWD) {
      fprintf(outfile,"_ushort");
    } else {
      fprintf(outfile,"unsigned short");
    }
    break;
  case DT_INTEGER:
    fprintf(outfile,"int");
    break;
  case DT_UNSIGNED_INT:
    if (type&DTF_ONEWD) {
      fprintf(outfile,"_uint");
    } else {
      fprintf(outfile,"unsigned int");
    }
    break;
  case DT_LONG:
    fprintf(outfile,"long");
    break;
  case DT_UNSIGNED_LONG:
    if (type&DTF_ONEWD) {
      fprintf(outfile,"_ulong");
    } else {
      fprintf(outfile,"unsigned long");
    }
    break;
  case DT_REAL:
    fprintf(outfile,"float");
    break;
  case DT_DOUBLE:
    fprintf(outfile,"double");
    break;
  case DT_QUAD:
    fprintf(outfile,"_zquad");
    break;
  case DT_STRING:
    if (type & DTF_CSTBL) {
      fprintf(outfile,"char *");
    } else {
      fprintf(outfile,"_zstring");
    }
    break;
  case DT_ARRAY:
    if ((type&DTF_SHORT || type&DTF_RECRS) && D_NAME(pdt)!=NULL) {
      fprintf(outfile,"%s",S_IDENT(D_NAME(pdt)));
    } else {
      gen_pdt(outfile,D_ARR_TYPE(pdt),type|DTF_RECRS);
      retval = D_ARR_DIM(pdt);
    }
    break;
  case DT_ENSEMBLE:		/*** sungeun ***/
    if (type&DTF_SHORT && D_NAME(pdt) != NULL) {
      fprintf(outfile,"%s",S_IDENT(D_NAME(pdt)));
    } else {
      if (type&DTF_SETUP) {
	fprintf(outfile,"_array_fnc");
      } else {
	if (type&DTF_PARAM) {
	  fprintf(outfile,"_array");
	} else {
	  fprintf(outfile,"_array_nc");
	}
      }
    }
    break;
  case DT_DIRECTION:
    if (type&DTF_SHORT && 
	D_NAME(pdt) != NULL && S_IDENT(D_NAME(pdt)) != NULL) {
      fprintf(outfile, "%s", S_IDENT(D_NAME(pdt)));
    } else {
      if (type&DTF_SETUP) {
	fprintf(outfile, "_dir_info_nc");
      } else {
	if (type&DTF_PARAM) {
	  fprintf(outfile, "_dir_info");
	} else {
	  fprintf(outfile, "_dir_info_nc");
	}
      }
    }
    break;
  case DT_STRUCTURE:
    if ((type&DTF_SHORT || type&DTF_RECRS) && D_NAME(pdt) != NULL) {
      fprintf(outfile,"%s",S_IDENT(D_NAME(pdt)));
    } else {
      fprintf(outfile,"struct ");
      gen_pst_scope(outfile,D_STRUCT(pdt));
    }
    break;
  case DT_ENUM:
    if (type&DTF_SHORT && D_NAME(pdt) != NULL) {
      fprintf(outfile,"%s",S_IDENT(D_NAME(pdt)));
    } else {
      fprintf(outfile,"enum ");
      gen_enumlist(outfile,D_STRUCT(pdt));
    }
    break;
  case DT_VOID:
    fprintf(outfile, "void");
    break;
  case DT_FILE:			/* linc */
    fprintf(outfile, "_zfile");
    break;
  case DT_REGION:		/* only found in procedure prototypes */
    if (type&DTF_SHORT && D_NAME(pdt) != NULL && S_IDENT(D_NAME(pdt)) != NULL) {
      fprintf(outfile,"%s",S_IDENT(D_NAME(pdt)));
    } else {
      if (type&DTF_SETUP) {
	fprintf(outfile,"_region_fnc");
      } else {
	if (type&DTF_PARAM) {
	  fprintf(outfile,"_region");
	} else {
	  fprintf(outfile,"_region_nc");
	}
      }
    }
    break;
  case DT_GRID:
    if (type&DTF_SHORT && D_NAME(pdt) != NULL && S_IDENT(D_NAME(pdt)) != NULL) {
      fprintf(outfile,"%s",S_IDENT(D_NAME(pdt)));
    } else {
      if (type&DTF_SETUP) {
	fprintf(outfile,"_grid_fnc");
      } else {
	if (type&DTF_PARAM) {
	  fprintf(outfile,"_grid");
	} else {
	  fprintf(outfile,"_grid_nc");
	}
      }
    }
    break;
  case DT_DISTRIBUTION:
    if (type&DTF_SHORT && D_NAME(pdt) != NULL && S_IDENT(D_NAME(pdt)) != NULL) {
      fprintf(outfile,"%s",S_IDENT(D_NAME(pdt)));
    } else {
      if (type&DTF_SETUP) {
	fprintf(outfile,"_distribution_fnc");
      } else {
	if (type&DTF_PARAM) {
	  fprintf(outfile,"_distribution");
	} else {
	  fprintf(outfile,"_distribution_nc");
	}
      }
    }
    break;
  case DT_PROCEDURE:		/* only found in procedure prototypes */
    fprintf(outfile, "void *"); /*linc*/
    break;
  case DT_OPAQUE:
    if (D_NAME(pdt) != NULL) {
      fprintf(outfile,"%s",S_IDENT(D_NAME(pdt)));
    } else {
      fprintf(outfile, "void");
    }
    break;
  case DT_COMPLEX:
    fprintf(outfile,"fcomplex");
    break;
  case DT_DCOMPLEX:
    fprintf(outfile,"dcomplex");
    break;
  case DT_QCOMPLEX:
    fprintf(outfile,"qcomplex");
    break;
  default:
    INT_FATAL(NULL, "Unknown Typeclass %d",D_CLASS(pdt));
  }

  if (type&DTF_SPACE) {
    fprintf(outfile," ");
  }

  return retval;
}


char* return_pdt(datatype_t *pdt,int type, char *buf) {

  if (pdt == NULL) {
    return NULL;
  }

  switch(D_CLASS(pdt)) {
  case DT_BOOLEAN:
    sprintf(buf, "boolean");
    break;
  case DT_CHAR:
    sprintf(buf, "char");
    break;
  case DT_SIGNED_BYTE:
    if (type&DTF_ONEWD) {
      sprintf(buf, "_schar");
    } else {
      sprintf(buf, "signed char");
    }
    break;
  case DT_UNSIGNED_BYTE:
    if (type&DTF_ONEWD) {
      sprintf(buf, "_uchar");
    } else {
      sprintf(buf, "unsigned char");
    }
    break;
  case DT_SHORT:
    sprintf(buf, "short");
    break;
  case DT_UNSIGNED_SHORT:
    if (type&DTF_ONEWD) {
      sprintf(buf, "_ushort");
    } else {
      sprintf(buf, "unsigned short");
    }
    break;
  case DT_INTEGER:
    sprintf(buf, "int");
    break;
  case DT_UNSIGNED_INT:
    if (type&DTF_ONEWD) {
      sprintf(buf, "_uint");
    } else {
      sprintf(buf, "unsigned int");
    }
    break;
  case DT_LONG:
    sprintf(buf, "long");
    break;
  case DT_UNSIGNED_LONG:
    if (type&DTF_ONEWD) {
      sprintf(buf, "_ulong");
    } else {
      sprintf(buf, "unsigned long");
    }
    break;
  case DT_REAL:
    sprintf(buf, "float");
    break;
  case DT_DOUBLE:
    sprintf(buf, "double");
    break;
  case DT_QUAD:
    sprintf(buf, "_zquad");
    break;
  case DT_STRING:
    sprintf(buf, "_zstring");
    break;
  case DT_ARRAY:
    if (type&DTF_SHORT && D_NAME(pdt)!=NULL) {
      sprintf(buf, S_IDENT(D_NAME(pdt)));
    } else {
      return return_pdt(D_ARR_TYPE(pdt),type|DTF_RECRS, buf);
    }
    break;
  case DT_ENSEMBLE:		/*** sungeun ***/
    if (type&DTF_SHORT && D_NAME(pdt) != NULL) {
      sprintf(buf, S_IDENT(D_NAME(pdt)));
    } else {
      if (type&DTF_PARAM) {
	sprintf(buf, "_array");
      } else {
	if (type&DTF_RECRS) {
	  sprintf(buf, "_array_nc");
	} else {
	  sprintf(buf, "_array_fnc");  /* brad gen typedef ensemble */
	}
      }
    }
    break;
  case DT_STRUCTURE:
  case DT_ENUM:
  case DT_VOID:
  case DT_FILE:			/* linc */
  case DT_REGION:		/* only found in procedure prototypes */
  case DT_PROCEDURE:		/* only found in procedure prototypes */
  default:
    INT_FATAL(NULL, "Unknown Typeclass %d for scan/reduce",D_CLASS(pdt));
  }
  return buf;

}


void gen_finalize(module_t* module) {
  symboltable_t* pst;
  int i;

  fprintf(initfile, "\n");
  for (i = 0; i < MAXRANK; i++) {
    fprintf(initfile, "_InitDistributionDimdist(_DefaultDistribution, %d, blk);\n", i);
    fprintf(initfile, "_blkSetup(_DefaultDistribution, %d, "
	    "_BND_LO(_invisibleBounds[%d]), _BND_HI(_invisibleBounds[%d]));\n", i, i, i);
  }
  fprintf(initfile, "_InitDistribution(_DefaultDistribution, 1);\n"); 
  for (pst = T_DECL(module); pst != NULL; pst = S_SIBLING(pst)) {
    if (S_CLASS(pst) != S_SUBPROGRAM) {
      gen_pst_finalize(initfile, pst);
    }
  }
}
