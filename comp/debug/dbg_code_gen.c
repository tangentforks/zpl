/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#if (!RELEASE_VER)
#include <stdio.h>
#include "../include/error.h"
#include "../include/global.h" 
#include "../include/parsetree.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/stmtutil.h"
#include "../include/db.h"
#include "../include/macros.h"
#include "../include/iostmt.h"
#include "../include/name.h"
#include "../include/buildzplstmt.h"
#include "../include/dbg_code_gen.h"
#include "../include/genlist.h"
#include "../include/passes.h"
#include "../include/expr.h"

#define debug_constant 	300

static void dbg_error_recovery(FILE *,statement_t *);
static void dbg_gen_pst(FILE *,symboltable_t *);
static void dbg_gen_symlist(FILE *,symboltable_t *);

static void dbg_gen_if(FILE *fp,if_t *ifstmt) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_if \n");
  
  if (ifstmt == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_if due to null\n");
    return;
  }
  
  if (ST_TYPE(ifstmt) != IF_T) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)ifstmt);
    return;
  }
  
  fprintf(fp, "if (");		fflush(fp);
  dbg_gen_expr(fp, T_IFCOND(ifstmt));
  fprintf(fp, ") {\n");		fflush(fp);
  
  dbg_gen_stmtls(fp, T_THEN(ifstmt));
  
  fprintf(fp, "}\nelse {");	fflush(fp);
  
  dbg_gen_stmtls(fp, T_ELSE(ifstmt));
  
  fprintf(fp, "}\n");		fflush(fp);
  
  fflush(stdout);
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_if\n");
}



static void dbg_gen_variant_struct(FILE *fp,typeclass class) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_variant_struct \n");
  
  if (class == DT_STRUCTURE) 
    fprintf(fp, "struct ");
  else if (class == DT_ENUM)
    fprintf(fp, "enum ");
  else 
    fprintf(fp, "/* dbg_gen_variant_struct : unknown class */\n");
  fflush(fp);
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_variant struct\n");
}


static void dbg_gen_enumlist(FILE *fp, symboltable_t *pst) {
  symboltable_t *pstTemp;
  
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_enumlist \n");
  
  if (pst == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_enumlist due to null\n");
    return;
  }
  
  if (ST_TYPE(pst) != SYMBOLTABLE_T) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)pst);
    return;
  }
  
  fprintf(fp, "{\n");
  fflush(fp);
  for (pstTemp = pst; pstTemp != NULL; pstTemp = S_SIBLING(pstTemp)) {
    dbg_gen_pst(fp, pstTemp);
    if (S_SIBLING(pstTemp) != NULL)
      fprintf(fp, ",\n");
    else
      fprintf(fp, "\n");
  }
  fprintf(fp, "} ");
  fflush(fp);
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_enumlist\n");
}


