/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __SCANRED_H_
#define __SCANRED_H_

#include <limits.h>
#include <float.h>    /* float must come after limits on dept. sgis */
#include "md_zinc.h"
#include "type.h"
#include "quad.h"

#define _OP_ADD  1
#define _OP_MULT 2

#define _OP_MAX  3
#define _OP_MIN  4

#define _OP_AND  5
#define _OP_OR   6

#define _OP_BAND 7
#define _OP_BOR  8
#define _OP_XOR  9

#ifndef COMPLEX_TYPE
#ifndef FLOAT_TYPE
#define DO_OP(op,x,y) switch (op) {\
				   case _OP_ADD: \
				     _ADD_STMT(x,y); \
				     break; \
				   case _OP_MULT: \
				     _MULT_STMT(x,y); \
				     break; \
				   case _OP_MAX: \
				     _MAX_STMT(x,y); \
				     break; \
				   case _OP_MIN: \
				     _MIN_STMT(x,y); \
				     break; \
				   case _OP_AND: \
				     _AND_STMT(x,y); \
				     break; \
				   case _OP_OR: \
				     _OR_STMT(x,y); \
				     break; \
				   case _OP_BAND: \
				     _BAND_STMT(x,y); \
				     break; \
				   case _OP_BOR: \
				     _BOR_STMT(x,y); \
				     break; \
				   case _OP_XOR: \
				     _XOR_STMT(x,y); \
				     break; \
				 }
#else
#define DO_OP(op,x,y) switch (op) {\
				   case _OP_ADD: \
				     _ADD_STMT(x,y); \
				     break; \
				   case _OP_MULT: \
				     _MULT_STMT(x,y); \
				     break; \
				   case _OP_MAX: \
				     _MAX_STMT(x,y); \
				     break; \
				   case _OP_MIN: \
				     _MIN_STMT(x,y); \
				     break; \
				   case _OP_AND: \
				     _AND_STMT(x,y); \
				     break; \
				   case _OP_OR: \
				     _OR_STMT(x,y); \
				     break; \
				 }
#endif
#else
#define DO_OP(op,x,y) switch (op) {\
				   case _OP_ADD: \
				     _ADD_STMT_CMPLX_HELP(x,y,TYPE); \
				     break; \
				   case _OP_MULT: \
				     _MULT_STMT_CMPLX_HELP(x,y,TYPE); \
				     break; \
				 }
#endif

#define _DO_ADD(x,y)  ((x) + (y))
#define _DO_MULT(x,y) ((x) * (y))
#define _DO_MAX(x,y)  (max(x,y))
#define _DO_MIN(x,y)  (min(x,y))
#define _DO_AND(x,y)  ((x) && (y))
#define _DO_OR(x,y)   ((x) || (y))
#define _DO_XOR(x,y)  ((x) ^ (y))
#define _DO_BAND(x,y) ((x) & (y))
#define _DO_BOR(x,y)  ((x) | (y))

#define _ADD_STMT(x,y)  ((x)+=(y))
#define _ADD_STMT_CMPLX_BASE(x,y) _ADD_STMT(x.re,y.re); _ADD_STMT(x.im,y.im)
#define _ADD_STMT_fcomplex(x,y) _ADD_STMT_CMPLX_BASE(x,y)
#define _ADD_STMT_dcomplex(x,y) _ADD_STMT_CMPLX_BASE(x,y)
#define _ADD_STMT_qcomplex(x,y) _ADD_STMT_CMPLX_BASE(x,y)
#define __ADD_STMT_CMPLX_HELP(x,y,t) _ADD_STMT_##t(x,y)
#define _ADD_STMT_CMPLX_HELP(x,y,t) __ADD_STMT_CMPLX_HELP(x,y,t)

#define _MULT_STMT(x,y) ((x)*=(y))
#define _MULT_STMT_CMPLX_BASE(x,y,t) (t) = ((x.re)*(y.re))-((x.im)*(y.im));\
                                     (x.im) = ((x.re)*(y.im))+((x.im)*(y.re));\
                                     (x.re) = (t)
