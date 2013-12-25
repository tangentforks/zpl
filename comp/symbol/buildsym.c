/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "../include/error.h"
#include "../include/const.h"
#include "../include/global.h"
#include "../include/cglobal.h"
#include "../include/scan.h"
#include "../include/struct.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/macros.h"
#include "../include/parsetree.h"
#include "../include/parser.h"
#include "../include/db.h"
#include "../include/buildsym.h"


symboltable_t *combine(pst, s)
symboltable_t *pst;
char *s;
{
	char *temp;

	if (pst == NULL) {
		temp = (char *)PMALLOC(strlen(s) + 3);
		temp[0] = '\"';
		temp[1] = '\0';
		strcat(temp, s);
		strcat(temp, "\"");
		pst = add_constant(temp); /* linc: call add_uniq_const()*/
					  /*       this doesn't call lu()*/
		S_CLASS(pst) = S_LITERAL;
		S_DTYPE(pst) = pdtSTRING;
		if (strlen(s) > 0) 
		  PFREE(s, strlen(s));
		return(pst);
	} else {
		temp = (char *)PMALLOC((strlen(s)+strlen(S_IDENT(pst))+1)*sizeof(char));
		strcpy(temp, S_IDENT(pst));
		temp[strlen(S_IDENT(pst))-1] = '\0';
		strcat(temp, s);
		strcat(temp, "\"");
		PFREE(s, strlen(s));
		PFREE(S_IDENT(pst), strlen(S_IDENT(pst)));
		S_IDENT(pst) = temp;
		return(pst);
	}
}


symboltable_t *build_int(i)
int i;
{
	symboltable_t *temp;

	temp = add_constant(buffer);
	if (S_CLASS(temp) == S_UNKNOWN) {
		S_CLASS(temp) = S_LITERAL;
	}
	S_IS_INT(temp) = 1;
	if (buffer[strlen(buffer)-1] == 'l' ||
		buffer[strlen(buffer)-1] == 'L')
		S_DTYPE(temp) = pdtLONG;
	else
		S_DTYPE(temp) = pdtINT;

	return temp;
}


symboltable_t *build_real(double r) {
	symboltable_t *temp;

	temp = add_constant(buffer);
	if (S_CLASS(temp) == S_UNKNOWN) {
		S_CLASS(temp) = S_LITERAL;
	}
	if (buffer[strlen(buffer)-1] == 'l' ||
	    buffer[strlen(buffer)-1] == 'L')
	    S_DTYPE(temp) = pdtDOUBLE;
	else
	    S_DTYPE(temp) = pdtFLOAT;

	return temp;
}


symboltable_t *build_char(char ch) {
	symboltable_t *temp;

	temp = add_constant(buffer);
	if (S_CLASS(temp) == S_UNKNOWN) {
		S_CLASS(temp) = S_LITERAL;
	}
	S_DTYPE(temp) = pdtCHAR;
	return(temp);
}


symboltable_t *check_var(char* ident) {
  symboltable_t *pst, *pstL;

  DB1(10, "enter check_var %s\n", ident);
  pst = lu(ident);
  if (yychar == -1) {
    DB0(70, "looking ahead\n");
    yychar = yylex();
  }
  if (yychar == TLP) {
    if (pst == NULL) {
      
      pst = alloc_st(S_SUBPROGRAM, ident);
      S_LEVEL(pst) = 0;
      S_FG_SHADOW(pst) = 1;
      
      pst = add_unresolved(pst);
      return pst;
    }
    S_CLASS(pst) = S_SUBPROGRAM;
    if (S_LEVEL(pst) != 0) {
      
      USR_FATAL(NULL, "ZPL functions may not be nested");
      pstL = maplvl[S_LEVEL(pst)];
      if (pstL == pst)
	maplvl[S_LEVEL(pst)] = S_SIBPREV(pst);
      else {
	while (pstL != NULL && pstL != pst)
	  pstL = S_SIBPREV(pstL);
	if (pstL == NULL) {
	  INT_FATAL(NULL, "BLECCH!-call Bruce");
	}
	if (S_SIBPREV(pst) != NULL)
	  S_SIBLING(S_SIBPREV(pst)) = S_SIBLING(pst);
	if (S_SIBLING(pst) != NULL)
	  S_SIBPREV(S_SIBLING(pst)) = S_SIBPREV(pst);
      }
      S_LEVEL(pst) = 0;
    }
  } else
    pst = lu_only(ident);
  
  return pst;
}
