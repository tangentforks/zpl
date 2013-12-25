/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: dvlist.c - dist vect list implementation code
 * DATE: 25 September 1996
 * CREATOR: echris
 *
 * SUMMARY
 *   dvlist_add
 *   dvlist_test
 *   distvect_equal
 *   dvlist_free
 *   dvlist_copy
 *   dvlist_print
 *   dvlist_rm_dups
 */

#include <stdio.h>
#include "../include/struct.h"
#include "../include/const.h"
#include "../include/contraction.h"
#include "../include/db.h"
#include "../include/parsetree.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/generic_stack.h"
#include "../include/macros.h"
#include "../include/glist.h"
#include "../include/glistmac.h"
#include "../include/idfa_traverse.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/depgraph.h"


static int distvect_equal(distvect_t,distvect_t);


static int dvlist_test(dvlist_t dvlist, distvect_t dv)
{
  while(dvlist) {
    if (distvect_equal(DVLIST_DV(dvlist), dv))
      return (TRUE);
    else
      dvlist = DVLIST_NEXT(dvlist);
  }
  return (FALSE);
}


dvlist_t dvlist_add(dvlist_t dvlist, distvect_t dv)
{
  dvlist_t result;
  int i;

  if (dv == NULL) {
    result = dvlist;
  } else if (dvlist_test(dvlist, dv)) {   /* already in list */
    result = dvlist;
  } else {
    result = (dvlist_t) PMALLOC(DVLIST_NODE_SIZE);
    
    DVLIST_NEXT_LHS(result) = (glist)dvlist;
    for (i=0; i <MAXRANK+1; i++) {
      DVLIST_DV(result)[i] = (dv)[i];
    }
  }

  return (result);

}


static int distvect_equal(distvect_t dv1, distvect_t dv2)
{
  int i, rank, flag;
  
  rank = (dv1)[MAXRANK];
  if (rank != (dv2)[MAXRANK]) {
    return (FALSE);
  }

  flag = TRUE;
  for (i=0; i<rank; i++) {
    flag &= ((dv1)[i] == (dv2)[i]);
  }

  return (flag);
}


/* return true if dv is a zero dist vect */
int dv_zero(distvect_t dv) 
{
  int rank, i;
  
  rank = (dv)[MAXRANK];
  for(i=0; i<rank; i++) {
    if ((dv)[i] != 0)
      return (FALSE);
  }
  return (TRUE);
}


dvlist_t dvlist_copy(dvlist_t dvlist)
{
  dvlist_t newlist;

  newlist = NULL;

  while(dvlist) {
    newlist = dvlist_add(newlist, DVLIST_DV(dvlist));
    dvlist = DVLIST_NEXT(dvlist);
  }

  return (newlist);
}

void dv_print(distvect_t dv)
{
  int rank, i;
  
  rank = (dv)[MAXRANK];
  printf("%d(", rank);
  for(i=0; i<rank; i++) {
    printf("%d", (dv)[i]);
    if (i+1 < rank) 
      printf(",");
  }
  printf(")");
}


void dvlist_print(dvlist_t dvlist)
{
  while(dvlist) {
    dv_print(DVLIST_DV(dvlist));
    printf(", ");
    dvlist = DVLIST_NEXT(dvlist);
  }
  printf("\n");
}

dvlist_t dvlist_rm_dups(dvlist_t dvlist)
{
  dvlist_t tmp, tmp2, prev;

  if (dvlist == NULL) return (NULL);

  tmp = dvlist;

  while (tmp) {
    tmp2 = DVLIST_NEXT(tmp);
    prev = tmp;
    while (tmp2) {
      if (!distvect_equal(DVLIST_DV(tmp), DVLIST_DV(tmp2))) {
	prev = tmp2;
      } else {
	/* free node */
	DVLIST_NEXT_LHS(prev) = (glist) tmp2;
      }
      tmp2 = DVLIST_NEXT(tmp2);
    }

    tmp = DVLIST_NEXT(tmp);
  }

  return (dvlist);
}
