/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#include "../include/dbg_code_gen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/checkpoint.h"
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/main.h"
#include "../include/rmstack.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"
#include "../include/glist.h"
#include "../include/glistmac.h"
#include "../include/symboltable.h"

/*
 * WHAT does not work?  multi-arrays, multi-regions, sparse arrays,
 *                      indexed arrays of ensembles,
 *                      structs containing ensembles
 *
 * ALL functions leading up to a checkpoint must be the only
 * expression in the statement!
 */

/* This file contains the checkpoint pass as well as the functions
   used to generate code related to checkpoints. */

/*
 * NOTE on checkpoint labels. Odd numbers are goto statements, even
 * numbers are labels. This is taken care of during code generation.
 */

int call_checkpoint(module_t*, char*);

#define MAXCHECKPOINTS 100

static int numchks = 0; /* number of user-inserted checkpoints */
static int numoths = 0; /* number of other checkpoints, for stack support */
static statement_t* chkls[MAXCHECKPOINTS]; /* list of checkpoints */
static expr_t *chklsreg[MAXCHECKPOINTS]; /*** sungeun *** covering region ***/

static int naive = FALSE; /*** sungeun *** save all vars at checkpoint
                                       i.e., ignore live variable info ***/

/*
 * find checkpoint calls in the program and store the statements in
 * chkls, report illegal placements, report warnings if there are
 * checkpoint calls and do_checkpoint is disabled.
 */
static void findcheckpoints(expr_t* expr)
{
  if (expr == NULL || T_TYPE(expr) != FUNCTION) {
    return;
  }
  if (strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "checkpoint") == 0) {
    if (T_EXPR(T_STMT(expr)) != expr) {
      USR_FATAL(T_STMT(expr), "Checkpoint function must stand alone");
    }
    if (!do_checkpoint) {
      USR_WARN(T_STMT(expr), "Checkpoint function ignored; use -checkpoint");
    }
    if (numchks == MAXCHECKPOINTS) {
      USR_FATAL(T_STMT(expr), "Max number of checkpoints exceeded");
    }
    chklsreg[numchks] = RMSCurrentRegion();
    chkls[numchks++] = T_STMT(expr);
  }
}

static void insert_label(int i)
{
  expr_t* te;
  expr_t* te2;
  statement_t* new;
  int lineno;
  char* filename;

  lineno = T_LINENO(chkls[i]);
  filename = T_FILENAME(chkls[i]);
  te = build_typed_0ary_op(VARIABLE, chklabel);
  te2 = new_const_int(i*2);
  /*** T_NEXT(te2) = copy_expr(T_NEXT(T_OPLS(T_EXPR(chkls[i])))); ***/
  te = build_Nary_op(FUNCTION, te, te2);
  new = build_expr_statement(te, lineno, filename);
  insertbefore_stmt(new, chkls[i]);
}

static void insert_donerecover(int i)
{
  expr_t* te;
  statement_t* new;
  int lineno;
  char* filename;

  lineno = T_LINENO(chkls[i]);
  filename = T_FILENAME(chkls[i]);
  te = build_typed_0ary_op(VARIABLE, chklabel);
  te = build_Nary_op(FUNCTION, te, new_const_int(-2));
  new = build_expr_statement(te, lineno, filename);
  insertbefore_stmt(new, chkls[i]);
}

static void insert_donecheckpoint(int i)
{
  expr_t* te;
  statement_t* new;
  int lineno;
  char* filename;

  lineno = T_LINENO(chkls[i]);
  filename = T_FILENAME(chkls[i]);
  te = build_typed_0ary_op(VARIABLE, chklabel);
  te = build_Nary_op(FUNCTION, te, new_const_int(-1));
  new = build_expr_statement(te, lineno, filename);
  insertafter_stmt(new, chkls[i]);
}

static void insert_startcheckpoint(int i)
{
  expr_t* te;
  statement_t* new;
  int lineno;
  char* filename;
  expr_t* te2;

  lineno = T_LINENO(chkls[i]);
  filename = T_FILENAME(chkls[i]);
  te = build_typed_0ary_op(VARIABLE, chklabel);
  te2 = new_const_int(-6);
  T_NEXT(te2) = new_const_int(i);
  te = build_Nary_op(FUNCTION, te, te2);
  new = build_expr_statement(te, lineno, filename);
  insertafter_stmt(new, chkls[i]);
}

