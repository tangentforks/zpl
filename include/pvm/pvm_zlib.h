/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __PVM_ZLIB_H_
#define __PVM_ZLIB_H_

#include "zplglobal.h"
#include "grid.h"

/* newargs for alpha version:  (0) groupname, (1) cwd */
#define NUMNEWARGS 2

#define _PRINTFLEN        1024

#define _TOKENID             0
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

void _InitHalt(void);

char* _GetSliceName(_grid, int, char*);

extern char* _groupname;
extern char _slicename[_MAXRANK][256];

#endif
