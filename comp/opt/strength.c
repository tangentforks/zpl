/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/***
 ***
 *** FILE: strength.c
 *** DATE: 5 September 1997 (sungeun@cs.washington.edu)
 ***
 ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/struct.h"
#include "../include/macros.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/const.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/expr.h"
#include "../include/buildzplstmt.h"
#include "../include/bb_traverse.h"
#include "../include/traverse.h"
#include "../include/buildsym.h"
#include "../include/createvar.h"
#include "../include/stmtutil.h"
#include "../include/passes.h"


#define is_integer(x) ((x) == pdtINT || (x) == pdtSHORT || (x) == pdtLONG ||\
		       (x) == pdtSIGNED_BYTE || (x) == pdtUNSIGNED_BYTE || \
		       (x) == pdtUNSIGNED_INT || (x) == pdtUNSIGNED_LONG || \
		       (x) == pdtUNSIGNED_SHORT)

#define is_float(x) ((x) == pdtFLOAT || (x) == pdtDOUBLE /*** || (x) == pdtQUAD ***/)


/* local globals */

static int SR_temp_counter = 0;


/* function prototypes */

static expr_t* reduce_expr_ls(expr_t *);


static char * get_temp_name(char *buffer) {
  sprintf(buffer, "_srtemp%d", SR_temp_counter++);
  return buffer;
}


static expr_t* reduce_expr(expr_t *e) {
  expr_t *new, *temp;

  if (e == NULL) {
    return NULL;
  }

  new = e;
  switch(T_TYPE(e)) {
  case BIEXP:                      /*** x^y ==> x*x*.. or sqrt(x) ***/
    T_OPLS(e) = reduce_expr_ls(T_OPLS(e));
    if (expr_computable_const(T_NEXT(T_OPLS(e)))) {
      int i_expo = expr_intval(T_NEXT(T_OPLS(e)));
      int a_expo = abs(i_expo);
      /*** the exponent is a constant ***/
      if ((a_expo <= 4) &&                              /*** <= 4  AND   ***/
	  (is_integer(T_TYPEINFO(T_NEXT(T_OPLS(e)))) || /*** integer OR  ***/
	  (is_float(T_TYPEINFO(T_NEXT(T_OPLS(e)))) &&   /*** whole float ***/
	   (expr_doubleval(T_NEXT(T_OPLS(e))) == (double) i_expo)))) {
	int i;
	if (a_expo == 0) {
	  /******************************/
	  /*** just replace it with 1 ***/
	  /******************************/
	  new = e;
	  strcpy( buffer, "1"); /*** for build_int() ***/
	  T_TYPE(new) = CONSTANT;
	  T_IDENT(new) = build_int(1);
	}
	else if (a_expo == 1) {
	  /*******************************************/
	  /*** just replace it with the expression ***/
	  /*******************************************/
	  if (i_expo == -1) {
	    /*** take the inverse ***/
	    new = e;
	    T_TYPE(new) = BIDIVIDE;
	    temp = T_OPLS(new);
	    T_OPLS(new) = alloc_expr(CONSTANT);
	    strcpy( buffer, "1"); /*** for build_int() ***/
	    T_IDENT(T_OPLS(new)) = build_int(1);
	    T_NEXT(T_OPLS(new)) = temp;
	    T_NEXT(temp) = NULL;  /*** don't bother freeing exponent ***/
	  }
	  else {
	    new = T_OPLS(e);
	    T_NEXT(e) = NULL;     /*** don't bother freeing exponent ***/
	  }
	}
	else if (a_expo <= 4) {
	  /***********************************************/
	  /*** replace it with repeated multiplication ***/
	  /***********************************************/
	  char name[256];
	  expr_t *new_expr;
	  statement_t *new_stmt;
	  symboltable_t *new_temp;
	  datatype_t *newdt;
	  expr_t *reg;

	  if (T_TYPE(T_OPLS(e)) != VARIABLE) {
	    /*** create a temp to hold the expression ***/
	    new_stmt = alloc_statement(S_EXPR, T_LINENO(T_STMT(e)),
				       T_FILENAME(T_STMT(e)));
	    T_EXPR(new_stmt) = alloc_expr(BIASSIGNMENT);
	    T_OPLS(T_EXPR(new_stmt)) = alloc_expr(VARIABLE);
	    new = T_OPLS(T_EXPR(new_stmt));

	    reg = T_TYPEINFO_REG(e);
	    if (reg == NULL) {
	      newdt = T_TYPEINFO(e); /*** not an ensemble, use typeinfo ***/
	    } else {
	      /*** build an ensemble type with the "base type" only ***/
	      newdt = build_ensemble_type(T_TYPEINFO(e),reg, 0, 1, T_LINENO(T_STMT(e)),
					  T_FILENAME(T_STMT(e)));
	    }
	    new_temp = create_named_local_var(newdt,
					      T_PARFCN(T_STMT(e)),
					      get_temp_name(name));
	    T_IDENT(new) = new_temp;
	    T_NEXT(new) = T_OPLS(e);    /*** reuse base expression ***/
	    T_NEXT(T_NEXT(new)) = NULL; /*** don't bother freeing exponent ***/
	    insertbefore_stmt(new_stmt, T_STMT(e)); /*** insert before this stmt ***/
	  }
	  else {
	    /*** just use the variable ***/
	    new_temp = T_IDENT(T_OPLS(e));
	  }

	  new = e;             /*** reuse e expression for multiplication ***/
	  if (i_expo < 0) {
	    /*** take the inverse ***/
	    T_TYPE(new) = BIDIVIDE;
	    T_OPLS(new) = alloc_expr(CONSTANT);
	    strcpy( buffer, "1"); /*** for build_int() ***/
	    T_IDENT(T_OPLS(new)) = build_int(1);
	    T_NEXT(T_OPLS(new)) = alloc_expr(BITIMES);
	    new_expr = T_NEXT(T_OPLS(new));
	  }
	  else {
	    new_expr = new;
	  }
	  T_TYPE(new_expr) = BITIMES;
	  T_OPLS(new_expr) = alloc_expr(VARIABLE);
	  T_IDENT(T_OPLS(new_expr)) = new_temp;
	  for (i = 2; i < a_expo; i++) {
	    T_NEXT(T_OPLS(new_expr)) = alloc_expr(BITIMES);
	    new_expr = T_NEXT(T_OPLS(new_expr));
	    T_OPLS(new_expr) = alloc_expr(VARIABLE);
	    T_IDENT(T_OPLS(new_expr)) = new_temp;
	  }
	  T_NEXT(T_OPLS(new_expr)) = alloc_expr(VARIABLE);
	  T_IDENT(T_NEXT(T_OPLS(new_expr))) = new_temp;
	}
	else { /*** do nothing ***/ }
      }
      else if (is_float(T_TYPEINFO(T_NEXT(T_OPLS(e))))) {
	double d_expo = expr_doubleval(T_NEXT(T_OPLS(e)));
	if (d_expo == 0.5) {
	  /*************************************/
	  /*** replace with a call to sqrt() ***/
	  /*************************************/
	  new = e;              /*** reuse e expression for sqrt() ***/
	  temp = T_OPLS(e);     /*** reuse base expression ***/
	  T_TYPE(new) = FUNCTION;
	  T_OPLS(new) = alloc_expr(VARIABLE);
	  T_IDENT(T_OPLS(new)) = lookup(S_SUBPROGRAM, "sqrt");
	  T_NEXT(T_OPLS(new)) = temp;
	  T_NEXT(temp) = NULL;  /** don't bother freeing exponent ***/

	}
	else { /*** do nothing ***/ }
      }
      else { /*** do nothing ***/ }
    }
    else { /*** do nothing ***/ }
    break;
  default:
    new = e;
    T_OPLS(new) = reduce_expr_ls(T_OPLS(e));
    break;
  }

  return new;
}