static void insert_goto(int i)
{
  statement_t* first;
  expr_t* te;
  statement_t* new;
  int lineno;
  char* filename;

  lineno = T_LINENO(chkls[i]);
  filename = T_FILENAME(chkls[i]);
  first = T_STLS(T_PARFCN(chkls[i]));
  if (T_TYPE(first) == S_COMPOUND) {
    first = (T_CMPD_STLS(first));
  }
  te = build_typed_0ary_op(VARIABLE, chklabel);
  te = build_Nary_op(FUNCTION, te, new_const_int(i*2+1));
  new = build_expr_statement(te, lineno, filename);
  insertbefore_stmt(new, first);
}

static void insert_gotostart(int i)
{
  statement_t* first;
  expr_t* te;
  statement_t* new;
  int lineno;
  char* filename;

  lineno = T_LINENO(chkls[i]);
  filename = T_FILENAME(chkls[i]);
  first = T_STLS(T_PARFCN(chkls[i]));
  if (T_TYPE(first) == S_COMPOUND) {
    first = (T_CMPD_STLS(first));
  }
  te = build_typed_0ary_op(VARIABLE, chklabel);
  te = build_Nary_op(FUNCTION, te, new_const_int(-3));
  if (T_TYPE(first) != S_EXPR || !expr_equal(T_EXPR(first), te)) {
    new = build_expr_statement(te, lineno, filename);
    insertbefore_stmt(new, first);
  }
}

static void insert_gotostop(int i)
{
  statement_t* first;
  expr_t* te;
  statement_t* new;
  int lineno;
  char* filename;

  lineno = T_LINENO(chkls[i]);
  filename = T_FILENAME(chkls[i]);
  first = T_STLS(T_PARFCN(chkls[i]));
  if (T_TYPE(first) == S_COMPOUND) {
    first = (T_CMPD_STLS(first));
  }
  te = build_typed_0ary_op(VARIABLE, chklabel);
  te = build_Nary_op(FUNCTION, te, new_const_int(-4));
  if (T_TYPE(first) != S_EXPR || !expr_equal(T_EXPR(first), te)) {
    new = build_expr_statement(te, lineno, filename);
    insertbefore_stmt(new, first);
  }
}

