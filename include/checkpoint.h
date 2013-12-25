/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _CHECKPOINT_H_
#define _CHECKPOINT_H_

extern char *_chkdata;
extern char *_chktemp;

extern unsigned long _chksize;
extern int _recovering;
extern int _checkpointing;
extern char _recoverfilename[128];

typedef struct _chkdataliststruct {
  void* ptr;
  unsigned long size;
  struct _chkdataliststruct* next;
} _chkdatalist;

extern _chkdatalist* _cdl;
extern _chkdatalist* _cdlptr;

void _ParseCheckpointArgs(int argc, char *argv[]);

void _StartSave(unsigned long);
void _StartDataSave(void);
void _EndDataSave(void);
void _SaveFinished(int);
void _CompleteSave(void);

#define _NewSave() _chksize = 0;

void _StartRecover(void);
void _CompleteRecover(void);
#define _RecoverFinished() ((unsigned long) (_chktemp - _chkdata) == _chksize)

void _DoneCheckpoint(void);


/*** _getalignedsize() uses this  ***/
#ifndef _BYTE_ALIGNMENT_
#define _BYTE_ALIGNMENT_ 8
#endif
unsigned long _getalignedsize(unsigned long);

/*** functions for checkpointing data ***/
void _AddString(char *);
void _SaveString(char *);
void _RecoverString(char *);

void _AddArray(_array);
void _SaveArray(_array);
void _RecoverArray(_array_nc);

void _AddRegion(_region);
void _SaveRegion(_region);
void _RecoverRegion(_region_nc);

void _AddRMStack(_rmsframe *);
void _SaveRMStack(_rmsframe *);
void _RecoverRMStack(_rmsframe *);

/*** macros for checkpointing data ***/
#define _AddBasic(type)         _chksize += _getalignedsize(sizeof(type));
#define _SaveBasic(x, type)     *((type *) _chktemp) = x;\
                                _chktemp += _getalignedsize(sizeof(type));
#define _RecoverBasic(x, type)  x = *((type *) _chktemp);\
                                _chktemp += _getalignedsize(sizeof(type));

#define _AddIArray(type)        _chksize +=\
                                     _getalignedsize(sizeof(unsigned long)) +\
                                     _getalignedsize(sizeof(type));
#define _SaveIArray(a, type)    *((unsigned long *)) _chktemp =\
                                     _getalignedsize(sizeof(type));\
                                _chktemp += _getalignedsized(unsigned long);\
                                memcpy(_chktemp, a, sizeof(type));\
                                chktemp += _getalignedsize(sizeof(type));
#define _RecoverIArray(a, type) {unsigned long _chkjump;\
                                _chkjump = *((unsigned long *) _chktemp);\
                                _chktemp += _getalignedsized(unsigned long);\
                                memcpy(a, _chktemp, sizeof(type));\
                                _chktemp += _chkjump;

#endif
