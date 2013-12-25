/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/struct.h"
#include "../include/parsetree.h"
#include "../include/symtab.h"
#include "../include/macros.h"
#include "../include/const.h"
#include "../include/global.h"
#include "../include/allocstmt.h"
#include "../include/treemac.h"
#include "../include/symmac.h"
#include "../include/buildstmt.h"
#include "../include/buildsymutil.h"
#include "../include/buildzplstmt.h"
#include "../include/stmtutil.h"
#include "../include/db.h"
#include "../include/symboltable.h"
#include "../include/buildsym.h"
#include "../include/name.h"
#include "../include/typeinfo.h"
#include "../include/expr.h"
#include "../include/runtime.h"


expr_t	*build_null_op() {
  expr_t *new;

  DB0(30,"build_null_op()\n");

  new = alloc_expr(NULLEXPR);
  T_IDENT(new) = NULL;
  T_OPLS(new) = NULL;
  return new;
}


static expr_t *continue_strcat_list(expr_t *expr1,expr_t *expr2) {
  expr_t *list;
  expr_t *new;
  expr_t *op1;
  expr_t *op2;

  if (T_TYPE(expr2) == BIPLUS) {
    op1 = T_OPLS(expr2);
    op2 = T_NEXT(op1);
    T_NEXT(op1) = NULL;
    new = continue_strcat_list(expr1,op1);
    new = continue_strcat_list(new,op2);
  } else {
    list = T_MAKE_LIST(expr1);
    cat_expr_ls(list,expr2);
    new = build_Nary_op(FUNCTION,build_0ary_op(VARIABLE,pstSTRNCAT),list);
  }
  return new;
}


static expr_t *build_strcat_list(expr_t *expr1,expr_t *expr2) {
  expr_t *list;
  expr_t *new;

  list = T_MAKE_LIST(expr1);
  cat_expr_ls(list,pexprNULLSTR);
  new = build_Nary_op(FUNCTION,build_0ary_op(VARIABLE,pstSTRNCPY),list);
  new = continue_strcat_list(new,expr2);

  return new;
}


static int building_stringop(expr_t* expr) {
  return (T_TYPE(expr) == VARIABLE && 
	  (S_DTYPE(T_IDENT(expr))==pdtSTRING ||
	   (D_CLASS(S_DTYPE(T_IDENT(expr)))==DT_ENSEMBLE &&
	    D_ENS_TYPE(S_DTYPE(T_IDENT(expr))) == pdtSTRING)));
}


expr_t	*build_binary_op(exprtype op,expr_t *expr2,expr_t *expr1) {
  expr_t *new;
  expr_t *tmp;
  expr_t *list;

  DB2(30,"build_binary_op(%d == %s)\n",op,OpName[(int)op]);

  if (expr1 == NULL) {
    INT_FATAL(NULL, "expr1 == NULL in build_binary_op()");
  }
  if (expr2 == NULL) {
    INT_FATAL(NULL, "expr2 == NULL in build_binary_op()");
  }

  if (building_stringop(expr1)) {
    switch (op) {
    case BIASSIGNMENT:
      if (T_TYPE(expr2) == BIPLUS) {
	new=build_strcat_list(expr1,expr2);
      } else {
	list = T_MAKE_LIST(expr1);
	cat_expr_ls(list,expr2);
	new=build_Nary_op(FUNCTION,build_0ary_op(VARIABLE,pstSTRNCPY),list);
      }
      return new;
    case BIGREAT_THAN:
    case BILESS_THAN:
    case BIG_THAN_EQ:
    case BIL_THAN_EQ:
    case BIEQUAL:
    case BINOT_EQUAL:
      list = T_MAKE_LIST(expr1);
      cat_expr_ls(list,expr2);
      expr1 = build_Nary_op(FUNCTION,build_0ary_op(VARIABLE,pstSTRNCMP),list);
      sprintf(buffer,"0");
      expr2 = build_0ary_op(CONSTANT,build_int(0));
      break;
    default:
      break;
    }
  }
  
  new = alloc_expr(op);
	
  T_OPLS(new) = T_MAKE_LIST(expr2);
  T_OPLS(new) = T_ADD_LIST(expr1,T_OPLS(new));
  T_SET_PARENT(T_OPLS(new),new,tmp);
  if (T_STMT(expr2) == T_STMT(expr1)) {
    T_STMT(new) = T_STMT(expr1);
  }
  return new;
}


