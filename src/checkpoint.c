/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "md_zlib.h"
#include "zlib.h"
#include "ensemble.h"
#include "checkpoint.h"
#include "start.h"
#include <string.h>
#include <errno.h>

extern char entry_name[];

char *_chkdata;
char *_chktemp;

unsigned long _chksize;
unsigned long _fullchksize;

int _recovering = 0;
int _checkpointing = 0;
char _recoverfilename[128];

char _recovered_region_name[] = "recovered region";

_chkdatalist* _cdl;
_chkdatalist* _cdlptr;

void _ParseCheckpointArgs(int argc,char *argv[])
{
  int i;

  for (i = 0; i < argc; i++) {
    if ((strncmp(argv[i], "-x", 2) == 0) ||
	(strncmp(argv[i], "-X", 2) == 0)) {
      _recovering = 1;
      sprintf(_recoverfilename, "%s.recover.%d", entry_name, _INDEX);
    }
  }
}

unsigned long
_getalignedsize(unsigned long size) {
  unsigned long newsize = _BYTE_ALIGNMENT_;
  if (size == 0) {
    return 0;
  }
  while (size > newsize) {
    newsize += _BYTE_ALIGNMENT_;
  }
  return newsize;
}

void _AddString(char *s) {
  _chksize += _getalignedsize(sizeof(unsigned long));
  if (s != NULL) {
    _chksize += _getalignedsize((strlen(s)+1)*sizeof(char));
  } else {
    _chksize += _getalignedsize(1*sizeof(char));
  }
}

void _SaveString(char *s) {
  unsigned long _chkjump;
  if (s != NULL) {
    _chkjump = *((unsigned long *) _chktemp) =
      _getalignedsize((strlen(s)+1)*sizeof(char));
  } else {
    _chkjump = *((unsigned long *) _chktemp) =
      _getalignedsize(1*sizeof(char));
  }
  _chktemp += _getalignedsize(sizeof(unsigned long));
  if (s != NULL) {
    strcpy(_chktemp, s);
  } else {
    *((char *)_chktemp) = 0;
  }
  _chktemp += _chkjump;
}

void _RecoverString(char *s)  {
  unsigned long _chkjump;
  _chkjump = *((unsigned long *) _chktemp);
  _chktemp += _getalignedsize(sizeof(unsigned long));
  if (*((char *) _chktemp) != 0) {
    strcpy(s, (char *) _chktemp);
  } else {
    s = NULL;
  }
  _chktemp += _chkjump;
}

static unsigned long _SizeArray(_array A)
{
  return (A == NULL) ? 0 : _ARR_DATA_SIZE(A);
}

void _AddArray(_array A) {
  _chksize += _getalignedsize(sizeof(unsigned long)) +
    _getalignedsize(_SizeArray(A));
}

void _SaveArray(_array A)
{
  *((unsigned long *) _chktemp) = _ARR_DATA_SIZE(A);
  _chktemp += _getalignedsize(sizeof(unsigned long));
  memcpy(_chktemp, _ARR_DATA(A), _ARR_DATA_SIZE(A));
  _chktemp += _getalignedsize(_ARR_DATA_SIZE(A));
}

void _RecoverArray(_array_nc A)
{
  _ARR_DATA_SIZE(A) = *(unsigned long *) _chktemp;
  _chktemp += _getalignedsize(sizeof(unsigned long));
  memcpy(_ARR_DATA(A), _chktemp, _ARR_DATA_SIZE(A));
  _chktemp += _getalignedsize(_ARR_DATA_SIZE(A));
}

static unsigned long _SizeRegion(_region R) {
  return (R == NULL) ? 0 : sizeof(_reg_info);
}

void _AddRegion(_region R) {
  _chksize += _getalignedsize(sizeof(unsigned long)) +
    _getalignedsize(_SizeRegion(R));
}

void _SaveRegion(_region R) {
  *((unsigned long *) _chktemp) = _getalignedsize(_SizeRegion(R));
  _chktemp += _getalignedsize(sizeof(unsigned long));

  if (R != NULL) {
    *((_reg_info*) _chktemp) = *R;
    _chktemp += _getalignedsize(_SizeRegion(R));
  }
}

