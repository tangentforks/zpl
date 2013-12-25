/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../include/Agen.h"
#include "../include/Cgen.h"
#include "../include/Cname.h"
#include "../include/Dgen.h"
#include "../include/Fgen.h"
#include "../include/IOgen.h"
#include "../include/Irongen.h"
#include "../include/IronCCgen.h"
#include "../include/Mgen.h"
#include "../include/Privgen.h"
#include "../include/Profgen.h"
#include "../include/Redgen.h"
#include "../include/Repgen.h"
#include "../include/SRgen.h"
#include "../include/Scangen.h"
#include "../include/Spsgen.h"
#include "../include/Stackgen.h"
#include "../include/WRgen.h"
#include "../include/Wgen.h"
#include "../include/beautify.h"
#include "../include/buildstmt.h"
#include "../include/cmplx_ens.h"
#include "../include/contraction.h"
#include "../include/datatype.h"
#include "../include/db.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/generic_stack.h"
#include "../include/getopt.h"
#include "../include/macros.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/setcid.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symboltable.h"
#include "../include/symtab.h"
#include "../include/typeinfo.h"
#include "../include/version.h"
#include "../include/checkpoint.h"

#define DESTPATHBASE "."
#define TEMP_OUTFILE ".ugly_zpl_outfile.c"

/* global variables defined in global.h */
int mask_no_access = 0;
char* DESTPATH;
char* trunc_in_file;
char* trunc_in_filename;

symboltable_t* mi_reg=NULL;
int codegen_imaginary=0;
int disable_new_access=1;

expr_t* currentDirection = NULL;

/* static variables */

static mloop_t *GLOBAL_MLOOP = 0;

static int function_call = 0;

static int pointeraccess = 0;

static int arrayref_height=0;

/* OutputUserSpecIncludes - output includes for the command line specified
 *                          header files
 * echris - 4/30/98
 */

static void OutputUserSpecIncludes(FILE * fp) {
  char *arg, ch;
  int flag = 0;

  reset_args();
  arg = get_arg(&flag,&ch);
  while ((arg = get_arg(&flag,&ch)) != NULL) {
    if (flag) {
    } else {
      if ((strlen(arg)>2) && (arg[(strlen(arg)-2)] == '.') &&
	  (arg[(strlen(arg)-1)] == 'h')) {
	fprintf(fp, "#include \"%s\"\n", arg);
      }
    }
  }
}


/* BLC -- This routine is used to do things at the end of codegen */
static void EndCodeGen(void) {
  char origoutfile[256];
  char realoutfile[256];

  sprintf(origoutfile, "%s%s",DESTPATH,TEMP_OUTFILE);
  sprintf(realoutfile, "%s%s.c", DESTPATH, trunc_in_filename);
  beautify(origoutfile, realoutfile);
}


void gen_complex_temp_name(FILE *outfile,datatype_t *resdt,int num) {
  fprintf(outfile,"_");
  switch (D_CLASS(resdt)) {
  case DT_COMPLEX:
    fprintf(outfile,"f");
    break;
  case DT_DCOMPLEX:
    fprintf(outfile,"d");
    break;
  case DT_QCOMPLEX:
    fprintf(outfile,"q");
    break;
  default:
    INT_FATAL(NULL,"Unexpected type in gen_complex_temp_name");
  }
  fprintf(outfile,"complex_temp%d",num);
}


void gen_setup_complex_temp(FILE *outfile,expr_t *expr,datatype_t *resdt,
			    int num) {
  if (expr_is_atom(expr) || !datatype_complex(T_TYPEINFO(expr))) {
  } else {
    gen_complex_temp_name(outfile,resdt,num);
    if (T_TYPE(expr) == FUNCTION) {
      fprintf(outfile," = ");
      gen_expr(outfile, expr);
      fprintf(outfile,";\n");
    } else {
      fprintf(outfile,".re = ");
      gen_real_expr(outfile, expr);
      fprintf(outfile,";\n");
      gen_complex_temp_name(outfile,resdt,num);
      fprintf(outfile,".im = ");
      gen_imag_expr(outfile, expr);
      fprintf(outfile,";\n");
    }
  }
}


void gen_real_expr(FILE* outfile, expr_t *expr) {
  int save_imaginary;

  save_imaginary = codegen_imaginary;
  codegen_imaginary = 0;
  gen_expr(outfile, expr);
  codegen_imaginary = save_imaginary;
}


void gen_imag_expr(FILE* outfile, expr_t *expr) {
  int save_imaginary;

  save_imaginary = codegen_imaginary;
  codegen_imaginary = 1;
  gen_expr(outfile, expr);
  codegen_imaginary = save_imaginary;
}


void gen_noncomplex_expr(FILE* outfile, expr_t *expr) {
  int save_imaginary;

  save_imaginary = codegen_imaginary;
  codegen_imaginary = -1;
  gen_expr(outfile, expr);
  codegen_imaginary = save_imaginary;
}


static void gen_if(FILE* outfile, if_t *ifstmt) {
  statement_t* elsestmts;
  int elsif;

  if (ifstmt == NULL) {
    INT_FATAL(NULL, "Null ifstmt in gen_if()");
    return;
  }
  
  fprintf(outfile, "if (");
  gen_expr(outfile, T_IFCOND(ifstmt));
  fprintf(outfile, ") {\n");
    
  if (T_THEN(ifstmt) != NULL) {
    gen_stmtls(outfile, T_THEN(ifstmt));
  }
  fprintf(outfile, "}");
    
  elsestmts = T_ELSE(ifstmt);
  if (elsestmts != NULL) {
    elsif = T_TYPE(elsestmts) == S_IF && T_SUBTYPE(elsestmts) == C_ELSIF;
    fprintf(outfile, " else ");
    if (elsif) {
      gen_stmtls(outfile, elsestmts);
    } else {
      fprintf(outfile," {\n");
      gen_stmtls(outfile, T_ELSE(ifstmt));
      fprintf(outfile, "}");
    }
  }
  fprintf(outfile,"\n");
}


static void gen_loop(FILE* outfile, loop_t *loop) {
  if (loop == NULL) {
    INT_FATAL(NULL, "Null loop in gen_loop()");
    return;
  }
  switch (T_TYPE(loop)) {
  case L_WHILE_DO:
    fprintf(outfile, "while (");
    gen_expr(outfile, T_LOOPCOND(loop));
    fprintf(outfile, ") {\n");
    gen_stmtls(outfile, T_BODY(loop));
    fprintf(outfile, "}\n");
    break;
  case L_REPEAT_UNTIL:
    fprintf(outfile, "do {\n");
    gen_stmtls(outfile, T_BODY(loop));
    fprintf(outfile, "} while (!(");
    gen_expr(outfile, T_LOOPCOND(loop));
    fprintf(outfile, "));\n");
    break;
  case L_DO:
    fprintf(outfile,"for (");
    gen_expr(outfile, T_IVAR(loop));
    fprintf(outfile,"=");
    gen_expr(outfile, T_START(loop));
    fprintf(outfile,"; ");
    gen_expr(outfile, T_IVAR(loop));
    if (T_UPDOWN(loop) == L_DO_UP) {
      fprintf(outfile,"<=");
    } else {
      fprintf(outfile,">=");
    }
    gen_expr(outfile, T_STOP(loop));
    fprintf(outfile,"; ");
    gen_expr(outfile, T_IVAR(loop));
    if (T_UPDOWN(loop) == L_DO_UP) {
      fprintf(outfile,"+=");
    } else {
      fprintf(outfile,"-=");
    }
    if (T_STEP_VAL(loop) != 0) {
      INT_FATAL(NULL, "Steve: T_STEP_VAL is used!");
    }
    gen_expr(outfile, T_STEP(loop));
    fprintf(outfile, ") {\n");
    gen_stmtls(outfile, T_BODY(loop));
    fprintf(outfile, "}\n");
    break;
  default:
    INT_FATAL(NULL, "Bad looptype (%d) in gen_loop()",T_TYPE(loop));
  }
}


void gen_dim_lo_bound(FILE* outfile, dimension_t* dimptr) {
  gen_expr(outfile, DIM_LO(dimptr));
}


void gen_dim_hi_bound(FILE* outfile, dimension_t* dimptr) {
  gen_expr(outfile, DIM_HI(dimptr));
}