static dimension_t *dbg_gen_pdt(FILE *fp,datatype_t *pdt,int flag) {
  dimension_t 	*pd = NULL;
  typeclass 	class;

  DB0(debug_constant, "\t\t\t\tEN dbg_gen_pdt \n");

  if (pdt == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_pdt due to null\n");
    return(NULL);
  }
  if (ST_TYPE(pdt) != DATATYPE_T) {
    DB0(debug_constant, "dbg_gen_pdt : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)pdt);
    return(NULL);
  }

  switch(D_CLASS(pdt)) {
    
  case DT_INTEGER:
    fprintf(fp, "int ");
    fflush(fp);
    break;
    
  case DT_REAL:
    fprintf(fp, "float ");
    fflush(fp);
    break;

  case DT_SHORT:
    fprintf(fp, "short ");
    fflush(fp);
    break;

  case DT_LONG:
    fprintf(fp, "long ");
    fflush(fp);
    break;

  case DT_DOUBLE:
    fprintf(fp, "double ");
    fflush(fp);
    break;

  case DT_QUAD:
    fprintf(fp, "quad ");
    fflush(fp);
    break;

  case DT_UNSIGNED_INT:
    fprintf(fp, "unsigned ");
    fprintf(fp, "int ");
    fflush(fp);
    break;

  case DT_UNSIGNED_SHORT:
    fprintf(fp, "unsigned ");
    fprintf(fp, "short ");
    fflush(fp);
    break;

  case DT_UNSIGNED_LONG:
    fprintf(fp, "unsigned ");
    fprintf(fp, "long ");
    fflush(fp);
    break;
    
  case DT_UNSIGNED_BYTE:
    fprintf(fp, "unsigned ");
    fprintf(fp, "char ");
    fflush(fp);
    break;

  case DT_SIGNED_BYTE:
    fprintf(fp, "unsigned ");
    fprintf(fp, "char ");
    fflush(fp);
    break;

  case DT_BOOLEAN:
    fprintf(fp, "boolean ");
    fflush(fp);
    break;
    
  case DT_CHAR:
    fprintf(fp, "char ");
    fflush(fp);
    break;
    
  case DT_ARRAY:
    break;
    
  case DT_ENUM:
  case DT_STRUCTURE:
    class = D_CLASS(pdt);
    
    switch (flag) {
      
    case 0:  
      if (D_NAME(pdt) != NULL) 
	fprintf(fp, "%s ", S_IDENT(D_NAME(pdt)));
      else {
	dbg_gen_variant_struct(fp, class);
	if (D_STAG(pdt) != NULL) {
	  fprintf(fp, "%s ", S_IDENT(D_STAG(pdt)));
	  fflush(fp);
	  if ((S_FG_SHADOW(D_STAG(pdt)) == 0) &&
	      (S_FG_1(D_STAG(pdt)) == 0)) {
	    S_FG_1(D_STAG(pdt)) = 1;
	    if (class == DT_ENUM) 
	      dbg_gen_enumlist(fp, D_STRUCT(pdt));
	    else 
	      dbg_gen_symlist(fp, D_STRUCT(pdt));
	  }  
	}  
	else if (class == DT_ENUM)  
	  dbg_gen_enumlist(fp, D_STRUCT(pdt));
	else 
	  dbg_gen_symlist(fp, D_STRUCT(pdt));
      } 
      break;
    case 1:
      dbg_gen_variant_struct(fp, class);
      if (D_STAG(pdt) != NULL) {
	fprintf(fp, "%s ", S_IDENT(D_STAG(pdt)));
	fflush(fp);
	if (S_FG_SHADOW(D_STAG(pdt)) == 0) { 
	  if (class == DT_ENUM) 
	    dbg_gen_enumlist(fp, D_STRUCT(pdt));
	  else 
	    dbg_gen_symlist(fp, D_STRUCT(pdt));
	}  
      } 
      else {
	if (class == DT_ENUM) 
	  dbg_gen_enumlist(fp, D_STRUCT(pdt));
	else 
	  dbg_gen_symlist(fp, D_STRUCT(pdt));
      } 
      break; 
    default: 
      dbg_gen_variant_struct(fp, class);
      if (D_STAG(pdt) != NULL) {
	fprintf(fp, "%s ", S_IDENT(D_STAG(pdt)));
	fflush(fp);
	S_FG_1(D_STAG(pdt)) = 1;
      }
      if (class == DT_ENUM)
	dbg_gen_enumlist(fp, D_STRUCT(pdt));
      else
	dbg_gen_symlist(fp, D_STRUCT(pdt));
      break;
    }
    break;
    
  case DT_VOID    : fprintf(fp, "void ");     break;
  case DT_GENERIC: fprintf(fp, "generic "); break;
  case DT_GENERIC_ENSEMBLE: fprintf(fp, "generic ensemble"); break;
  case DT_FILE:			/* linc */
    fprintf(fp, "zpl_file ");
    break;
  case DT_STRING:		/* linc */
    fprintf(fp, "char * ");
    break;
  default: 
    DB0(debug_constant, "/* dbg_gen_pdt : <UNKNOWN TYPECLASS> */\n");
  } 

  DB0(debug_constant, "\t\t\t\tEX dbg_gen_pdt\n");
  return(pd);
}


static void dbg_gen_psc(FILE *fp,int sc) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_psc \n");
  
  if (sc & SC_STATIC)
    fprintf(fp, "static ");
  if (sc & SC_EXTERN)
    fprintf(fp, "extern ");
  if (sc & SC_REGISTER)
    fprintf(fp, "register ");
  fflush(fp);
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_psc\n");
}


static void dbg_gen_array_decl(FILE *fp,dimension_t *pd) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_array_decl \n");
  
  if ((pd != NULL) && (ST_TYPE(pd) != DIMENSION_T)) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)pd);
    return;
  }
  
  for (; pd != NULL; pd = DIM_NEXT(pd)) {
    fprintf(fp, "[");
    fflush(fp);
    dbg_gen_expr(fp, DIM_HI(pd));
    fprintf(fp, "]"); 	
    fflush(fp);
  }
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_array_decl\n");
} 


