/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/buildsymutil.h"
#include "../include/cglobal.h"
#include "../include/db.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/stack.h"
#include "../include/symbol.h"
#include "../include/symmac.h"
#include "../include/treemac.h"

symboltable_t *build_enumelem(symboltable_t *ident,expr_t *v) {
  int value;
  
  if (v != NULL) {
    enum_value = expr_intval(v);
    DB1(10, "enum_value set to %d\n", enum_value);
    S_EINIT(ident) = SC_EINIT;
  }
  value = enum_value;
		
	
  S_CLASS(ident) = S_ENUMELEMENT;
  S_LEVEL(ident) = 0;
  S_FG_ACTIVE(ident) = S_ACTIVE;
  S_EVALUE(ident) = value;
  enum_value++;
  
  unlink_sym(ident, S_LEVEL(ident));
  
  return ident;
}


datatype_t *build_enum(char *tag,symboltable_t *enum_list) {
  datatype_t	*pdt = NULL;
  symboltable_t	*pst = NULL;
  symboltable_t	*psym = NULL;
  symboltable_t	*ident = NULL;
  symboltable_t   *pstptr;

  if (tag == NULL) {
    pdt = alloc_dt(DT_ENUM);
    D_STAG(pdt) = NULL;
    D_STRUCT(pdt) = enum_list;
    
    pstptr = enum_list;
    while (pstptr!=NULL) {
      S_DTYPE(pstptr) = pdt;
      pstptr = S_SIBLING(pstptr);
    }
    return(pdt);
  }
  
  pst = lu(tag);
  
  if (pst == NULL) {
    ident = LU_INS(tag);
    DB0(10, "pst == NULL\n");
    pdt = alloc_dt(DT_ENUM);
    D_STAG(pdt) = ident;
    D_STRUCT(pdt) = enum_list;
    S_CLASS(ident) = S_ENUMTAG;
    S_DTYPE(ident) = pdt;
    S_FG_ACTIVE(ident) = S_ACTIVE;
    
  } else {
    DB0(10, "pst != NULL\n");
    if ((pdt = S_DTYPE(pst)) != NULL) {
      psym = D_STRUCT(pdt);
    }
    if (psym == NULL) {
      DB0(10, "PSYM IS NULL\n");
      D_STAG(pdt) = ident;
      D_STRUCT(pdt) = enum_list;
    }
  }
  pstptr = enum_list;
  while (pstptr!=NULL) {
    S_DTYPE(pstptr) = pdt;
    pstptr = S_SIBLING(pstptr);
  }

  return pdt;
}


initial_t *build_init(expr_t *value,initial_t *list) {
  initial_t *new;

  new = alloc_initial();
  if (value != NULL) {
    IN_BRA(new) = FALSE;
    IN_VAL(new) = value;
    IN_REP(new) = 1;
    return(new);
  } else {
    IN_BRA(new) = TRUE;
    IN_LIS(new) = list;
    IN_REP(new) = 1;
    return(new);
  }
}


initial_t *build_initlist(initial_t *list,initial_t *new) {
  initial_t *temp;

  if (list == NULL) {
    return(new);
  } else {
    temp = list;
    while (S_NEXT(temp) != NULL) {
      temp = S_NEXT(temp);
    }
    S_NEXT(temp) = new;
    return(list);
  }
}


void enter_block(symboltable_t *par) {
  DB(20) {
    printf("enter_block(builds.c):parent=");
    if (par == NULL) {
      printf("NULL");
    } else {
      printf("%s", S_IDENT(par));
    }
    printf("\n");
  }
  current_level++;
  
  push(par);
}


void exit_block() {
  symboltable_t *temp;

  if (current_level == 0) {
    return;
  }
  DB0(20, "exit_block(builds.c):\n");
  temp = maplvl[ current_level ];
  while (temp != NULL) {
    deactivate(temp);
    temp = S_SIBPREV(temp);
  }
  maplvl[ current_level ] = NULL;
  pop();
  current_level--;
}


symboltable_t  *findstructmember(expr_t *pe,char *ident,int deref) {
  symboltable_t *pst, *pstL;
  datatype_t    *pdt;
  int           found=FALSE;
  
  while (!found) {  /* find a VARIABLE or BIDOT node */
    switch (T_TYPE(pe)) {
    case VARIABLE:
    case BIDOT:
      pst = T_IDENT(pe);
      found = TRUE;
      break;
    case ARRAY_REF:
      pe = T_OPLS(pe);
      break;
    case BIAT:
      pe = T_OPLS(pe);
      break;
    default:
      YY_FATAL_CONTX(yylineno, in_file, 
		    "unrecognized struct operation %d", T_TYPE(pe));
      return NULL;
    }
  }
  
  if (pst == NULL) {  /* this can happen if trying to bidot into a non- */
    return NULL;      /* existent record field */
  }
  if ((S_CLASS(pst) != S_VARIABLE) && (S_CLASS(pst) != S_COMPONENT) &&
      (S_CLASS(pst) != S_PARAMETER)) {
    YY_FATAL_CONTX(yylineno, in_file, 
		   "Invalid member reference %s", S_IDENT(pst));
    return NULL;
  }
  if ((pdt = S_DTYPE(pst)) == NULL) {
    INT_FATAL(NULL, "NULL pdt in findstructmember - %s",ident);
  }
  
  /* Find the DT_STRUCTURE */
  found = FALSE;
  /* trace down a datatype chain */
  while (!found) {
    switch (D_CLASS(pdt)) {
    case DT_ARRAY:
      pdt = D_ARR_TYPE(pdt);
      break;
    case DT_ENSEMBLE:  
      pdt = D_ENS_TYPE(pdt);
      break;
    case DT_STRUCTURE: 
      found = TRUE;
      break;
    case DT_COMPLEX:  /* remove these cases if we don't want the user to */
    case DT_DCOMPLEX: /* be able to access the fields by hand */
    case DT_QCOMPLEX: /* Also, would remove them from conditional below */
      found = TRUE;
      break;
    default:
      YY_FATAL_CONTX(yylineno, in_file, "Invalid member reference: %s",ident);
      return NULL;
    }
  }
  
  if (D_CLASS(pdt) != DT_STRUCTURE && D_CLASS(pdt) != DT_COMPLEX &&
      D_CLASS(pdt) != DT_DCOMPLEX && D_CLASS(pdt) != DT_QCOMPLEX) {
    YY_FATAL_CONTX(yylineno, in_file,
		   "Invalid member reference %s", S_IDENT(pst));
    return NULL;
  }
  
  /* requires a ptr to the dt_struct */
  if ((pstL = D_STRUCT(pdt)) == NULL) {
    INT_FATAL(NULL, "NULL list in findstructmember - %s",ident);
  }
  while (pstL != NULL) {
    DB1(25, "findstructmember checking %s\n", S_IDENT(pstL));
    if (strcmp(ident, S_IDENT(pstL)) == 0)
      return pstL;
    pstL = S_SIBLING(pstL);
  }
  YY_FATAL_CONTX(yylineno, in_file, "Invalid structure member %s", ident);
  return NULL;
}