static void gen_nloop(FILE* outfile, nloop_t *nloop) {
  int dim, depth;
  dimension_t *dimptr;
  symboltable_t* pst;

  depth = T_NLOOP_DEPTH(nloop);

  fprintf(outfile, "{  /* begin NLOOP for line %d */\n",
	  T_LINENO(T_NLOOP_BODY(nloop)));
  fprintf(outfile, "int " );
  for (dim = 0; dim < T_NLOOP_DIMS(nloop); dim++) {
    pst = get_blank_arrayref_index(depth+1, dim+1);
    fprintf(outfile, "%s", S_IDENT(pst));
    if ((dim + 1) == T_NLOOP_DIMS(nloop)) {
      fprintf(outfile, ";\n");
    } else {
      fprintf(outfile, ", ");
    }
  }

  dimptr = T_NLOOP_DIMLIST(nloop);
  for (dim = 0; dim < T_NLOOP_DIMS(nloop); dim++) {
    pst = get_blank_arrayref_index(depth+1, dim+1);

    fprintf(outfile, "for (%s = ",S_IDENT(pst));
    gen_dim_lo_bound(outfile, dimptr);
    fprintf(outfile,"; %s <= ",S_IDENT(pst));
    gen_dim_hi_bound(outfile, dimptr);
    fprintf(outfile, "; %s++) {\n", S_IDENT(pst));

    dimptr = DIM_NEXT(dimptr);
  }

  gen_stmtls(outfile, T_NLOOP_BODY(nloop));

  for (dim = 0; dim < T_NLOOP_DIMS(nloop); dim++) {
    fprintf(outfile, "}\n");
  }

  fprintf(outfile, "}  /* end NLOOP */\n");
}


static void gen_return_strcpy(FILE* outfile, expr_t* expr) {
  fprintf(outfile, "%s(_zretstring, ", S_IDENT(pstSTRNCPY));
  gen_noncomplex_expr(outfile, expr);
  fprintf(outfile, ");\n");  
}


static void gen_return(FILE* outfile, statement_t* stmt) {
  expr_t* rexpr;
  int retcomplex = 0;
  int retstring = 0;

  rexpr = T_RETURN(stmt);
  if (rexpr != NULL) {
    retcomplex = datatype_complex(T_TYPEINFO(rexpr));
    retstring = (T_TYPEINFO(rexpr) == pdtSTRING);
  }
  if (T_PARALLEL(T_PARFCN(stmt))) { 
    if (rexpr != NULL) {
      if (retstring) {
	gen_return_strcpy(outfile, T_RETURN(stmt));
	fprintf(outfile, "_retval = _zretstring;\n");
      } else if (retcomplex && !expr_is_atom(rexpr)) {
	gen_setup_complex_temp(outfile,rexpr,T_TYPEINFO(rexpr),1);
	fprintf(outfile,"  _retval = ");
	gen_complex_temp_name(outfile,T_TYPEINFO(rexpr),1);
      } else {
	fprintf(outfile,"  _retval = ");
	gen_noncomplex_expr(outfile, T_RETURN(stmt));
	fprintf(outfile,";\n");
      }
    }
    fprintf(outfile, "goto _retpt;\n");
    retpt_used = 1;
  } else {
    if (rexpr == NULL) {
      fprintf(outfile,"return");
    } else {
      if (retstring) {
	gen_return_strcpy(outfile, T_RETURN(stmt));
	fprintf(outfile, "return _zretstring");
      } else if (retcomplex && !expr_is_atom(rexpr)) {
	gen_setup_complex_temp(outfile,rexpr,T_TYPEINFO(rexpr),1);
	fprintf(outfile,"return ");
	gen_complex_temp_name(outfile,T_TYPEINFO(rexpr),1);
      } else {
	fprintf(outfile,"return ");
	gen_noncomplex_expr(outfile, T_RETURN(stmt));
      }
    }
    fprintf(outfile,";\n");
  }
}


void gen_stmt(FILE* outfile, statement_t *stmt) {
  expr_t * fcall;
  genlist_t *gptr;

  if (stmt == NULL) {
    return;
  }
  if (T_LABEL(stmt) != NULL) {
    fprintf(outfile, "%s : ",S_IDENT(T_LABEL(stmt)));
  }

  gptr = T_PRE(stmt);
  while (gptr != NULL) {
    symboltable_t* pst = G_IDENT(gptr);

    force_pst_init(outfile, pst);
    force_pst_finalize(outfile, pst);

    gptr = G_NEXT(gptr);
  }

  switch (T_TYPE(stmt)) {
  case S_NULL:
    break;
  case S_EXIT:
    LINEINFO(stmt);
    fprintf(outfile, "break;\n");
    break;
  case S_COMPOUND:
    fprintf(outfile, "{ /* begin compound statement */\n");
    gen_pst_ls(outfile,T_CMPD_DECL(stmt));
    gen_pst_init_ls(outfile,T_CMPD_DECL(stmt));
    gen_pst_finalize_ls(outfile,T_CMPD_DECL(stmt));
    gen_stmtls(outfile, T_CMPD_STLS(stmt));
    gen_pst_done_ls(outfile,T_CMPD_DECL(stmt));
    fprintf(outfile, "} /* end compound statement */\n");
    break;
  case S_MSCAN:
    INT_FATAL(NULL, "Unexpected MSCAN in gen_stmt");
    break;
  case S_EXPR:
    if (T_TYPE(T_EXPR(stmt)) == FUNCTION &&
	(T_IDENT(T_OPLS(T_EXPR(stmt))) == chklabel ||
	 T_IDENT(T_OPLS(T_EXPR(stmt))) == chksave ||
	 T_IDENT(T_OPLS(T_EXPR(stmt))) == chkrecover)) {
      gen_checkpoint(outfile, T_EXPR(stmt));
      break;
    }
    LINEINFO(stmt);
    /* rea - I added this in for kepart's scan stuff.  He will generate
       the entire var := scan statement in gen_scan()  */
    if (T_IS_ASSIGNOP(T_TYPE(T_EXPR(stmt))) && 
	T_IS_CCOMM(T_TYPE(T_NEXT(T_OPLS(T_EXPR(stmt))))) )  {
      switch (T_TYPE(T_NEXT(T_OPLS(T_EXPR(stmt))))) {
      case SCAN:
	gen_scan(outfile,T_EXPR(stmt));
	break;
      case FLOOD:
	gen_flood(outfile,T_EXPR(stmt));
	break;
      default:
	INT_FATAL(NULL,"Unexected expression type in gen_stmt()");
      }
    } else if (T_TYPE(T_EXPR(stmt)) == REDUCE) {
      /* currently, deliberately not using MASK field of stmt */
      /* reason is that the mask expression may be changed and havn't */
      /* yet decided how to propagate the information to all the stmts */
      /* same for gen_scan */
      gen_reduce(outfile,T_EXPR(stmt));
    } else {
      fcall = SearchForFunctionCall(T_EXPR(stmt));
      if (fcall!=NULL) {
	TestExprForComplexEnsParams(outfile, fcall);
      }
      gen_expr(outfile, T_EXPR(stmt));
      fprintf(outfile, ";\n");
      if (fcall != NULL) {
	FinishExprComplexParams(outfile, fcall);
      }
    }
    break;
  case S_HALT:
    LINEINFO(stmt);
    fprintf(outfile,"_io_in_ens=1;\n");  /* let anyone print */
    gen_stmtls(outfile, T_HALT(stmt));
    fprintf(outfile, "_ZPL_halt(%d);\n",T_LINENO(stmt));
    break;
  case S_CONTINUE:
    LINEINFO(stmt);
    fprintf(outfile, "continue;\n");
    break;
  case S_IF:
    LINEINFO(stmt);
    gen_if(outfile, T_IF(stmt));
    break;
  case S_LOOP:
    LINEINFO(stmt);
    gen_loop(outfile, T_LOOP(stmt));
    break;
  case S_RETURN:
    LINEINFO(stmt);
    gen_return(outfile, stmt);
    break;
  case S_IO:
    LINEINFO(stmt);
    TestIOStmtForComplexEnsParams(outfile, stmt);
    gen_io(outfile,stmt);
    FinishIOStmt(outfile, stmt);
    break;
  case S_REGION_SCOPE:
    {
      expr_t *prev_expr;
      if (expr_is_qmask(T_MASK_EXPR(T_REGION(stmt)))) {
	T_MASK_EXPR(T_REGION(stmt)) = NULL;
      }
      
      if (T_MASK_EXPR(T_REGION(stmt))) { /* masked region node */
	prev_expr = TestMaskStmtForComplexEnsMask(outfile, stmt);
      }

      {
	RMSPushScope(T_REGION((stmt)));
	push_region_scope(outfile, T_REGION((stmt)));
	gen_stmtls(outfile, T_BODY(T_REGION(stmt)));
	pop_region_scope(outfile, (T_REGION(stmt)));
	RMSPopScope(T_REGION(stmt));
      }

      if (T_MASK_EXPR(T_REGION(stmt))) { /* masked region node */
	
	FinishMaskStmt(outfile, stmt, prev_expr);
      }
      
    }
    break;
    
  case S_MLOOP:		/* rea - generates code for MLOOPs 5-7 */
    LINEINFO(stmt);
    GLOBAL_MLOOP = T_MLOOP(stmt);
    /* if collective operation overlapped then do special codegen */
    gen_mloop(outfile,T_MLOOP(stmt));
    GLOBAL_MLOOP = 0;
    break;
  case S_NLOOP:
    LINEINFO(stmt);
    gen_nloop(outfile, T_NLOOP(stmt));
    break;
  case S_WRAP:	/* rea 6-17-93 */
    LINEINFO(stmt);
    gen_wrap(outfile, stmt);
    T_WRAP_SEND(T_WRAP(stmt)) = !(T_WRAP_SEND(T_WRAP(stmt)));
    gen_wrap(outfile, stmt);
    T_WRAP_SEND(T_WRAP(stmt)) = !(T_WRAP_SEND(T_WRAP(stmt)));
    break;
  case S_REFLECT: /* rea */
    LINEINFO(stmt);
    gen_reflect(outfile, stmt);
    break;
  case S_COMM:
    gen_iron_comm(outfile,stmt);
    break;
  case S_ZPROF:
    LINEINFO(stmt);
    gen_zprof(outfile, stmt);
    break;
  default:
    INT_FATAL(stmt, "Bad statement type (%d) in gen_stmt()",T_TYPE(stmt));
  }

  gptr = T_POST(stmt);
  while (gptr != NULL) {
    symboltable_t* pst = G_IDENT(gptr);

    force_pst_done(outfile, pst);

    gptr = G_NEXT(gptr);
  }
}


