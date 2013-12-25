/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "md_zinc.h"

#include "md_zlib.h"
#include "priv_access.h"
#include "zc_proto.h"
#include "zerror.h"
#include "zlib.h"
#include "checkpoint.h"


void _ParseCfgFile(char *filename) {
  FILE *cfgfile;
  char buffer[80];
  char *equals;
  _PRIV_TID_DECL;

  _ResetToken();
  if (_INDEX!=0) {
    _RecvToken();
  }
  if (*filename!='\0') {
    cfgfile=fopen(filename,"r");
    if (cfgfile==NULL) {
      printf("Error -- could not open config file %s\n",filename);
      return;
    }
    while (fscanf(cfgfile,"%s",buffer)==1) {
      equals=strchr(buffer,'=');
      if (equals!=NULL) {
	*equals='\0';
	_AssignToConfig(buffer,equals+1);
      }
    }
  }
  fclose(cfgfile);
  _PassToken();
  _BarrierSynch();
}


void _ParseFileArgs(int argc,char *argv[]) {
  int i;

  for (i=0;i<argc;i++) {
    if (*(argv[i]) == '-') {
      switch (*(argv[i]+1)) {
      case 'f':
      case 'F':
        _ParseCfgFile(argv[i]+2);
        break;
      default:
        break;
      }
    }
  }
}


void _ParseSetArgs(int argc,char *argv[]) {
  char *equals;
  int i;
  _PRIV_TID_DECL;

  for (i=0;i<argc;i++) {
    if (*(argv[i]) == '-') {
      switch (*(argv[i]+1)) {
      case 's':
      case 'S':
        equals=strchr(argv[i]+2,'\0');
        if (equals!=NULL && *(equals+1)!='\0') {
	  _AssignToConfig(argv[i]+2,(equals+1));
	} else {
	  _RT_ALL_FATAL1("Missing value for config var %s",argv[i]+2);
        }
        break;
      default:
        break;
      }
    }
  }
}


void _ParseConfigArgs(int argc,char *argv[]) {
  _PRIV_TID_DECL;

  _PRIV_ONCE_BEGIN;
  _ParseFileArgs(argc,argv);
  _ParseSetArgs(argc,argv);
  _ParseCheckpointArgs(argc,argv);
  _PRIV_ONCE_END;
}
