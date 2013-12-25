/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __DIRECTION_H_
#define __DIRECTION_H_

#include <stdlib.h>
#include "type.h"
#include "zplglobal.h"

typedef int _dir_info_nc[_MAXRANK];
typedef _CONST int _dir_info[_MAXRANK];

typedef int* _direction_nc;
typedef _CONST int* _direction_pnc;
typedef int* _CONST _direction_fnc;
typedef _CONST int* _CONST _direction;

typedef int _dir_def_mult[_MAXRANK];
typedef _dir_def_mult* _direction_mult;
#define _dir_def_dyn _dir_def_mult

#define _DIRCOMP(dir,dim) dir[dim]

#define _NO_DIRECTION NULL

#define _DIR_CHECK_POS(dim, dir, val) \
  if (val <= 0) _RT_ANY_WARN0("Dimension #dim of direction #dir assigned non-positive value")

#define _DIR_CHECK_NEG(dim, dir, val) \
  if (val >= 0) _RT_ANY_WARN0("Dimension #dim of direction #dir assigned non-negative value")

#define _DIR_CHECK_ZERO(dim, dir, val) \
  if (val != 0) _RT_ANY_WARN0("Dimension #dim of direction #dir assigned non-zero value")

#define _DIR_CHECK_UNKNOWN(dim, dir, val)

#define _DIR_ASSIGN(dim, dir, sign, val) \
  _DIR_CHECK_##sign(dim, dir, val); \
  dir[dim] = val

#define _DIR_ASSIGN_1D(dir, sign1, val1) \
  _DIR_ASSIGN(0, dir, sign1, val1)

#define _DIR_ASSIGN_2D(dir, sign1, val1, sign2, val2) \
  _DIR_ASSIGN_1D(dir, sign1, val1); \
  _DIR_ASSIGN(1, dir, sign2, val2)

#define _DIR_ASSIGN_3D(dir, sign1, val1, sign2, val2, sign3, val3) \
  _DIR_ASSIGN_2D(dir, sign1, val1, sign2, val2); \
  _DIR_ASSIGN(2, dir, sign3, val3)

#define _DIR_ASSIGN_4D(dir, sign1, val1, sign2, val2, sign3, val3, sign4, val4) \
  _DIR_ASSIGN_3D(dir, sign1, val1, sign2, val2, sign3, val3); \
  _DIR_ASSIGN(3, dir, sign4, val4)

#define _DIR_ASSIGN_5D(dir, sign1, val1, sign2, val2, sign3, val3, sign4, val4, sign5, val5) \
  _DIR_ASSIGN_4D(dir, sign1, val1, sign2, val2, sign3, val3, sign4, val4); \
  _DIR_ASSIGN(4, dir, sign5, val5)

#define _DIR_ASSIGN_6D(dir, sign1, val1, sign2, val2, sign3, val3, sign4, val4, sign5, val5, sign6, val6) \
  _DIR_ASSIGN_5D(dir, sign1, val1, sign2, val2, sign3, val3, sign4, val4, sign5, val5); \
  _DIR_ASSIGN(5, dir, sign6, val6)

#endif