static statement_t *
insert_locals(int i, int save) {
  statement_t* stmt;
  expr_t* list;
  expr_t* tmp;
  expr_t* te;
  statement_t* new;
  symboltable_t* pst;
  expr_t* reg;
  int lineno;
  char* filename;

  lineno = T_LINENO(chkls[i]);
  filename = T_FILENAME(chkls[i]);
  list = NULL;
  stmt = chkls[i];
  if (save == TRUE) {
    DBS2(10, "Checkpoint save %i at line %d...\n", i, lineno);
  } else {
    DBS2(10, "Checkpoint recover %i at line %d...\n", i, lineno);
  }

  /*** sungeun *** always save the current region stack ***/
  IFDB(10) {
    if (save == TRUE) {
      printf("\tsaving ");
    } else {
      printf("\trecovering ");
    }
    IFDB(30) {
      print_symtab_entry(rmstack);
    } else {
      printf("%s\n", S_IDENT(rmstack));
    }
  }
  te = build_typed_0ary_op(VARIABLE, rmstack);
  list = tmp = te;

  /*** sungeun *** always save dynamic region info ***/
  reg = chklsreg[i];
  if ((reg != NULL) && (expr_dyn_reg(reg))) {
    IFDB(10) {
      if (save == TRUE) {
	printf("\tsaving dynamic region ");
      } else {
	printf("\trecovering dynamic region ");
      }
      IFDB(30) {
	dbg_gen_expr(stdout, reg);
      } else {
	if (T_IDENT(reg)) {
	  printf("%s\n", S_IDENT(T_IDENT(reg)));
	} else {
	  dbg_gen_expr(stdout, reg);
	}
      }
    }
    te = reg;
    T_NEXT(tmp) = te;
    tmp = te;
  }

  if (naive == FALSE) {
    /*** sungeun *** use live variable information to dump checkpoint ***/
    int inMAIN = 0;
    glist live;

    /*** sungeun *** always save region stack "Save" state ***/
    while (T_PARENT(stmt)) {
      stmt = T_PARENT(stmt);
      if (T_TYPE(stmt) == S_COMPOUND) {
	pst = T_DECL(T_CMPD(stmt));
	while (pst) {
	  if ((D_CLASS(S_DTYPE(pst)) == DT_STRUCTURE) &&
	      (S_DTYPE(pst) == rmsframe)) {
	    IFDB(10) {
	      if (save == TRUE) {
		DBS1(10, "\tsaving region stack variable %s\n", S_IDENT(pst));
	      } else {
		DBS1(10, "\trecovering region stack variable %s\n", S_IDENT(pst));
	      }
	    }
	    te = build_typed_0ary_op(VARIABLE, pst);
	    T_NEXT(tmp) = te;
	    tmp = te;
	  }
	  pst = S_SIBLING(pst);
	}
      }
    }

    /*** reset stmt pointer ***/
    stmt = chkls[i];

    /*** only save live global variables in the entry function ***/
    if (T_FCN(T_PARFCN(stmt)) == pstMAIN) {
      DBS0(10, "In entry function...\n");
      inMAIN = 1;
    }
    for (live = (glist) T_LIVE(stmt); live != NULL; live = GLIST_NEXT(live)) {
      pst = (symboltable_t *) GLIST_DATA(live);
      INT_COND_FATAL((pst != NULL), stmt,
		     "Checkpointing failure: NULL live variable");
      if ((symtab_is_global(pst) == 0) /*** save/recover any locals ***/ ||
	  (symtab_is_global(pst) && (!(S_STYPE(pst) & SC_CONFIG)) &&
	   (inMAIN == 1))) /*** and globals (NOT config vars) in main ***/ {
	IFDB(10) {
	  if (save == TRUE) {
	    printf("\tsaving ");
	  } else {
	    printf("\trecovering ");
	  }
	  IFDB(30) {
	    print_symtab_entry(pst);
	  } else {
	    printf("%s\n", S_IDENT(pst));
	  }
	}
	te = build_typed_0ary_op(VARIABLE, pst);
	T_NEXT(tmp) = te;
	tmp = te;
      }
      else {
	IFDB(10) {
	  if (!(S_STYPE(pst) & SC_CONFIG)) {
	    /*** globals are added above ***/
	    printf("IGNORING live global %s until main\n", S_IDENT(pst));
	  } else {
	    /*** config vars are added below in insert_globals() ***/
	    printf("IGNORING live config var %s for now\n", S_IDENT(pst));
	  }
	}
      }
    }
  } else {
    /*** sungeun *** this is the naive approach of dumping everything ***/
    while (T_PARENT(stmt)) {
      stmt = T_PARENT(stmt);
      if (T_TYPE(stmt) == S_COMPOUND) {
	pst = T_DECL(T_CMPD(stmt));
	while (pst) {
	  IFDB(10) {
	    if (save == TRUE) {
	      DBS1(10, "\tsaving variable %s\n", S_IDENT(pst));
	    } else {
	      DBS1(10, "\trecovering variable %s\n", S_IDENT(pst));
	    }
	  }
	  te = build_typed_0ary_op(VARIABLE, pst);
	  T_NEXT(tmp) = te;
	  tmp = te;
	  pst = S_SIBLING(pst);
	}
      }
    }
  }

  te = build_typed_0ary_op(VARIABLE, (save) ? chksave : chkrecover);
  te = build_Nary_op(FUNCTION, te, list);
  new = build_expr_statement(te, lineno, filename);
  if (save) {
    insertafter_stmt(new, chkls[i]);
  }
  else {
    insertbefore_stmt(new, chkls[i]);
  }
  return new;
}

static expr_t *
is_symtab_in_exprls(expr_t *list, symboltable_t *pst) {
  expr_t *te = list;

  INT_COND_FATAL((pst != NULL), NULL,
		  "Checkpointing error: NULL symboltable pointer");

  while (te != NULL) {
    INT_COND_FATAL((T_TYPE(te) == VARIABLE), NULL,
		  "Checkpointing error: wrong expression type");
    if (T_IDENT(te) == pst) {
      break;
    }
    te = T_NEXT(te);
  }

  return te;

}

