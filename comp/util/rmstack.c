/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/Redgen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/dimension.h"
#include "../include/dtype.h"
#include "../include/expr.h"
#include "../include/error.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/struct.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"


typedef struct _dimval {
  int known;
  int val;
  expr_t *expr;
} dimval;

typedef struct _diminfo {
  dimtypeclass type;
  dimval lo;
  dimval hi;
} diminfo;

typedef diminfo regdiminfo[MAXRANK];

typedef struct _dimnode {
  diminfo data;
  struct _dimnode *next;
} dimnode;

typedef struct _scopeinfo {
  expr_t* reg;
  int with;
  expr_t* mask;
} scopeinfo;

typedef struct _rmsnode {
  scopeinfo data;
  struct _rmsnode *next;
} rmsnode;


static rmsnode *CurrentRegion;
static rmsnode *FreeList=NULL;
static dimnode *CurrentDim[MAXRANK+1];
static dimnode *FreeDimList=NULL;
static int stackon=0;


static rmsnode *NewNode(scopeinfo *data,rmsnode *next) {
  rmsnode *newnode;

  if (FreeList==NULL) {
    newnode = (rmsnode *)malloc(sizeof(rmsnode));
  } else {
    newnode = FreeList;
    FreeList = newnode->next;
  }
  newnode->data = *data;
  newnode->next = next;

  return newnode;
}


static rmsnode *FreeNode(rmsnode *oldnode) {
  rmsnode *nextnode;
  
  nextnode = oldnode->next;
  oldnode->next = FreeList;
  FreeList = oldnode;
  
  return nextnode;
}


static dimnode *NewDimNode(diminfo *data,dimnode *next) {
  dimnode *newnode;

  if (FreeDimList == NULL) {
    newnode = (dimnode *)malloc(sizeof(dimnode));
  } else {
    newnode = FreeDimList; 
    FreeDimList = newnode->next;
  }
  newnode->data = *data;
  newnode->next = next;

  return newnode;
}


static dimnode *FreeDimNode(dimnode *oldnode) {
  dimnode *nextnode;

  nextnode = oldnode->next;
  oldnode->next = FreeDimList;
  FreeDimList = oldnode;

  return nextnode;
}


static void DetermineScopeInfo(region_t *scope,scopeinfo *info) {
  expr_t *reg;
  int with;
  expr_t *mask;
  rmsnode *currentnode;

  reg = T_REGION_SYM(scope);
  mask = T_MASK_EXPR(scope);
  with = T_MASK_BIT(scope);
  currentnode = CurrentRegion;
  if (expr_is_qreg(reg)) {
    if (currentnode != NULL) {
      reg = currentnode->data.reg;
    }
  }
  info->reg = reg;
  if (mask == NULL) {
    info->mask = NULL;
  } else {
    if (expr_is_qmask(mask)) {
      if (currentnode != NULL) {
	mask = currentnode->data.mask;
      }
    }
    info->mask = mask;
    info->with = with;
  }
}


static void InheritType(diminfo *diminfo,int dim) {
  dimnode *currentnode;

  currentnode = CurrentDim[dim];
  if (currentnode == NULL) {
    diminfo->type = DIM_INHERIT;
  } else {
    *diminfo = currentnode->data;
  }
}


static void ZeroVal(dimval *val) {
  val->known = 0;
  val->val = 0;
  val->expr = 0;
}



static void SetVal(dimval *val,expr_t *expr) {
  if (expr_computable_const(expr)) {
    val->known = 1;
    val->val = expr_intval(expr);
    val->expr = NULL;
  } else {
    val->known = 0;
    val->val = 0;
    val->expr = expr;
  }
}


static void AdjustVal(dimval *val,int offset) {
  val->val += offset;
}


