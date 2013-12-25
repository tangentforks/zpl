/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Mgen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/createvar.h"
#include "../include/datatype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/parsetree.h"
#include "../include/runtime.h"
#include "../include/symboltable.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"

static glist temps=NULL;
static expr_t* glb_pattern;
static symboltable_t* glb_reg;


/* base helper functions */

static void gen_sps_numvals(FILE* outfile, char* regname) {
  fprintf(outfile,"_SPS_NUMVALS(%s)", regname);
}


static void gen_sps_numnodes(FILE *outfile, char* regname) {
  fprintf(outfile,"_SPS_NUMNODES(%s)", regname);
}


/* intermediate helper functions */

static void gen_sps_test_start(FILE *outfile,expr_t *pattern) {
  fprintf(outfile,"if (");
  gen_expr(outfile, pattern);
  fprintf(outfile,") {\n");
}


static void gen_sps_test_stop(FILE *outfile,expr_t *pattern) {
  fprintf(outfile,"}\n");
}



/* exported functions and their immediate helpers */

void gen_sps_loc_decls(FILE* outfile,char* regname, datatype_t* regdt) {
  int numdims;
  int i;

  numdims = D_REG_NUM(regdt);

  fprintf(outfile,"_index_list _inds;\n");
  fprintf(outfile,"int _newind[%d];\n",numdims);
  gen_fluff_decls(outfile,NULL,0);

  for (i=0; i<numdims; i++) {
    fprintf(outfile, "_SPS_REG_FLUFF_LO(%s, %d) = 0;\n", regname, i);
    fprintf(outfile, "_SPS_REG_FLUFF_HI(%s, %d) = 0;\n", regname, i);
  }
}


void gen_sparse_fluff(FILE* outfile,char* regname, symboltable_t* reg,
		      datatype_t* regdt) {
  gen_fluff_setup(outfile,regname,reg,D_REG_NUM(regdt),0);
  fprintf(outfile,"_SPS_FLUFF_BOUNDS(%s);\n", regname);
}


static expr_t* build_ens_expr(expr_t* reg,datatype_t* basetype) {
  char name[20];
  datatype_t* newdt;
  symboltable_t* newvar;
  expr_t* newexpr;

  sprintf(name,"_spstemp%d",GlobalTempCounter++);
  newdt = build_ensemble_type(basetype,reg,1,1,0,"");
  /* 
     Last two arguments should be lineno, filename, but none is
     assigned to some reg expressions
     T_LINENO(T_STMT(reg)),T_FILENAME(T_STMT(reg))); */
  newvar = create_named_var(newdt,NULL,name);
  newexpr = build_0ary_op(VARIABLE, newvar);

  if (temps == NULL) {
    temps = glist_create(newexpr,GLIST_NODE_SIZE);
  } else {
    temps = glist_prepend(temps,newexpr,GLIST_NODE_SIZE);
  }

  return newexpr;
}


