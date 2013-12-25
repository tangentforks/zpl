/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* sparc_zlib.c:
   Brad Chamberlain
   developed 1/94 - 3/94
   commented 3/11/94
   
   This is the bulk of the machine-dependent code for ZPL.  The only
   other bit of machine-dependent code is used to launch the whole
   process, and is found in <machine_name>_start.c

   The code which defines the required machine-dependent interface is
   set off below by a marker which says "REQUIRED INTERFACE"

   All other routines are for internal use only by this program.
   Note that this code was adapted from a shared-memory version for
   the KSR which was adapted from a shared-memory version for the
   butterfly, so some of its apparent silliness is due to artifacts
   from those versions.

   All questions to brad@cs.washington.edu, please.
*/
   

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <memory.h>

#include "md_zinc.h"
#include "md_zlib.h"
#include "start.h"
#include "grid.h"
#include "distribution.h"
#include "timer.h"
#include "zlib.h"
#include "zc_proto.h"
#include "zlib.h"


/* static integers set by the machine-independent libraries which hold
   the values of the number of processors, rows, and columns that the
   user initially specifies. */

static int _INITPROCS=0;
int _quiet_mode=0;

void _MDSetupGrid(_grid_fnc grid) { }
void _MDInitGrid(_grid_fnc grid) { }

/* Initialize contains those functions which need to be called by
   a single processor in setting things up. */


static void _Initialize(void) {
  int i;

  /* first, it needs to initialize certain values in its state vector.
     This is normally done in _PerNode(), but may be needed in
     subsequent procedures called herein, so is done here to be
     safe. */

  _PROCESSORS=_INITPROCS;
  for (i=0; i<_MAXRANK; i++) {
    _GRID_SIZE(_DefaultGrid, i) = 1;
  }

  /* print out information to the user inidcating the mesh
     characteristics which the problem will be run on. */

  if (_PROCESSORS != 1) {
    printf("This is the sequential version of the code.  You may only run "
	   "with one processor\n");
    _ZPL_halt(0);
  } else {
    if (_quiet_mode <= 0) {
      _PrintGridConfiguration(_DefaultGrid);
    }
  }
}


/* This function is responsible for initializing the unspecified
   fields of the state vector.  At this point, the only two are the
   processor's location in terms of rows and columns in the mesh. */

/* This function contains all of the calls which need to be done once
   per processor.  Depending on whether main is run once per
   processor, or once total, this may be the point at which the
   threads split apart.  For the sparc version, however, main() is
   invoked per processor, and thus, this division is merely indicative
   of how things have been done on other machines. */

static void _PerNode(int argc,char * argv[]) {
  /* init our node's state vector. */

  _InitNodeStateVector();


  _InitRuntime(argc,argv);


  /* Barrier synch to make sure all threads are ready to go before calling
     main() */


  /* Call the user's main() function. */
  
  _zplmain();
  

  /* Wait until all threads have completed running. Print out a message
     so it's clear that the user program has completed. */

  if (_quiet_mode <= 0) {
    printf("Your zpl program has completed running.\n");
    fflush(stdout);
  }

  /* exit the program */

  exit(0);
}


static void _NullifyEquals(int argc,char * argv[]) {
  int i;
  char * equals;

  for (i=1;i<argc;i++) {
    equals=strchr(argv[i],'=');
    if (equals!=NULL) {
      *equals='\0';
    }
  }
}


/* REQUIRED INTERFACE */


int main(int argc,char *argv[]) {
  int initgrid[_MAXRANK];

  _InitTimer();   /*** for machine-specific timing routines ***/

  _INITPROCS = 1;

  /* Call this function to get the values of _INITPROCS, _INITROWS, and
     _INITCOLS established. */

  _ParseArgs(argc,(char **)argv,1);


  _NullifyEquals(argc,(char **)argv);

  /* I can't remember what this one does, and nautilus seems to be down */

  _QueryProcInfo(&_INITPROCS,initgrid);
  _QueryQuietMode(&_quiet_mode);


  /* Initialize all things which need to be done by a single processor */

  _INDEX = 0;
  _Initialize();


  /* Do whatever needs to be done on a per-node basis (including calling
     main() */
  _PerNode(argc,argv);

  exit(0);
}