static void dbg_gen_init_list(FILE *fp,initial_t *p) {
  initial_t *pit;
  
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_init_list \n");
  
  if ((p != NULL) && (ST_TYPE(p) != INITIAL_T)) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)p);
    return;
  }
  
  for (pit = p; pit != NULL; pit = S_NEXT(pit)) {
    if (IN_BRA(pit)) {
      fprintf(fp, "{");
      fflush(fp);
      dbg_gen_init_list(fp, IN_LIS(pit));
      fprintf(fp, "}");
      fflush(fp);
    } else {
      dbg_gen_expr(fp, IN_VAL(pit));
    }
    if (S_NEXT(pit) != NULL) {
      fprintf(fp, ",\n");
    }
  } 
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_init_list\n");
} 


static void dbg_gen_pst(FILE *fp,symboltable_t *pst) {
  symboltable_t *pstTemp;
  genlist_t *glp;
  statement_t *pstmt;
  dimension_t *pdTemp = NULL;
  int comma_flag = 0; 
  int fflag      = 1; 

  DB0(debug_constant, "\t\t\t\tEN dbg_gen_pst \n");

  if (pst == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_pst due to null\n");
    return;
  }

  if (ST_TYPE(pst) != SYMBOLTABLE_T) {
    DB0(debug_constant, "dbg_gen_pst : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)pst);
    return;
  }

  switch(S_CLASS(pst)) {
    
  case S_VARIABLE:
    if (S_IS_CONSTANT(pst)) break;
    dbg_gen_psc(fp, S_STYPE(pst)); 
    pdTemp = dbg_gen_pdt(fp, S_DTYPE(pst), 0);
    fprintf(fp, "%s", S_IDENT(pst));
    fflush(fp);
    dbg_gen_array_decl(fp, pdTemp);
    if (S_VAR_INIT(pst) != NULL) {
      fprintf(fp, " = ");
      fflush(fp);
      dbg_gen_init_list(fp, S_VAR_INIT(pst));
    }
    if ((S_FG_TYPE(pst) == 0) && (!fflag))
      fprintf(fp, ";\n");
    break;
    
  case S_TYPE:
    fprintf(fp, "typedef ");
    fflush(fp);
    pdTemp = dbg_gen_pdt(fp, S_DTYPE(pst), 1);
    fprintf(fp, "%s;\n", S_IDENT(pst));
    fflush(fp);
    break;
    
  case S_LITERAL:    	
    
    break;
    
  case S_SUBPROGRAM:
    DB0(debug_constant, "dbg_gen_pst:in S_SUBPROGRAM\n");
    pdTemp = dbg_gen_pdt(fp, S_FUN_TYPE(pst), 0);
    if (S_FG_1(pst) == 1)
      fprintf(fp, "PROGRAM ");
    else if (S_FG_2(pst) == 1)
      fprintf(fp, "SUBROUTINE ");
    else if (S_FG_3(pst) == 1)
      fprintf(fp, "FUNCTION ");
    fprintf(fp, "%s(", S_IDENT(pst));
    fflush(fp);
    for (glp=T_PARAMLS(S_FUN_BODY(pst));glp!=NULL;glp=G_NEXT(glp)){
      pstTemp = G_IDENT(glp);
      if (S_CLASS(pstTemp) == S_PARAMETER) {
	if (comma_flag)
	  fprintf(fp, ",");
	fprintf(fp, "%s", S_IDENT(pstTemp));
	fflush(fp);
	comma_flag = 1;
      }  
    }
    fprintf(fp, ")");
    fflush(fp);
    
    
    DB0(debug_constant, "dbg_gen_pst:in formal parameters\n");
    glp = T_PARAMLS(S_FUN_BODY(pst));
    if (glp != NULL)
      fprintf(fp, "\n");
    while (glp != NULL) {
      pstTemp = G_IDENT(glp);
      if (S_CLASS(pstTemp) == S_PARAMETER) 
	dbg_gen_pst(fp, pstTemp);
      glp = G_NEXT(glp);
    } 
    
    DB0(debug_constant, "dbg_gen_pst:in sub body\n");
    pstmt = T_STLS(S_FUN_BODY(pst));
    if (pstmt != NULL) {
      fprintf(fp, "{\n");
      fflush(fp);
      dbg_gen_stmtls(fp, pstmt);
      fprintf(fp, "}\n");
      fflush(fp);
    } else if ((S_FG_TYPE(pst) == 0) && (!fflag))
      fprintf(fp, ";\n");
    DB0(debug_constant, "dbg_gen_pst:out\n");
    break;
    
  case S_PARAMETER:
    pdTemp = dbg_gen_pdt(fp, S_PAR_TYPE(pst), 0);
    fprintf(fp, "%s", S_IDENT(pst));
    fflush(fp);
    dbg_gen_array_decl(fp, pdTemp);
    fprintf(fp, ";\n");
    fflush(fp);
    break;
    
  case S_STRUCTURE:
    fprintf(fp, "struct %s ", S_IDENT(pst));
    fflush(fp);
    if (!fflag) {
      dbg_gen_symlist(fp, D_STRUCT(S_STRUCT(pst)));
      S_FG_1(pst) = 1;
      fprintf(fp, ";\n");
      fflush(fp);
    }
    break;
    
  case S_VARIANT:
    fprintf(fp, "union %s ", S_IDENT(pst));
    fflush(fp);
    if (!fflag) {
      dbg_gen_symlist(fp, D_STRUCT(S_VARI(pst)));
      S_FG_1(pst) = 1;
      fprintf(fp, ";\n");
      fflush(fp);
    }
    break;
    
  case S_ENUMTAG:
    fprintf(fp, "enum %s ", S_IDENT(pst));
    fflush(fp);
    if (!fflag) {
      dbg_gen_enumlist(fp, D_STRUCT(S_DTYPE(pst)));
      S_FG_1(pst) = 1;
      fprintf(fp, ";\n");
      fflush(fp);
    }
    break;
    
  case S_COMPONENT:
    pdTemp = dbg_gen_pdt(fp, S_COM_TYPE(pst), 0);
    fprintf(fp, "%s", S_IDENT(pst));
    fflush(fp);
    dbg_gen_array_decl(fp, pdTemp); 
    if (S_COM_BITLHS(pst) != NULL) {
      fprintf(fp, " : %d",S_COM_BIT(pst));
      fflush(fp);
    }
    fprintf(fp, ";\n");
    fflush(fp);
    break;
    
  case S_ENUMELEMENT:
    fprintf(fp, "%s", S_IDENT(pst));
    fflush(fp);
    if (S_EINIT(pst) == SC_EINIT) {
      fprintf(fp, "= %d",S_EVALUE(pst));
      fflush(fp);
    }
    break;
    
  case S_UNKNOWN:
    fprintf(fp, "/* dbg_gen_pst : UNKNOWN */\n");
    fflush(fp);
    break;
    
  default:
    DB0(debug_constant, "/* dbg_gen_pst : <UNKNOWN SYMBOLCLASS> */\n");
  } 
  
  if (S_FG_2(pst) == 1)
    fprintf(fp, ")");
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_pst\n");
} 


static void dbg_gen_symlist(FILE *fp,symboltable_t *pst) {
  symboltable_t *pTemp; 	
  
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_symlist \n");
  
  if (pst == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_symlist due to null\n");
    return;
  }
  
  if (ST_TYPE(pst) != SYMBOLTABLE_T) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)pst);
    return;
  }
  
  fprintf(fp, "{\n");
  fflush(fp);
  for (pTemp=pst; pTemp!=NULL; pTemp=S_SIBLING(pTemp)) 
    dbg_gen_pst(fp, pTemp);
  fprintf(fp, "} ");
  fflush(fp);
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_symlist\n");
}