expr_t* build_binary_op_gets(exprtype op,binop_t subop,expr_t* expr2,
			     expr_t* expr1) {
  expr_t* new;
  expr_t *list;

  if (building_stringop(expr1)) {
    if (subop == PLUS) {
      if (T_TYPE(expr2) == BIPLUS) {
	new = continue_strcat_list(expr1,expr2);
      } else {
	list = T_MAKE_LIST(expr1);
	if (expr_is_char(expr2, 0)) {
	  expr2 = build_Nary_op(FUNCTION,
				build_0ary_op(VARIABLE,pstCHR2STR),expr2);
	}
	cat_expr_ls(list,expr2);
	new = build_Nary_op(FUNCTION,build_0ary_op(VARIABLE,pstSTRNCAT),list);
      }
    } else {
      USR_FATAL(NULL, "Can only assign strings using := or +=");
    }
  } else {
    new = build_binary_op(op,expr2,expr1);
    T_SUBTYPE(new) = subop;
  }

  return new;
}


expr_t	*build_struct_op(exprtype op,expr_t *expr,symboltable_t *stp) {
  expr_t *new;

  DB2(30,"build_struct_op(%d == %s)\n",op,OpName[(int)op]);

  new = alloc_expr(op);
  T_OPLS(new) = T_MAKE_LIST(expr);
  T_PARENT(expr) = new;
  T_IDENT(new) = stp;
  return new;
}


expr_t *build_unary_at_op(expr_t *expr,expr_t *dir,attype_t type) {
  int i;
  expr_t *retval;
  long dirval;
  int success;

  if (T_TYPE(expr) == VARIABLE) {
    i = symtab_is_indexi(T_IDENT(expr));
    if (i) {
      switch (type) {
      case AT_PRIME:
	YY_FATAL_CONTX(yylineno,in_file,"Cannot apply ' to an Index variable");
	break;
      case AT_WRAP:
	YY_FATAL_CONTX(yylineno,in_file,"Cannot apply @^ to an Index variable");
	break;
      default:
	dirval = dir_comp_to_int(dir, i, &success);
	if (success) {
	  sprintf(buffer,"%ld", dirval);
	  return build_binary_op(BIPLUS,expr,
				 build_0ary_op(CONSTANT, build_int(dirval)));
	} else {
	  INT_FATAL(NULL, "problem with dir_comp_to_int in build_unary_at_op");
	}
      }
    }
  }
  retval = build_binary_op(BIAT,dir,expr);
  T_SUBTYPE(retval) = type;

  return retval;
}


expr_t	*build_unary_op(exprtype op,expr_t *expr) {
  expr_t *new;

  DB2(30,"build_unary_op(%d == %s);\n",op,OpName[(int)op]);

  new = alloc_expr(op);
  T_MAKE_LIST(expr);
  T_OPLS(new) = expr;
  T_PARENT(expr) = new;
  return new;
}


expr_t	*build_Nary_op(exprtype op,expr_t *expr,expr_t *expr_ls) {
  expr_t *new,*t;

  DB2(30,"build_Nary_op(%d == %s)\n",op,OpName[(int)op]);

  new = alloc_expr(op);
  T_MAKE_LIST(expr);
  T_OPLS(new) = cat_expr_ls(expr,expr_ls);
  T_SET_PARENT(T_OPLS(new),new,t)

    return new;
}


expr_t	*build_0ary_op(exprtype op,symboltable_t *id) {
  expr_t *new;

  DB2(30,"build_0ary_op(%d == %s);\n",op,OpName[(int)op]);

  new = alloc_expr(op);
  T_IDENT(new) = id;

  DB0(30,"DBG 30 : leaving build_0ary_op()\n");
  return new;
}


expr_t* build_typed_0ary_op(exprtype op,symboltable_t* id) {
  expr_t* new;
  
  new = build_0ary_op(op,id);
  typeinfo_expr(new);

  return new;
}


expr_t* build_typed_unary_op(exprtype op,expr_t* expr) {
  expr_t* new;
  
  new = build_unary_op(op,expr);
  typeinfo_expr(new);

  return new;
}


expr_t* build_typed_binary_op(exprtype op,expr_t* expr2,expr_t* expr1) {
  expr_t* new;
  
  new = build_binary_op(op,expr2,expr1);
  typeinfo_expr(new);
  
  return new;
}

expr_t* build_typed_Nary_op(exprtype op,expr_t* expr2,expr_t* expr1) {
  expr_t* new;
  
  new = build_Nary_op(op,expr2,expr1);
  typeinfo_expr(new);
  
  return new;
}


statement_t *build_expr_statement(expr_t *expr, int lineno, char *filename) {
  statement_t *new;

  DB2(30,"build_expr_statement(,%d,%s)\n",lineno,filename);

  new = alloc_statement(S_EXPR,lineno,filename);
  T_EXPR(new) = expr;
  return new;
}


