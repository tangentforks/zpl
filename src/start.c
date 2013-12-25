/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md_zinc.h"
#include "zc_proto.h"
#include "zplglobal.h"
#include "start.h"

#define _MAXFACTORS 32

static int initgrid[_MAXRANK] = {0,0,0,0,0,0};
static int initprocs=0;

static int factor[_MAXFACTORS];

static int _quiet_mode=0;

static int _memtrace=0;

static int _mpi_print=0;
static int _mpi_print_node=0;

static int _mpi_collective=-1;

static int _checkpoint_interval=0;
static char _checkpoint_location[256]=".";

static int _verbose_remap=0;
static int _verbose_hierarchy=0;

static int _call_valgrind=0;

int _MAXDIM_GRID = 0;


#define BUF 256


static int ParseGrid(char *s, int grid[]) {
  char grids[BUF];
  char *val;
  int i;

  /* copy s, because strtok() is destructive */
  if (strlen(s) >= BUF) {
    fprintf(stderr,"Processor grid specification too long.");
    exit(1);
  }
  strncpy(grids, s, BUF);

  i = 0;
  val = strtok(grids, "x");
  if (val != NULL) grid[i++] = atoi(val);

  while (val != NULL) {
    if (i >= _MAXRANK) {
      fprintf(stderr,"Too many dimensions in grid specification.");
      exit(1);
    }

    val = strtok(NULL, "x");
    if (val != NULL) {
      grid[i] = atoi(val);
      i++;
      if (i > _MAXDIM_GRID) {
	_MAXDIM_GRID = i;
      }
    }
  }

  return (i);
}


void _ParseArgs(int argc,char *argv[],int print) {
  int exitnow = 0;

  for (argc--, argv++; *argv && !exitnow; argv++, argc--) {
    if (**argv == '-') {
      switch(*(*argv+1)) {
      case 'p':
      case 'P':
	if (_MAXDIM_GRID < 1) {
	  _MAXDIM_GRID = 1;
	}
	initprocs=atoi(*argv+2);
	break;
      case 'r':
      case 'R':
	if (_MAXDIM_GRID < 1) {
	  _MAXDIM_GRID = 1;
	}
	initgrid[0]=atoi(*argv+2);
	break;
      case 'C':
      case 'c':
	if (!strncmp(*argv+2,"heckpoint_interval=",19)) {
	  _checkpoint_interval = atoi(*argv+21);
	} else if (!strncmp(*argv+2,"heckpoint_location=",19)) {
	  strcpy(_checkpoint_location, *argv+21);
	} else {
	  if (_MAXDIM_GRID < 2) {
	    _MAXDIM_GRID = 2;
	  }
	  initgrid[1]=atoi(*argv+2);
	}
	break;
      case 'L':
      case 'l':
	if (_MAXDIM_GRID < 3) {
	  _MAXDIM_GRID = 3;
	}
	initgrid[2]=atoi(*argv+2);
	break;
      case 'g':
      case 'G':
	ParseGrid(*argv+2, initgrid);
	break;
      case 'H':
      case 'h':
	printf("FLAGS:\n");
	printf("      -p<n>            -- set number of processors\n");
	printf("      -r<n>            -- set number of rows\n");
	printf("      -c<n>            -- set number of columns\n");
	printf("      -l<n>            -- set number of levels\n");
	printf("      -g<grid spec>    -- set processor grid specification "
               "(e.g., 2x2x3)\n");
	printf("\n");
	
	printf("      -f<filename>     -- read in a file of config var values\n");
	printf("      -s<cfgvar>=<val> -- set the value of a config var\n\n");
	
	printf("      -q               -- run in quiet mode\n\n");
	
	printf("      -h               -- print this message\n");
	printf("CHECKPOINTING FLAGS:\n");
	printf("      -checkpoint_interval=n\n");
	printf("                       -- minimum seconds between checkpoints (default=0)\n");
	printf("      -checkpoint_location=<filename>\n");
	printf("                       -- directory for checkpoint files (default=.)\n");
	_PrintConfigs();
	exit(0);
	break;
      case 'm':
      case 'M':
	if (!strcmp(*argv+2,"piargs")) {
	  exitnow = 1;
	} else if (!strncmp(*argv+2,"pi_collective",13)) {
	  _mpi_collective = 1;
	} else if (!strncmp(*argv+2,"pi_no_collective",16)) {
	  _mpi_collective = 0;
	} else if (!strncmp(*argv+2,"pi_print_task",13)) {
	  _mpi_print = 1;
	  _mpi_print_node = 0;
	  if(!strncmp(*argv+2,"pi_print_task=",13)){
	    _mpi_print_node = atoi(*argv+16);
	  } 
	} else if (!strncmp(*argv+2,"emtrace", 7)) {
	  _memtrace = 1;
	  if (!strncmp(*argv+2,"emtrace=",8)) {
	    _memtrace = atoi(*argv+10);
	  }
	} else if (print) {
	  fprintf(stderr,"unknown flag: -%s\n",(*argv+1));
	}
	break;
      case 's':
      case 'S':
      case 'f':
      case 'F':
      case 'i':
      case 'I':
      case 'x':
      case 'X':
	break;
      case 'q':
      case 'Q':
	_quiet_mode = 1;
	break;
      case 'v': /* caught by front-end to generate verbose output */
	if (!strcmp(*argv+2,"erbose_remap")) {
	  _verbose_remap = 1;
	}
	else if (!strcmp(*argv+2,"erbose_hierarchy")) {
	  _verbose_hierarchy = 1;
	}
	else if (!strcmp(*argv+2,"algrind")) {
	  _call_valgrind = 1;
	  printf("using valgrind\n");
	}
	else {
	  _quiet_mode = -1;
	}
	break;
      case '-':
	break;
      default:
	if (print) {
	  fprintf(stderr,"unknown flag: -%s\n",(*argv+1));
	}
	break;
      }
    } else {
      if (print) {
	fprintf(stderr,"unknown argument: %s\n",*argv);
      }
    }
  }
}