/* changes made to this routine will probably need to be made to Mgen.c's
   gen_mloop_stmtls as well */

void gen_stmtls(FILE* outfile, statement_t* stmtls) {
  while (stmtls != NULL) {
    gen_stmt(outfile, stmtls);

    stmtls = T_NEXT(stmtls);
  }
}

static void gen_function(FILE* outfile, expr_t *expr) {
  expr_t *f_expr;			/*** expr for function ***/
  expr_t *exprls;			/*** expr list of arguments ***/
  genlist_t *glp;			/*** genlist of parameters ***/
  symboltable_t *pst;			/*** symbol table entry ***/
  int found_ensemble;
  datatype_t *pdt;

  pdt = T_TYPEINFO(expr);
  f_expr = nthoperand(expr, 1);
  exprls = nthoperand(expr, 2);

  if (codegen_imaginary == 1 && !datatype_complex(pdt)) {
    /* real functions don't have an imaginary component */
    fprintf(outfile,"0");
    return;
  }

  gen_expr(outfile, f_expr);			/*** output function name ***/

  fprintf(outfile, "(");

  pst = T_IDENT(f_expr);

  if (!T_PROMOTED_FUNC(expr)) {
    brad_no_access++;
  }
  if ((pst == NULL) || (S_DTYPE(pst) == NULL)) {
    /*** assume it is an external function, e.g. sin() ***/
    if (strncmp(S_IDENT(T_IDENT(f_expr)), "_", 1) == 0) {
      function_call++;
    }
    for (glp = NULL; (exprls != NULL); exprls = T_NEXT(exprls)) {
      gen_noncomplex_expr(outfile, exprls);      /*** output the argument ***/
      if (T_NEXT(exprls) != NULL) {
	fprintf(outfile, ",");
      }
    }
    if (strncmp(S_IDENT(T_IDENT(f_expr)), "_", 1) == 0) {
      function_call--;
    }
  } else {
    /*** match the arguments to the parameter list ***/
    for (glp = T_PARAMLS(S_FUN_BODY(pst));
	 ((exprls != NULL) && (glp != NULL));
	 exprls = T_NEXT(exprls), glp = G_NEXT(glp)) {
      /*** assume that the number of arguments to the function matches ***/
      /*** the number of formal parameters in the declaration ***/

      pst = G_IDENT(glp);			/*** formal parameter ***/

      found_ensemble=0;
      if (D_CLASS(S_DTYPE(pst)) == DT_GENERIC_ENSEMBLE ||
	  datatype_find_ensemble(S_DTYPE(pst)) != NULL) {
	function_call++;		/*** flag to gen_expr() ***/
	                                /*** supresses access macros ***/
	found_ensemble=1;
      }
      switch (D_CLASS(S_DTYPE(pst))) {
      case DT_BOOLEAN:
	if (S_SUBCLASS(pst) == SC_INOUT || S_SUBCLASS(pst) == SC_OUT) {
	  if (T_TYPEINFO(exprls) != pdtBOOLEAN) {
	    fprintf(outfile,"(boolean *)");
	  }
	  fprintf(outfile,"&");
	}
	break;
      case DT_FILE:
      case DT_INTEGER:
      case DT_ENUM:
      case DT_REAL:
      case DT_CHAR:
      case DT_SHORT:
      case DT_LONG:
      case DT_DOUBLE:
      case DT_QUAD:
      case DT_UNSIGNED_INT:
      case DT_UNSIGNED_SHORT:
      case DT_UNSIGNED_LONG:
      case DT_UNSIGNED_BYTE:
      case DT_SIGNED_BYTE:
      case DT_STRUCTURE:
      case DT_OPAQUE:
      case DT_COMPLEX:
      case DT_DCOMPLEX:
      case DT_QCOMPLEX:
	if (S_SUBCLASS(pst) == SC_INOUT || S_SUBCLASS(pst) == SC_OUT) {
	  fprintf(outfile, "&");
	}
	break;
      case DT_STRING:
      case DT_PROCEDURE:
      case DT_ARRAY:
      case DT_REGION:
      case DT_DISTRIBUTION:
      case DT_GRID:
	break;
	/* BLC -- added case for Generic Ensembles 4/5/94 */
      case DT_GENERIC_ENSEMBLE:
      case DT_ENSEMBLE:

	/*** BLC -- 5/26/94 has to search deep in the datatype.  Abstracted
	  it out above... */

	/*** We are passing elements and entire ensemble into a function. ***/
	/*function_call++;*/		/*** flag to gen_expr() ***/
					/*** supresses access macros ***/
	break;
	/* BLC -- added case for Generic 4/11/94 */
      case DT_GENERIC:
	break;
      case DT_VOID:
      case DT_SUBPROGRAM:
      default:
	INT_FATAL(NULL, "Invalid type class %d for formal parameter in "
		  "gen_function()",D_CLASS(S_DTYPE(pst)));
	break;
      }
      gen_noncomplex_expr(outfile, exprls);      /*** output the argument ***/
      if (T_NEXT(exprls) != NULL) {
	fprintf(outfile, ",");
      }

      /* BLC -- searching deep for ensembles now.  This flag reflects the
	 result of the above search. */

      if (found_ensemble) {
	function_call--;
      }
    }
    if ((exprls != NULL) || (glp != NULL)) {
      /*** arguments and parameters did not match up correctly ***/
      /*** there is a bug in the parser! ***/
      INT_FATAL(NULL, "Number of arguments to function %s() does not match "
		"number of formal parameters in gen_function()",
		S_IDENT(T_IDENT(f_expr)));
    }
  }
  fprintf(outfile, ")");
  if (!T_PROMOTED_FUNC(expr)) {
    brad_no_access--;
  }

}


/* FUNCTION: search_upfor_uat - search up from expr e, stop at BIAT and return
 * NOTE: modeled after search_upfor_ensemble_dt()
 * echris  1-20-95
 * BLC: moved here 12/4/98
 */
static expr_t *search_upfor_uat(expr_t *e, int *found) {
  int           at_top = FALSE;
  expr_t        *parent;
  
  *found = FALSE;

  if (e == NULL)
    return NULL;

  while (!at_top)   {  /* exits with e as a var, arrayref or bidot */
    if (T_PARENT(e) && T_OPLS(T_PARENT(e)) == e) {
      parent = T_PARENT(e);
    }
    else  {
      parent = NULL;
    }
    if (parent) {
      switch (T_TYPE(T_PARENT(e))) {
      case VARIABLE:
      case ARRAY_REF:
      case BIDOT:
	if (T_PARENT(e)  && T_OPLS(T_PARENT(e)) == e) {
	  e = T_PARENT(e);
	}
	else 
	  at_top = TRUE;
	break;
      case BIAT:
	at_top = TRUE;
	*found = TRUE;
	e = T_PARENT(e);
	break;
      default:
	at_top = TRUE;
	break;
      }
    }
    else {
      at_top = TRUE;
    }
  }
  return e;
}


/* FUNCTION: search_upfor_reg
 * WAS: search_upfor_ensemble_dt
 * Search up the expression tree for a BIAT via search_upfor_uat().  If one
 * is not found, return NULL (at_found cleared).  If a BIAT is found, set
 * at_found and return its datatype (which is stored in its ensemble_var 
 * field).
 *
 * This function replaces a function by the same name written by rea last
 * appearing in r1.184.  The old version was not sufficient for it did not
 * make use of typeinfo.  The new version is much simpler.
 * echris  1-28-95
 */

static expr_t *search_upfor_reg(expr_t *e, int *at_found) {
  expr_t *found_expr;

  if (expr_is_qmask(e)) {
    return T_TYPEINFO_REG(e);
  } else {

    found_expr = search_upfor_uat(e, at_found);

    INT_COND_FATAL((found_expr!=NULL), NULL,
		   "search_upfor_uat() returned NULL; at_found is true");
    return T_TYPEINFO_REG(found_expr);
  }
}


/********************************************
  gen_arrayref(exprls)
  
  Prints out array references.
  ***********************************************/
