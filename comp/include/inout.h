/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __INOUT_H_
#define __INOUT_H_

#include "const.h"
#include "struct.h"

/*** sungeun ***/
/*** direction and depth in each direction ***/
typedef struct dir_depend {
  genlist_t	*ident;
} dir_depend;

#define DEP_DIR_IDENT(x)	((x).ident)

/* routines */

set_t* getin(expr_t* expr);
set_t* getout(expr_t* expr);

set_t* prune_set(set_t*,expr_t*);

#endif
