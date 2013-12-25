/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "../include/error.h"
#include "../include/global.h"
#include "../include/const.h"
#include "../include/struct.h"
#include "../include/symmac.h"
#include "../include/db.h"
#include "../include/stack.h"


static struct {
  symboltable_t	*stack[MAXLEVEL];
  int	top;
} pstack;


void initialize_pstack() {
  int	i;

  for (i = 0; i < MAXLEVEL; i++)
    pstack.stack[i] = NULL;
  pstack.top = -1;
}


symboltable_t *top(void) {
  if (pstack.top == -1)	
    return NULL;
  DB(30) {
    DBS0(30, "top(stack.c):");
    if (pstack.stack[pstack.top] != NULL) {
      DBS1(30, "%s", S_IDENT(pstack.stack[pstack.top]));
    }
    else {
      DBS0(30, "NULL");
    }
    DBS0(30, "\n");
  }
  return pstack.stack[pstack.top];
}


int pop(void) {
  if (pstack.top == -1)	
    return -1;
  pstack.top--;
  pstack.stack[pstack.top] = NULL;
  return 0;
}


int push(symboltable_t *element) {
  if (pstack.top >= MAXLEVEL-1) {	
    INT_FATAL(NULL, "stack overflow - level %d",current_level);
  }
  pstack.top++;
  pstack.stack[pstack.top] = element;
  DB(30) {
    DBS0(30, "push(stack.c):");
    if (element == NULL) {
      DBS0(30, "NULL\n");
    } else {
      DBS2(30, "element=%s,stack=%d\n", S_IDENT(element), pstack.top);
    }
  }
  return(0);
}

