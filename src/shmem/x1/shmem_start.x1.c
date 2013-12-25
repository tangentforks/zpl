/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "md_zinc.h"
#include "start.h"
#include "zerror.h"
#include "zlib.h"

static int _INITPROCS=0;

static void CheckNumProcs(void) {
  int i;
  
  i=1;
  do {
    if (i==_INITPROCS) {
      return;
    }
    i*=2;
  } while (i <= _INITPROCS);
  fprintf(stderr,"ERROR: for shmem, must use a number of processors = 2^k");
  exit(1);
}

int main(int argc,char * argv[]) {
  int i;
  char buffer[4096];
  char av1buff[256];
  char av2buff[256];
  int gridsize[_MAXRANK];
  int quiet_mode=0;

  /* Get the arguments parsed properly to find out how many threads to spawn */
  _ParseArgs(argc,argv,1);
  _QueryProcInfo(&_INITPROCS,gridsize);

  /* set up the command we want to run */
  sprintf(buffer, "aprun -n%d -p16m:16m %s_real", _INITPROCS, argv[0]);
  for (i=1; i<argc; i++) {
    sprintf(buffer, "%s %s", buffer, argv[i]);
  }

  _QueryQuietMode(&quiet_mode);
  if (quiet_mode < 0) {
    printf("system: %s\n", buffer);
  }

  
  i = system(buffer);

  if (i==-1) {
    fprintf(stderr, "ERROR calling aprun\n");
  }

  return 0;
}

