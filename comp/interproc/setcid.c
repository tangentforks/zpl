/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../include/Cgen.h"
#include "../include/error.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/setcid.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symmac.h"
#include "../include/tasklist.h"
#include "../include/traverse.h"
#include "../include/treemac.h"


static int *commIDs=NULL;
static int numallocated = 0;
static int maxcommid=0;
static int startingpoint = 0;


int NumStaticCIDs(void) {
  return maxcommid+1;
}


static int GetNewCommID(int origcid) {
  int i;
  int freecid = -1;
  int newnumallocated;

  for (i=startingpoint;i<numallocated;i++) {
    if (commIDs[i] == 0) {
      freecid = i;
      commIDs[i] = origcid;
      break;
    }
  }

  if (freecid == -1) {
    newnumallocated = (numallocated*2)+1;
    if (commIDs != NULL) {
      commIDs = (int *)realloc(commIDs,newnumallocated * sizeof(int));
    } else {
      commIDs = (int *)PMALLOC(newnumallocated * sizeof(int));
    }
    INT_COND_FATAL((commIDs!=NULL), NULL, "Realloc failed in GetNewCommID() ",
		   "request: %d errno: %d", (newnumallocated*sizeof(int)),
		   errno);
   
    for (i=numallocated;i<newnumallocated;i++) {
      commIDs[i] = 0;
    }
    numallocated=newnumallocated;
    freecid = GetNewCommID(origcid);
  }

  if (freecid > maxcommid) {
    maxcommid = freecid;
  }

  return freecid;
}


static int GetExistingCommID(int origcid) {
  int i;
  int ironcid = -1;
  
  for (i=startingpoint;i<numallocated;i++) {
    if (commIDs[i] == origcid) {
      ironcid = i;
      break;
    }
  }

  if (ironcid == -1) {
    INT_FATAL(NULL, "Error in GetExistingCommID()");
  }

  return ironcid;
}


static void FreeCommID(int origcid) {
  int i;

  for (i=0;i<numallocated;i++) {
    if (commIDs[i] == origcid) {
      commIDs[i] = 0;
      break;
    }
  }
}


static int FindTopCommID(void) {
  int i;

  for (i=numallocated-1;i>=0;i--) {
    if (commIDs[i] != 0) {
      break;
    }
  }
  i++;

  return i;
}


static void AnnotateFunction(symboltable_t *pst,int commid) {
  int prevlow;

  if (commid > 64) {
    INT_FATALX(S_LINENO(pst), S_FILENAME(pst),
	       "Infinite Recursion Suspected in AnnotateFunction()");
  }
  prevlow = T_LO_COMMID(S_FUN_BODY(pst));
  if (commid > prevlow) {
    T_LO_COMMID(S_FUN_BODY(pst)) = commid;
    AddTask(S_FUN_BODY(pst));
  }
}


static void AnnotateScanReds(statement_t *stmt,expr_t *expr) {
  switch (T_TYPE(expr)) {
  case VARIABLE:
  case CONSTANT:
  case SIZE:
  case ARRAY_REF:
  case BIDOT:
  case BIAT:
  case FUNCTION:
    break;
  case REDUCE:
  case SCAN:
  case FLOOD:
  case PERMUTE:
    T_IRON_COMMID(stmt) = GetNewCommID(-1);
    FreeCommID(-1);
    break;
  default:
    if (T_IS_UNARY(T_TYPE(expr))) {
      AnnotateScanReds(stmt,T_OPLS(expr));
    } else if (T_IS_BINARY(T_TYPE(expr))) {
      AnnotateScanReds(stmt,left_expr(expr));
      AnnotateScanReds(stmt,right_expr(expr));
    } else {
      INT_FATAL(T_STMT(expr),
		"Bad exprtype (%d) in AnnotateScanReds()",T_TYPE(expr));
    }
  }
}


