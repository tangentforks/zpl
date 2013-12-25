/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MD_ZINC_H_
#define __MD_ZINC_H_

#include <stdio.h>

/* moved quad stuff into quad.h */

#ifndef TRUE
#define TRUE (1==1)
#endif

#ifndef FALSE
#define FALSE (0==1)
#endif

int _ZPL_FPRINTF(FILE *,const char *,...);
int _ZPL_PRINTF(const char *,...);
int _ZPL_FSCANF(FILE *,const char *,void *,int);
int _ZPL_FSCANF2(FILE *,const char*,void*,void*,int,int);
int _ZPL_SCANF(const char *,void *,int);
int _ZPL_FWRITE(const void *,int,int,FILE *);
int _ZPL_FREAD(void *,int,int,FILE *);

#ifdef _PRINT_OK_
#define _MPI_PRINT_OK_
#endif


#ifndef _MPI_PRINT_OK_
#define printf        _ZPL_PRINTF
#define fprintf       _ZPL_FPRINTF
#define fwrite        _ZPL_FWRITE
#define scanf(c,p)    _ZPL_SCANF(c,p,sizeof(*(p)))
#define fscanf(f,c,p) _ZPL_FSCANF(f,c,p,sizeof(*(p)))
#define fscanf0(f,c)  _ZPL_FSCANF(f,c,NULL,0)
#define fscanf2(f,c,p1,p2) _ZPL_FSCANF2(f,c,p1,p2,sizeof(*(p1)),sizeof(*(p2)))
#define fread         _ZPL_FREAD
#endif


#endif
