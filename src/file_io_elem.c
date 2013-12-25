/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "md_zinc.h"
#include "file_io.h"
#include "zerror.h"
#include "zlib.h"

/* routines to read/write element from/to a stream */

void _StreamOut_bool(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,(*(char *)elem==0) ? "false":"true");
}


void _StreamOut_char(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,*(char *)elem);
}


void _StreamOut_uchar(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,*(unsigned char *)elem);
}


void _StreamOut_short(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,*(short *)elem);
}


void _StreamOut_int(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,*(int *)elem);
}


void _StreamOut_long(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,*(long *)elem);
}


void _StreamOut_float(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,*(float *)elem);
}


void _StreamOut_double(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,*(double *)elem);
}


void _StreamOut_quad(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,*(_zquad *)elem);
}


void _StreamOut_string(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,(char *)elem);
}


void _StreamOut_fcomplex(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,(*((fcomplex *)elem)).re,
	  (*((fcomplex *)elem)).im);
}


void _StreamOut_dcomplex(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,(*((dcomplex *)elem)).re,
	  (*((dcomplex *)elem)).im);
}


void _StreamOut_qcomplex(_zfile outfile,void *elem,char *control) {
  fprintf(outfile->fptr,control,(*((qcomplex *)elem)).re,
	  (*((qcomplex *)elem)).im);
}


void _StreamIn_bool(_zfile infile,void *elem,char *control) {
  char val[5];
  _PRIV_TID_DECL;

  fscanf(infile->fptr,control,val);
  if (strcmp(val,"false") == 0) {
    *(unsigned char *)elem = 0;
  } else {
    *(unsigned char *)elem = 1;
  }
}


void _StreamIn_char(_zfile infile,void *elem,char *control) {
  int tempval;

  fscanf(infile->fptr,control,&tempval);
  *(char *)elem = tempval;
}


void _StreamIn_uchar(_zfile infile,void *elem,char *control) {
  unsigned int tempval;

  fscanf(infile->fptr,control,&tempval);
  *(unsigned char *)elem = tempval;
}


void _StreamIn_short(_zfile infile,void *elem,char *control) {
  fscanf(infile->fptr,control,(short *)elem);
}


void _StreamIn_int(_zfile infile,void *elem,char *control) {
  fscanf(infile->fptr,control,(int *)elem);
}


void _StreamIn_long(_zfile infile,void *elem,char *control) {
  fscanf(infile->fptr,control,(long *)elem);
}


void _StreamIn_float(_zfile infile,void *elem,char *control) {
  fscanf(infile->fptr,control,(float *)elem);
}


void _StreamIn_double(_zfile infile,void *elem,char *control) {
  fscanf(infile->fptr,control,(double *)elem);
}


void _StreamIn_quad(_zfile infile,void *elem,char *control) {
  fscanf(infile->fptr,control,(_zquad *)elem);
}


void _StreamIn_string(_zfile infile,void *elem,char *control) {
  fscanf(infile->fptr,control,(char *)elem);
}


void _StreamIn_fcomplex(_zfile outfile,void *elem,char *control) {
  fscanf2(outfile->fptr,control,&((*((fcomplex *)elem)).re),
	  &((*((fcomplex *)elem)).im));
}


void _StreamIn_dcomplex(_zfile outfile,void *elem,char *control) {
  fscanf2(outfile->fptr,control,&((*((dcomplex *)elem)).re),
	  &((*((dcomplex *)elem)).im));
}


void _StreamIn_qcomplex(_zfile outfile,void *elem,char *control) {
  fscanf2(outfile->fptr,control,&((*((qcomplex *)elem)).re),
	  &((*((qcomplex *)elem)).im));
}




