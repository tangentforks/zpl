/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#include "pvm3.h"

#ifndef _PVM_PRINT_OK_
#define _PVM_PRINT_OK_
#endif

#include "md_zinc.h"
#include "start.h"
#include "pvm_zlib.h"
#include "zlib.h"

int ptid=PvmNoParent;

static int _INITPROCS=0;
static int _INITGRID[_MAXRANK];
static int _quiet_mode=0;

int main(int argc,char *argv[]) {
  char buffer[256];
#ifndef PATH_MAX
  #define PATH_MAX 512
#endif
  char pwd[PATH_MAX];
  char group[256];
  int i;
  int stop;
  int *tids;
  int cc;
  char *exename;
  struct timeval tv;
  struct timezone tz;
  char **newargv;
  int outbufflen;
  char *outbuffer;
  int inbufflen;
  char *inbuffer;
  char *inbuffer2;
  int size;
  int msgid;
  int msgtag;
  int sender;
  int linenum;
  FILE *outfile;
  int mytid;

  mytid = pvm_mytid();
  if (mytid < 0) {
    fprintf(stderr,"pvmd doesn't seem to be running.\n");
    exit(1);
  }
  outbufflen = _PRINTFLEN*sizeof(char); 
  outbuffer = (char *)malloc(outbufflen);
  inbufflen = outbufflen;
  inbuffer = (char *)malloc(inbufflen);
  inbuffer2 = (char *)malloc(inbufflen);

  /* Get the arguments parsed properly to find out how many threads to spawn */
  _ParseArgs(argc,argv,1);

  _QueryProcInfo(&_INITPROCS,_INITGRID);
  _QueryQuietMode(&_quiet_mode);
  

  /* pull the executable name out of the argv[0] place.  Pluck off path */
  
  exename=strrchr(argv[0],'/');
  if (exename==NULL) {
    exename=argv[0];
  } else {
    exename++;
  }


  /* start out by trying to just spawn the program with no path */

  sprintf(buffer,"%s_real",exename);


  /* make new arg vector, preserving old arguments */

  newargv = (char **)malloc((argc+NUMNEWARGS)*sizeof(char *));
  for (i=1;i<=argc;i++) {
    newargv[i+NUMNEWARGS-1]=argv[i];
  }


  /* make a groupname composed to exename + time stamp */

  gettimeofday(&tv,&tz);
    
  sprintf(group,"%d",INT_MAX);


  sprintf(group,"%s@%d:%d",exename,(int)tv.tv_sec,(int)tv.tv_usec);


  /* allocate a new buffer just to be safe... */
  newargv[0] = group;
  newargv[1] = pwd;

  tids = (int *)malloc(_INITPROCS * sizeof(int));

  /*
  printf("%s ",buffer);
  for (i=1;i<argc+NUMNEWARGS-1;i++) {
    printf("%s ",newargv[i]);
  }
  printf("\n");
  */

#ifdef PVM_DEBUG
  cc = pvm_spawn(buffer,newargv,PvmTaskDebug,(char *)0,_INITPROCS,tids);
#else
  cc = pvm_spawn(buffer,newargv,0,(char *)0,_INITPROCS,tids);
#endif


  /* if the default spawn didn't work, use smarter techniques */

  if ((cc == 0) && (tids[0] == PvmNoFile)) {
    /* find the current directory so that the children can run in it... */
    /*** if ZPL_PVM_BIN is set, use it ***/
    if (getenv("ZPL_PVM_BIN")) {
      sprintf(pwd,"%s", getenv("ZPL_PVM_BIN"));
    } else {
      getcwd(pwd,PATH_MAX);
    }
    sprintf(buffer,"%s/%s_real",pwd,exename);
#ifdef PVM_DEBUG
    cc = pvm_spawn(buffer,newargv,PvmTaskDebug,(char *)0,_INITPROCS,tids);
#else
    cc = pvm_spawn(buffer,newargv,0,(char *)0,_INITPROCS,tids);
#endif
  }

  if (cc != _INITPROCS) {
    fprintf(stderr,"Error spawning %s ",buffer);
    if (cc < 0) {
      fprintf(stderr,"(return code: %d)\n",cc);
    } else {
      fprintf(stderr,"(%d tasks spawned)\n",cc);
      for (i=0;i<=cc;i++) {
	fprintf(stderr,"   task %d : %d\n",i,tids[i]);
      }
    }
    pvm_exit();
    exit(1);
  }
   
  pvm_notify(PvmTaskExit,_DEADTAG,_INITPROCS,tids);
  stop = 0;
  while (!stop) {
    msgid = pvm_recv(-1,-1);
    pvm_bufinfo(msgid,&size,&msgtag,&sender);
      
    switch (msgtag) {
    case _PRINTTAG:
    case _ERRPRINTTAG:
      if (msgtag == _PRINTTAG) {
	outfile = stdout;
      } else {
	outfile = stderr;
      }
      pvm_upkstr(outbuffer);
      fprintf(outfile,"%s",outbuffer);
      fflush(outfile);
      pvm_initsend(PvmDataRaw);
      pvm_send(sender,msgtag);
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
	outbuffer = (char *)realloc(outbuffer,outbufflen);
      }
      pvm_upkbyte(outbuffer,size,1);
      fwrite(outbuffer,size,1,outfile);
      fflush(outfile);
      pvm_initsend(PvmDataRaw);
      pvm_send(sender,msgtag);
      break;
    case _SCANTAG:
      {
	int elemsize;
	pvm_upkint(&elemsize,1,1);
	pvm_upkbyte(outbuffer,size-sizeof(int),1);
	outbuffer[size-sizeof(int)] = '\0';
/*	printf("Console is scanning %d bytes using %s\n",elemsize,outbuffer);*/
	scanf(outbuffer,inbuffer);
/*	printf("Got %d\n",*(int *)inbuffer);*/
	pvm_initsend(PvmDataRaw);
	if (elemsize != 0) {
	  pvm_pkbyte(inbuffer,elemsize,1);
	}
	pvm_send(sender,_SCANTAG);
      }
      break;
    case _SCAN2TAG:
      {
	int elemsize;
	int elemsize2;
	pvm_upkint(&elemsize,1,1);
	pvm_upkint(&elemsize2,1,1);
	pvm_upkbyte(outbuffer,size-2*sizeof(int),1);
	outbuffer[size-sizeof(int)] = '\0';
/*	printf("Console is scanning %d bytes using %s\n",elemsize,outbuffer);*/
	scanf(outbuffer,inbuffer,inbuffer2);
/*	printf("Got %d\n",*(int *)inbuffer);*/
	pvm_initsend(PvmDataRaw);
	if (elemsize != 0) {
	  pvm_pkbyte(inbuffer,elemsize,1);
	  pvm_pkbyte(inbuffer2,elemsize2,1);
	}
	pvm_send(sender,_SCAN2TAG);
      }
      break;
    case _READTAG:
      pvm_upkint(&size,1,1);
      if (size > inbufflen) {
	inbufflen = size;
	inbuffer = (char *)realloc(inbuffer,inbufflen);
      }
      fread(inbuffer,size,1,stdin);
      pvm_initsend(PvmDataRaw);
      pvm_pkbyte(inbuffer,size,1);
      pvm_send(sender,msgtag);
      break;
    case _DEADTAG:
      pvm_upkint(&sender,1,1);
      for (i=0;i<_INITPROCS;i++) {
	if (tids[i]==sender) {
	  fprintf(stderr,"Task %d died -- killing all others\n",i);
	} else {
	  pvm_sendsig(tids[i],SIGKILL);
	}
      }
      _INITPROCS--;
      stop=1;
      break;
    case _HALTTAG:
      pvm_upkint(&linenum,1,1);
      if ((_quiet_mode<=0) && (linenum != 0)) {
	fprintf(stderr,"halt at line %d reached\n",linenum);
      }
      for (i=0;i<_INITPROCS;i++) {
	pvm_sendsig(tids[i],SIGKILL);
      }
      stop=1;
      break;
    case _DONETAG:
      pvm_initsend(PvmDataRaw);
      pvm_send(sender,_DONETAG);
      stop = 1;
      break;
    default:
      fprintf(stderr,"Unknown message type received by console: %d\n",msgtag);
      break;
    }
  }
  for (i=0;i<_INITPROCS;i++) {
    pvm_recv(-1,_DEADTAG);
  }
  if (_quiet_mode <= 0) {
    printf("All tasks terminated -- exiting\n");
  }

  pvm_exit();
  exit(0);
}

