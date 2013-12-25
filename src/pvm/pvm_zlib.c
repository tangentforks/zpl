/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* pvm_zlib.c:
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
#include <unistd.h>

#include "pvm3.h"

#include "md_zinc.h"

#include "md_zlib.h"
#include "start.h"
#include "pvm_zlib.h"
#include "grid.h"
#include "distribution.h"
#include "timer.h"
#include "zc_proto.h"
#include "zerror.h"
#include "zlib.h"


/* CLLEN specifies the maximum command line length we expect to see.
   This is used in allocating buffers. */

#define _CLLEN 256


/* point in the argument list to the string which will specify our group
   name. */

char *_groupname;


/* external integers in the machine-independent libraries which hold the
   values of the number of processors, rows, and columns that the user
   initially specifies. */

int ptid=PvmNoParent;

static int _INITPROCS=0;
static int _INITGRID[_MAXRANK];
static int _quiet_mode=0;


/* information for conveying the command line to the compiler-output
   code.  _INITARGC tells the number of arguments we got.  _INITARGV
   points to the individual arguments in the buffer _COMMAND_LINE,
   where they are saved. */

int _INITARGC;
char ** _INITARGV;
char _COMMAND_LINE[_CLLEN];


/* a vector which saves the task id's (PVM-specific), so that we can
   map user-specified indices to PVM task ID's for our communication
   routines. */

int *_tids;

char* _GetSliceName(_grid grid, int slicenum,char* basename) {
  static char* slicename;
  int i;
  int dimbit;

  if (slicenum == 0) {
    return basename;
  }

  if (slicename == NULL) {
    slicename = (char*)_zmalloc(256*sizeof(char),"slicename");
  }

  if (basename) {
    sprintf(slicename,"%s_%d",basename,grid->gridid);
  } else {
    sprintf(slicename,"anon_%d",grid->gridid);
  }
  dimbit = 1;
  for (i=0; i<_MAXRANK; i++) {
    if (dimbit & slicenum) {
      sprintf(slicename,"%s_%d",slicename,_GRID_LOC(grid, i));
    } else {
      sprintf(slicename,"%s_*",slicename);
    }
    dimbit = dimbit << 1;
  }

  return slicename;
}

static void _JoinRowColGroups(_grid_fnc grid) {

}



/* This function contains all of the calls which need to be done
   once per processor.  Depending on whether main is run once per
   processor, or once total, this may be the point at which the
   threads split apart.  For the alpha version, however, main() is
   invoked per processor, and thus, this division is merely indicative
   of how things have been done on other machines. */