static statement_t *
add_globals(statement_t *stmt, int save) {
  symboltable_t* pst;
  expr_t* exprls;
  expr_t* te;

  INT_COND_FATAL((T_TYPE(stmt) == S_EXPR), NULL,
		 "Checkpointing failure: bad statement");
  INT_COND_FATAL((T_EXPR(stmt) != NULL), NULL,
		 "Checkpointing failure: bad statement");
  INT_COND_FATAL((T_OPLS(T_EXPR(stmt)) != NULL), NULL,
		 "Checkpointing failure: bad statement");

  if (T_NEXT(T_OPLS(T_EXPR(stmt))) == NULL) {
    exprls = NULL;
  } else {
    exprls = T_NEXT(T_OPLS(T_EXPR(stmt)));
  }

  if (!is_symtab_in_exprls(exprls, rmstack)) {
    /*** sungeun *** dump out region stack (could be optimized) ***/
    IFDB(10) {
      if (save == TRUE) {
	printf("\tsaving ");
      } else {
	printf("\trecovering ");
      }
      IFDB(30) {
	print_symtab_entry(rmstack);
      } else {
	printf("%s\n", S_IDENT(rmstack));
      }
    }

    /*** don't bother setting other pointers *** i trust fixup ***/
    te = build_typed_0ary_op(VARIABLE, rmstack);
    if (exprls == NULL) {
      exprls = te;
    } else {
      T_NEXT(te) = exprls;
      exprls = te;
    }
  }

  pst = pstMAIN;
  while (S_SIBPREV(pst) != NULL) {
    pst = S_SIBPREV(pst);
  }
  while (pst != NULL) {
    if (((naive == TRUE) && (S_CLASS(pst) == S_VARIABLE &&
			     !S_IS_CONSTANT(pst) &&
			     D_CLASS(S_DTYPE(pst)) != DT_GENERIC &&
			     S_IDENT(pst)[0] != '_')) ||
	/*** dump all global vars *** is that last bit a hack? ***/
	((S_CLASS(pst) == S_VARIABLE) && (!S_IS_CONSTANT(pst)) && (S_STYPE(pst) & SC_CONFIG) &&
	 (S_VAR_INIT(pst) != NULL))) {
      /*** dump only config vars that are used ***/
      /*** NOTE: this last part a hack required for "_mi" config var ***/
      /***       that is in the standard context but not used ***/
      /****      (and others like it that may be added in the future) ***/
      IFDB(10) {
	if (save == TRUE) {
	  DBS2(10, "\tsaving %s variable %s\n",
	       (S_STYPE(pst) & SC_CONFIG) ? "config" : "global",
	       S_IDENT(pst));
	} else {
	  DBS2(10, "\trecovering %s variable %s\n",
	       (S_STYPE(pst) & SC_CONFIG) ? "config" : "global",
	       S_IDENT(pst));
	}
      }
      /*** don't bother setting other pointers *** i trust fixup ***/
      te = build_typed_0ary_op(VARIABLE, pst);
      if (exprls == NULL) {
	exprls = te;
      } else {
	T_NEXT(te) = exprls;
	exprls = te;
      }
    }
    pst = S_SIBLING(pst);
  }

  T_NEXT(T_OPLS(T_EXPR(stmt))) = exprls;

  return stmt;
}

static void insert_savecode(int i, int doglobal) {
  statement_t *pst;
  insert_donecheckpoint(i);
  insert_startcheckpoint(i);
  pst = insert_locals(i, 1);
  if (doglobal) {
    add_globals(pst, 1);
  }
}

static void insert_recovercode(int i, int doglobal) {
  statement_t *pst;
  pst = insert_locals(i, 0);
  if (doglobal) {
    add_globals(pst, 0);
  }
  insert_donerecover(i);
}

static void support_stack(function_t* func) {
  genlist_t* gl;
  int newcheckpoint;
  int i;

  gl = T_INVOCATIONS(func);
  while (gl != NULL) {
    newcheckpoint = 1;
    for (i = 0; i < numoths; i++) {
      if (chkls[numchks+i] == G_STATEMENT(gl)) {
	newcheckpoint = 0;
      }
    }
    if (newcheckpoint) {
      chkls[numchks+numoths] = G_STATEMENT(gl);
      numoths++;
      support_stack(T_PARFCN(G_STATEMENT(gl)));
    }
    gl = G_NEXT(gl);
  }
}

