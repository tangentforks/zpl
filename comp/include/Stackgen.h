/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __STACKGEN_H_
#define __STACKGEN_H_

#include <stdio.h>
#include "parsetree.h"

void genStackReg(FILE*);
void push_region_scope(FILE*, region_t*);
void pop_region_scope(FILE*, region_t*);
void OpenFloodReduceDynamicRegion(FILE*, expr_t*);
void CloseFloodReduceDynamicRegion(FILE*, expr_t*);

/*** make sure this access macro is consistent to the one used in Cgen.h ***/
#define RMS_NAME "_RMStack"
#define RMS (priv_access ? "_PRIV_ACCESS(_RMStack)" : RMS_NAME)
/* seems like this could be _region_pnc, except that the comm info for 
   dynamic regions can change after the region has been put onto the
   stack */
#define RMS_REG_TYPE "_region_nc"
#define RMS_REG "reg"
#define RMS_MASK_TYPE "_array_pnc"
#define RMS_MASK "mask"
#define RMS_WITH_TYPE "int"
#define RMS_WITH "withflag"

#endif
