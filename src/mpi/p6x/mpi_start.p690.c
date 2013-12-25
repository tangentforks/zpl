/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#ifndef _MPI_PRINT_OK_
#define _MPI_PRINT_OK_
#endif
#include "md_zinc.h"
#include "start.h"
#include "zplglobal.h"

#define DEFAULT_STR_SIZE 1024
#define _INDEX 0

int main(int argc,char * argv[]) {
  int numprocs=0;
  int gridsize[_MAXRANK] = {0,0,0,0,0,0};
  int i;
  int currsize= DEFAULT_STR_SIZE, strsize=0;
  char *command = (char *) malloc(currsize);

  /* Get the arguments parsed properly to find out how many threads to
     spawn */

  _ParseArgs(argc,argv,1);
  _QueryProcInfo(&numprocs,gridsize);

  sprintf(command, "./%s_real -procs %d",argv[0],numprocs);

  strsize = strlen(command);
  if (strsize > currsize) {
    fprintf(stderr,"Buffer size exceeded (cannot recover).  Please contact "
	    "zpl-bugs@cs.washington.edu\n");
    exit(1);
  }

  for (i = 1; i < argc; i++) {
    if (strsize+1+strlen(argv[i]) > currsize) {
      currsize = 2*strlen(command);
      command = (char *) realloc((void *) command, currsize);
    }
    strcat(command, " ");
    strcat(command, argv[i]);
  }

  system(command);

  return 0;
}
