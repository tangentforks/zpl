/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __STACK_H_
#define __STACK_H_

#define G_SUCCESS 0
#define G_ERROR -1

typedef struct element stack_element;
typedef struct element {
  void *data;
  stack_element *next;
} element;

typedef struct stack {
  stack_element *top;
  stack_element *bot;
} stack;

stack* g_new_stack(void);
int g_free_stack(stack*);
void g_push(stack*,void*);
void* g_pop(stack*);
void* g_top(stack*);
void* g_bot(stack*);
int g_empty(stack*);
void g_apply_function(stack*,void (*)(void*));

#endif