#define _MULT_STMT_fcomplex(x,y) _MULT_STMT_CMPLX_BASE(x,y,_fcomplex_temp1.re)
#define _MULT_STMT_dcomplex(x,y) _MULT_STMT_CMPLX_BASE(x,y,_dcomplex_temp1.re)
#define _MULT_STMT_qcomplex(x,y) _MULT_STMT_CMPLX_BASE(x,y,_qcomplex_temp1.re)

#define _DIV_STMT_CMPLX_BASE(x,y,t) \
  (t)    = ((((x.re)*(y.re))+((x.im)*(y.im)))/((y.re)*(y.re) + (y.im)*(y.im)));\
  (x.im) = ((((x.im)*(y.re))-((x.re)*(y.im)))/((y.re)*(y.re) + (y.im)*(y.im)));\
  (x.re) = (t)
#define _DIV_STMT_fcomplex(x,y) _DIV_STMT_CMPLX_BASE(x,y,_fcomplex_temp1.re)
#define _DIV_STMT_dcomplex(x,y) _DIV_STMT_CMPLX_BASE(x,y,_dcomplex_temp1.re)
#define _DIV_STMT_qcomplex(x,y) _DIV_STMT_CMPLX_BASE(x,y,_qcomplex_temp1.re)

#define __MULT_STMT_CMPLX_HELP(x,y,t) _MULT_STMT_##t(x,y)
#define _MULT_STMT_CMPLX_HELP(x,y,t) __MULT_STMT_CMPLX_HELP(x,y,t)

#define _MAX_STMT(x,y)  ((x)=max(x,y))
#define _MIN_STMT(x,y)  ((x)=min(x,y))
#define _AND_STMT(x,y)  ((x)=(x)&&(y))
#define _OR_STMT(x,y)   ((x)=(x)||(y))
#define _XOR_STMT(x,y)  ((x)=(x)^(y))
#define _BAND_STMT(x,y) ((x)=(x)&(y))
#define _BOR_STMT(x,y)  ((x)=(x)|(y))

#define _RED_DIM(RS,RD,dim) \
  (_FLOODED_DIM(RS,dim) != _DIM_FLOODED && \
   ((_REG_GLB_NUMELMS(RS,dim)!=_REG_GLB_NUMELMS(RD,dim)) || \
   (_FLOODED_DIM(RD,dim))))

#define _BCS_DIM(RS,RD,dim) \
  (_FLOODED_DIM(RD,dim) == _DIM_FLOODED && _FLOODED_DIM(RS,dim) != _DIM_FLOODED)


#define _RAD(R,d)  (_i##d-_REG_MYLO(R,d))*_block[d]

#define _R_ACCESS_1D(x,R) (x[_RAD(R,0)])
#define _R_ACCESS_2D(x,R) (x[_RAD(R,0)+_RAD(R,1)])
#define _R_ACCESS_3D(x,R) (x[_RAD(R,0)+_RAD(R,1)+_RAD(R,2)])
#define _R_ACCESS_4D(x,R) (x[_RAD(R,0)+_RAD(R,1)+_RAD(R,2)+_RAD(R,3)])
#define _R_ACCESS_5D(x,R) (x[_RAD(R,0)+_RAD(R,1)+_RAD(R,2)+_RAD(R,3)+_RAD(R,4)])
#define _R_ACCESS_6D(x,R) (x[_RAD(R,0)+_RAD(R,1)+_RAD(R,2)+_RAD(R,3)+\
			     _RAD(R,4)+_RAD(R,5)])

#define _SAD(R,n,d) (_i##d-_REG_MYLO(R,d))*_block[n][d]

#define _S_ACCESS_1D_OFF(n,x,R,o) (x[_SAD(R,n,0)+(o)])
#define _S_ACCESS_2D_OFF(n,x,R,o) (x[_SAD(R,n,0)+_SAD(R,n,1)+(o)])
#define _S_ACCESS_3D_OFF(n,x,R,o) (x[_SAD(R,n,0)+_SAD(R,n,1)+_SAD(R,n,2)+(o)])
#define _S_ACCESS_4D_OFF(n,x,R,o) (x[_SAD(R,n,0)+_SAD(R,n,1)+_SAD(R,n,2)+\
				     _SAD(R,n,3)+(o)])
#define _S_ACCESS_5D_OFF(n,x,R,o) (x[_SAD(R,n,0)+_SAD(R,n,1)+_SAD(R,n,2)+\
				     _SAD(R,n,3)+_SAD(R,n,4)+(o)])
