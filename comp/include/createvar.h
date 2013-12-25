/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __CREATESYM_H_
#define __CREATESYM_H_

#include "struct.h"

symboltable_t *create_named_var(datatype_t *, function_t *, char *);
symboltable_t *create_named_local_var(datatype_t *, function_t *, char *);

#endif

