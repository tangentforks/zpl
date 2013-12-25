/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __QUAD_H_
#define __QUAD_H_

#include "md_zinc.h"

#ifdef _QUAD_SUPPORTED

typedef long double _zquad;

#define _Z_W_quad   "%9.4Lf "
#define _Z_W_qcomplex "%9.4Lf + %9.4Lfi"
#define _Z_R_quad   "%Lf"
#define _Z_R_qcomplex "%Lf+%Lfi"

#else

/*** for platforms with no support for quad precision arithmetic ***/
/*** use double precision instead ***/

typedef double _zquad;

#define atold atof
#define _QUAD_MAX DBL_MAX
#define _QUAD_MIN DBL_MIN

#define _Z_W_quad     _Z_W_double
#define _Z_W_qcomplex _Z_W_dcomplex
#define _Z_R_quad     _Z_R_double
#define _Z_R_qcomplex _Z_R_dcomplex

/*** ANSI-C math library routines ***/
#define sinl sin
#define cosl cos
#define tanl tan
#define asinl asin
#define acosl acos
#define atanl atan
#define atan2l atan2
#define sinhl sinh
#define coshl cosh
#define tanhl tanh
#define expl exp
#define logl log
#define log10l log10
#define powl pow
#define sqrtl sqrt
#define ceill ceil
#define floorl floor
#define fabsl fabs
#define ldexpl ldexp
#define frexpl frexp
#define modfl modf
#define fmodl fmod

#endif

#ifdef _LONGLONG_SUPPORTED

typedef long long _zlonglong;

typedef unsigned long long _zulonglong;

#else

typedef long _zlonglong;

typedef unsigned long _zulonglong;

#endif

#endif