int get_array_size_multiplier(dimension_t* dim) {
  int size = 1;

  if (dim == NULL) {
    INT_FATAL(NULL, "no multiplier, last dimension?");
  }
  while (dim != NULL) {
    if (!expr_computable_const(DIM_LO(dim)) ||
	!expr_computable_const(DIM_HI(dim))) {
      USR_FATAL(NULL, "Cannot initialize non-constant indexed arrays\nIf this is a"
		"problem, contact zpl-info@cs.washington.edu");
    }
    size *= expr_intval(DIM_HI(dim)) - expr_intval(DIM_LO(dim)) + 1;
    dim = DIM_NEXT(dim);
  }
  return size;
}


expr_t* build_array_size_multiplier(dimension_t* dim) {
  expr_t* expr = NULL;

  if (dim == NULL) {
    INT_FATAL(NULL, "no multiplier, last dimension?");
  }
  while (dim != NULL) {
    expr_t* tmp;
    tmp = build_binary_op(BIMINUS, copy_expr(DIM_LO(dim)),  copy_expr(DIM_HI(dim)));
    tmp = build_binary_op(BIPLUS, new_const_int(1), tmp);
    if (expr) {
      expr = build_binary_op(BITIMES, expr, tmp);
    }
    else {
      expr = tmp;
    }
    dim = DIM_NEXT(dim);
  }
  return expr;
}

void gen_arrayref(FILE* outfile, expr_t *exprls,datatype_t *pdt) {
  dimension_t * pdim;
  
  if (pdt==NULL) {
    INT_FATAL(NULL, "EEK -- Brad, something's wrong gen_arrayref()!!");
  }
  
  pdim = D_ARR_DIM(pdt);
  
  if (exprls != NULL) {
    fprintf(outfile, "[");
    for (; exprls != NULL; exprls = T_NEXT(exprls)) {
      if (pdim == NULL) {
	INT_FATAL(NULL, "late type mismatch, too few array dimensions");
      }
      if (DIM_NEXT(pdim)) {
	fprintf(outfile, "(");
      }
      gen_expr(outfile, exprls);
      fprintf(outfile, " - ");
      gen_expr(outfile, DIM_LO(pdim));
      if (DIM_NEXT(pdim)) {
	fprintf(outfile, ")*(");
	gen_expr(outfile, build_array_size_multiplier(DIM_NEXT(pdim)));
	fprintf(outfile, ")");
      }
      if (T_NEXT(exprls) != NULL) {
	fprintf(outfile, "+");
      }
      pdim = DIM_NEXT(pdim);
    }
    fprintf(outfile, "]");
  } else {
    /* NULL ARRAY REFERENCE */
  }
}


static void gen_multiref(FILE* outfile, expr_t *exprls) {
  if (exprls != NULL) {
    fprintf(outfile,"[");
    while (exprls != NULL) {
      gen_expr(outfile, exprls);
      exprls = T_NEXT(exprls);
      if (exprls) {
	fprintf(outfile,"][");
      }
    }
    fprintf(outfile,"]");
  }
}


static void gen_complex_extension(FILE *outfile) {
  switch (codegen_imaginary) {
  case -1:
    break;
  case 0:
    fprintf(outfile,".re");
    break;
  case 1:
    fprintf(outfile,".im");
    break;
  default:
    INT_FATAL(NULL,"Unknown imaginary type");
  }
}


static void gen_biopgets(FILE* outfile,expr_t* expr) {
  expr_t* lhs;
  expr_t* rhs;
  binop_t subtype;
  datatype_t* lhsdt;
  datatype_t* rhsdt;
  int lcomplex=0;
  int rcomplex=0;
  int rcomplexcomplex=0;
  
  lhs = T_OPLS(expr);
  lhsdt = T_TYPEINFO(lhs);
  rhs = T_NEXT(lhs);
  rhsdt = T_TYPEINFO(rhs);
  subtype = T_SUBTYPE(expr);

  lcomplex = datatype_complex(lhsdt);
  rcomplex = datatype_complex(rhsdt);
  if (lcomplex && rcomplex) {
    rcomplexcomplex = !expr_is_atom(rhs);
    if (rcomplexcomplex && codegen_imaginary == 0) {
      gen_setup_complex_temp(outfile,rhs,lhsdt,2);
    }
  }

  switch (subtype) {
  case MIN:
  case MAX:
  case AND:
  case OR:
    fprintf(outfile,"_");
    switch (subtype) {
    case MIN:
      fprintf(outfile,"MIN");
      break;
    case MAX:
      fprintf(outfile,"MAX");
      break;
    case AND:
      fprintf(outfile,"AND");
      break;
    case OR:
      fprintf(outfile,"OR");
      break;
    default:
      INT_FATAL(NULL,"big problem in gen_biopgets");
      break;
    }
    fprintf(outfile,"_STMT(");
    gen_expr(outfile, lhs);
    fprintf(outfile,",");
    gen_expr(outfile, rhs);
    fprintf(outfile,");\n");
    break;
  case TIMES:
  case DIVIDE:
    if (lcomplex && rcomplex) {
      if (subtype == TIMES) {
	fprintf(outfile,"_MULT");
      } else {
	fprintf(outfile,"_DIV");
      }
      fprintf(outfile,"_STMT_");
      gen_pdt(outfile,lhsdt,PDT_NAME);
      fprintf(outfile,"(");
      gen_noncomplex_expr(outfile,lhs);
      fprintf(outfile,",");
      if (rcomplexcomplex) {
	gen_complex_temp_name(outfile,lhsdt,2);
      } else {
	gen_noncomplex_expr(outfile,rhs);
      }
      fprintf(outfile,");\n");
      break;
    }
  default:
    gen_expr(outfile, lhs);
    fprintf(outfile, " %s ",OpGetsName[subtype]);
    if (codegen_imaginary == 1 && lcomplex && !rcomplex && 
	(subtype == TIMES || subtype == DIVIDE)) {
      /* this case is for *= or /= using a scalar rhs... */
      gen_real_expr(outfile, rhs);
    } else {
      if (rcomplexcomplex) {
	gen_complex_temp_name(outfile,lhsdt,2);
	gen_complex_extension(outfile);
      } else {
	gen_expr(outfile, rhs);
      }
    }
    if (datatype_complex(lhsdt) && codegen_imaginary==0) {
      fprintf(outfile,";\n");
      gen_imag_expr(outfile, expr);
    }
  }
}

static void gen_dir_assign(FILE* outfile, expr_t* expr) {
  int numdims;
  datatype_t* lhstype;
  datatype_t* rhstype;
  symboltable_t* lhs;
  symboltable_t* rhs;
  int i;

  if (T_TYPE(expr) != BIASSIGNMENT) {
    INT_FATAL(NULL, "Critical error in gen_dir_assign");
  }
  if (T_OPLS(expr) == NULL) {
    INT_FATAL(NULL, "Critical error in gen_dir_assign");
  }
  if (T_NEXT(T_OPLS(expr)) == NULL) {
    INT_FATAL(NULL, "Critical error in gen_dir_assign");
  }
  if (D_CLASS(T_TYPEINFO(T_OPLS(expr))) != DT_DIRECTION) {
    INT_FATAL(NULL, "Critical error in gen_dir_assign");
  }
  if (D_CLASS(T_TYPEINFO(T_NEXT(T_OPLS(expr)))) != DT_DIRECTION) {
    INT_FATAL(NULL, "Critical error in gen_dir_assign");
  }
  if (S_IS_CONSTANT(T_IDENT(T_OPLS(expr)))) {
    INT_FATAL(NULL, "Critical error in gen_dir_assign");
  }

  lhs = T_IDENT(T_OPLS(expr));
  rhs = T_IDENT(T_NEXT(T_OPLS(expr)));
  lhstype = T_TYPEINFO(T_OPLS(expr));
  rhstype = T_TYPEINFO(T_NEXT(T_OPLS(expr)));
  numdims = D_DIR_NUM(lhstype);

  if (D_DIR_NUM(lhstype) != D_DIR_NUM(rhstype)) {
    INT_FATAL(NULL, "Direction dimensions do not match in gen_dir_assign");
  }

  fgenf(outfile, "_DIR_ASSIGN_%dD(%E", numdims, T_OPLS(expr));
  for (i = 0; i < numdims; i++) {
    switch (D_DIR_SIGN(lhstype, i)) {
    case SIGN_ZERO:
      fprintf(outfile, ", ZERO");
      break;
    case SIGN_POS:
    case SIGN_POSONE:
      fprintf(outfile, ", POS");
      break;
    case SIGN_NEG:
    case SIGN_NEGONE:
      fprintf(outfile, ", NEG");
      break;
    case SIGN_UNKNOWN:
      fprintf(outfile, ", UNKNOWN");
      break;
    }
    fgenf(outfile, ", %E", expr_direction_value(IN_VAL(S_VAR_INIT(rhs)), i));
  }
  fgenf(outfile, ")");
}


static void gen_value(FILE* outfile, expr_t* val, datatype_t* pdt) {
  int i;

  switch (D_CLASS(pdt)) {
  case DT_DIRECTION:
    fprintf(outfile, "{");
    for (i=0; i<MAXRANK; i++) {
      expr_t* compexpr = expr_direction_value(val,i); 

      if (i) {
	fprintf(outfile, ", ");
      }
      if (compexpr) {
	gen_expr(outfile, compexpr);
      } else {
	fprintf(outfile, "0");
      }
    }
    fprintf(outfile, "}");
    break;
  case DT_REGION:
    fprintf(outfile, "/* need to generate region value here */");
    break;
  default:
    INT_FATAL(NULL, "Unexpected type in gen_value\n");
  }
}