static void insert_startlabel(void)
{
  symboltable_t* mainfunc;
  statement_t* first;
  expr_t* te;
  statement_t* new;

  mainfunc = lookup(S_SUBPROGRAM, entry_name);
  first = T_STLS(S_FUN_BODY(mainfunc));
  if (T_TYPE(first) == S_COMPOUND) {
    first = (T_CMPD_STLS(first));
  }
  te = build_typed_0ary_op(VARIABLE, chklabel);
  te = build_Nary_op(FUNCTION, te, new_const_int(-5));
  new = build_expr_statement(te, 0, "unknown.c");
  insertbefore_stmt(new, first);
}

static void checkpoint(void)
{
  int i;

  if (numchks == 0) {
    USR_WARN(NULL, "No checkpoints; cannot use -checkpoint");
  }

  for (i = 0; i < numchks; i++) {
    support_stack(T_PARFCN(chkls[i]));
  }
  for (i = 0; i < numchks+numoths; i++) {
    insert_gotostop(i);
  }
  for (i = 0; i < numchks; i++) {
    insert_goto(i);
    insert_label(i);
    insert_savecode(i, 1);
    insert_recovercode(i, 1);
  }
  for (i = numchks; i < numchks+numoths; i++) {
    insert_goto(i);
    insert_label(i);
    insert_savecode(i, 0);
    insert_recovercode(i, 0);
  }
  for (i = 0; i < numchks+numoths; i++) {
    insert_gotostart(i);
  }
  insert_startlabel();
}

/*
 * remove checkpoint function calls since they do nothing.
 */
static void removecheckpoints(void)
{
  int i;

  for (i = 0; i < numchks; i++) {
    remove_stmt(chkls[i]);
  }
}

int call_checkpoint(module_t* modls, char* s)
{
  module_t* mod;
  symboltable_t* pst;
  statement_t* stmt;
  char *p;

  /*** use naive mode by default ***/
  naive = TRUE;
  p = s;
  while ((*p != '\0') && (*p != '-')) p++;
  if (*p != '\0') p++;
  while (*p != '\0') {
    switch (*p) {
    case 'n':
      /*** naive mode *** save ALL variables at each checkpoint ***/
      IFDB(5) {
	printf("*** using NAIVE mode ***\n");
      }
      naive = TRUE;
      break;
    case 'o':
      /*** optimized mode *** save only live variables at each checkpoint ***/
      IFDB(5) {
	printf("*** using OPTIMIZED mode ***\n");
      }
      naive = FALSE;
      break;
    default:
      break;
    }
    while ((*p != '\0') && (*p != '-')) p++;
    if (*p != '\0') p++;
  }

  RMSInit();
  for (mod = modls; mod != NULL; mod = T_NEXT(mod)) {
    for (pst = T_DECL(mod); pst != NULL; pst = S_SIBLING(pst)) {
      if (S_CLASS(pst) == S_SUBPROGRAM) {
	if ((stmt = T_STLS(S_FUN_BODY(pst))) != NULL) {
	  traverse_stmtls(T_STLS(T_CMPD(stmt)), 0, 0, findcheckpoints, 0);
	}
      }
    }
  }
  RMSFinalize();

  if (do_checkpoint) {
    checkpoint();
  }

  removecheckpoints();

  return 0;
}

/*** CODE GENERATION FUNCTIONS BELOW ***/

static void gen_checksize(FILE* outfile, expr_t* expr) {
  symboltable_t* pst;

  pst = T_IDENT(expr);
  switch (D_CLASS(S_DTYPE(pst))) {
  case DT_BOOLEAN:
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
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
    fprintf(outfile, "_AddBasic(");
    gen_pdt(outfile, S_DTYPE(pst), PDT_SIZE);
    fprintf(outfile, ");\n");
    break;
  case DT_ARRAY:
    fprintf(outfile, "_AddIArray(");
    gen_pdt(outfile, S_DTYPE(pst), PDT_SIZE);
    fprintf(outfile, ");\n");
    break;
  case DT_STRING:
    fprintf(outfile, "_AddString(");
    gen_expr(outfile, expr);
    fprintf(outfile, ");\n");
    break;
  case DT_ENSEMBLE:
    fprintf(outfile, "_AddArray(");
    brad_no_access++;
    gen_expr(outfile, expr);
    brad_no_access--;
    fprintf(outfile, ");\n");
    break;
  case DT_STRUCTURE:
    if (S_DTYPE(pst) == rmsframe) {
      fprintf(outfile, "_AddRMStack(&");
      gen_expr(outfile, expr);
      fprintf(outfile, ");\n");
    }
    else {
      fprintf(outfile, "_AddBasic(");
      gen_pdt(outfile, S_DTYPE(pst), PDT_SIZE);
      fprintf(outfile, ");\n");
    }
    break;
  case DT_REGION:
    fprintf(outfile, "_AddRegion(");
    gen_expr(outfile, expr);
    fprintf(outfile, ");\n");
    break;
  case DT_FILE:
  case DT_OPAQUE:
  case DT_GENERIC_ENSEMBLE:
  case DT_GENERIC:
  case DT_PROCEDURE:
  case DT_VOID:
  case DT_SUBPROGRAM:
  default:
    INT_FATAL(NULL, "Checkpointing failure: cannot check all data");
    break;
  }
}

