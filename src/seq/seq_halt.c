/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "md_zinc.h"
#include "md_zlib.h"
#include "zlib.h"

extern int _quiet_mode;


/* Halt all processes in this application */

void _ZPL_halt(int linenum) {
  if (!_quiet_mode && (linenum != 0)) {
    fprintf(stderr,"halt at line %d reached\n",linenum);
  }
  exit(0);
}