/***************************************************
  Generate expressions 
  (Updated to include expressions from the bodies of mloops) 
  Ruth Anderson  5/12/93
  
  Assumptions:  
  1) that all statements referring to regions inside of
  an mloop refer to ensembles that are the same region type (i.e. same
  number of dimensions, same dimensions) as the mloop is. 
  2) Remember, this code is used to generate declarations also.
  *****************************************************************/

void gen_expr(FILE* outfile, expr_t* expr) {
  expr_t *reg;
  int        at_found=0;
  int        closeParens;	/* True if we need to close parens.  Used to
				 * print out record parameters that are passed
				 * by references.  eg. (*rec).field;
				 */
  int use_walker;
  int save_imaginary;
  expr_t *rexpr;
  expr_t *lexpr;
  int rcomplex;
  int lcomplex;
  symboltable_t* varpst;
  int instnum;

  closeParens = FALSE;
  if (expr == NULL) {
    return;
  }
  if (in_mloop) {
    if (T_PARENT(expr) == NULL) {
      replacement_t* repl;
      repl = T_MLOOP_REPLS(current_mloop);
      while (repl != NULL) {
	if (T_TYPE(expr) != BIASSIGNMENT ||
	    assignment_replace(current_mloop, expr)) {
	  replaceall_expr (expr, repl->origexpr, repl->newexpr);
	}
	repl = repl->next;
      }
    }
  }



  if ((codegen_imaginary == 1) && expr_is_atom(expr) && 
      !datatype_complex(T_TYPEINFO(expr))) {
    /* All simple expressions should generate as 0 if we're generating
       the imaginary components */
    fprintf(outfile,"0");
    return;
  }

  if (expr_requires_parens(expr)) {
    fprintf(outfile, "(");
  }

  /* handle all walkers and bumpers here */
  if (expr_at_ensemble_root(expr)) {
    /* SPARSE CHANGE #1: to make sparse_compute work and other things
       fail, switch the comment on the following two lines: */
 /* if (!expr_is_indexi(expr)) { */
    if (!expr_is_indexi(expr) && D_CLASS(T_TYPEINFO(expr)) != DT_REGION) { 
      if (!mask_no_access && !function_call && !brad_no_access) {
	if (T_RANDACC(expr) != NULL) {
	  gen_access_begin(outfile, expr, 1);
	  gen_randaccess_end(outfile, expr);
	  if (datatype_complex(T_TYPEINFO(expr))) {
	    gen_complex_extension(outfile);
	  }
	} else {
	  use_walker = walker_sparse(expr) || 
	    (new_access && !disable_new_access && walker_in_use(expr));
	  if (use_walker) {
	    use_walker = gen_walker_with_offset(outfile,expr);
	  }
	  if (!use_walker) {
	    if (!mask_no_access && !function_call && !brad_no_access) {
	      gen_access_begin(outfile,expr,1);
	      gen_access_end(outfile,expr,currentDirection);
	    }
	  }
	  if (datatype_complex(T_TYPEINFO(expr))) {
	    gen_complex_extension(outfile);
	  }
	  if (expr_requires_parens(expr)) {
	    fprintf(outfile, ")");
	  }
	}
	return;
      }
    }
  }


  switch (T_TYPE(expr)) {
  case VARIABLE: 
    DB0(30, "\nVARIABLE\n");
    
    /*************************************************************
      In general, the last node encountered on a downward traversal of
      a variable operand expression will be the VARIABLE node. (I think this
      is always the case.)  Thus it is at this point that printing
      out of the expression begins on the returning upwards traversal.
      
      A test is made to see if this variable includes an ensemble.
      (Note that structures as complex as an ensemble of arrays of
      records OR an array of records of arrays of ensembles are possible.
      The only restriction being that ensembles cannot be nested, i.e.
      the word "ensemble" can only appear once in a description of
      the variable.)  This test requires tracing back up the parse tree
      to see if this VARIABLE node is an ensemble or if any nodes
      above (i.e. BIDOT or ARRAY_REF nodes) contain a DT_ENSEMBLE.
      
      If the an ensemble is found, then an access macro must be used to 
      access this variable. 
      ***********************************************************/

    arrayref_height = -1;  /* reset arrayref_height */

    /* Decide if we need to create a cast and an access macro. 
       Takes a ptr to a variable, searches UP the parsetree for an ensemble */
    if (!expr_contains_indexi(expr)) {
      reg = search_upfor_reg(expr, &at_found);
    }
    
    varpst = T_IDENT(expr);

    /* This converts user's indexi references to indexi[1] */
    {
      int indexinum = symtab_is_ia_index(varpst);
      int dim = 1;
      
      if (indexinum) {
	if (T_PARENT(expr) && 
	    (T_TYPE(T_PARENT(expr)) == ARRAY_REF) &&
	    (T_OPLS(T_PARENT(expr)) == expr)) {
	  dim = expr_intval(T_NEXT(expr));
	  
	  varpst = get_blank_arrayref_index(indexinum, dim);
	}
      }
    }

    /* Dereference reference parameters */
    if (S_CLASS(varpst) == S_PARAMETER && S_SUBCLASS(varpst) == SC_INOUT) { /** IN AND OUT **/
      if (D_CLASS(S_DTYPE(varpst)) == DT_STRUCTURE ||
	  (reg == NULL && expr_find_at_up(expr) == NULL && /* !ensemble */
	   !pointeraccess &&
	   D_CLASS(S_DTYPE(varpst)) != DT_GRID &&
	   D_CLASS(S_DTYPE(varpst)) != DT_DISTRIBUTION &&
	   D_CLASS(S_DTYPE(varpst)) != DT_REGION &&
	   D_CLASS(S_DTYPE(varpst)) != DT_ARRAY)) {
	fprintf(outfile, "(*");
	closeParens = TRUE;
      }
    }

    if (symtab_is_global(varpst) && symtab_var_has_lvalue(varpst)) {
      /*** global variable *** use private access macro ***/
      priv_named_access(outfile, S_IDENT(varpst));
    } else {
      fprintf(outfile, "%s",S_IDENT(varpst));
      instnum = S_NUM_INSTANCES(varpst);
      if (instnum > 1) {
	instnum = (instnum + T_INSTANCE_NUM(expr) +
		   mgen_unroll_number(S_INSTANCE_LOOP(varpst)))%instnum;
	fprintf(outfile,"%d",instnum);
      }
    }
    if (closeParens) {
      fprintf(outfile,")");
    }
    if (datatype_complex(T_TYPEINFO(expr)) && 
	(T_TYPEINFO_REG(expr) == NULL || 
	 expr_is_grid_scalar(T_TYPEINFO_REG(expr)))) {
      gen_complex_extension(outfile);
    }
    break;
  case SIZE:
    if (T_SUBTYPE(expr)) {
      gen_pdt(outfile, T_TYPEINFO(expr), PDT_CAST);
    }
    else {
      fprintf(outfile,"sizeof(");
      gen_pdt(outfile,T_TYPEINFO(expr),PDT_SIZE);
      fprintf(outfile,")");
    }
    break;
  case CONSTANT:
    DB0(30, "\nCONSTANT\n");
    varpst = T_IDENT(expr);
    if (printing_string && *(S_IDENT(varpst)) == '"') {
      fprintf(outfile, "\\\"\"");
    }
    if (S_IDENT(varpst) != NULL) {
      fprintf(outfile, "%s", S_IDENT(varpst));
    } else {
      gen_value(outfile, IN_VAL(S_VAR_INIT(varpst)), S_DTYPE(varpst));
    }
    if (printing_string && *(S_IDENT(varpst)) == '"') {
      fprintf(outfile, "\"\\\"");
    }
    if (datatype_complex(T_TYPEINFO(expr)) && 
	(T_TYPEINFO_REG(expr) == NULL || 
	 expr_is_grid_scalar(T_TYPEINFO_REG(expr)))) {
      gen_complex_extension(outfile);
    }
    break;
  case ARRAY_REF:
    DB1(30, "\nARRAY_REF %d\n", T_TYPE(expr));
    if (T_OPLS(expr) == NULL) {
      INT_FATAL(NULL, "no array name in array reference in gen_expr()");
      break;
    }

    save_imaginary = codegen_imaginary;
    if (datatype_complex(T_TYPEINFO(expr)) && 
        !datatype_complex(T_TYPEINFO(T_OPLS(expr)))) {
      codegen_imaginary = -1;
    }

    gen_expr(outfile, nthoperand(expr,1));

    /* indexing into indexi... taken care of in VARIABLE case, so break */
    if (T_TYPE(T_OPLS(expr)) == VARIABLE &&
	symtab_is_ia_index(T_IDENT(T_OPLS(expr)))) {
      break;
    }

    arrayref_height++;
    { 
      expr_t * arr_base;
      datatype_t * pdt;

      arr_base=nthoperand(expr,1);

      pdt=T_TYPEINFO(arr_base);
      if (pdt == NULL) {
	if (T_TYPE(arr_base)==BIAT) {
	  arr_base = arr_base->expr_p;
	  pdt = T_TYPEINFO(arr_base);
	}
      } else {
	if (T_SUBTYPE(expr) == ARRAY_IND) {
	  if (D_CLASS(pdt) != DT_ARRAY && D_CLASS(pdt) != DT_GENERIC && D_CLASS(pdt) != DT_GENERIC_ENSEMBLE) {
	    /*
	      INT_FATAL(T_STMT(expr), "Indexing a non-array (BLC)");
	      exit(3);
	    */
	  }
	} else {
	  /* should probably check here to make sure we're indexing a 
	     multiarray */
	}
      }

      if (T_SUBTYPE(expr) == ARRAY_IND) {
	gen_arrayref(outfile, nthoperand(expr,2),pdt);
      } else {
	printf("IS THIS EVER CALLED????");
	mi_reg = pst_qreg[D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr)))];
	gen_multiref(outfile, nthoperand(expr,2));
	mi_reg =NULL;
      }

    }

    codegen_imaginary = save_imaginary;
    if (datatype_complex(T_TYPEINFO(expr)) &&
        !datatype_complex(T_TYPEINFO(T_OPLS(expr)))) {
      gen_complex_extension(outfile);
    }
    break;
  case FUNCTION:			/*** annotated by sungeun ***/
    /*** function call ***/
    DB0(30, "\nFUNCTION\n");

    if (T_OPLS(expr) == NULL) {
      INT_FATAL(NULL, "no func name in func call in gen_expr()");
      break;
    }
    gen_function(outfile, expr);
    break;
  case BIDOT:
    save_imaginary = codegen_imaginary;
    if (datatype_complex(T_TYPEINFO(T_OPLS(expr)))) {
      if (!strcmp(S_IDENT(T_IDENT(expr)),"im") ||
	  !strcmp(S_IDENT(T_IDENT(expr)),"re")) {
	codegen_imaginary = -1;
      }
    }
    gen_expr(outfile, T_OPLS(expr));
    fprintf(outfile, ".%s",S_IDENT(T_IDENT(expr)));
    codegen_imaginary = save_imaginary;
    break;
  case BIEXP:
    fprintf(outfile,"pow((double)");
    gen_expr(outfile, T_OPLS(expr));
    fprintf(outfile,",(double)");
    gen_expr(outfile, T_NEXT(T_OPLS(expr)));
    fprintf(outfile,")");
    break;
  case DISTRIBUTION:
    /*    if (D_CLASS(T_TYPEINFO(T_OPLS(expr))) != DT_DISTRIBUTION) {
      fprintf(outfile, "_REG_DIST(_ARR_DECL_REG(");
      fprintf(outfile, "%s", S_IDENT(expr_find_ens_pst(T_OPLS(expr))));
      pointeraccess++;
      gen_expr(outfile, T_OPLS(expr));
      pointeraccess--;
      fprintf(outfile, "))");
      }
    else {*/
      if (T_OPLS(expr)) { /*** I don't know why I'm doing this ***/
	fprintf(outfile, "%s", S_IDENT(T_IDENT(T_OPLS(expr))));
      }
      else {
        fprintf(outfile, "&%s", S_IDENT(T_IDENT(expr)));
      }
    break;
  case BIAT:
    /* If the new_access flag is set (and not disabled), stop the traversal
     * and output a cast and a _Walker_x_, e.g. (*(int *) (_Walker_0_)).
     * Otherwise, continue traversal down. */

    currentDirection = T_NEXT(T_OPLS(expr));
    if (T_SUBTYPE(expr) == AT_RANDACC) {
      gen_access_begin(outfile,T_OPLS(expr),1);
      gen_randaccess_end(outfile,expr);
      if (datatype_complex(T_TYPEINFO(expr))) {
	gen_complex_extension(outfile);
      }
    } else {
      gen_expr(outfile, T_OPLS(expr));
    }

    currentDirection = NULL;
    break;
  case REDUCE:
    INT_FATAL(NULL, 
	      "reduce in gen_expr (reduces should be handled in gen_stmt)");
    break;
  case SCAN:
    INT_FATAL(NULL, "scan in gen_expr (scans should be handled in gen_stmt)");
    break;
  case FLOOD:
    INT_FATAL(NULL, 
	      "flood in gen_expr (floods should be handled in gen_stmt)");
    break;
  case PERMUTE:
    INT_FATAL(NULL,"scatter/gather in gen_expr (should be handled in gen_stmt");
    break;
  case BIASSIGNMENT:
    if (D_CLASS(T_TYPEINFO(T_OPLS(expr))) == DT_DIRECTION) {
      gen_dir_assign(outfile, expr);
      break;
    }
    lexpr = left_expr(expr);
    rexpr = right_expr(expr);
    lcomplex = datatype_complex(T_TYPEINFO(lexpr)); 
    rcomplex = datatype_complex(T_TYPEINFO(rexpr));
    if (lcomplex && T_TYPE(rexpr) == FUNCTION && rcomplex) {
      lcomplex = 0;
      gen_noncomplex_expr(outfile,lexpr);
    } else {
      gen_expr(outfile, left_expr(expr));
    }
    fprintf(outfile, " %s ",OpName[(int)T_TYPE(expr)]);
    gen_expr(outfile, right_expr(expr));
    if (lcomplex && codegen_imaginary==0) {
      fprintf(outfile,";\n");
      gen_imag_expr(outfile, expr);
    }
    break;

  case BIOP_GETS:
    gen_biopgets(outfile,expr);
    break;

  case BIEQUAL:
    lexpr = T_OPLS(expr);
    rexpr = T_NEXT(T_OPLS(expr));
    rcomplex = datatype_complex(T_TYPEINFO(rexpr));
    lcomplex = datatype_complex(T_TYPEINFO(lexpr));
    if (rcomplex || lcomplex) {
      fprintf(outfile,"(");
    }
    gen_real_expr(outfile, lexpr);
    fprintf(outfile," == ");
    gen_real_expr(outfile, rexpr);
    if (rcomplex || lcomplex) {
      fprintf(outfile,") && (");
      gen_imag_expr(outfile, lexpr);
      fprintf(outfile," == ");
      gen_imag_expr(outfile, rexpr);
      fprintf(outfile,")");
    }
    break;
  case BINOT_EQUAL:
    lexpr = T_OPLS(expr);
    rexpr = T_NEXT(T_OPLS(expr));
    rcomplex = datatype_complex(T_TYPEINFO(rexpr));
    lcomplex = datatype_complex(T_TYPEINFO(lexpr));
    if (rcomplex || lcomplex) {
      fprintf(outfile,"(");
    }
    gen_real_expr(outfile, lexpr);
    fprintf(outfile," != ");
    gen_real_expr(outfile, rexpr);
    if (rcomplex || lcomplex) {
      fprintf(outfile,") || (");
      gen_imag_expr(outfile, lexpr);
      fprintf(outfile," != ");
      gen_imag_expr(outfile, rexpr);
      fprintf(outfile,")");
    }
    break;
  case BITIMES:
    lexpr = T_OPLS(expr);
    rexpr = T_NEXT(T_OPLS(expr));
    lcomplex = datatype_complex(T_TYPEINFO(lexpr));
    rcomplex = datatype_complex(T_TYPEINFO(rexpr));
    if (lcomplex && rcomplex) {
      fprintf(outfile,"(");
      if (codegen_imaginary == 0) {
	gen_real_expr(outfile, lexpr);
	fprintf(outfile," * ");
	gen_real_expr(outfile, rexpr);
	fprintf(outfile," - ");
	gen_imag_expr(outfile, lexpr);
	fprintf(outfile," * ");
	gen_imag_expr(outfile, rexpr);
      } else {
	gen_real_expr(outfile, lexpr);
	fprintf(outfile," * ");
	gen_imag_expr(outfile, rexpr);
	fprintf(outfile," + ");
	gen_imag_expr(outfile, lexpr);
	fprintf(outfile," * ");
	gen_real_expr(outfile, rexpr);
      }
      fprintf(outfile,")");
    } else if (lcomplex || rcomplex) {
      if (lcomplex) {
	gen_expr(outfile, lexpr);
      } else {
	gen_real_expr(outfile, lexpr);
      }
      fprintf(outfile," * ");
      if (rcomplex) {
	gen_expr(outfile, rexpr);
      } else {
	gen_real_expr(outfile, rexpr);
      }
    } else {
      gen_expr(outfile, lexpr);
      fprintf(outfile," * ");
      gen_expr(outfile, rexpr);
    }
    break;
  case BIDIVIDE:
    lexpr = T_OPLS(expr);
    rexpr = T_NEXT(T_OPLS(expr));
    lcomplex = datatype_complex(T_TYPEINFO(lexpr));
    rcomplex = datatype_complex(T_TYPEINFO(rexpr));
    if (rcomplex) { /* rcomplex implies that we should do the messy division */
      fprintf(outfile,"((");
      if (codegen_imaginary == 0) {
	gen_real_expr(outfile, lexpr);
	fprintf(outfile," * ");
	gen_real_expr(outfile, rexpr);
	fprintf(outfile," + ");
	gen_imag_expr(outfile, lexpr);
	fprintf(outfile," * ");
	gen_imag_expr(outfile, rexpr);
      } else {
	gen_imag_expr(outfile, lexpr);
	fprintf(outfile," * ");
	gen_real_expr(outfile, rexpr);
	fprintf(outfile," - ");
	gen_real_expr(outfile, lexpr);
	fprintf(outfile," * ");
	gen_imag_expr(outfile, rexpr);
      }
      fprintf(outfile,") / (");
      gen_real_expr(outfile, rexpr);
      fprintf(outfile,"*");
      gen_real_expr(outfile, rexpr);
      fprintf(outfile," + ");
      gen_imag_expr(outfile, rexpr);
      fprintf(outfile,"*");
      gen_imag_expr(outfile, rexpr);
      fprintf(outfile,"))");
    } else {
      if (lcomplex) {
	gen_expr(outfile, lexpr);
	fprintf(outfile," / ");
	gen_real_expr(outfile, rexpr);
      } else {
	if (codegen_imaginary == 1) {
	  /* we're generating imaginary stuff and neither l or r is complex,
	     so it'll come out as something / 0 and cause an error.  Just spit
	     out a 0 which is what we must want? */
	  fprintf(outfile,"0");
	} else {
	  gen_expr(outfile, lexpr);
	  fprintf(outfile," / ");
	  gen_expr(outfile, rexpr);
	}
      }
    }
    break;
  case BIPREP:
    gen_name(outfile, expr);
    break;
  default:
    if (T_IS_UNARY(T_TYPE(expr))) {
      fprintf(outfile, " %s",OpName[(int)T_TYPE(expr)]);
      gen_expr(outfile, T_OPLS(expr));
    } else if (T_IS_BINARY(T_TYPE(expr))) {
      /*rea - 11-2, returns an illegal ptr. for re.floor := 43 */
      gen_expr(outfile, left_expr(expr));
      fprintf(outfile, " %s ",OpName[(int)T_TYPE(expr)]);
      gen_expr(outfile, right_expr(expr));
    } else {
      INT_FATAL(NULL, "Bad exprtype (%d) in gen_expr()",T_TYPE(expr));
    }
  }
  if (expr_requires_parens(expr)) {
    fprintf(outfile, ")");
  }
}


