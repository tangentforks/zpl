/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __UTIL_DIMENSION_H_
#define __UTIL_DIMENSION_H_

#include "parsetree.h"

int dimension_const(dimension_t *);        /* is it constant?  (0/1) */
int dimension_rt_const(dimension_t *);     /* is it runtime constant? (0/1) */
int dimension_flat(dimension_t *);         /* is it flat? (0/1) */
int dimension_floodable(dimension_t *);    /* is it floodable? (0/1) */
int dimension_rgrid(dimension_t *);
int dimension_dynamic(dimension_t*);        /* is dimension dynamic? */
int dimension_inherit(dimension_t *);      /* is it blank? (0/1) */

int dimlist_const(dimension_t *);          /* are they all constant? (0/1) */
int dimlist_rt_const(dimension_t *);       /* are they all rt constant? (0/1) */
int dimlist_inherits(dimension_t*);        /* does it inherit in any dim? */
int dimlist_is_qreg(dimension_t*);         /* are all dims inheriting? */

#endif