static void dbg_gen_iostmt(FILE *fp,statement_t *stmt) {
  io_t	*this;

  this = T_IO(stmt);
  switch (IO_TYPE(this)) {
  case IO_READ:
    fprintf(fp, "read(");
    dbg_gen_expr(fp, IO_EXPR1(this));
    fprintf(fp, ")");
    break;
  case IO_HALT:
  case IO_WRITE:
    fprintf(fp, "write(");
    dbg_gen_expr(fp, IO_EXPR1(this));
    fprintf(fp, ")");
    break;
  case IO_HALTLN:
  case IO_WRITELN:
    fprintf(fp, "writeln(");
    dbg_gen_expr(fp, IO_EXPR1(this));
    fprintf(fp, ")");
    break;
  default:
    INT_FATAL(stmt, "Incorrect IO type %d", IO_TYPE(T_IO(stmt)));
  }

  fprintf(fp, "\n");
  fflush(fp);
  return;
}  


static void dbg_gen_end(FILE *fp,endtype end_t) {
  switch (end_t) {
    
  case E_ENDDO: fprintf(fp, "END DO"); break;
  case E_ENDIF: fprintf(fp, "END IF"); break;
  default: DB0(debug_constant, "Default taken in dbg_gen_end\n");
    
  }
  fflush(fp);
}