static void DetermineRegDimInfo(expr_t *reg,int numdims,
				regdiminfo reginfo) {
  int i;
  expr_t *dir;
  long dirval;
  long dirmag;
  int dirsign;
  int unknown;
  int success;
  datatype_t* regdt;

  switch (T_TYPE(reg)) {
  case VARIABLE:
  case CONSTANT:
    if (expr_is_qreg(reg)) {
      for (i=0;i<numdims;i++) {
	InheritType(&(reginfo[i]),i);
      }
      return;
    } else {
      for (i=0; i<numdims; i++) {
	reginfo[i].type = D_REG_DIM_TYPE(S_DTYPE(T_IDENT(reg)), i);
	switch (reginfo[i].type) {
	case DIM_FLOOD:
	case DIM_GRID:
	case DIM_SPARSE:
	  ZeroVal(&reginfo[i].lo);
	  ZeroVal(&reginfo[i].hi);
	  break;
	case DIM_FLAT:
	case DIM_RANGE:
	  SetVal(&reginfo[i].lo, NULL);
	  SetVal(&reginfo[i].hi, NULL);
	  break;
	case DIM_INHERIT:
	  InheritType(&(reginfo[i]),i);
	  break;
	case DIM_DYNAMIC:
	  INT_FATAL(NULL, "Not sure what to do here");
	  break;
	}
      }
    }
    break;
  case BIWITH:
    DetermineRegDimInfo(T_OPLS(reg), numdims, reginfo);
    break;
  case ARRAY_REF:
    for (i=0; i<numdims; i++) {
      reginfo[i].type = D_REG_DIM_TYPE(T_TYPEINFO(reg), i);
      switch (reginfo[i].type) {
      case DIM_FLOOD:
      case DIM_GRID:
      case DIM_SPARSE:
	ZeroVal(&reginfo[i].lo);
	ZeroVal(&reginfo[i].hi);
	break;
      case DIM_FLAT:
      case DIM_RANGE:
	SetVal(&reginfo[i].lo, NULL);
	SetVal(&reginfo[i].hi, NULL);
	break;
      case DIM_INHERIT:
	InheritType(&(reginfo[i]),i);
	break;
      case DIM_DYNAMIC:
	INT_FATAL(NULL, "Not sure what to do here");
	break;
      }
    }
    break;
  default:
    switch (T_SUBTYPE(reg)) {
    case SIMPLE_REGION:
    case DYNAMIC_REGION:
    case BY_REGION:
    case SPARSE_REGION:
      regdt = T_TYPEINFO(reg);
      if (regdt == NULL) {
	for (i=0;i<numdims;i++) {
	  InheritType(&(reginfo[i]),i);
	}
      } else {
	for (i=0;i<numdims;i++) {
	  switch (D_REG_DIM_TYPE(regdt, i)) {
	  case DIM_INHERIT:
	    InheritType(&(reginfo[i]),i);
	    break;
	  case DIM_FLOOD:
	    reginfo[i].type = DIM_FLOOD;
	    ZeroVal(&reginfo[i].lo);
	    ZeroVal(&reginfo[i].hi);
	    break;
	  case DIM_GRID:
	    reginfo[i].type = DIM_GRID;
	    ZeroVal(&reginfo[i].lo);
	    ZeroVal(&reginfo[i].hi);
	    break;
	  case DIM_FLAT:
	    reginfo[i].type = DIM_FLAT;
	    SetVal(&reginfo[i].lo, NULL);
	    SetVal(&reginfo[i].hi, NULL);
	    break;
	  case DIM_RANGE:
	    reginfo[i].type = DIM_RANGE;
	    SetVal(&reginfo[i].lo, NULL);
	    SetVal(&reginfo[i].hi, NULL);
	    break;
	  case DIM_DYNAMIC:
	    reginfo[i].type = DIM_DYNAMIC;
	    ZeroVal(&reginfo[i].lo);
	    ZeroVal(&reginfo[i].hi);
	  case DIM_SPARSE:
	    reginfo[i].type = DIM_SPARSE;
	    ZeroVal(&reginfo[i].lo);
	    ZeroVal(&reginfo[i].hi);
	  }
	}
      }
      break;
    case OF_REGION:
    case IN_REGION:
    case AT_REGION:
      dir = T_NEXT(T_OPLS(reg));
      DetermineRegDimInfo(T_OPLS(reg),numdims,reginfo);
      for (i=0;i<numdims;i++) {
	unknown = 0;
	if (reginfo[i].type == DIM_FLOOD || reginfo[i].type == DIM_GRID) {
	  dirval = 0;
	  dirmag = 0;
	  dirsign = 0;
	} else {
	  dirval = dir_comp_to_int(dir, i, &success);
	  if (success) {
	    dirmag = dirval;
	    if (dirval > 0) {
	      dirsign = 1;
	    } else if (dirval == 0) {
	      dirsign = 0;
	    } else {
	      dirsign = -1;
	      dirmag = -dirmag;
	    }
	  }
	  else {
	    unknown = 1;
	  }
	}
	if (unknown) {
	  reginfo[i].lo.known = 0;
	  reginfo[i].hi.known = 0;
	  reginfo[i].type = DIM_RANGE;
	}
	else if (dirmag == 0) {
	  /* this dimension stays the same and is already set */
	} else {
	  switch (T_SUBTYPE(reg)) {
	  case OF_REGION:
	  case IN_REGION:
	    if (dirmag > 1) {
	      reginfo[i].type = DIM_RANGE;
	    } else {
	      reginfo[i].type = DIM_FLAT;
	    }
	    switch (T_SUBTYPE(reg)) {
	    case OF_REGION:
	      if (dirsign > 0) {
		reginfo[i].lo = reginfo[i].hi;
		AdjustVal(&(reginfo[i].lo),1);
		AdjustVal(&(reginfo[i].hi),dirval-1);
	      } else {
		reginfo[i].hi = reginfo[i].lo;
		AdjustVal(&(reginfo[i].hi),-1);
		AdjustVal(&(reginfo[i].lo),dirval+1);
	      }
	      break;
	    case IN_REGION:
	      if (dirsign > 0) {
		reginfo[i].lo = reginfo[i].hi;
		AdjustVal(&(reginfo[i].lo),-(dirmag-1));
	      } else {
		reginfo[i].hi = reginfo[i].lo;
		AdjustVal(&(reginfo[i].hi),dirmag-1);
	      }
	      break;
	    default:
	      break;
	    }
	    break;
	  case AT_REGION:
	    /* type stays same for at regions */
	    AdjustVal(&(reginfo[i].lo),dirval);
	    AdjustVal(&(reginfo[i].hi),dirval);
	    break;
	  default:
	    break;
	  }
	}
      }
      break;
    default:
      INT_FATAL(NULL,"Unexpected region class in DetermineRegDimInfo()\n");
    }
  }
}