static void _PerNode(void) {
  int i;
 
 /* PVM-specific -- save the tids of all the processors in our
     group so that we have a mapping from logical processor number
     to PVM tid. */

  /* Call the zpl-compiler output procedure which will process the
     command line in order to pull out all the user-initialized 
     configuration variables. */

  _tids = (int *)_zmalloc(_PROCESSORS*sizeof(int),"pvm tid list");
  for (i=0;i<_PROCESSORS;i++) {
    _tids[i] = pvm_gettid(_groupname,i);
  }

  if (!_LEGALPROC) {
    exit(33);
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
    if (ptid!=PvmNoParent) {
      pvm_initsend(PvmDataRaw);
      pvm_send(ptid,_DONETAG);
      pvm_recv(ptid,_DONETAG);
    }
  }
  _BarrierSynch();

  /* quit the group and exit PVM */

  for (i= _GRID_NUMSLICES(_DefaultGrid) - 1; i >= 1; i--) {
    char* slicename;

    slicename = _GetSliceName(_DefaultGrid, i,_groupname);
    _BarrierSynch();
    pvm_lvgroup(slicename);
    _BarrierSynch();
  }

  if (_groupname) {
    pvm_lvgroup(_groupname);
  }

  pvm_exit();


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
  
  _INITARGV=(char **)_zmalloc((argc-1)*sizeof(char *),"initial arguments");
  nextarg=_COMMAND_LINE;

  /* copy the arguments one at a time, skipping the first one. */

  for (i=1;i<argc;i++) {
    len = strlen(argv[i]);
    if (nextarg+len > _COMMAND_LINE+_CLLEN) {
      printf("Command line too long (> %d chars) -- exiting\n",_CLLEN);
      exit(0);
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
  int i;

  _InitTimer();   /*** for machine-specific timing routines ***/

  /* PVM-specific call in order to set us up for future PVM calls */
  
  pvm_mytid();
  ptid = pvm_parent();
  _InitHalt();


  /* Join PVM group as the name of the executable that we are running.
     This allows barrier synchs to take place between the processors,
     and allows us to get our _INDEX value */

#ifdef _PVM_DEFAULT_GROUP
  _groupname = NULL;
  _INDEX = _my_pe();
#else
  _groupname = argv[1];
  _INDEX = pvm_joingroup(_groupname);
#endif

  /* change directory so that we'll be in the same one as out parent */

  chdir(argv[2]);

  
  argc-=NUMNEWARGS;
  argv+=NUMNEWARGS;

  /* Call this function to get the values of _INITPROCS, _INITROWS, and
     _INITCOLS established. */

  _ParseArgs(argc,(char **)argv,0);


  /* This is so that all of the arguments are available for the config
     var parser which is output with the user's code. */

  _CopyArgsToGlobals(argc,(char **)argv);


  /* I can't remember what this one does, and nautilus seems to be down */

  _QueryProcInfo(&_INITPROCS,_INITGRID);
  _QueryQuietMode(&_quiet_mode);


  /* PVM-specific -- this is to be sure that all processors have joined
     the group before we continue. */

/*  while (pvm_gsize(_groupname) != _INITPROCS) {
  }*/   /* the following should do this implicitly, and is stronger */

  
  /* A barrier synch to make sure that all processors have started. */

  pvm_barrier(_groupname,_INITPROCS);


  _PROCESSORS=_INITPROCS;
  for (i=0;i<_MAXRANK;i++) {
    _GRID_SIZE(_DefaultGrid, i) = _INITGRID[i];
  }

  if (_quiet_mode <= 0) {
    _PrintGridConfiguration(_DefaultGrid);
  }

  /* Do whatever needs to be done on a per-node basis (including calling
     main() */

  _PerNode();

  return 0;
}


/* A general barrier synch, implemented using PVM's barrier */

void _BarrierSynch(void) {
  if (_PROCESSORS != 1) {
    pvm_barrier(_groupname,_PROCESSORS);
  }
}


/* Receive a general message.  The sender is the  specified processor id.
   the portid specifies the port that it was sent on.  The commid is a
   unique per-message ID set up by the user.  Note that those three 
   pieces of information may be more than any message needs in order to
   be uniquely identified.  At this point we are taking the position that
   it is better to overspecify than to underspecify.  Some of the pieces
   of information work better than others, depending on the underlying
   message passing library.  For example, in this case, we don't use
   the port id, but simply the processor number and comm id. */

void _ReadPort(int proc,int pid,int commid,void * dest,int length) {
  
  /* receive the message of the given commid from the specified processor */
  
  pvm_recv(_tids[proc],commid);
  pvm_upkbyte((char *)dest,length,1);
}


/* Write a general message.  The parameters are the same as the above,
   except that the specified processor number is the one to send to. */

void _WritePort(int proc,int pid,int commid,void * src,int length) {

  /* get ready to send the data via PVM, and treat the data as unformatted
     bytes. */

  pvm_initsend(PvmDataRaw);
  pvm_pkbyte((char *)src,length,1);


  /* send the data */

  pvm_send(_tids[proc],commid);  /* should be pid, or fn of pid? */
}

static int gridid = 0;

void _MDSetupGrid(_grid_fnc grid) {
}

void _MDInitGrid(_grid_fnc grid) {
  int i;
  int pos;
  char* slicename;

  grid->gridid = gridid++;
 
  for (i=1; i<_GRID_NUMSLICES(grid); i++) {
    slicename = _GetSliceName(grid, i, _groupname);
    _BarrierSynch();
    _ResetToken();
    if (!_BegOfSlice(grid, i)) {
      _RecvToken();
    }
    /* printf("[%d] joining %s\n",_INDEX,slicename); */
    pos = pvm_joingroup(slicename);
    if (pos != _MyLocInSlice(grid, i)) {
      fprintf(stderr,"[%d] ERROR: pos didn't match in group %s (%d != %d)\n",
	      _INDEX,slicename,pos,_MyLocInSlice(grid, i));
    }
    if (!_EndOfSlice(grid, i)) {
      _PassTokenSameSlice(grid, i);
    }
    _BarrierSynch();
    /* if (_INDEX == 0) { printf("\n\n"); } */
  }
}
