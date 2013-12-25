/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _GLIST_H__
#define _GLIST_H__

typedef struct nodet 
{
  struct nodet *next;
  void *data;
} gnode_t, *glist;

glist glist_create(void *data, unsigned long size);
gnode_t *glist_append(glist list, void *data, unsigned long size);
gnode_t *glist_prepend(glist list, void *data, unsigned long size);
gnode_t *glist_pop_head(glist list, void **data);
glist glist_append_list(glist list1, glist list2);
glist glist_remove(glist list, gnode_t *element);
glist glist_remove_equal(glist list,void *data,int (*equal_fn)(void *,void *));
glist glist_difference(glist,glist,int (*)(void*,void*),glist*);

gnode_t *glist_find_equal(glist list,void *data,int (*equal_fn)(void *,void *));
gnode_t *glist_find(glist list, void *data);
void glist_apply(glist list, void (*func)(void *));
glist glist_copy(glist list, unsigned long size,void* (*copy)(void *));
int glist_length(glist list);
void *glist_top(glist list);
gnode_t *glist_join(glist list1, glist list2, unsigned long size, 
		    int (*equal_fn)(void *, void *));
gnode_t *glist_prepend_new(glist list, void *data, unsigned long size,
			   int (*equal_fn)(void *, void *));
int glist_equal(glist list1, glist list2, int (*equal_fn)(void *, void *));

void glist_destroy(glist list, unsigned long size);
void glist_kill(glist,unsigned int,unsigned int);

/* an insert(), delete(), and match() would be nice ! */

/* generic list access macros */
#define GLIST_NEXT(l) ((l)->next)
#define GLIST_DATA(l) ((l)->data)
#define GLIST_NODE_SIZE (sizeof(gnode_t))

#endif