void _RecoverRegion(_region_nc R) {
  unsigned long _chkjump = *((unsigned long *) _chktemp);
  _chktemp += _getalignedsize(sizeof(unsigned long));

  if (_chkjump != 0) {
    *R = *(_reg_info*) _chktemp;
    _REG_LABEL(R) = _recovered_region_name;
    _chktemp += _chkjump;
  }
}

void _AddRMStack(_rmsframe *R) {
  _AddRegion(R->reg);
  /*** don't save masks yet ***/
  _AddBasic(int);
}

void _SaveRMStack(_rmsframe *R) {
  _SaveRegion(R->reg);
  /*** don't save masks yet ***/
  _SaveBasic(R->withflag, int);
}

void _RecoverRMStack(_rmsframe *R) {
  unsigned long _chkjump = *((unsigned long *) _chktemp);
  _chktemp += _getalignedsize(sizeof(unsigned long));

  if (_chkjump != 0) {
    R->reg = (_region_nc) _zmalloc(sizeof(_reg_info), "");
    *(R->reg) = *(_reg_info*) _chktemp;
    _REG_LABEL(R->reg) = _recovered_region_name;
    _chktemp += _chkjump;
  } else {
    R->reg = NULL;
  }

  R->mask = NULL; /*** don't save masks yet ***/
  _RecoverBasic(R->withflag, int);


}

static void _reverse(_chkdatalist** _cdl)
{
  _chkdatalist* first;
  _chkdatalist* rest;

  if (*_cdl == NULL) return;
  first = *_cdl;
  rest = first->next;
  if (rest == NULL) return;
  _reverse(&rest);
  first->next->next = first;
  first->next = NULL;
  *_cdl = rest;
}

void _CompleteSave(void) {
  FILE* _F;
  char *_ckpt_location = _QueryCheckpointLocation();
  int filenamelen;
  char *name, *name1, *name2;

  sprintf(_recoverfilename, "%s.recover.%d", entry_name, _INDEX);

  /*** filenamelen = strlen(_ckpt_location)   +         ***/
  /***               "/" = 1                  +         ***/
  /***               strlen(_recoverfilename) +         ***/
  /***               ".tmp" or ".old" = 4     +         ***/
  /***               \0 = 1                             ***/
  filenamelen = strlen(_ckpt_location)+1+strlen(_recoverfilename)+4+1;
  name = (char *) _zmalloc(filenamelen*sizeof(char), "checkpoint filename 0");
  name1 = (char *) _zmalloc(filenamelen*sizeof(char), "checkpoint filename 1");
  name2 = (char *) _zmalloc(filenamelen*sizeof(char), "checkpoint filename 2");

  sprintf(name, "%s/%s.tmp", _ckpt_location, _recoverfilename);
  if ((_F = fopen(name, "wb")) == NULL) {
    printf("checkpoint failure: cannot open save file: %s\n", name);
    exit(0);
  }

  _reverse(&_cdl);

  _cdlptr = _cdl;
  do {
    if (fwrite(_cdlptr->ptr, _cdlptr->size, 1, _F) != 1) {
      printf("checkpoint failure: cannot write to save file: %s\n", name);
      exit(0);
    }
    _zfree(_cdlptr->ptr, "");
    _cdlptr = _cdlptr->next;
  } while (_cdlptr != NULL);
  if (fclose(_F) != 0) {
    printf("checkpoint failure: cannot close save file: %s\n", name);
    exit(0);
  }

  _BarrierSynch();

  sprintf(name1, "%s/%s", _ckpt_location, _recoverfilename);
  sprintf(name2, "%s/%s.old", _ckpt_location, _recoverfilename);
  rename(name1, name2);

  _BarrierSynch();

  rename(name, name1);

  _BarrierSynch();

  remove(name2);

  _checkpointing = 0;
  _recovering = 1;

  _zfree(name, "checkpoint filename 0");
  _zfree(name1, "checkpoint filename 1");
  _zfree(name2, "checkpoint filename 2");
}

