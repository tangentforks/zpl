/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _SLIST_H_
#define _SLIST_H_

#include "glist.h"

typedef symboltable_t *regcover[MAXRANK+1];

/* an slist is a region cover list */
typedef struct _rclist {
  gnode_t basic;
  regcover cover;
} rclist;

#define SLIST_DATA(rcl) (GLIST_DATA(&((rcl)->basic)))
#define SLIST_NEXT(rcl) ((slist)(GLIST_NEXT(&((rcl)->basic))))
#define SLIST_PST(rcl)  ((rcl)->cover)

#endif