static expr_t* gen_setup_complex_patterns(FILE* outfile,expr_t* pattern,
					  expr_t* reg) {
  expr_t* next;
  expr_t* op;
  datatype_t* retpdt;
  expr_t* new_pattern;
  genlist_t* formals;
  symboltable_t* fnpst;
  char* name;

  switch (T_TYPE(pattern)) {
  case NULLEXPR:
  case CONSTANT:
  case VARIABLE:
    break;
  case UNEGATIVE:
  case UPOSITIVE:
  case UCOMPLEMENT:
    T_OPLS(pattern) = gen_setup_complex_patterns(outfile,pattern,reg);
    break;
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:
  case BIEXP:
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
  case BILOG_AND:
  case BILOG_OR:
    op = T_OPLS(pattern);
    next = T_NEXT(op);
    next = gen_setup_complex_patterns(outfile,next,reg);
    op = gen_setup_complex_patterns(outfile,op,reg);
    T_NEXT(op) = next;
    T_OPLS(pattern) = op;
    break;
  case FUNCTION:
    fnpst = T_IDENT(T_OPLS(pattern));
    retpdt = S_FUN_TYPE(fnpst);
    if (D_CLASS(retpdt) == DT_VOID) {  /* void DT -> fn w/ ens ret type remvd */

      /* find last formal, which used to be the function return type */
      formals = T_PARAMLS(S_FUN_BODY(fnpst));
      while (G_NEXT(formals) != NULL) {
	formals = G_NEXT(formals);
      }
      retpdt = S_DTYPE(G_IDENT(formals));
      if (D_CLASS(retpdt) != DT_ENSEMBLE) {
	INT_FATAL(NULL,"Got wrong formal in gen_setup_complex_patterns()");
      }
      new_pattern = build_ens_expr(reg,D_ENS_TYPE(retpdt));
      name = S_IDENT(T_IDENT(new_pattern));

      fprintf(outfile,"{\n");
      fprintf(outfile,"_arr_info _%s;\n",name);
      fprintf(outfile,"_array_fnc %s = &_%s;\n",name,name);
      fprintf(outfile,"_ConstructTempEnsemble(");
      gen_expr(outfile, reg);
      fprintf(outfile,",%s,sizeof(",name);
      gen_pdt(outfile,D_ENS_TYPE(retpdt),PDT_SIZE);
      fprintf(outfile,"));\n");

      /* add new var in as last parameter */
      op = T_OPLS(pattern);
      while (T_NEXT(op) != NULL) {
	op = T_NEXT(op);
      }
      T_NEXT(op) = new_pattern;
      T_PREV(new_pattern) = op;
      T_PARENT(new_pattern) = T_PARENT(op);

      gen_expr(outfile, pattern);
      fprintf(outfile,";\n");
      fprintf(outfile,"\n");

      pattern = new_pattern;
    }
    break;
  case ARRAY_REF:
  case BIDOT:
    break;
  default:
    INT_FATAL(NULL,"Unrecognized expr type in gen_setup_complex_patterns");
  }
  return pattern;
}


static void gen_destroy_complex_patterns(FILE* outfile) {
  expr_t* tmp_expr;
  char* name;

  while (temps != NULL) {
    tmp_expr = (expr_t*)GLIST_DATA(temps);
    name = S_IDENT(T_IDENT(tmp_expr));
    fprintf(outfile,"_DestroyTempEnsemble(%s);\n",name);
    fprintf(outfile,"}\n");
    temps = GLIST_NEXT(temps);
  }
}


static void gen_sps_mloop_countnodes_pre(FILE* outfile) {

  /* should do something with fluff here */

}


static void gen_sps_mloop_countnodes_post(FILE* outfile) {

  /* should do something with fluff here */

}

static void gen_sps_mloop_countnodes(FILE* outfile, int numdims) {
  int i;

  gen_sps_test_start(outfile,glb_pattern);

  for (i=0; i<numdims; i++) {
    fprintf(outfile,"_newind[%d] = _i%d;\n",i,i);
  }
  fprintf(outfile,"_add_sparse_index(_inds,_newind);\n");

  gen_sps_test_stop(outfile,glb_pattern);
}


void gen_sps_special_mloop_stmt(FILE* outfile,statement_t* stmt,
				int numdims) {
  switch (T_SUBTYPE(stmt)) {
  case N_SPS_INIT_COUNT_PRE:
    gen_sps_mloop_countnodes_pre(outfile);
    break;
  case N_SPS_INIT_COUNT:
    gen_sps_mloop_countnodes(outfile, numdims);
    break;
  case N_SPS_INIT_COUNT_POST:
    gen_sps_mloop_countnodes_post(outfile);
    break;
  }
}


