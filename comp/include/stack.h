/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _STACK_H_
#define _STACK_H_

#include "struct.h"

int push(symboltable_t*);
int pop(void);
symboltable_t* top(void);

#endif
