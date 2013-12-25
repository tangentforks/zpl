/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "../include/genlist.h"
#include "../include/parsetree.h"
#include "../include/tasklist.h"

static genlist_t *tasklisthead=NULL;
static genlist_t *tasklisttail=NULL;


void *GetTask(void) {
  void *task;
  genlist_t *oldtask;

  if (tasklisthead == NULL) {
    task = NULL;
  } else {
    task = G_GENERIC(tasklisthead);
    oldtask = tasklisthead;
    tasklisthead = tasklisthead->next_p;
    free(oldtask);
    if (tasklisthead == NULL) {
      tasklisttail = NULL;
    }
  }

  return task;
}


void AddTask(void *task) {
  genlist_t *newtask;

  newtask = (genlist_t *)malloc(sizeof(genlist_t));
  G_GENERIC(newtask) = task;
  newtask->next_p = NULL;

  if (tasklisthead == NULL) {
    tasklisthead = newtask;
    tasklisttail = newtask;
  } else {
    tasklisttail->next_p = newtask;
    tasklisttail = newtask;
  }
}

