/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __DTYPE_H_
#define __DTYPE_H_

#include "struct.h"
#include "symtab.h"

datatype_t *alloc_dt(typeclass);
void initialize_datatypes(void);
datatype_t *copy_dt(datatype_t *);

symboltable_t *makepst(char *);

datatype_t* init_direction_type(signclass);
datatype_t* compose_direction_type(datatype_t*, datatype_t*);
datatype_t* compose_region_type(dimtypeclass, datatype_t*);
datatype_t* init_distribution_type(distributionclass);
datatype_t* compose_distribution_type(datatype_t*, datatype_t*);
datatype_t* init_grid_type(dimtypeclass);
datatype_t* compose_grid_type(datatype_t*, datatype_t*);

#endif
