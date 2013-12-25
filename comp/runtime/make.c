/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/***************************************************************\
* make.c is used to output a Makefile for use in constructing   *
* the executable.  I don't know what the future of this idea    *
* will be, but for now it creates an easy method of building    *
* the executable for multiple targets.                          *
\***************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../include/error.h"
#include "../include/db.h"
#include "../include/getopt.h"
#include "../include/runtime.h"

#define FILEPATHLEN 256


/***************************************************************\
* GenerateMakefile() is used to generate the Makefile.  It      *
* opens the file, writes commands to it, and then closes it.    *
\***************************************************************/

static void AddIs(FILE *makefile) {
  char *arg, ch;
  int flag = 0;

  reset_args();
  arg = get_arg(&flag,&ch);
  while ((arg = get_arg(&flag,&ch)) != NULL) {
    if (flag) {
      switch(ch) {
      case 'I':
	if (*arg == '\0') {
	  arg = get_arg(&flag,&ch);
	}
	if (arg != NULL) {
	  fprintf(makefile," -I\\\"%s\\\"",arg);
	}
	break;
      default:
	break;
      }
    }
  }
}


static void AddOsAndCs(FILE *makefile) {
  char *arg, ch;
  int flag = 0;
  char *eop;

  reset_args();
  arg = get_arg(&flag,&ch);
  while ((arg = get_arg(&flag,&ch)) != NULL) {
    if (flag) {
    } else {
      if (arg[(strlen(arg)-2)] == '.') {
	if (arg[(strlen(arg)-1)] == 'c') {
	  arg[(strlen(arg)-1)] = 'o';
	  eop = strrchr(arg,'/');
	  if (eop != NULL) {
	    eop++;
	    fprintf(makefile," \\\n\t%s",eop);
	    arg[(strlen(arg)-1)] = 'c';
	  } else {
	    fprintf(makefile," \\\n\t%s",arg);
	  }
	} else if (arg[(strlen(arg)-1)] == 'o') {
	  fprintf(makefile," \\\n\t%s",arg);
	}
      }
    }
  }
}


static void AddBigLs(FILE *makefile) {
  char *arg, ch;
  int flag = 0;

  reset_args();
  arg = get_arg(&flag,&ch);
  while ((arg = get_arg(&flag,&ch)) != NULL) {
    if (flag) {
      switch(ch) {
      case 'L':
	if (*arg == '\0') {
	  arg = get_arg(&flag,&ch);
	}
	if (arg != NULL) {
	  fprintf(makefile," -L\\\"%s\\\"",arg);
	}
	break;
      default:
	break;
      }
    }
  }
}


static void AddLilLs(FILE *makefile) {
  char *arg, ch;
  int flag = 0;

  reset_args();
  arg = get_arg(&flag,&ch);
  while ((arg = get_arg(&flag,&ch)) != NULL) {
    if (flag) {
      switch(ch) {
      case 'l':
	if (*arg == '\0') {
	  arg = get_arg(&flag,&ch);
	}
	if (arg != NULL) {
	  fprintf(makefile," -l%s",arg);
	}
	break;
      default:
	break;
      }
    }
  }
}