static int CalcFactors(int val,int numslots) {
  int initval;
  int numfactors;
  int i;
  int newfactor;
  int foundslot;

  if (val <= 1) {
    numfactors = 1;
    factor[0] = 1;
  } else {
    initval = val;
    numfactors = 0;
    for (i=2; i<=initval; i++) {
      while (val%i == 0) {
	factor[numfactors] = i;
	numfactors++;
	val/=i;
      }
    }
  }

  if (numfactors > _MAXFACTORS) {
    fprintf(stderr,"ERROR: Brad knows nothing about number theory\n");
    exit(1);
  }

  if (numfactors > numslots) {
    while ((numfactors % numslots) != 0) {
      numfactors--;
      newfactor = factor[0] * factor[1];

      foundslot = 0;
      for (i=0; !foundslot && i<(numfactors-1); i++) {
	if (newfactor < factor[i+2]) {
	  factor[i] = newfactor;
	  foundslot = 1;
	} else {
	  factor[i] = factor[i+2];
	}
      }
      if (foundslot) {
	for ( ; i<numfactors; i++) {
	  factor[i] = factor[i+1];
	}
      } else {
	factor[numfactors-1] = newfactor;
      }
    }
  }

  /*
    printf("Factors: %d",factor[0]);
    for (i=1;i<numfactors;i++) {
    printf(", %d",factor[i]);
    }
    printf("\n");
  */


  return numfactors;
}


/* numprocs will return the number of processors specified by the
   user.  If the user has left the number unspecified, the value in
   numprocs will be checked to see if it contains a nonzero value,
   which will serve as the number of procs (allowing a client to
   specify a default number).  Currently this is only used in the
   sequential build of ZPL. */

void _QueryProcInfo(int *numprocs,int gridsize[]) {
  int i;
  int alldimsspecified=1;
  int blankdim[_MAXRANK] = {1, 1, 1, 1, 1, 1};
  int numfactors;
  int dim;
  int numslots;

  if (_MAXDIM > _MAXDIM_GRID) {
    _MAXDIM_GRID = _MAXDIM;
  }

  for (i=0; i<_MAXDIM_GRID; i++) {
    if (initgrid[i] == 0) {
      alldimsspecified=0;
    } else if (initgrid[i] < 0 || initprocs < 0) {
      fprintf(stderr,"Illegal processor information specified\n");
      exit(1);
    }
  }
  
  if (initprocs == 0 && alldimsspecified==0) {
    if (*numprocs != 0) {
      initprocs = *numprocs;
    } else {
      fprintf(stderr,"Inadequate processor information specified\n");
      exit(1);
    }
  }

  if (alldimsspecified) {
    initprocs = 1;
    for (i=0; i<_MAXDIM_GRID; i++) {
      initprocs *= initgrid[i];
    }
    *numprocs = initprocs;
  } else {
    *numprocs = initprocs;

    /* remove any dimensions specified by the user */
    numslots = 0;
    for (i=0; i<_MAXDIM_GRID; i++) {
      if (initgrid[i] != 0) {
	if (initprocs % initgrid[i] != 0) {
	  fprintf(stderr,"Impossible processor information specified\n");
	  exit(1);
	}
	initprocs /= initgrid[i];
	blankdim[i] = 0;
      } else {
	numslots++;
	initgrid[i] = 1;  /* set for mult later */
      }
    }

    /* break remaining processors down into prime factors */
    numfactors = CalcFactors(initprocs,numslots);

    /* divvy up factors giving biggest/most to lowest dims */
    dim = -1;
    for (i=numfactors-1; i>=0; i--) {
      do {
	dim++;
	if (dim == _MAXDIM_GRID) {
	  dim = 0;
	}
      } while (!blankdim[dim]);
      initgrid[dim] *= factor[i];
    }
  }

  for (i=_MAXDIM_GRID; i<_MAXRANK; i++) {
    initgrid[i] = 1;
  }

  for (i=0; i<_MAXRANK; i++) {
    gridsize[i] = initgrid[i];
  }
}  
    

void _QueryQuietMode(int *quiet_mode) {
  if (_quiet_mode != 0) {
    *quiet_mode = _quiet_mode;
  }
}

int _QueryMemTrace(void) {
  return _memtrace;
}

int _QueryMPIPrintHelp(void) {
  return _mpi_print;
}

int _QueryMPIPrintNode(void) {
  return _mpi_print_node;
}

int _QueryMPICollective(void) {
  return _mpi_collective;
}

int _QueryCheckpointInterval(void) {
  return _checkpoint_interval;
}

char *_QueryCheckpointLocation(void) {
  return _checkpoint_location;
}

int _QueryVerboseHierarchy(void) {
  return _verbose_hierarchy;
}

int _QueryVerboseRemap(void) {
  return _verbose_remap;
}

int _QueryValgrind(void) {
  return _call_valgrind;
}