static void dbg_gen_arrayref(FILE *fp,expr_t *exprls) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_arrrayref \n");

  if (exprls == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_arrayref due to null\n");
    return;
  }
  
  if (ST_TYPE(exprls) != EXPR_T) {
    DB0(debug_constant, "dbg_gen_arrayref : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)EXPR_T);
    return;
  }
  fprintf(fp, "[");
  fflush(fp);
  for (; exprls != NULL; exprls = T_NEXT(exprls)) {
    dbg_gen_expr(fp, exprls);
    if (T_NEXT(exprls) != NULL)
      fprintf(fp, "][");
    else
      fprintf(fp, "]");
  } 
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_arrayref\n");
} 



static void dbg_gen_exprls(FILE *fp,expr_t *exprls) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_exprls \n");
  
  if ((exprls != NULL) && (ST_TYPE(exprls) != EXPR_T)) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)exprls);
    return;
  }
  for (; exprls != NULL; exprls = T_NEXT(exprls)) {
    dbg_gen_expr(fp, exprls);
    if (T_NEXT(exprls) != NULL)
      fprintf(fp, ",");
  }
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_exprls\n");
}



void dbg_gen_expr(FILE *fp,expr_t *expr) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_expr \n");
  
  if (expr == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_expr due to null\n");
    return;
  }
  
  if (ST_TYPE(expr) != EXPR_T) {
    DB0(debug_constant, "dbg_gen_expr : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)expr);
    return;
  }
  
  if (expr_requires_parens(expr))
    fprintf(fp, "(");
  
  switch (T_TYPE(expr)) {
  case NULLEXPR:
    break;
  case VARIABLE:
  case CONSTANT:
    fprintf(fp, "%s",S_IDENT(T_IDENT(expr)));
    fflush(fp);
    break;
    
  case ARRAY_REF:
    if (T_OPLS(expr) == NULL) {
      fprintf(fp, "dbg_gen_expr : no array name in array ref");
      fflush(fp);
      break;
    }
    dbg_gen_expr(fp, nthoperand(expr,1));
    dbg_gen_arrayref(fp, nthoperand(expr,2));
    break;
    
  case FUNCTION:
    if (T_OPLS(expr) == NULL) {
      fprintf(fp, "dbg_gen_expr : no func name in func call");
      fflush(fp);
      break;
    }
    dbg_gen_expr(fp, nthoperand(expr,1));
    fprintf(fp, "(");
    fflush(fp);
    dbg_gen_exprls(fp, nthoperand(expr,2));
    fprintf(fp, ")");
    fflush(fp);
    break;
    
  case BIAT:
    dbg_gen_expr(fp, T_OPLS(expr));
    if (T_SUBTYPE(expr) == AT_PRIME) {
      fprintf(fp, "'");
    }
    fprintf(fp, "@%s", S_IDENT(expr_find_root_pst(T_NEXT(T_OPLS(expr)))));
    fflush(fp);
    break;
    
  case BIDOT:
    dbg_gen_expr(fp, T_OPLS(expr));
    fprintf(fp, "%s%s",OpName[(int)T_TYPE(expr)],S_IDENT(T_IDENT(expr)));
    fflush(fp);
    break;
    
  default:
    if (T_IS_UNARY(T_TYPE(expr))) {
      fprintf(fp, " %s",OpName[(int)T_TYPE(expr)]);
      fflush(fp);
      dbg_gen_expr(fp, T_OPLS(expr));
    }
    else if (T_IS_BINARY(T_TYPE(expr))) {
      dbg_gen_expr(fp, left_expr(expr));
      fprintf(fp, " %s ",OpName[(int)T_TYPE(expr)]);
      fflush(fp);
      dbg_gen_expr(fp, right_expr(expr));
    }
    else
      DB1(debug_constant, "ERROR bad type (%d) in dbg_gen_expr",T_TYPE(expr));
  }
  
  if (expr_requires_parens(expr))
    fprintf(fp, ")");
  fflush(stdout);
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_expr\n");
}