void GenerateZMakefile(void) {
  FILE *zmakefile;
  char buffer[FILEPATHLEN];
  mode_t cmask;

  sprintf(buffer,"%s/zmake",DESTPATH);
  if ((zmakefile=fopen(buffer,"w"))==NULL) {
    USR_FATAL(NULL, "Cannot open file '%s'",buffer);
  }

  fprintf(zmakefile, "#!/bin/sh\n\n");

  fprintf(zmakefile, "#\n");
  fprintf(zmakefile, "# zc automatically generated script to build %s binaries\n", trunc_in_filename);
  fprintf(zmakefile, "#\n\n");
  
  fprintf(zmakefile, "# quote all args in string QARGS\n");
  fprintf(zmakefile, "QARGS=\"\"\n");
  fprintf(zmakefile, "while [ $# -gt 0 ] ; do\n");
  fprintf(zmakefile, "\tQARGS=\"$QARGS\\\"$1\\\" \"\n");
  fprintf(zmakefile, "\tshift\n");
  fprintf(zmakefile, "done\n\n");

  fprintf(zmakefile, "PROG=%s\n", trunc_in_filename);
  fprintf(zmakefile, "ZC_INCLS=\"");
  AddIs(zmakefile);
  fprintf(zmakefile, "\"\n");
  fprintf(zmakefile, "ZC_ADDITIONAL_OBJS=\"");
  AddOsAndCs(zmakefile);
  fprintf(zmakefile, "\"\n");
  fprintf(zmakefile, "ZC_LIBPATHS=\"");
  AddBigLs(zmakefile);
  fprintf(zmakefile, "\"\n");
  fprintf(zmakefile, "ZC_LIBS=\"");
  AddLilLs(zmakefile);
  fprintf(zmakefile, "\"\n");
  if (debug) {
    fprintf(zmakefile,"CFLAGS=\"-g\"\n");
  }
  fprintf(zmakefile,"\n");

  fprintf(zmakefile, "export PROG\n");
  fprintf(zmakefile, "export ZC_INCLS\n");
  fprintf(zmakefile, "export ZC_ADDITIONAL_OBJS\n");
  fprintf(zmakefile, "export ZC_LIBPATHS\n");
  fprintf(zmakefile, "export ZC_LIBS\n");
  if (debug) {
    fprintf(zmakefile,"export CFLAGS\n");
  }
  fprintf(zmakefile,"\n");

  fprintf(zmakefile, "# ZPLHOME must be set to something non-null\n");
  fprintf(zmakefile, "if [ \"$ZPLHOME\" = \"\" ] ; then\n");
  fprintf(zmakefile, "\techo \"ERROR: Environment variable ZPLHOME must be set to ZPL home directory.\"\n");
  fprintf(zmakefile, "\techo \"Aborting.\"\n");
  fprintf(zmakefile, "\texit 2\n");
  fprintf(zmakefile, "fi\n\n");

  fprintf(zmakefile, "# ZMAKEBASE is the script used to actually compile PROG\n");
  fprintf(zmakefile, "ZMAKEBASE=\"$ZPLHOME/etc/zmake.base\"\n\n");

  fprintf(zmakefile, "# file $ZMAKEBASE must exist and be readable\n");
  fprintf(zmakefile, "if [ ! -r \"$ZMAKEBASE\" ] ; then\n");
  fprintf(zmakefile, "\techo \"ERROR: Unable to read zmake.base file.\"\n");
  fprintf(zmakefile, "\techo \"       ($ZMAKEBASE)\"\n");
  fprintf(zmakefile, "\techo \"       Make sure environment variable ZPLHOME is properly set.\"\n");
  fprintf(zmakefile, "\techo \"Aborting.\"\n");
  fprintf(zmakefile, "\texit 2\n");
  fprintf(zmakefile, "fi\n\n");

  fprintf(zmakefile, "# run the ZMAKEBASE script to actually compile PROG; pass along any arguments\n");
  fprintf(zmakefile, "COMMAND=\"\\\"$ZMAKEBASE\\\" $QARGS\"\n");
  fprintf(zmakefile, "eval $COMMAND\n\n");

  fprintf(zmakefile, "exit $?\n");

  fclose(zmakefile);

  /* make zmake an executable file */
  cmask = umask((mode_t)0);
  umask(cmask);

  cmask = cmask ^ (S_IRWXU | S_IRWXG | S_IRWXO);

  chmod(buffer, cmask | S_IXUSR | S_IXGRP | S_IXOTH);

  /* the warning below is unnec., for if the file exists it has been 
   * chmoded already
   * if (chmod(buffer, cmask | S_IXUSR | S_IXGRP | S_IXOTH)) {
   *   USR_WARN(NULL, "Cannot chmod file '%s'",buffer);
   * }
   */
}