/****************************************************
  find_element_size(dtptr, outfile)
  1-22-94
  Ruth Anderson
  
  Takes a datatype ptr which is presumably a ptr to a DT_ENSEMBLE and a
  pointer to a file.  Prints out to the file an expression that will
  calculate the size of one "element" of the ensemble.
  
  Examples:
  For an ensemble of integers, --> sizeof(int)
  For an ensemble of 2Darrays of integers, --> 
  (arraydim1hi-arraydim1lo+1)*(arraydim2hi-arraydim2lo+1)*sizeof(int)
  For an ensemble of records of arrays of reals, --> sizeof(recordtype)
  
  Assumptions:
  1) The sizeof operator will take integer, real, and record types.
  
  2) When DIM_CLASS == VARIABLE, we have an expression as an
  array bound.  This is assumed to be a simple integer.  We currently
  do not support array bounds that are expresions such as 1+3 or n+4 etc.
  (I think we now handle array bounds that are expressions. rea 3-11-94)

  BLC -- 1/13/96 -- changed to support simple elements as well 

  ****************************************************/
void find_element_size(datatype_t *ensemble_dt, FILE *outfile) {
  dimension_t   *dimptr;
  
  /* Switch File pointers for the calls to gen_expr.  Be sure to
     switch them back at the end of this call. */
  
  if ((ensemble_dt == NULL) || (D_CLASS(ensemble_dt) != DT_ENSEMBLE)) {
  } else {  
    ensemble_dt = D_ENS_TYPE(ensemble_dt);
  }
  
  while (ensemble_dt)
    switch (D_CLASS(ensemble_dt)) {
    case DT_ARRAY:
      dimptr = D_ARR_DIM(ensemble_dt);
      while (dimptr) {
	fprintf(outfile, "((");
	gen_expr(outfile, DIM_HI(dimptr));
	fprintf(outfile, ")-(");
	gen_expr(outfile, DIM_LO(dimptr));
	fprintf(outfile, ")+1)*");

	dimptr = DIM_NEXT(dimptr);
      }
      ensemble_dt = D_ARR_TYPE(ensemble_dt);
      break;
    case DT_BOOLEAN:
    case DT_CHAR:
    case DT_REAL:
    case DT_INTEGER:
    case DT_ENUM:
    case DT_SHORT:
    case DT_LONG:
    case DT_DOUBLE:
    case DT_QUAD:
    case DT_UNSIGNED_INT:
    case DT_UNSIGNED_SHORT:
    case DT_UNSIGNED_LONG:
    case DT_UNSIGNED_BYTE:
    case DT_SIGNED_BYTE:
    case DT_STRUCTURE: 
    case DT_OPAQUE:
    case DT_COMPLEX:
    case DT_DCOMPLEX:
    case DT_QCOMPLEX:
      fprintf(outfile, "sizeof(%s)", S_IDENT(D_NAME(ensemble_dt))); 
      ensemble_dt = NULL;
      break;
    default:
      INT_FATAL(NULL, "Unknown variable type in Cgen.c (class is %d)",
		D_CLASS(ensemble_dt));
    }
}


