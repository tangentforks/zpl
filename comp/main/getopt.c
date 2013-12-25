/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/error.h"
#include "../include/getopt.h"

char **argv;
int argc;
static int current;

void init_args(int c, char *v[]) {
  int i;

  argc = c;
  argv = (char **)malloc((argc+1)*sizeof(char *));
  current = 0;
  for (i = 0; i < argc; i++) {
    argv[i] = (char *)malloc(strlen(v[i]) + 1);
    strcpy(argv[i], v[i]);
  }
}


char *get_arg(int *flag, char *ch) {
  char *temp = NULL;

  *flag = 0;

  if (current >= argc) {
    return NULL;
  }

  temp = argv[current];
  if (argv[current][0] == '-') {
    *flag = 1;
    temp++;
    *ch = *temp++;	
  }
  
  current++;
  return temp;
}


void reset_args() {
  current = 0;
}