static void dbg_gen_loop(FILE *fp,loop_t *loop) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_loop \n");
  
  if (loop == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_loop due to null\n");
    return;
  }
  
  if (ST_TYPE(loop) != LOOP_T) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)loop);
    return;
  }
  
  switch (T_TYPE(loop)) {
    
  case L_WHILE_DO:
    fprintf(fp, "while (");
    fflush(fp);
    dbg_gen_expr(fp, T_LOOPCOND(loop));
    fprintf(fp, ")\n");
    fflush(fp);
    dbg_gen_stmt(fp, T_BODY(loop)); 
    break;
    
  case L_REPEAT_UNTIL:
    fprintf(fp, "repeat");
    fflush(fp);
    dbg_gen_stmt(fp, T_BODY(loop)); 
    fprintf(fp, " until (\n");
    fflush(fp);
    dbg_gen_expr(fp, T_LOOPCOND(loop));
    fprintf(fp, ");\n");
    fflush(fp);
    break;
    
  case L_DO:  
    fprintf(fp, "DO ");
    fflush(fp);
    if (T_ENDDO(loop) != NULL)
      fprintf(fp, "%s ", S_STRIP(S_IDENT(T_ENDDO(loop))));
    dbg_gen_expr(fp, T_IVAR(loop));
    fprintf(fp," = ");
    fflush(fp);
    dbg_gen_expr(fp, T_START(loop));
    fprintf(fp, ","); 
    fflush(fp);
    dbg_gen_expr(fp, T_STOP(loop));
    if (T_STEP(loop) != NULL) {
      fprintf(fp, ","); 
      fflush(fp);
      dbg_gen_expr(fp, T_STEP(loop));
    }
    fprintf(fp, " {\n"); 
    fflush(fp);
    dbg_gen_stmtls(fp, T_BODY(loop));
    fprintf(fp, "} end_do;\n"); 
    break;
    
  default:
    DB0(debug_constant, "default taken in dbg_gen_loop\n");
  }
  
  fflush(stdout);
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_loop\n");
}


/*GHF dump a WRAP/REFLECT statement */
static void dbg_gen_wrap(FILE *fp,statement_t *stmt) {
  expr_t *p;

/*  fprintf(fp, "%s [", StmtName[T_TYPE(stmt)]);
  if (T_WRAP_REG(T_WRAP(stmt)))
    fprintf(fp, "%s", S_IDENT(T_WRAP_REG(T_WRAP(stmt))));
  else
    fprintf(fp, "???");
  fprintf(fp, "] ");
*/  
  p = T_OPLS(T_WRAP(stmt));
  while (p != NULL) {
    /* fprintf( fp, "%s, ", S_IDENT(T_IDENT(p))); */
    dbg_gen_expr(fp, p);
    p = T_NEXT(p);
  }
  fprintf(fp, ";\n"); fflush(fp);
}



static void dbg_gen_comm(FILE  *fp, statement_t *stmt) {
  comm_info_t *c;
  genlist_t *dirs;

  fprintf(fp, "%s%s%d {\n", 
	  CommStmtName[T_COMM_TYPE(stmt)],
	  (T_COMM_LHSCOMM(stmt) ? " (LHS) " : " " ),
	  T_COMM_ID(stmt));
  if (T_COMM_REG(stmt))
    fprintf(fp, "  [%s] ", S_IDENT(T_IDENT(T_COMM_REG(stmt))));
  else
    fprintf(fp, "  [???] ");

  for (c = T_COMM_INFO(stmt); c != NULL; c = T_COMM_INFO_NEXT(c)) {
    dbg_gen_expr(fp, T_COMM_INFO_ENS(c));

    for (dirs = T_COMM_INFO_DIR(c); dirs != NULL; dirs = G_NEXT(dirs))
      fprintf(fp, " @%s ", S_IDENT(expr_find_root_pst(G_EXPR(dirs))));
  }

  fprintf(fp, "\n}\n"); fflush(fp);
}


static void dbg_gen_decls(FILE *fp,symboltable_t *pst) {

  DB0(debug_constant, "\t\t\t\tEN dbg_gen_decls \n");
  
  if ((pst != NULL) && (ST_TYPE(pst) != SYMBOLTABLE_T)) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)pst);
    return;
  }
  for (; pst != NULL; pst = S_SIBLING(pst))  {
    dbg_gen_pst(fp, pst);
  }
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_decls\n");
} 


