/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ZPL_GLOBAL_H_
#define __ZPL_GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>

#define _MAXRANK  (6)

extern const int _MAXDIM;  /* maximum number of dims used by user vars */
extern int _MAXDIM_GRID;   /* maximum number of dims used by proc grid (set at
			      runtime;  set to always be >= _MAXDIM) */

#define _ZPL_NULL (NULL)

typedef int _vector[_MAXRANK];

#endif
