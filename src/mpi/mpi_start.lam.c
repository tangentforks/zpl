/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __MPI_PARAGON_
#include <nx.h>
#endif

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
  fprintf(stderr,"\nSignal %d received.  Terminating ZPL application!\n\n",sig);
  exit(sig);
}


int main(int argc,char* argv[]) {
  int numprocs=0;
  int gridsize[_MAXRANK];
  int i;
  int currsize= DEFAULT_STR_SIZE, strsize=0;
  char *mpirun_cmd = (char *) malloc(currsize);

  /* Get the arguments parsed properly to find out how many threads to
     spawn */

  _ParseArgs(argc,argv,1);
  _QueryProcInfo(&numprocs,gridsize);
  if (_QueryMPIPrintHelp()) {
    numprocs++;
  }

#ifdef _MPI_DEBUG_
  sprintf(mpirun_cmd, "mpirun -s h -c %d -w -O debugger -- %s_real", numprocs, argv[0]);
#else
  if (_QueryValgrind()) {
    sprintf(mpirun_cmd, "mpirun -s h -c %d -w -O valgrind -- --num-callers=16 %s_real", numprocs, argv[0]);
  }
  else {
    sprintf(mpirun_cmd, "mpirun -s h -c %d -w -O %s_real --", numprocs, argv[0]);
  }
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
  strcat(mpirun_cmd," | grep -v ') exited with status 1'");

  /* register signal handler for SIGHUP (1) SIGINT SIGQUIT SIGILL SIGTRAP
     SIGABRT SIGEMT SIGFPE SIGKILL SIGBUS SIGSEGV SIGSYS SIGPIPE SIGALRM
     SIGTERM (14) */
  for (i = 1; i <= SIGTERM; i++ )
    signal(i, signal_handler);

  system(mpirun_cmd);

  exit(0);
}

