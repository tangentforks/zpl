/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "md_zinc.h"
#include "start.h"
#include "zerror.h"


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
  char ** newargv;
  int i;
  char buffer[256];
  char av1buff[256];
  char av2buff[256];
  int gridsize[_MAXRANK];

  /* allocate a command line buffer for arguments to execv */
  newargv=(char **)_zmalloc((argc+3)*sizeof(char *), "");


  /* Get the arguments parsed properly to find out how many threads to spawn */
  _ParseArgs(argc,argv,1);
  _QueryProcInfo(&_INITPROCS,gridsize);


  /* Check that we're running on a power of 2 processors as per shmem rules */
  CheckNumProcs();

  /* Copy all the old arguments over to the new vector and set up new args */

  for (i=argc-1;i>0;i--) {
    newargv[i+2]=argv[i];
  }
  newargv[0]=buffer;
  newargv[1]=av1buff;
  newargv[2]=av2buff;

  /* set up the command we want to run */

  sprintf(buffer,"%s_real",argv[0]);
  sprintf(newargv[1],"-npes");
  sprintf(newargv[2],"%d",_INITPROCS);

  execv(buffer,newargv);

  return 0;
}

