/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "start.h"
#include "zlib.h"
#include "zerror.h"
#include "permute.h"
#include "zc_proto.h"

static FILE* memtrace=NULL;

static unsigned long memused;
static unsigned long memmax;

/* allcocate general-purpose memory on the heap */

void _InitMem(void) {
  int trace;
  char filename[256];

  trace = _QueryMemTrace();
  if (trace) {
    sprintf(filename,".memtrace.%d.of.%d",_INDEX+1,_PROCESSORS);
    memtrace = fopen(filename,"w");
    fprintf(memtrace,"OP  total size  address     memory use  memory max  comment                \n");
    fprintf(memtrace,"--  ----------  ----------  ----------  ----------  -----------------------\n");
    fflush(memtrace);
  }
}

void _MemLogMark(char* description) {
  if (memtrace) {
    fprintf(memtrace,"--  ----------  ----------  ----------  ----------  %s\n", description);
    fflush(memtrace);
  }
}


void _UnInitMem(void) {
  if (memtrace) {
    fclose(memtrace);
  }
}

typedef struct _tminfostruct {
  void* ptr;
  unsigned long size;
} _tminfo;

static unsigned long TrackMemory(void* ptr, unsigned long size, int addorfree) {
  static _tminfo* tm = NULL;
  static int num = 0;
  int i;
  unsigned long freed = 0;

  if (tm == NULL) {
    tm = (_tminfo*)calloc(++num, sizeof(_tminfo));
  }
  if (addorfree) {
    memused += size;
    if (memused > memmax) {
      memmax = memused;
    }
    for (i = 0; i < num; i++) {
      if (tm[i].size == 0) {
	tm[i].ptr = ptr;
	tm[i].size = size;
	return size;
      }
    }
    tm = (_tminfo*)realloc((void*)tm, ++num * sizeof(_tminfo));
    tm[num-1].ptr = ptr;
    tm[num-1].size = size;
    return size;
  }
  else {
    for (i = 0; i < num; i++) {
      if (tm[i].ptr == ptr) {
	freed = tm[i].size;
	tm[i].ptr = 0;
	tm[i].size = 0;
	memused -= freed;
	return freed;
      }
    }
    _RT_ANY_FATAL0("Attempt to free a pointer never allocated!");
    exit(1);
  }
}

void * restrict _zmalloc(unsigned long size, char* description) {
  void *data;
  if (size != 0) {
    data = (void *)malloc(size);
  } else {
    data = NULL;
  }
  if (memtrace) {
    TrackMemory(data, size, 1);
    fprintf(memtrace,"A   %10ld  0x%.8x  %10ld  %10ld  %s\n", 
            size, (_ZPL_PTR_AS_INT)data, memused, memmax, description);
    fflush(memtrace);
  }
  if ((data == NULL) && (size != 0)) {
    _RT_ANY_FATAL0("Out of memory");
  }
  return data;
}

void * restrict _zmalloc_reg(unsigned long size, char* description,_region reg) {
  char longstring[1024];

  if (memtrace) {
    sprintf(longstring,"%s%s",description,_REG_LABEL(reg));
  }
  return _zmalloc(size, longstring);
}

void * restrict _zcalloc(unsigned long numelems,unsigned long elemsize, char* description) {
  void *data;
  if (numelems != 0 && elemsize != 0) {
    data = (void *)calloc(numelems, elemsize);
  } else {
    data = NULL;
  }
  if (memtrace) {
    TrackMemory(data, elemsize*numelems, 1);
    fprintf(memtrace,"A   %10ld  0x%.8x  %10ld  %10ld  %s\n",
            numelems*elemsize,(_ZPL_PTR_AS_INT)data,memused,memmax,description);
    fflush(memtrace);
  }
  if ((data == NULL) && (numelems != 0) && (elemsize != 0)) {
    _RT_ANY_FATAL0("Out of memory");
  }
  return data;
}

void * restrict _zcalloc_reg(unsigned long numelems,unsigned long elemsize,
		     char* description,_region reg) {
  char longstring[1024];

  if (memtrace) {
    sprintf(longstring,"%s%s",description,_REG_LABEL(reg));
  }
  return _zcalloc(numelems,elemsize,longstring);
}

void * restrict _zrealloc(void* ptr, unsigned long size, char* description) {
  void* data;

  if (size != 0) {
    data = (void*)realloc(ptr,size);
    if (memtrace) {
      if (ptr != NULL) {
        unsigned long sizefreed;
        sizefreed = TrackMemory(ptr, 0, 0);
        fprintf(memtrace,"RF  %10ld  0x%.8x  %10ld  %10ld  %s\n", 
                sizefreed, (_ZPL_PTR_AS_INT)ptr, memused, memmax, description);
      }
    }
  } else {
    _zfree(ptr, description);
    return NULL;
  }
  if (memtrace) {
    TrackMemory(data, size, 1);
    fprintf(memtrace,"RA  %10ld  0x%.8x  %10ld  %10ld  %s\n", 
            size, (_ZPL_PTR_AS_INT)data, memused, memmax, description);
    fflush(memtrace);
  }
  if ((data == NULL) && (size != 0)) {
    _RT_ANY_FATAL0("Out of memory");
  }
  return data;
}

void * restrict _zrealloc_reg(void* ptr,unsigned long size, char* description,_region reg) {
  char longstring[1024];

  if (memtrace) {
    sprintf(longstring,"%s%s",description,_REG_LABEL(reg));
  }
  return _zrealloc(ptr, size, longstring);
}

void _zfree(void *ptr,char* description) {
  if (memtrace) {
    unsigned long sizefreed;
    sizefreed = TrackMemory(ptr, 0, 0);
    fprintf(memtrace, "F   %10ld  0x%.8x  %10ld  %10ld  %s\n", 
            sizefreed, (_ZPL_PTR_AS_INT)ptr, memused, memmax, description);
    fflush(memtrace);
  }
  free(ptr);
}

void _zfree_reg(void *ptr,char* description,_region reg) {
  char longstring[1024];

  if (memtrace) {
    sprintf(longstring,"%s%s",description,_REG_LABEL(reg));
  }
  _zfree(ptr,longstring);
}
