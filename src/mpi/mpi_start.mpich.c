/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#ifndef _MPI_PRINT_OK_
#define _MPI_PRINT_OK_
#endif
#include "md_zinc.h"
#include "start.h"
#include "zplglobal.h"

#define DEFAULT_STR_SIZE 1024
#define _INDEX 0

int _able_to_print = 1;

static void signal_handler(int sig) {
  fprintf(stderr, "\nSignal %d received.  Terminating ZPL application!\n\n", 
	  sig);
  exit(sig);
}

int main(int argc,char * argv[]) {
  int numprocs=0;
  int gridsize[_MAXRANK];
  int i;
  int currsize= DEFAULT_STR_SIZE, strsize=0;
  char *mpirun_cmd = (char *) malloc(currsize);
  int quiet_mode=0;

  /* Get the arguments parsed properly to find out how many threads to
     spawn */

  _ParseArgs(argc,argv,1);
  _QueryProcInfo(&numprocs,gridsize);

  if (_QueryMPIPrintHelp()) {
    numprocs++;
  }

#ifdef _MPI_DEBUG_
  sprintf(mpirun_cmd, "mpirun -gdb -np %d %s_real",
	  numprocs, argv[0]);
#else
  sprintf(mpirun_cmd, "mpirun -np %d %s_real",
	  numprocs, argv[0]);
#endif

  /*** Let's just assume this stuff won't go over the default ***/
  /***  string size and ungracefully catch it afterwards ***/

  strsize = strlen(mpirun_cmd);
  if (strsize > currsize) {
    fprintf(stderr,"Buffer size exceeded (cannot recover).  Please contact "
	    "zpl-bugs@cs.washington.edu\n");
    exit(1);
  }
  
  for (i = 1; i < argc; i++) {
    if (strsize+1+strlen(argv[i]) > currsize) {
      currsize = 2*strlen(mpirun_cmd);
      mpirun_cmd = (char *) realloc((void *) mpirun_cmd, currsize);
    }
    strcat(mpirun_cmd, " ");
    strcat(mpirun_cmd, argv[i]);
  }

  /* register signal handler for SIGHUP (1) SIGINT SIGQUIT SIGILL SIGTRAP
     SIGABRT SIGEMT SIGFPE SIGKILL SIGBUS SIGSEGV SIGSYS SIGPIPE SIGALRM
     SIGTERM (14) */
  for (i = 1; i <= SIGTERM; i++ )
    signal(i, signal_handler);

  _QueryQuietMode(&quiet_mode);
  if (quiet_mode < 0) {
    printf("system: %s\n", mpirun_cmd);
  }

  system(mpirun_cmd);

  exit(0);
}

