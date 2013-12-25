/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#ifndef _MPI_PRINT_OK_
#define _MPI_PRINT_OK_
#endif
#include "mpi.h"
#include "mpi_zlib.h"
#include "zlib.h"


static void _PrintStringToConsole(FILE *outfile,char *thestring) {
  int tag;
  int response;
  MPI_Status status;

  if (_able_to_print) {
    fprintf(outfile,"%s",thestring);
    fflush(outfile);
  } else {
    if (outfile == stderr) {
      tag = _ERRPRINTTAG;
    } else {
      tag = _PRINTTAG;
    }
    MPI_Send(thestring,strlen(thestring)+1,MPI_CHAR,_print_node,tag,
	     MPI_COMM_WORLD);
    MPI_Recv(&response,1,MPI_INT,_print_node,tag,MPI_COMM_WORLD,&status);
  }
}


static void _WriteDataToConsole(const void *pointer,int size,int num_items,
				FILE *str) {
  int tag;
  int response;
  MPI_Status status;

  if (_able_to_print) {
    fwrite(pointer,size,num_items,str);
  } else {
    if (str == stderr) {
      tag = _ERRWRITETAG;
    } else {
      tag = _WRITETAG;
    }

    MPI_Send((void*)pointer,size,MPI_BYTE,_print_node,tag,MPI_COMM_WORLD);
    MPI_Recv(&response,1,MPI_INT,_print_node,tag,MPI_COMM_WORLD,&status);
  }
}


static void _ReadDataFromConsole(void *pointer,int size,int num_items,
				 FILE *str) {
  int tot_size;
  MPI_Status status;

  if (_able_to_print) {
    fread(pointer,size,num_items,str);
  } else {
    tot_size = size * num_items;
    MPI_Send(&tot_size,1,MPI_INT,_print_node,_READTAG,MPI_COMM_WORLD);
    MPI_Recv(pointer,tot_size,MPI_BYTE,_print_node,_READTAG,MPI_COMM_WORLD,
	     &status);
  }
}


static int _ScanFromConsole(const char *fmt,void *ptr,int size) {
  int retval;
  int buffsize;
  char* buff;
  int* buffptr;
  MPI_Status status;

  if (_able_to_print) {
    retval = scanf(fmt,ptr);
  } else {
/*  _ZPL_PRINTF("Scanning %d bytes\n",size);*/
    buffsize = ((strlen(fmt)+1)*sizeof(char)) + sizeof(int);
    buff = _zmalloc(buffsize,"mpi console scan buffer");
    buffptr = (int*)buff;

    *buffptr = size;
    buffptr++;
    strcpy((char*)buffptr,fmt);

    MPI_Send(buff,buffsize,MPI_BYTE,_print_node,_SCANTAG,MPI_COMM_WORLD);
    MPI_Recv(ptr,size,MPI_BYTE,_print_node,_SCANTAG,MPI_COMM_WORLD,&status);

    if (size != 0) {
      retval = 1;
    } else {
      retval = 0;
    }

    _zfree(buff,"mpi console scan buffer");
  }
  return retval;
}


static int _Scan2FromConsole(const char *fmt,void *ptr1,void *ptr2,int size1,
			     int size2) {
  int retval;
  int buffsize;
  char* buff;
  int* buffptr;
  MPI_Status status;
  char* inbuff;

  if (_able_to_print) {
    retval = scanf(fmt,ptr1,ptr2);
  } else {
    buffsize = ((strlen(fmt)+1)*sizeof(char)) + 2*sizeof(int);
    buff = _zmalloc(buffsize,"mpi console scan2 buffer");
    buffptr = (int*)buff;

    *buffptr = size1;
    buffptr++;
    *buffptr = size2;
    buffptr++;
    strcpy((char*)buffptr,fmt);

    MPI_Send(buff,buffsize,MPI_BYTE,_print_node,_SCAN2TAG,MPI_COMM_WORLD);

    inbuff = _zmalloc((size1 + size2)*sizeof(char),
			"mpi console scan2 inbuffer");

    MPI_Recv(inbuff,size1+size2,MPI_BYTE,_print_node,_SCAN2TAG,MPI_COMM_WORLD,
	     &status);
    memcpy(ptr1,inbuff,size1);
    memcpy(ptr2,inbuff+size1,size2);

    if (size1 != 0 || size2 !=0) {
      retval = 1;
    } else {
      retval = 0;
    }

    _zfree(buff,"mpi console scan2 buffer");
    _zfree(inbuff,"mpi console scan2 inbuffer");
  }
  return retval;
}


int _ZPL_FPRINTF(FILE *outfile,const char *fmt,...) {
  va_list args;
  char buffer[_PRINTFLEN];
  int retval;

  va_start(args,fmt);

  retval=vsprintf(buffer,fmt,args);
  va_end(args);

  if (outfile == stdout || outfile == stderr) {

    _PrintStringToConsole(outfile,buffer);

  } else {
    fprintf(outfile,"%s",buffer);
  }
  return retval;
}


int _ZPL_PRINTF(const char *fmt,...) {
  va_list args;
  char buffer[_PRINTFLEN];
  int retval;

  va_start(args,fmt);

  retval=vsprintf(buffer,fmt,args);

  va_end(args);

  _PrintStringToConsole(stdout,buffer);
  return retval;
}


int _ZPL_FSCANF(FILE *infile,const char *fmt,void *ptr,int size) {
  int retval;

  if (infile == stdin) {
    retval = _ScanFromConsole(fmt,ptr,size);
  } else {
    retval = fscanf(infile,fmt,ptr);
  }

  return retval;
}


int _ZPL_FSCANF2(FILE *infile,const char *fmt,void *ptr1,void *ptr2,int size1,
		 int size2) {
  int retval;

  if (infile == stdin) {
    retval = _Scan2FromConsole(fmt,ptr1,ptr2,size1,size2);
  } else {
    retval = fscanf(infile,fmt,ptr1,ptr2);
  }

  return retval;
}


int _ZPL_SCANF(const char *fmt,void *ptr,int size) {
  return _ScanFromConsole(fmt,ptr,size);
}


int _ZPL_FWRITE(const void *pointer,int size,int num_items,FILE *str) {
  if (str == stdout || str == stderr) {
    _WriteDataToConsole(pointer,size,num_items,str);
    return num_items;
  } else {
    return fwrite(pointer,size,num_items,str);
  }
}


int _ZPL_FREAD(void *pointer,int size,int num_items,FILE *str) {
  if (str == stdin) {
    _ReadDataFromConsole(pointer,size,num_items,str);
    return num_items;
  } else {
    return fread(pointer,size,num_items,str);
  }
}
