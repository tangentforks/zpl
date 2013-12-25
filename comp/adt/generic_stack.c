/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/********** generic stack operations **********/
#include <stdio.h>
#define __STACK_C_
#include "../include/macros.h"
#include "../include/generic_stack.h"
#include "../include/db.h"

stack *
g_new_stack()
{
  stack *new;

  new = (stack *)PMALLOC(sizeof(stack));
  new->top = NULL;
  new->bot = NULL;

  return(new);
}

int
g_free_stack(s)
     stack *s;
{

  if (g_empty(s)) {
    PFREE(s, sizeof(stack));
    return(G_SUCCESS);
  }
  else {
    DBS0(1, "Freeing an non-empty stack!\n");
    while (!g_empty(s))
      g_pop(s);
    PFREE(s, sizeof(stack));
    return(G_ERROR);
  }
}

void
g_push(s, data)
     stack *s;
     void *data;
{
  stack_element *element;

  element = (stack_element *)PMALLOC(sizeof(stack_element));
  element->data = data;

  element->next = s->top;
  s->top = element;
  if (s->bot == NULL)
    s->bot = element;

}

void *
g_pop(s)
     stack *s;
{
  stack_element *element;
  void *data;

  if (g_empty(s))		/* empty stack */
    return(NULL);

  element = s->top;
  s->top = element->next;
  if (element == s->bot)
    s->bot = NULL;

  data = element->data;
  PFREE(element, sizeof(stack_element));

  return(data);

}

void *
g_top(s)
     stack *s;
{

  if (s->top)
    return((void *) (s->top)->data);
  else
    return(NULL);

}

void *
g_bot(s)
     stack *s;
{

  if (s->bot)
    return((void *) (s->bot)->data);
  else
    return(NULL);

}

int
g_empty(s)
     stack *s;
{
  if (s->top)
    return(FALSE);
  else
    return(TRUE);
}

static stack_element* g_next(stack_element* e) {
  if (e)
    return(e->next);
  else
    return(e);
}

void
g_apply_function(stack* s,void (*f)(void*)) {
  stack_element *marker;

  marker = s->top;

  while (marker) {
    f(marker->data);
    marker = g_next(marker);
  }

}
