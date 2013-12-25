/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* FILE: glist.c - some generic list routines
 * DATE: 27 June 1994
 * DESCRIPTION: These routines were written for use in ccodegen/Cgen.c
 *              in echris' bumper/walker ensemble access code.
 *              They are fairly general, but some nice routines are
 *              missing, for echris did not need them in Cgen.c.
 *
 * DATE: 26 September 1994
 *              sungeun added glist_copy() for use in a2nloops.c
 *              sungeun added glist_length() for use in a2nloops.c
 *              sungeun added glist_top() for use in a2nloops.c
 * DATE: 14 May 1995
 *              sungeun added glist_remove()
 * DATE: 11 September 1996
 *		echris added glist_prepend_new()
 */

#include <stdio.h>
#include "../include/error.h"
#include "../include/macros.h"
#include "../include/glist.h"
#include "../include/glistmac.h"


/* FUNCTION: glist_create
 * Create a generic list of length 1, with data element data.  Each list
 * node is of size size.  it is useful for the user to control the node
 * size so that he/she/it may have additional fields in the node (in 
 * addition to data).  See include/glistmac.h and ccodegen/Cgen.c for
 * an example of this.  
 * RETURN: the list (with one node)
 */
glist glist_create(void *data, unsigned long size)
{
  glist list;

  if (size < sizeof(gnode_t)) {
    return (0);
  } else {
    list = (glist)PMALLOC(size);
    GLIST_NEXT(list) = NULL;
    GLIST_DATA(list) = data;
    return (list);
  }
}


/* FUNCTION: glist_append
 * Append a new node of size size containing data element data on the end
 * of list list.
 * RETURN: the newly created node
 */
gnode_t *glist_append(glist list, void *data, unsigned long size) {
  glist tmp;
  gnode_t *n;

  if (!list || size < sizeof(gnode_t)) {
    return 0;
  }

  n = (gnode_t *)PMALLOC(size);
  GLIST_NEXT(n)   = NULL;
  GLIST_DATA(n)   = data;

  tmp = list;
  while (GLIST_NEXT(tmp)) {
    tmp = GLIST_NEXT(tmp);
  }
  GLIST_NEXT(tmp) = n;

  return n;
}

/* FUNCTION: glist_join
 * Prepend the elements of list1 on list2 that are not in list2
 * RETURN: the resulting list
 */
gnode_t *glist_join(glist list1, glist list2, unsigned long size, 
		   int (*equal_fn)(void *, void *))
{
  glist result;

  result = list2;

  while (list1) {
    if (!glist_find_equal(result, GLIST_DATA(list1), equal_fn))
      result = glist_prepend(result, GLIST_DATA(list1), size);
    list1 = GLIST_NEXT(list1);
  }

  return (result);
}

/* FUNCTION: glist_prepend_new
 * Prepends a new node of size size containing data element data at the front
 * of list list, if data is not already in the list according to equal_fn.
 * RETURN: the newly created node (and thus the new list)
 */
gnode_t *glist_prepend_new(glist list, void *data, unsigned long size,
			  int (*equal_fn)(void *, void *))
{
  if (!glist_find_equal(list, data, equal_fn)) {
    return (glist_prepend(list, data, size));
  } else {
    return (list);
  }
}

/* FUNCTION: glist_prepend
 * Prepend a new node of size size containing data element data at the front
 * of list list.
 * RETURN: the newly created node (and thus the new list)
 */
gnode_t *glist_prepend(glist list, void *data, unsigned long size)
{
  gnode_t *n;

/*  MDD: this looks unecessary if (!list || (size < sizeof(gnode_t)))
    return (0);
*/ 
  if (size < sizeof(gnode_t))
    return(0); /* MDD */

  n = (gnode_t *)PMALLOC(size);
  GLIST_NEXT(n)   = list;
  GLIST_DATA(n)   = data;

  return (n);
}

/* FUNCTION: glist_pop_head
 * Pop head off of list list
 * RETURN: if list is not empty
 *           list minus it's head,
 *           the head is returned in data
 *         else
 *           NULL list and NULL head
 */
gnode_t *glist_pop_head(glist list, void **data)
{
  if (list != NULL) {
    *data = GLIST_DATA(list);
    return(GLIST_NEXT(list));
  }
  else {
    *data = NULL;
    return(NULL);
  }

}

/* FUNCTION: glist_append_list
 * Append list2 to the end of list1.  No new node are create.  list1 is
 * mutated.
 * RETURN: list1
 */
glist glist_append_list(glist list1, glist list2) {
  glist tmp;

  if (!list1)
    return (list2);

  tmp = list1;
  while (GLIST_NEXT(tmp))
    tmp = GLIST_NEXT(tmp);

  GLIST_NEXT(tmp) = list2;

  return (list1);
}

/* FUNCTION: glist_remove
 * Remove element from glist
 * RETURN: list minus element
 */
glist glist_remove(glist list, gnode_t *element)
{
  gnode_t *l, *prev;

  if (list == element) {
    return (GLIST_NEXT(list));
  }
  else {
    for (prev = list, l = GLIST_NEXT(list);
	 l != NULL && l != element;
	 prev = l, l = GLIST_NEXT(l)) ;
    if (l == NULL) {
      INT_FATAL(NULL, "Element not found!  Not removing anything!");
    }
    else {
      GLIST_NEXT(prev) = GLIST_NEXT(l);
    }
    return (list);
  }

}

/* FUNCTION: glist_remove_equal
 * Remove all nodes in list that are equal (according to equal_fn) to data
 * 3/20/96 - echris
 */

