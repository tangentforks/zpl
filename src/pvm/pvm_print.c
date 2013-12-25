/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifndef _PVM_PRINT_OK_
#define _PVM_PRINT_OK_
#endif
#include "md_zinc.h"
#include "pvm_zlib.h"
#include "pvm3.h"


extern int ptid;


static void _PrintStringToConsole(FILE *outfile,char *thestring) {
  int tag;

  if (ptid == PvmNoParent) {
    fprintf(outfile,"%s",thestring);
    fflush(outfile);
  } else {
    if (outfile == stderr) {
      tag = _ERRPRINTTAG;
    } else {
      tag = _PRINTTAG;
    }
    pvm_initsend(PvmDataRaw);
    pvm_pkstr(thestring);
    pvm_send(ptid,tag);
    pvm_recv(ptid,tag);
  }
}


static void _WriteDataToConsole(const void *pointer,int size,int num_items,
				FILE *str) {
  int tag;

  if (ptid == PvmNoParent) {
    fwrite(pointer,size,num_items,str);
  } else {
    if (str == stderr) {
      tag = _ERRWRITETAG;
    } else {
      tag = _WRITETAG;
    }
    pvm_initsend(PvmDataRaw);
    pvm_pkbyte((char *)pointer,size,num_items);
    pvm_send(ptid,tag);
    pvm_recv(ptid,tag);
  }
}


static void _ReadDataFromConsole(void *pointer,int size,int num_items,
				 FILE *str) {
  int tot_size;

  if (ptid == PvmNoParent) {
    fread(pointer,size,num_items,str);
  } else {
    pvm_initsend(PvmDataRaw);
    tot_size = size * num_items;
    pvm_pkint(&tot_size,1,1);
    pvm_send(ptid,_READTAG);
    pvm_recv(ptid,_READTAG);
    pvm_upkbyte((char *)pointer,tot_size,1);
  }
}


static int _ScanFromConsole(const char *fmt,void *ptr,int size) {
  int retval;

  if (ptid == PvmNoParent) {
    retval = scanf(fmt,ptr);
  } else {
/*  _ZPL_PRINTF("Scanning %d bytes\n",size);*/
    pvm_initsend(PvmDataRaw);
    pvm_pkint(&size,1,1);
    pvm_pkbyte((char *)fmt,strlen(fmt),1);
    pvm_send(ptid,_SCANTAG);
    pvm_recv(ptid,_SCANTAG);
    if (size != 0) {
      pvm_upkbyte((char *)ptr,size,1);
/*    _ZPL_PRINTF("Client got %d\n",*(int *)ptr);*/
      retval = 1;
    } else {
      retval = 0;
    }
  }
  return retval;
}


static int _Scan2FromConsole(const char *fmt,void *ptr1,void *ptr2,int size1,
			     int size2) {
  int retval;

  if (ptid == PvmNoParent) {
    retval = scanf(fmt,ptr1,ptr2);
  } else {
/*  _ZPL_PRINTF("Scanning %d bytes\n",size);*/
    pvm_initsend(PvmDataRaw);
    pvm_pkint(&size1,1,1);
    pvm_pkint(&size2,1,1);
    pvm_pkbyte((char *)fmt,strlen(fmt),1);
    pvm_send(ptid,_SCAN2TAG);
    pvm_recv(ptid,_SCAN2TAG);
    if (size1 != 0 || size2 != 0) {
      pvm_upkbyte((char *)ptr1,size1,1);
      pvm_upkbyte((char *)ptr2,size2,1);
/*    _ZPL_PRINTF("Client got %d\n",*(int *)ptr);*/
      retval = 1;
    } else {
      retval = 0;
    }
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
