/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <malloc.h>
#include <mpp/shmem.h>
#include "zerror.h"
#include "zlib.h"

void *_ZPL_SYM_ALLOC(unsigned long size, char* description) {
  void *retval;

  if (size != 0) {
    retval = (void *)shmalloc(size);
    if (retval == NULL) {
      _RT_ANY_FATAL0("Out of memory");
    }
  } else {
    retval = NULL;
  }
  return retval;
}


/* free memory from the heap */

void _ZPL_SYM_FREE(void *ptr, char* description) {
  shfree(ptr);
}

