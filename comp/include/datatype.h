/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __UTIL_DATATYPE_H_
#define __UTIL_DATATYPE_H_

#include "symtab.h"


datatype_t* datatype_find_dtclass(datatype_t*,typeclass); /* look for DT_* */
datatype_t* datatype_find_ensemble(datatype_t*); /* contains an ensemble 
						     (NULL/pdt) */
datatype_t* datatype_find_region(datatype_t*);   /* ditto for region */
datatype_t* datatype_find_distribution(datatype_t*);
datatype_t* datatype_find_grid(datatype_t*);
datatype_t* datatype_find_direction(datatype_t*); /* same, but for direction */
datatype_t* datatype_find_indarr(datatype_t*); /* contains an ind. arr.
						     (NULL/pdt) */

int datatype_rank(datatype_t*);  /* tells a datatype's rank (0/rank) */

datatype_t* datatype_base(datatype_t*); /* return base of all ens/arr types */

int datatype_scalar(datatype_t*);  /* tells whether a datatype is a scalar */

int datatype_complex(datatype_t*); /* is datatype complex/dcomplex/complex */
int datatype_float(datatype_t*);   /* is datatype float/double/quad? */
int datatype_int(datatype_t*);      /* is datatype an integer type? */
int datatype_dyn_array(datatype_t*);  /* is this a dynamic indexed array? */
int datatype_is_dense_reg(datatype_t*); /* is this a dense region? */
int datatype_reg_inherits(datatype_t*); /* does this reg datatype inherit a dim? */
int datatype_reg_inherits_all(datatype_t*);  /* ... all dims? */
int datatype_reg_dynamicdim(datatype_t*); /* does it have any "?" dimensions? */

#endif
