/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "md_zinc.h"

#include "access.h"
#include "ensemble.h"
#include "file_io.h"
#include "md_zlib.h"
#include "region.h"
#include "start.h"
#include "zerror.h"
#include "zlib.h"

_PRIV_DECL(, int, _io_in_ens);

_zfile_struct _zout;
_zfile_struct _zin;
_zfile_struct _zerr;
_zfile_struct _znull;
_zfile zout = &(_zout);
_zfile zin  = &(_zin);
_zfile zerr = &(_zerr);
_zfile znull = &(_znull);

void _InitIO() {
  _zout.fptr = stdout;
  _zin.fptr = stdin;
  _zerr.fptr = stderr;
  _znull.fptr = NULL;
  _zout.pos = 0;
  _zin.pos = 0;
  _zerr.pos = 0;
  _znull.pos = 0;
  _InitEnsIO();
}


int _EOFile(_zfile thefile) {
  char nextchar;
  int retval;

  if (_INDEX == 0) {
    /* see if already at EOF */
    if (feof(thefile->fptr)) {
      retval = 1;
    } else {
      /* otherwise, see if we can read a character */
      nextchar = fgetc(thefile->fptr);
      if (feof(thefile->fptr)) {
	retval = 1;
      } else {
	ungetc(nextchar, thefile->fptr);
	retval = 0;
      }
    }
  }
  _BroadcastSimple(_DefaultGrid, &retval, sizeof(int), 0, 0);

  return retval;
}


_zfile _OpenFile(char *filename,char *type) {
  FILE *fp;
  _zfile retval;
  int quiet=0;
  _PRIV_TID_DECL;

  if (filename == NULL || strlen(filename) == 0) {
    retval = znull;
  } else {
    fp = fopen(filename,type);
    if (fp == NULL) {
      _QueryQuietMode(&quiet);
      if (!quiet) {
	_RT_ANY_WARN2("Problem opening file \"%s\": %d",filename,errno);
      }
      retval = znull;
    } else {
      retval = (_zfile)_ZPL_SYM_ALLOC(sizeof(_zfile_struct),
				      "file descriptor");
      retval->fptr = fp;
      retval->pos = 0;
    }
  }
  _BarrierSynch();

  return retval;
}


int _CloseFile(_zfile *stream) {
  int retval;

  if (*stream == zin) {
    _RT_ALL_WARN0("close(zin) called");
    return 0;
  }
  if (*stream == zout) {
    _RT_ALL_WARN0("close(zout) called");
    return 0;
  }
  if (*stream == zin) {
    _RT_ALL_WARN0("close(zerr) called");
    return 0;
  }

  _BarrierSynch();

  retval = fclose((*stream)->fptr);
  
  (*stream)->fptr = NULL;
  _ZPL_SYM_FREE(*stream,"file descriptor");
  *stream = NULL;

  return retval;
}


void bind_write_func(_array_fnc ens,_filepointfn fn) {
  ens->Writefn = fn;
}


void bind_read_func(_array_fnc ens,_filepointfn fn) {
  ens->Readfn = fn;
}


void unbind_write_func(_array_fnc ens) {
  ens->Writefn = NULL;
}


void unbind_read_func(_array_fnc ens) {
  ens->Readfn = NULL;
}