static void dbg_gen_exit(FILE *fp) {
  fprintf(fp, "break;\n");
  fflush(fp);
} 


static void dbg_gen_halt(FILE *fp) {
  fprintf(fp, "_ZPL_halt();\n");
  fflush(fp);
}


static void dbg_error_recovery(FILE *fp,statement_t *ptr) {
  DB0(debug_constant, "\t\t\t\tEN dbg_error_recovery \n");
  
  if (ptr == NULL) {
    DB0(debug_constant, "dbg_error_recovery : NULL pointer sent\n");
    return;
  }
  
  switch (ptr->st_type) {
    
  case STATEMENT_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to statement found\n");
    dbg_gen_stmt(fp, ptr);
    break;
  case EXPR_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to expr found\n");
    dbg_gen_expr(fp, (expr_t *)ptr);
    break;
  case IF_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to if found\n");
    dbg_gen_if(fp, (if_t *)ptr);
    break;
  case LOOP_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to loop found\n");
    dbg_gen_loop(fp, (loop_t *)ptr);
    break;
  case DIMENSION_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to dimension found\n");
    dbg_gen_array_decl(fp, (dimension_t *)ptr);
    break;
  case SYMBOLTABLE_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to symboltable found\n");
    dbg_gen_pst(fp, (symboltable_t *)ptr);
    break;
  case DATATYPE_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to datatype found\n");
    dbg_gen_pdt(fp, (datatype_t *)ptr, 2); 
    break;
  case COMPOUND_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to compound found\n");
    
    break;
  case FUNCTION_T : 
    DB0(debug_constant, "dbg_error_recovery : ptr to function found\n");
    dbg_gen_pst(fp, T_FCN((function_t *)ptr));
    break;
  case INITIAL_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to initial found\n");
    dbg_gen_init_list(fp, (initial_t *)ptr);
    break;
  case ASSIGN_T :
    DB0(debug_constant, "dbg_error_recovery : ptr to assign found\n");
    dbg_gen_expr(fp, (expr_t *)ptr);
    break;
  case MODULE_T : 
    DB0(debug_constant, "dbg_error_recovery : ptr to module found\n");
    DB0(debug_constant, "for now, I don't know what to do with this\n");
    break;
  default :
    DB0(debug_constant, "dbg_error_recovery : deefault taken !!!\n");
  } 
  
  DB0(debug_constant, "\t\t\t\tEX dbg_error_recovery\n");
} 