void gen_sparse_structure(FILE* outfile, symboltable_t *reg, char* regname,
			  datatype_t* regdt, expr_t* initexpr) {
  int numdims;
  expr_t *basereg;
  expr_t *pattern;
  region_t _base_scope;
  region_t* base_scope=&_base_scope;
  statement_t* mloop_body;
  statement_t* mloop;
  statement_t* mloop_pre;
  statement_t* mloop_post;
  int i;

  numdims = D_REG_NUM(regdt);
  if (T_TYPE(initexpr) == BIWITH) {
    pattern = T_NEXT(T_OPLS(initexpr));
  } else if (T_TYPE(initexpr) == BIPREP) {
    pattern = NULL;
  } else {
    INT_FATAL(NULL, "Unexpected initializer in gen_sparse_structure");
  }

  if (pattern != pexprUNKNOWN) {
    basereg = T_OPLS(initexpr);
    /* push a scope in case any functions in the pattern need it */
    T_REGION_SYM(base_scope) = basereg;
    T_MASK_EXPR(base_scope) = NULL;
    push_region_scope(outfile, base_scope);

    if (pattern == NULL && !expr_is_dense_reg(basereg)) {
      switch (T_SUBTYPE(initexpr)) {
      case AT_REGION:
	fprintf(outfile,"_SpsCopyShiftStructure(%s, ", regname);
	gen_expr(outfile,basereg);
	fprintf(outfile,",");
	gen_expr(outfile,T_NEXT(T_OPLS(initexpr)));
	fprintf(outfile,");\n");
	break;
      default:
	fprintf(outfile,"_SPS_INHERIT_STRUCTURE(%s, ", regname);
	gen_expr(outfile,basereg);
	fprintf(outfile,",%d);\n",numdims);
	break;
      }
    } else {

      /* get rid of any functions that return ensembles, etc.... */

      pattern = gen_setup_complex_patterns(outfile,pattern,basereg);
      typeinfo_expr(pattern);


      /* set up a structure to keep track of the indices that are true... */
      fprintf(outfile,"_inds = _new_index_list(%d, %s);\n",numdims, regname);

      /* count number of sparse nodes in main region */
      glb_pattern = pattern;
      glb_reg = reg;
      mloop_body = alloc_statement(S_NULL, 0, NULL);
      T_SUBTYPE(mloop_body) = N_SPS_INIT_COUNT;
      mloop = build_mloop_statement(basereg,mloop_body,numdims,NULL,0,-1,NULL);
      for (i=0; i<numdims; i++) {
	mloop_pre = alloc_statement(S_NULL, 0, NULL);
	T_SUBTYPE(mloop_pre) = N_SPS_INIT_COUNT_PRE;
	T_ADD_MLOOP_PREPRE(T_MLOOP(mloop),mloop_pre,i);

	mloop_post = alloc_statement(S_NULL, 0, NULL);
	T_SUBTYPE(mloop_post) = N_SPS_INIT_COUNT_POST;
	T_ADD_MLOOP_POSTPOST(T_MLOOP(mloop),mloop_post,i);
      }
      gen_fluffy_mloop(outfile,T_MLOOP(mloop),regname);
      fprintf(outfile,"\n");

      /* grab number of sparse nodes */
      fprintf(outfile,"/* _print_index_list(_inds); */\n");
      fprintf(outfile,"\n");

      fprintf(outfile,"_SparseBuildDirectory(%s, _inds);\n", regname);

      /* destroy temporary data structure */
      fprintf(outfile,"_old_index_list(_inds);\n");
      fprintf(outfile,"\n");

      fprintf(outfile,"/* _PrintSparseNodes(%s); */ \n", regname);
    }

    fprintf(outfile,"\n");
    gen_destroy_complex_patterns(outfile);

    pop_region_scope(outfile, base_scope);
  } else if (!datatype_is_dense_reg(regdt) && pattern == pexprUNKNOWN) {
    gen_sps_numnodes(outfile,regname);
    fprintf(outfile," = 3;\n");
    gen_sps_numvals(outfile,regname);
    fprintf(outfile," = 1;\n");
  }
}

    
