/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __FILE_IO_BASE_H_
#define __FILE_IO_BASE_H_

#include <stdio.h>

typedef struct _zfile_struct_ {
  FILE *fptr;
  long pos;
} _zfile_struct;
typedef _zfile_struct *_zfile;

#endif