static void PushScope(region_t *scope) {
  scopeinfo info;

  DetermineScopeInfo(scope,&info);
  CurrentRegion = NewNode(&info,CurrentRegion);
}


static void PushRegionDims(expr_t *reg,int numdims) {
  regdiminfo reginfo;
  int i;

  DetermineRegDimInfo(reg,numdims,reginfo);
  for (i=0;i<numdims;i++) {
    CurrentDim[i] = NewDimNode(&(reginfo[i]),CurrentDim[i]);
  }
}


static void PopRegionDims(int numdims) {
  int i;

  for (i=0;i<numdims;i++) {
    CurrentDim[i] = FreeDimNode(CurrentDim[i]);
  }
}


void RMSInit() {
  int j;
  
  CurrentRegion = NULL;
  for (j=1;j<MAXRANK;j++) {
    CurrentDim[j] = NULL;
  }
  stackon++;
}


void RMSFinalize() {
  int j;
  
  if (CurrentRegion != NULL) {
    INT_FATAL(NULL,"Error in RMSFinalize()\n");
  }
  for (j=1;j<MAXRANK;j++) {
    if (CurrentDim[j] != NULL) {
      INT_FATAL(NULL,"Error in RMSFinalize()\n");
    }
  }
  stackon--;
}


void RMSPushScope(region_t *scope) {
  int numdims;
  expr_t *reg;

  if (stackon) {
    reg = T_REGION_SYM(scope);
    numdims = D_REG_NUM(T_TYPEINFO(reg));
    PushScope(scope);

    PushRegionDims(reg,numdims);
  }
}


void RMSPopScope(region_t *scope) {
  int numdims;

  if (stackon) {
    numdims = D_REG_NUM(T_TYPEINFO(T_REGION_SYM(scope)));
    CurrentRegion = FreeNode(CurrentRegion);

    PopRegionDims(numdims);
  }
}


expr_t *RMSCurrentRegion(void) {
  rmsnode *tos;

  if (stackon) {
    tos = CurrentRegion;
    if (tos == NULL) {
      return buildqregexpr(0);
    } else {
      return tos->data.reg;
    }
  } else {
    return NULL;
  }
}

 
expr_t *RMSCurrentMask(int *with) {
  rmsnode *tos;

  if (stackon) {
    tos = CurrentRegion;
    if (tos == NULL) {
      return pexpr_qmask[0];
    } else {
      *with = tos->data.with;
      return tos->data.mask;
    }
  } else {
    return NULL;
  }
}


