/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _STRUCT_H_
#define _STRUCT_H_

typedef struct __grid_info _grid_info;
typedef const _grid_info * const restrict _grid;
typedef _grid_info * const restrict _grid_fnc;
typedef const _grid_info * restrict _grid_pnc;
typedef _grid_info * restrict _grid_nc;

typedef struct __distribution_info _distribution_info;
typedef const _distribution_info * const restrict _distribution;
typedef _distribution_info * const restrict _distribution_fnc;
typedef const _distribution_info * restrict _distribution_pnc;
typedef _distribution_info * restrict _distribution_nc;

typedef struct __dimdist_info _dimdist_info;
typedef const _dimdist_info * const restrict _dimdist;
typedef _dimdist_info * const restrict _dimdist_fnc;
typedef const _dimdist_info * restrict _dimdist_pnc;
typedef _dimdist_info * restrict _dimdist_nc;

typedef struct __reg_info _reg_info;
typedef const _reg_info* const _region;
typedef _reg_info* const _region_fnc;
typedef const _reg_info* _region_pnc;
typedef _reg_info* _region_nc;

typedef struct __arr_info _arr_info;
typedef const _arr_info* const _array;
typedef _arr_info* const _array_fnc;
typedef const _arr_info* _array_pnc;
typedef _arr_info* _array_nc;

typedef struct __array_action_list_info _array_action_list_info;
typedef _array_action_list_info* _array_action_list;

#endif
