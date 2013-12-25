/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MPI_ZLIB_H_
#define __MPI_ZLIB_H_

#include "mpi.h"

#define _PRINTFLEN        1024

#define _PRINTTAG            1
#define _ERRPRINTTAG         2
#define _WRITETAG            3
#define _ERRWRITETAG         4
#define _SCANTAG             5
#define _SCAN2TAG            6
#define _READTAG             7
#define _DEADTAG             8
#define _HALTTAG             9
#define _DONETAG            10
#define _NUM_RESERVED_IDS   11

void _HandlePrintRequests(int);

extern int _able_to_print;
extern int _print_node;

#endif
