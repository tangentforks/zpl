/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/callsite.h"
#include "../include/coverage.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"

#define CALLED_MASKED    0x01
#define CALLED_UNMASKED  0x02

#define NEVER_CALLED     0x00
#define MASKED_ALWAYS    0x01
#define MASKED_NEVER     0x02
#define MASKED_SOMETIMES 0x03

int RankOfCover(function_t* fn) {
  expr_t* reg;
  int numdims = 0;
  int newnumdims;
  callsite_t *callinfo;

  callinfo = T_CALLINFO(fn);
  if (callinfo != NULL) {
    reg = COV_REG(CALL_COVER(callinfo));
    if (reg != NULL) {
      numdims = D_REG_NUM(T_TYPEINFO(reg));
    }
    callinfo = CALL_NEXT(callinfo);
  }
  while (callinfo != NULL) {
    reg = COV_REG(CALL_COVER(callinfo));
    if (reg != NULL) {
      newnumdims = D_REG_NUM(T_TYPEINFO(reg));
      if (newnumdims != numdims) {
	INT_FATAL(NULL, "Function is covered by regions of differing ranks!");
      }
    }
    callinfo = CALL_NEXT(callinfo);
  }

  return numdims;
}

expr_t *BestCoveringMask(function_t *fn) {
  callsite_t *callinfo;
  expr_t *mask;
  expr_t *firstmask;
  int coverage;

  coverage = 0;
  firstmask = NULL;
  callinfo = T_CALLINFO(fn);
  while (callinfo != NULL) {
    mask = COV_MSK(CALL_COVER(callinfo));
    if (mask == NULL) {
      coverage |= CALLED_UNMASKED;
    } else {
      coverage |= CALLED_MASKED;
      if (firstmask == NULL) {
	firstmask = mask;
      }
    }
    callinfo = CALL_NEXT(callinfo);
  }
  
  switch (coverage) {
  case NEVER_CALLED:
    mask = pexpr_qmask[0];
    break;
  case MASKED_ALWAYS:
    mask = pexpr_qmask[0];
    break;
  case MASKED_NEVER:
    mask = NULL;
    break;
  case MASKED_SOMETIMES:
    mask = pexpr_qmask[0];
    break;
  default:
    INT_FATAL(NULL,"Unknown mask coverage type in build_mloop_wrapper");
    break;
  }
  return mask;
}


int NoStridedRegCover(function_t *fn) {
  callsite_t *callinfo;
  expr_t *reg;
  int nostrides=1;
  int foundregion=0;

  callinfo = T_CALLINFO(fn);
  while (callinfo != NULL) {
    reg = COV_REG(CALL_COVER(callinfo));
    if (reg != NULL) {
      foundregion=1;
      if (expr_is_strided_reg(reg,1)) {
	nostrides = 0;
	break;
      }
    }
    callinfo = CALL_NEXT(callinfo);
  }
  if (!foundregion) {
    nostrides = 0;
  }
  return nostrides;
}


int NoSparseRegCover(function_t *fn) {
  callsite_t *callinfo;
  expr_t *reg;
  int nosparse=1;
  int foundregion=0;

  callinfo = T_CALLINFO(fn);
  while (callinfo != NULL) {
    reg = COV_REG(CALL_COVER(callinfo));
    if (reg != NULL) {
      foundregion=1;
      if (expr_is_sparse_reg(reg)) {
	nosparse = 0;
	break;
      }
    }
    callinfo = CALL_NEXT(callinfo);
  }
  if (!foundregion) {
    nosparse = 0;
  }
  return nosparse;
}