void dbg_gen_stmt(FILE *fp,statement_t *stmt) {
  DB0(debug_constant, "\t\t\t\tEN dbg_gen_stmt \n");
  
  if (stmt == NULL) {
    DB0(debug_constant, "\t\t\t\tEX dbg_gen_stmt due to null\n");
    return;
  }
  if (ST_TYPE(stmt) != STATEMENT_T) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, (statement_t *)stmt);
    return;
  }
  
  switch (T_TYPE(stmt)) {
    
  case S_IO: dbg_gen_iostmt(fp, stmt); break;
  case S_IF: dbg_gen_if(fp, T_IF(stmt)); break;
  case S_NULL: fprintf(fp, ";\n"); fflush(fp); break;
  case S_LOOP: dbg_gen_loop(fp, T_LOOP(stmt)); break;
  case S_EXIT: dbg_gen_exit(fp); break;
  case S_HALT: dbg_gen_halt(fp); break;
    
  case S_WRAP:
  case S_REFLECT:		
    dbg_gen_wrap(fp, stmt); break;
    
  case S_REGION_SCOPE:
    /* NOTE: any changes here should be reflected below. */
    fprintf(fp, "REGION_SCOPE ");
    fprintf( fp, "[");
    if (T_REGION_SYM(T_REGION(stmt)) != NULL)
      fprintf( fp, "%s", S_IDENT(T_IDENT(T_REGION_SYM(T_REGION(stmt)))));
    if (T_MASK_EXPR(T_REGION(stmt)) != NULL) {
      if (T_MASK_BIT(T_REGION(stmt)))
	fprintf( fp, " with");
      else
	fprintf( fp, " without");
      dbg_gen_expr(fp, T_MASK_EXPR(T_REGION(stmt)));
    }
    fprintf( fp, "]");
    fprintf(fp, " {\n"); 
    fflush(fp);
    dbg_gen_stmtls(fp, T_BODY(T_REGION(stmt)));
    fprintf(fp, "}\n"); fflush(fp);
    break;
  case S_MLOOP:
    /* NOTE: any changes here should be reflected above. */
    if (T_MLOOP_REG(T_MLOOP(stmt)))
      fprintf(fp, "MLOOP %s", S_IDENT(T_IDENT(T_MLOOP_REG(T_MLOOP(stmt))))); 
    else
      fprintf(fp, "MLOOP ???");
    fprintf(fp, " {\n");
    fflush(fp);
    dbg_gen_stmtls(fp, T_BODY(T_MLOOP(stmt)));
    fprintf(fp, "}\n"); fflush(fp);
    break;
    
  case S_NLOOP:
    /* NOTE: any changes here should be reflected above. */
    fprintf(fp, "NLOOP");
    fprintf(fp, " {\n");
    fflush(fp);
    dbg_gen_stmtls(fp, T_NLOOP_BODY(T_NLOOP(stmt)));
    fprintf(fp, "}\n"); fflush(fp);
    break;
    
  case S_COMM:
    dbg_gen_comm(fp, stmt); break;
    
  case S_COMPOUND: 
    fprintf(fp, "{\n"); fflush(fp);
    dbg_gen_decls(fp, T_CMPD_DECL(stmt));
    dbg_gen_stmtls(fp, T_CMPD_STLS(stmt));
    fprintf(fp, "}\n"); fflush(fp);
    break;
    
  case S_MSCAN:
    fprintf(fp, "mscan {\n"); fflush(fp);
    dbg_gen_decls(fp, T_CMPD_DECL(stmt));
    dbg_gen_stmtls(fp, T_CMPD_STLS(stmt));
    fprintf(fp, "}\n"); fflush(fp);
    break;
    
  case S_EXPR:
    DB0(debug_constant, "\t\t\t\tIN S_EXPR\n");
    if (T_TYPE(T_EXPR(stmt)) == FUNCTION) 
      
      dbg_gen_expr(fp, T_EXPR(stmt));
    else 
      dbg_gen_expr(fp, T_EXPR(stmt));
    fprintf(fp, ";\n"); fflush(fp);
    break;
    
  case S_RETURN:
    fprintf(fp, "return "); fflush(fp);
    dbg_gen_expr(fp, T_RETURN(stmt));
    fprintf(fp, ";\n"); fflush(fp);
    break;
    
  case S_END:
    dbg_gen_end(fp, T_END(stmt));
    fprintf(fp, "\n"); fflush(fp);
    break;
    
  default:
    DB1(debug_constant, "default taken : dbg_gen_stmt type == %d",T_TYPE(stmt));
  }
  
  DB0(debug_constant, "\t\t\t\tEX dbg_gen_stmt\n");
} 


void dbg_gen_stmtls(FILE *fp,statement_t *stmtls) { 	
  DB0(debug_constant, "dbg_gen_stmtls(,)\n");

  if ((stmtls != NULL) && (ST_TYPE(stmtls) != STATEMENT_T)) {
    DB0(debug_constant, "dbg_gen_array_decl : wrong pointer type\n");
    dbg_error_recovery(fp, stmtls);
    return;
  }
  for (; stmtls != NULL; stmtls = T_NEXT(stmtls))
    dbg_gen_stmt(fp, stmtls);
  
  DB0(debug_constant, "dbg_gen_stmtls(,) exiting\n");
}


static void dbg_gen_code(FILE *fp,module_t *module) {
  function_t *fnp;   

  DB0(debug_constant, "in dbg_gen_code(,)\n");

  for (fnp = T_FCNLS(module); fnp != NULL; fnp = T_NEXT(fnp)) {
    dbg_gen_pst(fp, T_FCN(fnp));
  }

  DB0(debug_constant, "exiting dbg_gen_code\n");
} 


int call_debug_codegen(module_t *mod,char *s) {
  module_t *modls;
  FILE *fp;

  fp = fopen("dbg_gen_code.out", "w");
  for (modls=mod; modls!=NULL; modls=T_NEXT(modls)){
    dbg_gen_code(fp, modls);
    fprintf(fp,"################################\n");
  }
  fclose(fp);
  return 0;
}
#endif

