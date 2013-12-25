/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __RMSTACK_H_
#define __RMSTACK_H_

#include "parsetree.h"

void RMSInit(void);
void RMSFinalize(void);

void RMSPushScope(region_t *);
void RMSPopScope(region_t *);

expr_t *RMSCurrentRegion(void);
expr_t *RMSCurrentMask(int *);

expr_t *RMSCoveringRegion(void);  /* the reg covering the current one */
expr_t* RMSRegionAtDepth(int);    /* the covering region at the 
					     given depth 0 = current region,
					     1 = covering region, etc.*/

int RMSRegDimFloodable(int,int);
int RMSRegDimDegenerate(int,int);

void RMSLegalFlood(statement_t *,int);

int RMSLegalReduce(statement_t*,int);

int RMSClassifyPartialReduce(int);

dimtypeclass RMSCurrentDimType(int);

#endif