#define _S_ACCESS_6D_OFF(n,x,R,o) (x[_SAD(R,n,0)+_SAD(R,n,1)+_SAD(R,n,2)+\
				     _SAD(R,n,3)+_SAD(R,n,4)+_SAD(R,n,5)+(o)])

#define _S_ACCESS_1D(n,x,R) _S_ACCESS_1D_OFF(n,x,R,0)
#define _S_ACCESS_2D(n,x,R) _S_ACCESS_2D_OFF(n,x,R,0)
#define _S_ACCESS_3D(n,x,R) _S_ACCESS_3D_OFF(n,x,R,0)
#define _S_ACCESS_4D(n,x,R) _S_ACCESS_4D_OFF(n,x,R,0)
#define _S_ACCESS_5D(n,x,R) _S_ACCESS_5D_OFF(n,x,R,0)
#define _S_ACCESS_6D(n,x,R) _S_ACCESS_6D_OFF(n,x,R,0)

#define _ADD_int_IDENTITY    0
#define _ADD__uint_IDENTITY  0
#define _ADD_long_IDENTITY   0
#define _ADD__ulong_IDENTITY 0
#define _ADD_float_IDENTITY  0
#define _ADD_double_IDENTITY 0
#define _ADD__zquad_IDENTITY 0
static const fcomplex _ADD_fcomplex_IDENTITY = {0,0};
static const dcomplex _ADD_dcomplex_IDENTITY = {0,0};
static const qcomplex _ADD_qcomplex_IDENTITY = {0,0};

#define _MULT_int_IDENTITY    1
#define _MULT__uint_IDENTITY  1
#define _MULT_long_IDENTITY   1
#define _MULT__ulong_IDENTITY 1
#define _MULT_float_IDENTITY  1
#define _MULT_double_IDENTITY 1
#define _MULT__zquad_IDENTITY 1
static const fcomplex _MULT_fcomplex_IDENTITY = {1,0};
static const dcomplex _MULT_dcomplex_IDENTITY = {1,0};
static const qcomplex _MULT_qcomplex_IDENTITY = {1,0};

#define _MAX_int_IDENTITY    INT_MIN
#define _MAX__uint_IDENTITY  0
#define _MAX_long_IDENTITY   LONG_MIN
#define _MAX__ulong_IDENTITY 0
#define _MAX_float_IDENTITY  (-FLT_MAX)
#define _MAX_double_IDENTITY (-DBL_MAX)
#define _MAX__zquad_IDENTITY (-_QUAD_MAX)

#define _MIN_int_IDENTITY    INT_MAX
#define _MIN__uint_IDENTITY  UINT_MAX
#define _MIN_long_IDENTITY   LONG_MAX
#define _MIN__ulong_IDENTITY ULONG_MAX
#define _MIN_float_IDENTITY  FLT_MAX
#define _MIN_double_IDENTITY DBL_MAX
#define _MIN__zquad_IDENTITY _QUAD_MIN

#define _AND_int_IDENTITY    1
#define _AND__uint_IDENTITY  1
#define _AND_long_IDENTITY   1
#define _AND__ulong_IDENTITY 1
#define _AND_float_IDENTITY  1
#define _AND_double_IDENTITY 1
#define _AND__zquad_IDENTITY 1

#define _OR_int_IDENTITY    0
#define _OR__uint_IDENTITY  0
#define _OR_long_IDENTITY   0
#define _OR__ulong_IDENTITY 0
#define _OR_float_IDENTITY  0
#define _OR_double_IDENTITY 0
#define _OR__zquad_IDENTITY 0

#define _XOR_int_IDENTITY    0
#define _XOR__uint_IDENTITY  0
#define _XOR_long_IDENTITY   0
#define _XOR__ulong_IDENTITY 0

#define _BAND_int_IDENTITY    (-1)
#define _BAND__uint_IDENTITY  (-1)
#define _BAND_long_IDENTITY   (-1)
#define _BAND__ulong_IDENTITY (-1)

#define _BOR_int_IDENTITY    0
#define _BOR__uint_IDENTITY  0
#define _BOR_long_IDENTITY   0
#define _BOR__ulong_IDENTITY 0

#endif