expr_t *RMSCoveringRegion(void) {
  rmsnode *tos;

  if (stackon) {
    tos = CurrentRegion;
    if (tos == NULL) {
      return buildqregexpr(0);
    }
    tos = tos->next;
    if (tos == NULL) {
      return buildqregexpr(0);
    } else {
      return tos->data.reg;
    }
  } else {
    return NULL;
  }
}


expr_t* RMSRegionAtDepth(int depth) {
  rmsnode* tos;
  int i;

  if (stackon) {
    tos = CurrentRegion;
    for (i=0;i<depth;i++) {
      if (tos == NULL) {
	return NULL;
      }
      tos = tos->next;
    }
    if (tos == NULL) {
      return buildqregexpr(0);
    } else {
      return tos->data.reg;
    }
  } else {
    return NULL;
  }
}


int RMSRegDimFloodable(int dim,int unknown) {
  dimnode *regdim;
  int retval;

  regdim = CurrentDim[dim];
  if (regdim) {
    retval = (regdim->data.type == DIM_FLOOD);
  } else {
    retval = unknown;
  }
  return retval;
}


int RMSRegDimDegenerate(int dim,int unknown) {
  dimnode* regdim;

  regdim = CurrentDim[dim];
  if (regdim) {
    return (regdim->data.type == DIM_FLAT || 
	    regdim->data.type == DIM_FLOOD ||
	    regdim->data.type == DIM_GRID);
  } else {
    return unknown;
  }
}


static int dimval_equal(dimval *val1,dimval *val2) {
  if (val1->known) {
    if (val2->known) {
      return (val1->val == val2->val);
    } else {
      return 0;
    }
  } else {
    if (val1->val != val2->val) {
      return 0;
    } else {
      return expr_equal(val1->expr,val2->expr);
    }
  }
}


void RMSLegalFlood(statement_t *stmt,int numdims) {
  int i;
  dimnode *srcnode;
  dimnode *dstnode;
  diminfo *srcregdim;
  diminfo *dstregdim;
  int error;

  for (i=0;i<numdims;i++) {
    error = 0;
    srcnode = CurrentDim[i];
    if (srcnode) {
      srcregdim = &(srcnode->data);
      dstnode = srcnode->next;
      if (dstnode) {
	dstregdim = &(dstnode->data);
      } else {
	return;
      }
    } else {
      INT_FATAL(NULL,"No source region in RMSLegalFlood()\n");
    }
    switch (srcregdim->type) {
    case DIM_FLOOD:
      if (dstregdim->type != DIM_FLOOD) {
	error = 1;
      }
      break;
    case DIM_GRID:
      if (dstregdim->type != DIM_GRID) {
	error = 1;
      }
      break;
    case DIM_FLAT:
      break;
    case DIM_RANGE:
      if (dstregdim->type == DIM_FLOOD || dstregdim->type == DIM_GRID ||
	  dstregdim->type == DIM_FLAT) {
	error = 1;
      } else {
	if (!dimval_equal(&(srcregdim->lo),&(dstregdim->lo)) ||
	    !dimval_equal(&(srcregdim->hi),&(dstregdim->hi))) {
	  error = 1;
	}
      }
      break;
    default:
      INT_FATAL(stmt,"Unexpected dimension type in RMSLegalFlood()");
    }
    if (error) {
      USR_FATAL_CONT(stmt,"Source/Destination floodability mismatch (dimension "
		     "%d)",i+1);
    }
  }
}


/*
    <<+ | 1..n 2..n  1  2  *
--------+-------------------
   1..n |  OK   NO   NO NO NO
   1    |  OK   OK   OK OK OK
   *    |  OK   OK   NO NO OK
*/