static void gen_checksave(FILE* outfile, expr_t* expr) {
  symboltable_t* pst;

  pst = T_IDENT(expr);
  switch (D_CLASS(S_DTYPE(pst))) {
  case DT_BOOLEAN:
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
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
    fprintf(outfile, "_SaveBasic(");
    gen_expr(outfile, expr);
    fprintf(outfile, ", ");
    gen_pdt(outfile, S_DTYPE(pst), PDT_CAST);
    fprintf(outfile, ");\n");
    break;
  case DT_ARRAY:
    fprintf(outfile, "_SaveIArray(");
    gen_expr(outfile, expr);
    fprintf(outfile, ", ");
    gen_pdt(outfile, S_DTYPE(pst), PDT_CAST);
    fprintf(outfile, ");\n");
    break;
  case DT_STRING:
    fprintf(outfile, "_SaveString(");
    gen_expr(outfile, expr);
    fprintf(outfile, ");\n");
    break;
  case DT_ENSEMBLE:
    fprintf(outfile, "_SaveArray(");
    brad_no_access++;
    gen_expr(outfile, expr);
    brad_no_access--;
    fprintf(outfile, ");\n");
    break;
  case DT_STRUCTURE:
    if (S_DTYPE(pst) == rmsframe) {
      fprintf(outfile, "_SaveRMStack(&");
      gen_expr(outfile, expr);
      fprintf(outfile, ");\n");
    }
    else {
      fprintf(outfile, "_SaveBasic(");
      gen_expr(outfile, expr);
      fprintf(outfile, ", ");
      gen_pdt(outfile, S_DTYPE(pst), PDT_CAST);
      fprintf(outfile, ");\n");
    }
    break;
  case DT_REGION:
    fprintf(outfile, "_SaveRegion(");
    gen_expr(outfile, expr);
    fprintf(outfile, ");\n");
    break;
  case DT_FILE:
  case DT_OPAQUE:
  case DT_GENERIC_ENSEMBLE:
  case DT_GENERIC:
  case DT_PROCEDURE:
  case DT_VOID:
  case DT_SUBPROGRAM:
  default:
    INT_FATAL(NULL, "Checkpointing failure: cannot check all data");
    break;
  }
}

static void gen_checkrecover(FILE* outfile, expr_t* expr) {
  symboltable_t* pst;

  pst = T_IDENT(expr);

  switch (D_CLASS(S_DTYPE(pst))) {
  case DT_BOOLEAN:
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
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
    fprintf(outfile, "_RecoverBasic(");
    gen_expr(outfile, expr);
    fprintf(outfile, ", ");
    gen_pdt(outfile, S_DTYPE(pst), PDT_CAST);
    fprintf(outfile, ");\n");
    break;
  case DT_ARRAY:
    fprintf(outfile, "_RecoverIArray(");
    gen_expr(outfile, expr);
    fprintf(outfile, ", ");
    gen_pdt(outfile, S_DTYPE(pst), PDT_CAST);
    fprintf(outfile, ");\n");
    break;
  case DT_STRING:
    fprintf(outfile, "_RecoverString(");
    gen_expr(outfile, expr);
    fprintf(outfile, ");\n");
    break;
  case DT_ENSEMBLE:
    fprintf(outfile, "_RecoverArray(");
    brad_no_access++;
    gen_expr(outfile, expr);
    brad_no_access--;
    fprintf(outfile, ");\n");
    break;
  case DT_STRUCTURE:
    if (S_DTYPE(pst) == rmsframe) {
      fprintf(outfile, "_RecoverRMStack(&");
      gen_expr(outfile, expr);
      fprintf(outfile, ");\n");
    }
    else {
      fprintf(outfile, "_RecoverBasic(");
      gen_expr(outfile, expr);
      fprintf(outfile, ", ");
      gen_pdt(outfile, S_DTYPE(pst), PDT_CAST);
      fprintf(outfile, ");\n");
    }
    break;
  case DT_REGION:
    fprintf(outfile, "_RecoverRegion(");
    gen_expr(outfile, expr);
    fprintf(outfile, ");\n");
    break;
  case DT_FILE:
  case DT_OPAQUE:
  case DT_GENERIC_ENSEMBLE:
  case DT_GENERIC:
  case DT_PROCEDURE:
  case DT_VOID:
  case DT_SUBPROGRAM:
  default:
    INT_FATAL(NULL, "Checkpointing failure: cannot check all data");
    break;
  }
}

