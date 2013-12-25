/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __CONTRACTION_H_
#define __CONTRACTION_H_

#include "depgraph.h"
#include "struct.h"


/* contraction.c prototypes */

void hilow(dvlist_t,int,int*,int*);
int lsd(dvlist_t,distvect_t);
void set_mloop(module_t*);


/* dvlist.c prototypes */

dvlist_t dvlist_add(dvlist_t dvlist, distvect_t dv);
void dv_print(distvect_t dv);
void dvlist_print(dvlist_t dvlist);
int dv_zero(distvect_t);
dvlist_t dvlist_copy(dvlist_t dvlist);
dvlist_t dvlist_rm_dups(dvlist_t dvlist);

#endif /* __CONTRACTION_H_ */

