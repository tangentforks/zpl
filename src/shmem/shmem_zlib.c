/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <varargs.h>
#include <mpp/shmem.h>
#include "md_zinc.h"
#include "timer.h"
#include "grid.h"
#include "distribution.h"
#include "zlib.h"
#include "md_zlib.h"
#include "start.h"
#include "zerror.h"
#include "zc_proto.h"


/* CLLEN specifies the maximum command line length we expect to see.
   This is used in allocating buffers. */

#define _CLLEN 256


/* external integers in the machine-independent libraries which hold the
   values of the number of processors, rows, and columns that the user
   initially specifies. */

int _quiet_mode=0;



/* information for conveying the command line to the compiler-output
   code.  _INITARGC tells the number of arguments we got.  _INITARGV
   points to the individual arguments in the buffer _COMMAND_LINE,
   where they are saved. */

int _INITARGC;
char ** _INITARGV;
char _COMMAND_LINE[_CLLEN];




/* This function contains all of the calls which need to be done
   once per processor.  Depending on whether main is run once per
   processor, or once total, this may be the point at which the
   threads split apart.  For the alpha version, however, main() is
   invoked per processor, and thus, this division is merely indicative
   of how things have been done on other machines. */

static void _PerNode(void) {

  /* set up the three main fields of our state vector -- the number
     of processors in the mesh and the number of rows and columns
     in the mesh. */


  /* Call the zpl-compiler output procedure which will process the
     command line in order to pull out all the user-initialized 
     configuration variables. */

  if (!_LEGALPROC) {
    exit(33);
  }

  if (_quiet_mode <= 0) {
    _PrintGridConfiguration(_DefaultGrid);
  }

  _InitRuntime(_INITARGC,_INITARGV);


  /* Barrier synch to make sure all threads are ready to go before calling
     main() */

  _BarrierSynch();

  /* Call the user's main() function. */
  
  _zplmain();
  

  /* Wait until all threads have completed running. Print out a message
     so it's clear that the user program has completed. */

  _BarrierSynch();
  if (_INDEX==0) {
    if (_quiet_mode <= 0) {
      printf("Your zpl program has completed running.\n");
      fflush(stdout);
    }
  }
  _BarrierSynch();

  /* exit the program */

  exit(0);
}


/* This routine is used to copy the command line into a buffer usable
   by external functions. */

static void _CopyArgsToGlobals(int argc,char * argv[]) {
  char * nextarg;
  int len;
  int i;
  char * equals;


  /* allocate a number of argument pointers equal to the number of
     actual arguments we received. */
  
  _INITARGV=(char **)_zmalloc((argc-1)*sizeof(char *),"shmem arg copy");
  nextarg=_COMMAND_LINE;

  /* copy the arguments one at a time, skipping the first one. */

  for (i=1;i<argc;i++) {
    len = strlen(argv[i]);
    if (nextarg+len > _COMMAND_LINE+_CLLEN) {
      _RT_ANY_FATAL1("Command line too long (> %d chars) -- exiting\n",_CLLEN);
    }
    strcpy(nextarg,argv[i]);

    /* by convention, any equals signs will be blanked out to aid in
       config var matching later. */

    equals=strchr(nextarg,'=');
    if (equals!=NULL) {
      *equals='\0';
    }
    _INITARGV[i-1]=nextarg;
    nextarg+=(len+1);
  }
  _INITARGC=argc-1;
}




/* REQUIRED INTERFACE */


int main(int argc,char * argv[]) {
  _InitTimer();   /*** for machine-specific timing routines ***/

  _INDEX = _my_pe();
  
  /* Call this function to get the values of _INITPROCS, _INITROWS, and
     _INITCOLS established. */

  _ParseArgs(argc,((char **)(argv)),0);


  /* This is so that all of the arguments are available for the config
     var parser which is output with the user's code. */

  _CopyArgsToGlobals(argc,(char **)(argv));


  _PROCESSORS = 0;
  _QueryProcInfo(&_PROCESSORS,_GRID_SIZE_V(_DefaultGrid));
  _QueryQuietMode(&_quiet_mode);

  /* A barrier synch to make sure that all processors have started. */
  _BarrierSynch();

  /* Initialize all things which need to be done by a single processor */



  /* Do whatever needs to be done on a per-node basis (including calling
     main() */

  _PerNode();

  return 0;
}


void _MDSetupGrid(_grid_fnc grid) { }

void _MDInitGrid(_grid_fnc grid) { }