static void gen_outfile_top(FILE* outfile) {
  char command[256];
  time_t curr_time;
  int i;

  /*  fprintf(outfile,"# 2 \"%s.c\"\n",trunc_in_filename);*/
  fprintf(outfile,"/*** This program was compiled using the zc ZPL Compiler "
	  "***/\n");
  fprintf(outfile, "char __ZC_version_[]  = \"[compile_info] zc ZPL "
	  "Compiler Version ");
  version_fprint(outfile);
  fprintf(outfile,"\";\n");
  fprintf(outfile, "char __ZC_cwd_[]      = \"[compile_info] Compiled "
	  "from: %s\";\n",getcwd(command, 256));
  time(&curr_time);
  strcpy(command, ctime(&curr_time));
  command[strlen(command)-1] = '\0';   /*** get rid of the new line ***/
  fprintf(outfile, "char __ZC_date_[]     = \"[compile_info]            "
	  "on: %s\";\n",
	  command);
  if (zc_command != NULL) {
    /*** zc0 was called from zc, so print out zc command line ***/
    fprintf(outfile, "char __ZC_command_[]  = \"[compile_info]          "
	    "  as: ");
    fprintf(outfile, "%s\";\n", zc_command);
  }

  fprintf(outfile, "char __ZC0_command_[] = \"[compile_info]            "
	  "as: ");
  for (i = 0 ; i < argc; i++) {
    if ((zc_command != NULL) && (i == 2)) {
      /*** quote the string ***/
      fprintf(outfile, "\\\"%s\\\" ", zc_command);
    }
    else {
      fprintf(outfile, "%s ", argv[i]);
    }
  }
  fprintf(outfile, "\";\n");

  fprintf(outfile,"\n");
  fprintf(outfile,"#include <stdlib.h>\n");
  fprintf(outfile,"#include <stdio.h>\n");
  fprintf(outfile,"#include <math.h>\n");
  fprintf(outfile,"#include <string.h>\n");
  fprintf(outfile,"#include <limits.h>\n");
  if (hoist_access_ptrs) {
    fprintf(outfile, "#define _HOIST_ACCESS_POINTERS");
    if (hoist_access_ptrs == HOIST_PTRS_TO_MLOOP) {
      fprintf(outfile, "_TO_MLOOP");
    } else {
      fprintf(outfile, "_TO_SUBPROG");
    }
  }
  fprintf(outfile, "\n");
  if (hoist_access_mults) {
    fprintf(outfile, "#define _HOIST_ACCESS_MULTS");
  }
  fprintf(outfile, "\n");
  if (use_charstar_pointers) {
    fprintf(outfile, "#define _USE_CHARSTAR_PTRS");
  }
  fprintf(outfile, "\n");
  fprintf(outfile,"#include \"stand_inc.h\"\n");
  fprintf(outfile,"#include \"%s_%s\"\n",trunc_in_filename,ACCHEADFILE);
  fprintf(outfile,"#include \"%s_%s\"\n",trunc_in_filename,REGSPSFILE);

  OutputUserSpecIncludes(outfile);

  /*** generate profile initialization info ***/
  GenProfInit(outfile);
}