static expr_t* reduce_expr_ls(expr_t *e_ls) {
  expr_t *e, *new, *prev, *next;

  if (e_ls == NULL) {
    return NULL;
  }

  prev = NULL;
  next = T_NEXT(e_ls);
  for (e = e_ls; e != NULL; e = T_NEXT(e)) {
    new = reduce_expr(e);
    if (prev != NULL) {
      T_NEXT(prev) = new;
    }
    else {
      e_ls = new;
    }
    prev = new;
    if (next != NULL) {
      T_PREV(next) = new;
      next = T_NEXT(next);
    }
  }

  return e_ls;
}


static void reduce_statement(statement_t *s) {
  if (s == NULL) {
    return;
  }

  switch (T_TYPE(s)) {
  case S_EXPR:
    T_EXPR(s) = reduce_expr_ls(T_EXPR(s));
    break;
  case S_IF:
    T_IFCOND(T_IF(s)) = reduce_expr_ls(T_IFCOND(T_IF(s)));
    break;
  case S_LOOP:
    switch (T_TYPE(T_LOOP(s))) {
    case L_WHILE_DO:
    case L_REPEAT_UNTIL:
      T_LOOPCOND(T_LOOP(s)) = reduce_expr_ls(T_LOOPCOND(T_LOOP(s)));
      break;
    case L_DO:
      T_IVAR(T_LOOP(s)) = reduce_expr_ls(T_IVAR(T_LOOP(s)));
      T_START(T_LOOP(s)) = reduce_expr_ls(T_START(T_LOOP(s)));
      T_STOP(T_LOOP(s)) = reduce_expr_ls(T_STOP(T_LOOP(s)));
      T_STEP(T_LOOP(s)) = reduce_expr_ls(T_STEP(T_LOOP(s)));
      break;
    default:
      break;
    }
    break;
  case S_RETURN:
    T_RETURN(s) = reduce_expr_ls(T_RETURN(s));
    break;
  case S_IO:
    IO_EXPR1(T_IO(s)) = reduce_expr_ls(IO_EXPR1(T_IO(s)));
    break;
  case S_REGION_SCOPE:
    break;
  case S_MLOOP:
  case S_NLOOP:
  case S_NULL:
  case S_EXIT:
  case S_HALT:
  case S_CONTINUE:
  case S_COMPOUND:
  case S_MSCAN:
  case S_END:
  case S_WRAP:
  case S_REFLECT:
    break;
  default:
    break;
  }

}


static statement_t* strength_per_bb(statement_t *stmtls) {
  statement_t *s;

  if (stmtls == NULL) {
    return stmtls;
  }

  for (s = stmtls; s != NULL; s = T_NEXT_LEX(s)) {

    /*** do something ***/
    reduce_statement(s);

  }

  /*** a new statement may have been inserted before this ***/
  for (s = stmtls; T_PREV(s) != NULL; s = T_PREV(s));

  return s;
}


static void strength_per_module(module_t *mod) {
  bb_traversal(mod, strength_per_bb, BBTR_NONE);
}


static int strength_reduce(module_t *mod, char *options) {
  if (sr_opt == TRUE) {
    traverse_modules(mod, TRUE, strength_per_module, NULL);
  }

  return 0;
}


/***
 *** function: call_strength -- perform strength reduction
 *** date: 5 september 1997
 *** creator: sungeun
 ***/

int call_strength(module_t *mod, char *s) {
  return (strength_reduce(mod, s));
}