glist glist_remove_equal(glist list, void *data, 
			 int (*equal_fn)(void *, void *))
{
  glist tmp;

  INT_COND_FATAL((equal_fn!=NULL), NULL, 
		 "NULL equal_fn in glist_remove_equal()");

  if (list == NULL) {
    return (NULL);
  } else {
    tmp = glist_remove_equal(GLIST_NEXT(list), data, equal_fn);

    if (equal_fn(data, GLIST_DATA(list))) {
      /* free node at list */
      return (tmp);
    } else {
      GLIST_NEXT(list) = tmp;
      return (list);
    }
  }
}


glist glist_difference(glist list1,glist list2,int (*equal_fn)(void*,void*),
		       glist* deadnodes) {
  glist ptr1;
  glist ptr2;
  glist first_good;
  glist last_good;
  glist next1;
  int match;

  first_good = NULL;
  last_good = NULL;
  ptr1 = list1;
  while (ptr1 != NULL) {
    next1 = GLIST_NEXT(ptr1);

    ptr2 = list2;
    match = 0;
    while (ptr2 != NULL) {
      if (equal_fn(GLIST_DATA(ptr1),GLIST_DATA(ptr2))) {
	match = 1;
	ptr2 = NULL; /*** sungeun ***/
      } else { /*** sungeun ***/
      ptr2 = GLIST_NEXT(ptr2);
      }
    }
    if (!match) {
      if (first_good == NULL) {
	first_good = ptr1;
      }
      last_good = ptr1;
    } else {
      if (deadnodes) {
	*deadnodes = ptr1;
	GLIST_NEXT(ptr1) = NULL;
	deadnodes = &(GLIST_NEXT(ptr1));
      }
      if (last_good) {
	GLIST_NEXT(last_good) = next1;
      }
    }
    ptr1 = next1;
  }

  return first_good;
}


/* FUNCTION: glist_find_equal
 * Return gnode_t that contains data equal (according to equal_fn) to data
 * 3/20/96 - echris
 */

gnode_t *glist_find_equal(glist list, void *data,
			 int (*equal_fn)(void *, void *))
{
  INT_COND_FATAL((equal_fn!=NULL), NULL, 
		 "NULL equal_fn in glist_find_equal()");

  if (list == NULL) {
    return (NULL);
  } else if (equal_fn(data, GLIST_DATA(list))) {
      return ((gnode_t *) list);
  } else {
    return (glist_find_equal(GLIST_NEXT(list), data, equal_fn));
  }
}

/* FUNCTION: glist_find
 * Search through list for node with data element data.
 * RETURN: first node that matches data parameter
 */
gnode_t *glist_find(glist list, void *data)
{
  glist tmp;

  tmp = list;
  while (tmp) {
    if (GLIST_DATA(tmp) == data)
      return (tmp);
    else
      tmp = GLIST_NEXT(tmp);
  }
  return (0);
}

/* FUNCTION: glist_apply
 * Apply a function to the data element of each node in list list
 */
void glist_apply(glist list, void (*func)(void *))
{
  glist tmp;
  for (tmp = list; tmp != NULL; tmp = GLIST_NEXT(tmp)) {
    func(GLIST_DATA(tmp));
  }
}

/* FUNCTION: glist_destroy
 * Destroy list list.  I.e. free each node in list.
 */
void glist_destroy(glist list, unsigned long size)
{
  glist tmp, save;

  tmp = list;
  while (tmp) {
    save = tmp;
    tmp = GLIST_NEXT(save);
    PFREE(save, size);
  }
}

/*** sungeun ***/
glist glist_copy(glist list, unsigned long size, void* (*copy)(void *))
{
  glist new = NULL;
  glist tmp;
  glist current;
  void* data;

  current = list;
  while (current != NULL) {
    data = GLIST_DATA(current);
    if (copy) {
      data = copy(GLIST_DATA(current));
    }
    if (new == NULL) {
      new = glist_create(data,size);
      tmp = new;
    } else {
      tmp = glist_append(tmp,data,size);
    }
    current = GLIST_NEXT(current);
  }
  return new;
}


/*** sungeun ***/
int glist_length(glist list)
{
  glist current;
  int length = 0;

  for (current = list; current != NULL; current = GLIST_NEXT(current)) {
    length++;
  }

  return(length);

}

/*** sungeun ***/
void *glist_top(glist list)
{

  return(GLIST_DATA(list));

}


/* FUNCTION: glist_equal
 * Return TRUE if all the elements in list1 are in list2 and vice versa
 * Notice that order is unimportant
 * 09/16/96 - echris
 */

int glist_equal(glist list1, glist list2, int (*equal_fn)(void *, void *))
{
  glist tmplist;

  tmplist = list1;

  while (list1) {
    if (!glist_find_equal(list2, GLIST_DATA(list1), equal_fn))
      return (FALSE);
    list1 = GLIST_NEXT(list1);
  }

  list1 = tmplist;
  while (list2) {
    if (!glist_find_equal(list1, GLIST_DATA(list2), equal_fn))
      return (FALSE);
    list2 = GLIST_NEXT(list2);
  }

  return(TRUE);
}


void glist_kill(glist list,unsigned int nodesize,unsigned int datasize) {
  glist ptr;
  glist next;

  ptr = list;
  while (ptr) {
    next = GLIST_NEXT(ptr);
    PFREE(GLIST_DATA(ptr),datasize);
    PFREE(ptr,nodesize);
    ptr = next;
  }
}