void _SaveFinished(int label) {
  if (_cdl == NULL) {
    _cdl = (_chkdatalist *) _zmalloc(sizeof(_chkdatalist), "");
    _cdlptr = _cdl;
  } else {
    _cdlptr->next = (_chkdatalist *)_zmalloc(sizeof(_chkdatalist), "");
    _cdlptr = _cdlptr->next;
  }
  _chkdata = _zmalloc(_getalignedsize(sizeof(int)), "");
  *(int*)_chkdata = label;
  _cdlptr->ptr = _chkdata;
  _cdlptr->size = _getalignedsize(sizeof(int));
  _cdlptr->next = NULL;
  _fullchksize += _chksize + _getalignedsize(sizeof(int));
}

void _StartSave(unsigned long t) {
  if (!_recovering) {
    if (CheckCheckpointTimer() >= (double)t) {
      _checkpointing = 1;
      _cdl = NULL;
      _fullchksize = 0;
    }
  }
}

void _StartDataSave() {
  if (_cdl == NULL) {
    _cdl = (_chkdatalist *) _zmalloc(sizeof(_chkdatalist), "");
    _cdlptr = _cdl;
  } else {
    _cdlptr->next = (_chkdatalist *) _zmalloc(sizeof(_chkdatalist), "");
    _cdlptr = _cdlptr->next;
  }
  _chktemp = _chkdata = _zmalloc(_chksize, "");
}

void _EndDataSave() {
  _cdlptr->ptr = _chkdata;
  _cdlptr->size = _chksize;
  _cdlptr->next = NULL;
}

void _CompleteRecover(void) {
  ResetCheckpointTimer();
  _recovering = 0;
}

void _StartRecover(void) {
  FILE* _F;
  char _garbage;
  char *_ckpt_location;
  char *_fullrecoverfilename;

  if (!_recovering) {
    return;
  }

  _ckpt_location = _QueryCheckpointLocation();
  _fullrecoverfilename = (char *) malloc((strlen(_ckpt_location) + 1 +
					  strlen(_recoverfilename) + 1)*
					 sizeof(char));
  sprintf(_fullrecoverfilename, "%s/%s", _ckpt_location, _recoverfilename);

  if ((_F = fopen(_fullrecoverfilename, "rb")) == NULL) {
    printf("checkpoint failure: cannot open recover file %s, errno = %d\n",
	   _fullrecoverfilename, errno);
    exit(0);
  }
  _chksize = 0;
  while (!feof(_F)) {
    if (fread(&_garbage, sizeof(char), 1, _F) != 1) {
      if (!feof(_F)) {
	printf("checkpoint failure: cannot read recover file: %s\n",
	       _fullrecoverfilename);
	exit(0);
      }
      else {
	break;
      }
    }      
    (_chksize)++;
  }
  if (fclose(_F) != 0) {
    printf("checkpoint failure: cannot close recover file: %s\n",
	   _fullrecoverfilename);
    exit(0);
  }
  _chkdata = _zmalloc(_chksize, "");
  if ((_F = fopen(_fullrecoverfilename, "rb")) == NULL) {
    printf("checkpoint failure: cannot open recover file: %s\n",
	   _fullrecoverfilename);
    exit(0);
  }
  if (fread(_chkdata, _chksize, 1, _F) != 1) {
    printf("checkpoint failure: cannot read recover file: %s\n",
	   _fullrecoverfilename);
    exit(0);
  }
  if (fclose(_F) != 0) {
    printf("checkpoint failure: cannot close recover file: %s\n",
	   _fullrecoverfilename);
    exit(0);
  }
  _chktemp = _chkdata;
  free(_fullrecoverfilename);
}

void _DoneCheckpoint(void)
{
  char *ckpt_location = _QueryCheckpointLocation();
  char *name;

  name = (char*)malloc((strlen(ckpt_location)+1+strlen(_recoverfilename)+1)*sizeof(char));
  sprintf(name, "%s/%s", ckpt_location, _recoverfilename);
  remove(name);
  free(name);
}
