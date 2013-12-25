/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <strings.h>
#include <sys/time.h>
#include "md_zinc.h"
#include "zplglobal.h"


static int _INITPROCS=0;

main(int argc,char * argv[]) {
  char ** newargv;
  char *command;
  int i;
  char buffer[256];
  char av1buff[256];
  int gridsize[_MAXRANK];

  /* allocate command line buffers for arguments to system */
  newargv=(char **)malloc((argc+1)*sizeof(char *));
  command=(char *)malloc(256*(argc+1)*sizeof(char *));

  /* Get the arguments parsed properly to find out how many threads to spawn */
  _ParseArgs(argc,argv,1);
  _QueryProcInfo(&_INITPROCS,gridsize);


  /* Copy all the old arguments over to the new vector and set up new args */

  for (i=argc-1;i>0;i--) {
    newargv[i+1]=argv[i];
  }
  newargv[0]=buffer;
  newargv[1]=av1buff;

  /* set up the command we want to run */

  sprintf(buffer, "/bin/mpprun -n%d", _INITPROCS);
  sprintf(newargv[1], "%s_real", argv[0]);

  command[0] ='\0';
  for (i=0;i<(argc+1);i++) {
     strcat(command, newargv[i]);
     strcat(command, " ");
  }

  system(command);
  /*** execv(buffer, newargv); ***/

}