int RMSLegalReduce(statement_t *stmt,int numdims) {
  int i;
  dimnode *srcnode;
  dimnode *dstnode;
  diminfo *srcregdim;
  diminfo *dstregdim;
  int error;
  int reduce;

  reduce=0;
  for (i=0;i<numdims;i++) {
    error = 0;
    srcnode = CurrentDim[i];
    if (srcnode) {
      srcregdim = &(srcnode->data);
      dstnode = srcnode->next;
      if (dstnode) {
	dstregdim = &(dstnode->data);
      } else {
	return 1;
      }
    } else {
      INT_FATAL(NULL,"No source region in RMSLegalReduce()\n");
    }
    switch (srcregdim->type) {
    case DIM_RANGE:
      if (dstregdim->type == DIM_RANGE) {
	if (!dimval_equal(&(srcregdim->lo),&(dstregdim->lo)) ||
	    !dimval_equal(&(srcregdim->hi),&(dstregdim->hi))) {
	  error = 1;
	}
      } else {
	reduce = 1;
      }
      break;
    case DIM_FLAT:
      if (dstregdim->type == DIM_FLOOD || dstregdim->type == DIM_RANGE) {
	error = 1;
      }
      break;
    case DIM_FLOOD:
      if (dstregdim->type == DIM_RANGE) {
	error = 1;
      }
      break;
    case DIM_GRID:
      if (dstregdim->type == DIM_RANGE) {
	error = 1;
      } else {
	reduce = 1;
      }
      break;
    case DIM_SPARSE:
      if (dstregdim->type == DIM_RANGE || dstregdim->type == DIM_SPARSE) {
      } else {
	reduce = 1;
      }
      break;
    default:
      INT_FATAL(stmt,"Unexpected dimension type in RMSLegalReduce()");
    }
    if (error) {
      USR_FATAL_CONT(stmt,"Source/Destination reduction mismatch (dimension "
		     "%d)",i+1);
    }
  }
  if (!reduce) {
    error = 1;
    USR_FATAL_CONT(stmt,"Reduction doesn't reduce any dimensions");
  }
  return (error == 0);
}


int RMSClassifyPartialReduce(int numdims) {
  int retval = 0;
  int dimbit;
  int error = 0;
  int i;
  dimnode *srcnode;
  dimnode *dstnode;
  diminfo *srcregdim;
  diminfo *dstregdim;

  dimbit = 1;
  for (i=0; i<numdims; i++) {
    srcnode = CurrentDim[i];
    if (srcnode) {
      srcregdim = &(srcnode->data);
      dstnode = srcnode->next;
      if (dstnode) {
	dstregdim = &(dstnode->data);
      } else {
	return -1;
      }
    } else {
      INT_FATAL(NULL,"No source region in RMSClassifyPartialReduce()\n");
    }
    /* for each case, determine whether or not this implies that the 
       dimension is being reduced (do nothing) or not (|= dimbit).  The
       result will tell which dimensions should be inherited from the
       source region (1's) and which should be rgrid (0's) */
    switch (dstregdim->type) {
    case DIM_FLOOD:
      switch (srcregdim->type) {
      case DIM_FLOOD:
	retval |= dimbit;
	break;
      case DIM_FLAT:
	break;
      case DIM_RANGE:
	break;
      case DIM_GRID:
	break;
      case DIM_SPARSE:
	break;
      default:
	error=1;
      }
      break;

    case DIM_GRID:
      switch (srcregdim->type) {
      case DIM_FLOOD:
	retval |= dimbit;
	break;
      case DIM_FLAT:
	retval |= dimbit;
	break;
      case DIM_RANGE:
	break;
      case DIM_GRID:
	retval |= dimbit;
	break;
      case DIM_SPARSE:
	break;
      default:
	error=2;
      }
      break;

    case DIM_FLAT:
      switch (srcregdim->type) {
      case DIM_FLOOD:
	retval |= dimbit;
	break;
      case DIM_FLAT:
	retval = -1;
	break;
      case DIM_RANGE:
	break;
      case DIM_GRID:
	break;
      case DIM_SPARSE:
	break;
      default:
	error=3;
      }
      break;

    case DIM_SPARSE:
    case DIM_RANGE:
      switch (srcregdim->type) {
      case DIM_FLOOD:
	retval |= dimbit;
	break;
      case DIM_FLAT:
	retval |= dimbit;
	break;
      case DIM_RANGE:
	retval |= dimbit;
	break;
      case DIM_GRID:
	retval |= dimbit;
	break;
      case DIM_SPARSE:
	retval |= dimbit;
	break;
      default:
	error=4;
      }
      break;

    default:
      error=5;
    }
    if (error) {
      return -1;
    }
    dimbit = dimbit << 1;
  }

  return retval;
}


dimtypeclass RMSCurrentDimType(int dim) {
  dimnode* regdim;

  regdim = CurrentDim[dim];

  if (regdim) {
    return regdim->data.type;
  } else {
    return DIM_INHERIT;
  }
}