static void AnnotateFunctionCalls(expr_t *expr,int commid) {
  switch (T_TYPE(expr)) {
  case VARIABLE:
  case CONSTANT:
  case SIZE:
  case ARRAY_REF:
  case BIDOT:
  case BIAT:
  case REDUCE:
  case SCAN:
  case FLOOD:
  case PERMUTE:
    break;
  case FUNCTION:
    AnnotateFunction(T_IDENT(T_OPLS(expr)),commid);
    break;
  default:
    if (T_IS_UNARY(T_TYPE(expr))) {
      AnnotateFunctionCalls(T_OPLS(expr),commid);
    } else if (T_IS_BINARY(T_TYPE(expr))) {
      AnnotateFunctionCalls(left_expr(expr),commid);
      AnnotateFunctionCalls(right_expr(expr),commid);
    } else {
      INT_FATAL(T_STMT(expr),
		"Bad exprtype (%d) in AnnotateFunctionCalls()",T_TYPE(expr));
    }
  }
}


static void SetCommIDInStmt(statement_t *stmt) {
  switch (T_TYPE(stmt)) {
  case S_MLOOP:
    /* deal with mscan comm */
    traverse_stmtls_g(T_MLOOP_PREPRE(T_MLOOP(stmt),0),SetCommIDInStmt,
		      NULL,NULL,NULL);
    traverse_stmtls_g(T_MLOOP_POSTPOST(T_MLOOP(stmt),0),SetCommIDInStmt,
		      NULL,NULL,NULL);

    break;
  case S_NULL:
  case S_EXIT:
  case S_COMPOUND:
  case S_HALT:
  case S_CONTINUE:
  case S_IF:
  case S_LOOP:
  case S_RETURN:
  case S_IO:
  case S_REGION_SCOPE:
  case S_NLOOP:
  case S_MSCAN:
  case S_ZPROF:
    break;
  case S_WRAP:
    T_IRON_COMMID(stmt) = GetNewCommID(-1);
    FreeCommID(-1);
    break;
  case S_REFLECT:
    T_IRON_COMMID(stmt) = GetNewCommID(-1);
    FreeCommID(-1);
    break;
  case S_COMM:
    switch (T_COMM_TYPE(stmt)) {
    case C_NEW:
    case C_WRAP_PRERECV:
      T_IRON_COMMID(stmt) = GetNewCommID(T_COMM_ID(stmt));
      break;
    default:
      T_IRON_COMMID(stmt) = GetExistingCommID(T_COMM_ID(stmt));
      if (T_COMM_TYPE(stmt) == C_OLD) {
	FreeCommID(T_COMM_ID(stmt));
      }
      break;
    }
    break;
  case S_EXPR:
    AnnotateScanReds(stmt,T_EXPR(stmt));
    AnnotateFunctionCalls(T_EXPR(stmt),FindTopCommID());
    break;
  default:
    INT_FATAL(stmt,
	      "Bad statement type (%d) in SetCommIDInStmt()",T_TYPE(stmt));
    break;
  }
}


static void HandleFunction(function_t *fnptr) {
  startingpoint = T_LO_COMMID(fnptr);
  traverse_stmtls_g(T_STLS(fnptr),SetCommIDInStmt,NULL,NULL,NULL);
}


static void ProcessTasks(void) {
  function_t *nexttask;

  nexttask = GetTask();

  while (nexttask != NULL) {
    HandleFunction(nexttask);
    nexttask = GetTask();
  }
}


static void SetCommIDInModule(module_t *mod) {
  function_t *fnptr;

  fnptr = T_FCNLS(mod);

  while (fnptr != NULL) {
    if (!strcmp(entry_name,S_IDENT(T_FCN(fnptr)))) {
      break;
    }

    fnptr = T_NEXT(fnptr);
  }

  T_LO_COMMID(fnptr) = 0;
  AddTask(fnptr);
  ProcessTasks();
}


static void SetIronCommID(module_t *mod) {
  traverse_modules(mod, TRUE, SetCommIDInModule, NULL);
}


int call_setcid(module_t *mod,char *s) {
  SetIronCommID(mod);

  DBS1(2, "Used %d comm IDs\n",maxcommid);

  return 0;
}




