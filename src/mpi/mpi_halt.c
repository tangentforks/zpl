/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "mpi.h"
#include "md_zlib.h"

extern int _quiet_mode;


void _ZPL_halt(int linenum) {
  if (!_quiet_mode && (linenum != 0)) {
    fprintf(stderr,"halt at line %d reached\n",linenum);
  }

  MPI_Abort(MPI_COMM_WORLD, 1);
}
