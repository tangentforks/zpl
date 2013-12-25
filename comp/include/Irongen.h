/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __IRONMAN_H_
#define __IRONMAN_H_

#include "../include/parsetree.h"

/* Irongen.c routines */

void StartIronmanCodegen(int);
void EndIronmanCodegen(FILE*,int);
void gen_iron_comm(FILE*,statement_t*);

#endif
