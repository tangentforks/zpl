/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _CMPLX_ENS_H_
#define _CMPLX_ENS_H_

#include "../include/struct.h"

expr_t* SearchForFunctionCall(expr_t*);
void TestExprForComplexEnsParams(FILE*,expr_t *);
void FinishExprComplexParams(FILE*,expr_t *);
void TestIOStmtForComplexEnsParams(FILE*,statement_t *);
void FinishIOStmt(FILE*,statement_t *);
expr_t* TestMaskStmtForComplexEnsMask(FILE*,statement_t*);
void FinishMaskStmt(FILE*,statement_t*,expr_t*);
void TestFloodForComplexEns(FILE*,expr_t*);
void FinishFloodForComplexEns(FILE*,expr_t*);
void TestWRForComplexEns(FILE*,wrap_t*);
void FinishWRForComplexEns(FILE*,wrap_t *);

#endif