void gen_checkpoint(FILE* outfile, expr_t* expr) {
  if (T_IDENT(T_OPLS(expr)) == chklabel) {
    int label = expr_intval(T_NEXT(T_OPLS(expr)));

    if (label == -1) {
      if (strcmp(S_IDENT(T_FCN(T_PARFCN(T_STMT(expr)))), entry_name) == 0) {
	fprintf(outfile, "if (_checkpointing) {\n");
	fprintf(outfile, "  _CompleteSave();\n");
	fprintf(outfile, "  return;\n");
	fprintf(outfile, "}\n");
      }
      else {
	fprintf(outfile, "if (_checkpointing) {\n");
	fprintf(outfile, "  return;\n");
	fprintf(outfile, "}\n");
      }
      return;
    }

    if (label == -2) {
      fprintf(outfile,
	      "if (_recovering) {\n"
	      "  if (_RecoverFinished()) {\n"
	      "    _CompleteRecover();\n"
	      "  }\n"
	      "}\n"
	      );
      return;
    }

    if (label == -3) {
      fprintf(outfile,
	      "if (_recovering) {\n"
	      "  int gotolabel;\n"
	      "  _RecoverBasic(gotolabel, int);\n"
	      "  switch (gotolabel) {\n"
	      );
      return;
    }

    if (label == -4) {
      fprintf(outfile,
	      "  default: printf(\"Error in recovery file map\\n\");\n"
	      "           exit(0);\n"
	      "  }\n"
	      "}\n");
      return;
    }

    if (label == -5) {
      return;
    }

    if (label == -6) {
      int label2 = expr_intval(T_NEXT(T_NEXT(T_OPLS(expr))));
      fprintf(outfile,
	      "if (_checkpointing) {\n"
	      "  _SaveFinished(%d);\n"
	      "}\n",
	      label2);
      return;
    }

    if (label % 2 == 0) {
      fprintf(outfile, "checkpointlabel%d:\n", label/2);
      if (label/2 < numchks) {
	fprintf(outfile, "_StartSave(_QueryCheckpointInterval());\n");
      }
    }
    else {
      fprintf(outfile, "case %d: goto checkpointlabel%d;\n", label/2, label/2);
    }
    return;
  }
  
  if (T_IDENT(T_OPLS(expr)) == chksave) {
    expr_t* tmp;
    
    fprintf(outfile, "if (_checkpointing) ");
    fprintf(outfile, "{ /* checkpoint: save data */\n");
    fprintf(outfile, "_NewSave();\n");

    tmp = T_NEXT(T_OPLS(expr));
    while (tmp) {
      gen_checksize(outfile, tmp);
      tmp = T_NEXT(tmp);
    }
    fprintf(outfile, "if (_chksize > 0) {\n");
    fprintf(outfile, "_StartDataSave();\n");

    tmp = T_NEXT(T_OPLS(expr));
    while (tmp) {
      gen_checksave(outfile, tmp);
      tmp = T_NEXT(tmp);
    }
    fprintf(outfile, "_EndDataSave();\n");
    fprintf(outfile, "}\n");
    fprintf(outfile, "} /* checkpoint: save data */\n");
    return;
  }

  if (T_IDENT(T_OPLS(expr)) == chkrecover) {
    expr_t* tmp;

    fprintf(outfile, "if (_recovering) ");
    fprintf(outfile, "{ /* checkpoint: recover data */\n");

    tmp = T_NEXT(T_OPLS(expr));
    while (tmp) {
      gen_checkrecover(outfile, tmp);
      tmp = T_NEXT(tmp);
    }
    fprintf(outfile, "}\n");
    return;
  }
}
