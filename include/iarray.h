/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _IARRAY_H_
#define _IARRAY_H_

#include "type.h"

typedef struct _pdi {
  int blk;
  int off;
  int str;
} _per_dim_info;


typedef struct _la {
  unsigned long size;
  
  int numdims;
  
  char* data;

  _per_dim_info layout[_MAXRANK];
} _loc_arr_info;


typedef _CONST _loc_arr_info* _CONST _ind_array;
typedef        _loc_arr_info* _CONST _ind_array_fnc;
typedef _CONST _loc_arr_info*        _ind_array_pnc;
typedef        _loc_arr_info*        _ind_array_nc;


#define _IARR_SIZE(a)    ((a)->size)
#define _IARR_NUMDIMS(a) ((a)->numdims)
#define _IARR_DATA(a)    ((a)->data)
#define _IARR_OFF(a,d)   ((a)->layout[d].off)
#define _IARR_BLK(a,d)   ((a)->layout[d].blk)
#define _IARR_STR(a,d)   ((a)->layout[d].str)

#define _IAD(a,i,d) ((((i)-_IARR_OFF(a,d))*_IARR_BLK(a,d))/_IARR_STR(a,d))

#define __IACCESS_0D(a)                   _IARR_DATA(a)
#define __IACCESS_1D(a,i0)                __IACCESS_0D(a) + _IAD(a,i0,0)
#define __IACCESS_2D(a,i0,i1)             __IACCESS_1D(a,i0) + _IAD(a,i1,1)
#define __IACCESS_3D(a,i0,i1,i2)          __IACCESS_2D(a,i0,i1) + _IAD(a,i2,2)
#define __IACCESS_4D(a,i0,i1,i2,i3)       __IACCESS_3D(a,i0,i1,i2) + _IAD(a,i3,3)
#define __IACCESS_5D(a,i0,i1,i2,i3,i4)    __IACCESS_4D(a,i0,i1,i2,i3) + _IAD(a,i4,4)
#define __IACCESS_6D(a,i0,i1,i2,i3,i4,i5) __IACCESS_5D(a,i0,i1,i2,i3,i4) + _IAD(a,i5,5)

#define _IACCESS_0D(a)                   (__IACCESS_0D(a))
#define _IACCESS_1D(a,i0)                (__IACCESS_1D(a,i0))
#define _IACCESS_2D(a,i0,i1)             (__IACCESS_2D(a,i0,i1))
#define _IACCESS_3D(a,i0,i1,i2)          (__IACCESS_3D(a,i0,i1,i2))
#define _IACCESS_4D(a,i0,i1,i2,i3)       (__IACCESS_4D(a,i0,i1,i2,i3))
#define _IACCESS_5D(a,i0,i1,i2,i3,i4)    (__IACCESS_5D(a,i0,i1,i2,i3,i4))
#define _IACCESS_6D(a,i0,i1,i2,i3,i4,i5) (__IACCESS_6D(a,i0,i1,i2,i3,i4,i5))

#endif