/* BLC -- This routine is used to do things at the top of codegen */
static void StartCodeGen(FILE** outfile) {
  char command[256];
  char *pch;
  int using_stdin=0;

  InitMgen();
  if (strcmp(in_file,"stdin")) {
    if (zbasename == NULL) {
      INT_FATAL(NULL, "Basename is NULL.\n");
    }

    trunc_in_file=(char *)malloc((strlen(zbasename)+1)*(sizeof(char)));
    trunc_in_filename=(char *)malloc((strlen(zbasename)+1)*(sizeof(char)));

    strcpy(trunc_in_file, zbasename);
    strcpy(trunc_in_filename, zbasename);

    /* the following statements convert '-' and '.' to '_' in the filename;
     * this is necessary, for this string is used as a c macro;
     * there may be other chars we would like to convert to _ (echris) */
    while ((pch=strchr(trunc_in_file,'.'))!=NULL) {
      *pch='_';
    }
    while ((pch=strchr(trunc_in_file,'-'))!=NULL) {
      *pch='_';
    }

  } else {
    trunc_in_file=(char *)malloc((strlen(entry_name)+1)*(sizeof(char)));
    trunc_in_filename=(char *)malloc((strlen(entry_name)+1)*(sizeof(char)));
    sprintf(trunc_in_file,"%s",entry_name);
    sprintf(trunc_in_filename,"%s",entry_name);
    using_stdin = 1;
  }

  if (outputdir != NULL) {
    DESTPATH = (char *) malloc((strlen(outputdir)+2)*sizeof(char));
    sprintf(DESTPATH, "%s/", outputdir);
  }
  else {
    DESTPATH = (char *) malloc((strlen(trunc_in_filename)+
				strlen(DESTPATHBASE)+2+4+1) *
			       (sizeof(char)));
    sprintf(DESTPATH,"%s/%s.out/", DESTPATHBASE, trunc_in_filename);
  }

  if (using_stdin) {
    fprintf(stdout,"Output being written to %s\n",DESTPATH);
  }
  
  /*** remove the old temp file ***/
  sprintf(command, "rm -f %s%s",DESTPATH,TEMP_OUTFILE);
  system(command);

  sprintf(command,"%s%s",DESTPATH,TEMP_OUTFILE);
  if ((*outfile = fopen(command, "w")) == NULL) {
    USR_FATAL(NULL, "Cannot open '%s'.", command);
  }

  gen_outfile_top(*outfile);

  /*  fprintf(outfile,"const int _MAXDIM = %d;\n",maxdim);*/

  GenerateZMakefile();      /* output a Makefile */
}

static int gen_code(FILE *fp,module_t *module) {
  symboltable_t	*pst;
  FILE* outfile;

  DB0(20,"gen_code()\n");

  RMSInit();
  StartCodeGen(&outfile);  /* BLC -- output any general codegen-ish things */

  StartRuntime();
  StartConfig();
  StartDgen();
  StartIronmanCodegen(NumStaticCIDs());

  /**
   ** Output global variable declarations
   **/
  for (pst = T_DECL(module); pst != NULL; pst = S_SIBLING(pst)) {
    if (S_CLASS(pst) != S_SUBPROGRAM) {
      gen_pst(outfile, pst);
    }
  }

  fprintf(outfile, "\n");

  /**
   ** Output function prototypes
   **/
  for (pst = T_DECL(module); pst != NULL; pst = S_SIBLING(pst)) {
    if (S_CLASS(pst) == S_SUBPROGRAM) {
      if (S_STD_CONTEXT(pst) == FALSE && !(S_STYPE(pst)&SC_EXTERN)) {
	gen_function_header(outfile, pst, 1);
      }
    }
  }

  /**
   ** Output function definitions
   **/
  for (pst = T_DECL(module); pst != NULL; pst = S_SIBLING(pst)) {
    if (S_CLASS(pst) == S_SUBPROGRAM) {
      DB1( 5, "Sending %s to gen_pst()\n", S_IDENT(pst) );
      gen_pst(outfile,pst);
    }
  }
    
  EndIronmanCodegen(outfile,NumStaticCIDs());
  EndConfig(outfile);
  /**
   ** Output code to finalize initialization global variables
   **/
  gen_finalize(module);
  EndDgen();
  fprintf(outfile, "#include \"%s_%s\"\n", trunc_in_filename, INITCODEFILE);
  fprintf(outfile, "\n");
  fprintf(outfile, "char entry_name[] = \"%s\";\n", entry_name);
  fprintf(outfile, "void _zplmain() {\n");

  if (do_checkpoint) {
    fprintf(outfile,"do {\n");
    fprintf(outfile,"  _StartRecover();\n");
  }

  fprintf(outfile,"  _InitLibraries(_numcomms,_enspercommid,_max_numdirs,"
	  "_max_numwrapdirs,_numremapmaps);\n");
  fprintf(outfile, "  _initAllGlobals();\n");

  fprintf(outfile,"  %s();\n",entry_name);

  /**
   ** Output code to destroy global variables
   **/
  for (pst = T_DECL(module); pst != NULL; pst = S_SIBLING(pst)) {
    if (S_CLASS(pst) != S_SUBPROGRAM) {
      gen_pst_done(outfile, pst);
    }
  }

  if (do_checkpoint) {
    fprintf(outfile, "} while (_recovering);\n");
    fprintf(outfile, "  _DoneCheckpoint();\n");
  }
  fprintf(outfile,"}\n\n");
  
  fprintf(outfile,"#include \"./%s_%s\"\n",trunc_in_filename,CFGCODEFILE);
  fprintf(outfile,"#include \"./%s_%s\"\n",trunc_in_filename,CFGCODEFILE2);
  fprintf(outfile,"#include \"./%s_%s\"\n",trunc_in_filename,ACCCODEFILE);
  fclose(outfile);

  EndRuntime();

  EndCodeGen();
  RMSFinalize();

  return 0;
}


static void rename_pst(symboltable_t* pst, char* name) {
  free(S_IDENT(pst));
  S_IDENT(pst) = malloc(strlen(name)+1);
  sprintf(S_IDENT(pst), name);
}

int call_codegen(module_t *mod,char *s) {
  module_t *modls;
  int i;

  modls = mod;

  /* Rename Index1, Index2, etc. to _i0, _i1, etc. */

  for (i=0; i<MAXRANK; i++) {
    sprintf(S_IDENT(pstINDEX[i+1]),"_i%d",i);
    sprintf(S_IDENT(pstindex[i+1]),"_ia%d_%d", i+1, 1);
  }

  /* rename open to _OpenFile and close to _CloseFile */
  rename_pst(lu_only("open"), "_OpenFile");
  rename_pst(lu_only("eof"), "_EOFile");
  rename_pst(lu_only("close"), "_CloseFile");

  /* rename sqr and cube to _ZPL_SQR and _ZPL_CUBE */
  rename_pst(lu_only("sqr"), "_ZPL_SQR");
  rename_pst(lu_only("cube"), "_ZPL_CUBE");
  rename_pst(lu_only("trunc"), "_ZPL_TRUNC");

  while (modls != NULL) {
    gen_code(zstdout,modls);
    modls = T_NEXT(modls);
  }
  fflush(zstdout);

  gen_sps_structure_use();

  return 0;
}

static void gen_basic_name(FILE* F, const char* str) {
  int i;

  for (i=0; i<strlen(str); i++) {
    if (str[i] == '[') {
      fprintf(F, "Z");
    }
    else if (str[i] == ']') {
      fprintf(F, "Z");
    }
    else if (str[i] == '-') {
      fprintf(F, "_");
    }
    else if (str[i] == '>') {
      fprintf(F, "X");
    }
    else {
      fprintf(F, "%c", str[i]);
    }
  }

}

void fgenf(FILE* F, const char* format, ...) {
  va_list ap;
  int i, j;
  int num_args;

  num_args = 0;
  for (i=0; i<strlen(format); i++) {
    num_args += (format[i] == '%');
  }
  va_start(ap, format);
  for (i=0, j=0; i<num_args; i++) {
    while (format[j] != '%') {
      fprintf(F, "%c", format[j++]);
    }
    switch (format[++j]) {
    case 'E':
      {
	expr_t* expr = va_arg(ap, expr_t*);
	if (ST_TYPE(expr) != EXPR_T) {
	  INT_FATAL(NULL, "Expression expected in fgenf");
	}
	gen_expr(F, expr);
      }
      break;
    case 'D':
      {
	datatype_t* pdt = va_arg(ap, datatype_t*);
	int type = va_arg(ap, int);
	if (ST_TYPE(pdt) != DATATYPE_T) {
	  INT_FATAL(NULL, "Expression expected in fgenf");
	}
	gen_pdt(F, pdt, type);
      }
      break;
    case '_':
      {
	char* str = va_arg(ap, char*);
	gen_basic_name(F, str);
      }
      break;
    case 'd':
      {
	int num = va_arg(ap, int);
	fprintf(F, "%d", num);
      }
      break;
    case 's':
      {
	char* str = va_arg(ap, char*);
	fprintf(F, "%s", str);
      }
      break;
    default:
      INT_FATAL(NULL, "bad format character in fgenf: %c", format[j]);
    }
    j++;
  }
  while (j < strlen(format)) {
    fprintf(F, "%c", format[j++]);
  }
  va_end(ap);
}

void gen_updn(FILE* outfile,int up) {
  if (up > 0) {
    fprintf(outfile,"UP");
  } else {
    fprintf(outfile,"DN");
  }
}
