/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>

#ifndef _MPI_PRINT_OK_
#define _MPI_PRINT_OK_
#endif

#include "md_zlib.h"
#include "mpi.h"
#include "mpi_zlib.h"
#include "zlib.h"


void _HandlePrintRequests(int quiet_mode) {
  int outbufflen;
  char* outbuffer;
  char* fmtptr;
  int inbufflen;
  char* inbuffer;
  char* inbuffer2;
  FILE* outfile;
  int stop;
  MPI_Status status;
  int msgtag;
  int source;
  int size;
  int response = 1;
  int linenum;

  outbufflen = _PRINTFLEN*sizeof(char); 
  outbuffer = (char *)_zmalloc(outbufflen,"mpi print buffer");
  inbufflen = outbufflen;
  inbuffer = (char *)_zmalloc(inbufflen,"mpi in buffer");
  inbuffer2 = (char *)_zmalloc(inbufflen,"mpi in buffer2");

  stop = 0;
  while (!stop) {
    MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
    msgtag = status.MPI_TAG;
    source = status.MPI_SOURCE;
    size = MPI_Get_count(&status,MPI_BYTE,&size);

    switch (msgtag) {
    case _PRINTTAG:
    case _ERRPRINTTAG:
      MPI_Recv(outbuffer,outbufflen,MPI_CHAR,source,msgtag,MPI_COMM_WORLD,
	       &status);
      if (msgtag == _PRINTTAG) {
	outfile = stdout;
      } else {
	outfile = stderr;
      }
      fprintf(outfile,"%s",outbuffer);
      fflush(outfile);
      MPI_Send(&response,1,MPI_INT,source,msgtag,MPI_COMM_WORLD);
      break;
    case _WRITETAG:
    case _ERRWRITETAG:
      if (msgtag == _WRITETAG) {
	outfile = stdout;
      } else {
	outfile = stderr;
      }
      if (size > outbufflen) {
	outbufflen = size;
	outbuffer = (char *)_zrealloc(outbuffer,outbufflen, "");
      }
      MPI_Recv(outbuffer,outbufflen,MPI_BYTE,source,msgtag,MPI_COMM_WORLD,
	       &status);
      fwrite(outbuffer,size,1,outfile);
      fflush(outfile);
      MPI_Send(&response,1,MPI_INT,source,msgtag,MPI_COMM_WORLD);
      break;
    case _SCANTAG:
      {
	int elemsize;

	MPI_Recv(outbuffer,outbufflen,MPI_BYTE,source,msgtag,MPI_COMM_WORLD,
		 &status);
	elemsize = *(int*)outbuffer;
	fmtptr = (char*)(((int*)outbuffer)+1);
	outbuffer[size] = '\0';
/*	printf("Console is scanning %d bytes using %s\n",elemsize,outbuffer);*/
	scanf(fmtptr,inbuffer);
/*	printf("Got %d\n",*(int *)inbuffer);*/
	MPI_Send(inbuffer,elemsize,MPI_BYTE,source,msgtag,MPI_COMM_WORLD);
      }
      break;
    case _SCAN2TAG:
      {
	int elemsize;
	int elemsize2;

	elemsize = *(int*)outbuffer;
	elemsize2 = *(((int*)outbuffer)+1);
	fmtptr = (char*)(((int*)outbuffer)+2);
	outbuffer[size] = '\0';
/*	printf("Console is scanning %d bytes using %s\n",elemsize,outbuffer);*/
	scanf(fmtptr,inbuffer,inbuffer2);
/*	printf("Got %d\n",*(int *)inbuffer);*/
	memcpy(inbuffer+elemsize,inbuffer2,elemsize2);
	MPI_Send(inbuffer,elemsize+elemsize2,MPI_BYTE,source,msgtag,
		 MPI_COMM_WORLD);
      }
      break;
    case _READTAG:
      MPI_Recv(&size,1,MPI_INT,source,msgtag,MPI_COMM_WORLD,&status);
      if (size > inbufflen) {
	inbufflen = size;
	inbuffer = (char *)_zrealloc(inbuffer,inbufflen, "");
      }
      fread(inbuffer,size,1,stdin);
      MPI_Send(inbuffer,size,MPI_BYTE,source,msgtag,MPI_COMM_WORLD);
      break;
    case _DEADTAG:
      MPI_Recv(&source,1,MPI_INT,source,msgtag,MPI_COMM_WORLD,&status);
      fprintf(stderr,"Task %d died -- killing all others\n",source);
      _ZPL_halt(-1);
      stop=1;
      break;
    case _HALTTAG:
      MPI_Recv(&linenum,1,MPI_INT,source,msgtag,MPI_COMM_WORLD,&status);
      if ((quiet_mode <= 0) && (linenum != 0)) {
	fprintf(stderr,"halt at line %d reached\n",linenum);
      }
      _ZPL_halt(-1);
      stop=1;
      break;
    case _DONETAG:
      MPI_Recv(&response,1,MPI_INT,source,msgtag,MPI_COMM_WORLD,&status);
      MPI_Send(&response,1,MPI_INT,source,msgtag,MPI_COMM_WORLD);
      _PROCESSORS--;
      if (_PROCESSORS == 0) {
	stop = 1;
      }
      break;
    default:
      fprintf(stderr,"Unknown message type received by console: %d\n",msgtag);
      break;
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (quiet_mode <= 0) {
    printf("All tasks terminated -- exiting\n");
  }

  MPI_Finalize();
  exit(0);
}

