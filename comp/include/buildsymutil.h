/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __BUILD_SYM_UTIL_H_
#define __BUILD_SYM_UTIL_H_

#include "struct.h"
#include "symtab.h"

initial_t* build_init(expr_t*,initial_t*);
initial_t* build_initlist(initial_t*,initial_t*);
datatype_t* build_enum(char*,symboltable_t*);
symboltable_t* build_enumelem(symboltable_t*,expr_t*);
void enter_block(symboltable_t*);
void exit_block(void);
symboltable_t* findstructmember(expr_t*,char*,int);

#endif
