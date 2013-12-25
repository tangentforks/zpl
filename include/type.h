/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __TYPE_H_
#define __TYPE_H_

#include "quad.h"

#ifndef _ZPL_PTR_AS_INT
#define _ZPL_PTR_AS_INT unsigned int
#endif

typedef char boolean;
typedef char _zstring[256];

typedef unsigned int _uint;
typedef unsigned long _ulong;

typedef struct _fcomplex {
  float re;
  float im;
} fcomplex;

typedef struct _dcomplex {
  double re;
  double im;
} dcomplex;

typedef struct _qcomplex {
  _zquad re;
  _zquad im;
} qcomplex;

#define true  (1)
#define false (0)

#ifndef _CONST
#define _CONST const
#endif

typedef enum {
  _TYPE_CMPLX,  /* record or array type */
  _TYPE_BOOL,
  _TYPE_CHAR,
  _TYPE_UCHAR,
  _TYPE_SCHAR,
  _TYPE_SHORT,
  _TYPE_USHORT,
  _TYPE_INT,
  _TYPE_UINT,
  _TYPE_LONG,
  _TYPE_ULONG,
  _TYPE_FLOAT,
  _TYPE_DOUBLE,
  _TYPE_QUAD,
  _TYPE_STRING,
  _TYPE_ENUM
} _ztype;

extern fcomplex _fcomplex_temp1;
extern fcomplex _fcomplex_temp2;
extern dcomplex _dcomplex_temp1;
extern dcomplex _dcomplex_temp2;
extern qcomplex _qcomplex_temp1;
extern qcomplex _qcomplex_temp2;

#endif

