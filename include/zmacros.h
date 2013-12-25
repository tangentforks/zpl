/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ZMACROS_H_
#define __ZMACROS_H_

#define to_boolean(x) ((int)((x != 0) ? 1 : 0))
#define to_integer(x) ((int)(x))
#define to_float(x) ((float)(x))
#define to_longint(x) ((long)(x))
#define to_shortint(x) ((short)(x))
#define to_double(x) ((double)(x))
#define to_quad(x)   ((_zquad)(x))
#define to_string(x) ((char *)(x))
#define to_char(x) ((char)(x))
#define to_ubyte(x) ((unsigned char)(x))
#define to_sbyte(x) ((signed char)(x))
#define to_uinteger(x) ((unsigned int)(x))
#define to_ulongint(x) ((unsigned long)(x))
#define to_ushortint(x) ((unsigned short)(x))

/* the following macros may or may not be in macros.h */

#ifndef min
# define min(a,b)		( ((a) < (b)) ? (a) : (b) )
#endif /* min */

#ifndef max
# define max(a,b)		( ((a) > (b)) ? (a) : (b) )
#endif /* max */

#ifndef abs
# define abs(x)			(x>=0 ? x : -(x))
#endif /* abs */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#define bsl(x,y) ((x) << (y))
#define bsr(x,y) ((x) >> (y))
#define bor(x,y)    ((x) | (y))
#define band(x,y)   ((x) & (y))
#define bxor(x,y)   ((x) ^ (y))
#define bnot(x)     (~(x))

#define bpop(x) (_bpop(x))

#define _ZPL_TRUNC(x) ((x) > 0 ? (floor((double)x)) : (ceil((double)x)))

#endif