statement_t *build_return_statement(expr_t *expr, int lineno, char *filename) {
  statement_t *new;

  DB2(30,"build_return_statement(,%d,%s)\n",lineno,filename);
  
  new = alloc_statement(S_RETURN,lineno,filename);
  T_RETURN(new) = expr;
  return new;
}

statement_t *build_mscan_statement(symboltable_t *pst,statement_t *stmt,
				   int lineno, char *filename) {
  statement_t *new;
  compound_t *newcpd;
  
  DB2(30,"build_mscan_statement(,,%d,%s)\n",lineno,filename);

  new = alloc_statement(S_MSCAN,lineno,filename);
  newcpd = alloc_compound();
  T_DECL(newcpd) = pst;
  T_STLS(newcpd) = stmt;
  T_CMPD(new) = newcpd;
  return new;
}


statement_t *build_compound_statement(symboltable_t *stp,statement_t *stmt,
				      int lineno, char *filename) {
  statement_t *new;
  compound_t *newcpd;

  DB2(30,"build_compound_statement(,,%d,%s)\n",lineno,filename);

  new = alloc_statement(S_COMPOUND,lineno,filename);
  newcpd = alloc_compound();
  T_DECL(newcpd) = stp;
  T_STLS(newcpd) = stmt;
  T_CMPD(new) = newcpd;
  if (stmt != NULL) {
    T_PARENT(stmt) = new;
  }
  return new;
}


statement_t *build_if_statement(expr_t *expr,statement_t *s1,statement_t *s2,
				statement_t *s3, int lineno, char *filename) {
  statement_t *new;
  if_t	      *newif;

  DB2(30,"build_if_statement(,%d,%s)\n",lineno,filename);

  new = alloc_statement(S_IF,lineno,filename);
  T_SUBTYPE(new) = C_NORMAL;
  newif = alloc_if();
  T_IFCOND(newif) = expr;
  T_THEN(newif) = s1;
  T_ELSE(newif) = s2;
  T_IF(new) = newif;

  return new;
}


statement_t *build_loop_statement(looptype type,expr_t *expr,statement_t *body,
				  int lineno, char *filename) {
  statement_t *new;
  loop_t      *newloop;

  DB3(30,"build_loop_statement(%d,%d,%s)\n",type,lineno,filename);

  new = alloc_statement(S_LOOP,lineno,filename);
  newloop = alloc_loop(type);
  T_BODY(newloop) = body;

  T_LOOPCOND(newloop) = expr;
  T_LOOP(new) = newloop;
  return new;
}


statement_t *build_do_statement(looptype type,int updown,symboltable_t *lab,expr_t *ivar,
				expr_t *start,expr_t *stop,expr_t *step,
				statement_t *body,int lineno,char *filename) {
  statement_t *newstmt;
  loop_t      *new;

  newstmt = alloc_statement(S_LOOP,lineno,filename);
  new = alloc_loop(type);
  T_BODY(new)     = body;
  T_ENDDO(new)    = lab;
  T_IVAR(new)     = ivar;
  T_START(new)    = start;
  T_STOP(new)     = stop;
  T_STEP(new) = (step != NULL) ? step : new_const_int(1);
  T_UPDOWN(new)   = updown;
  T_LOOP(newstmt) = new;

  return newstmt;
}


function_t *build_function(symboltable_t *fcn,symboltable_t *locals,
			   genlist_t *params,statement_t *stmts,
			   function_t *next) {
  function_t	*new;

  new = alloc_function();
  T_FCN(new) = fcn;
  T_DECL(new) = locals;
  T_PARAMLS(new) = params;
  T_STLS(new) = stmts;
  T_PARALLEL(new) = FALSE;
  T_NEXT(new) = next;
  T_OVERLOAD_PREV(new) = NULL;
  T_OVERLOAD_NEXT(new) = NULL;

  return new;
}

module_t *add_module(module_t *module, symboltable_t *pst) {
  if (pst == NULL) {
    return module;
  }

  if (S_CLASS(pst) != S_SUBPROGRAM) {
    INT_FATAL(NULL, "Non-function passed to add_module");
  }

  DBS2(10, "ADD MODULE %s %d\n\n",S_IDENT(pst), yylineno);

  if (module == NULL) {
    module = alloc_module();
  }
  if (T_STLS(S_FUN_BODY(pst)) != NULL) {
    T_FCNLS(module) = cat_fcn_ls(T_FCNLS(module),S_FUN_BODY(pst));
  }
  return module;
}

module_t *finish_module(module_t *module) {
  if (module == NULL) {
    USR_FATAL(NULL, "Your ZPL program contains no procedure bodies");
  }
  T_DECL(module) = getlevel(0);
  if (T_DECL(module) == NULL) {
    INT_FATAL(NULL, "Unexpected NULL in finish_module()");
  }

  return module;
}
