/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <string.h>
#include "../include/createvar.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#ifdef DEBUG
#define DBVAL 30
#endif


/******************************************************
create_named_var(datatype_t, function_t, char*)
Modified 2-16-94 by Ruth Anderson
******************************************************/
symboltable_t *create_named_var(datatype_t *pdt,function_t *pft,char *name) {
  symboltable_t *stp;

  if ((stp = lu(name)) != NULL) {
    USR_FATAL(NULL, "Variable %s already declared", name);
  }
  stp = alloc_loc_st(S_VARIABLE, name);
  S_DTYPE(stp) = pdt;
  if (pft) {
    S_PARENT(stp) = T_FCN(pft);
  } else {
    S_PARENT(stp) = NULL;
  }
  insert_pst(stp);		
  unlink_sym(stp, S_LEVEL(stp));
  return stp;
}


/******************************************************
create_named_local_var(datatype_t, function_t, char*)
Modified 2-15-94 by Ruth Anderson

Assumptions:
1) The function_t we are passed has a compound statement as its
T_STLS.
2) The decl list of this compound statement is where any new 
declarations should be attached to the function_t.
******************************************************/
symboltable_t *create_named_local_var(datatype_t *pdt,function_t *pft,
				      char *name) {
  symboltable_t *stp;

  stp = create_named_var(pdt, pft, name);

  USR_COND_FATAL((T_CMPD(T_STLS(pft))!=NULL), NULL, 
		 "Cannot create a variable with out a scope");

  S_SIBPREV(stp) = NULL;
  S_SIBLING(stp) = T_CMPD_DECL(T_STLS(pft));
  if (S_SIBLING(stp)) {
    S_SIBPREV(S_SIBLING(stp)) = stp;
  }
  T_CMPD_DECL(T_STLS(pft)) = stp;

  return stp;
}
