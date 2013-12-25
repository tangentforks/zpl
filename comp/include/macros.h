/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef MACROS_H
#define MACROS_H

#include "../include/allocstmt.h"

/*
 * ADD_TO_END : assigns SRC to the end of DST where DST is a list of
 *              entities of type TYPE and where NEXT (DST) is the
 *              second entity in the list, NEXT( NEXT (DST)) is the
 *              third, etc.
 */
#define ADD_TO_END(dst,next,type,src) if (dst != NULL) {         \
                                        type* tmp;               \
                                        for (tmp = dst;          \
                                             next (tmp) != NULL; \
                                             tmp = next (tmp));  \
                                        next (tmp) = src;        \
                                      } else (dst) = src

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif 

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif 

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define	PMALLOC(x)	our_malloc(x)
#define	PFREE(x,y)	our_free((char *)x,y)

#endif
