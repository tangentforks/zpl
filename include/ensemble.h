/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ENSEMBLE_H_
#define __ENSEMBLE_H_

#include "struct.h"
#include "zplglobal.h"
#include "region.h"
#include "printfun.h"
#include "type.h"
#include "priv_access.h"

#define _ACCESSOR_CAST(fn) ((char *(* )(_array,_vector))(fn))

/* macros for field access */

#define _ARR_ORIGIN(e)      ((e)->origin)
#define _ARR_ELEMSIZE(e)    ((e)->elemsize)
#define _ARR_BLK(e,d)       ((e)->blocksize[(d)])

#define _ARR_SET_ACCESSOR(e,fn) ((e)->accessor = _ACCESSOR_CAST(fn))
#define _ARR_ACCESS(e,v)    ((e)->accessor((_array)e,v))
#define _ARR_ACCESSOR(e) ((e)->accessor)

#define _ARR_COPY_CAST(fn) ((void (*)(_region, _array, _array))(fn))
#define _ARR_SET_COPY(e,fn) ((e)->copyfn = _ARR_COPY_CAST(fn))
#define _ARR_COPY(r,e1,e2) ((e1)->copyfn(r,e1,e2))
#define _ARR_COPIER(e) ((e)->copyfn)

#define _ARR_DATA(e)        ((e)->data)

#define _ARR_OFF(e,d)       ((e)->s[d].offset)
#define _ARR_STR(e,d)       ((e)->s[d].stride)

#define _ARR_NUMDIMS(e)     ((e)->numdims)

#define _ARR_DECL_REG(e)    ((e)->decl_regptr)
#define _ARR_REG(e)         ((e)->decl_regptr)
#define _ARR_DIST(e)         (_REG_DIST((e)->decl_regptr))
#define _ARR_GRID(e)         (_REG_GRID((e)->decl_regptr))

#define _ARR_DATA_SIZE(e)   ((e)->size)

#define _ARR_TYPE(e)        ((e)->strbasetype)

#define _ARR_W(e)           ((e)->Writefn)
#define _ARR_R(e)           ((e)->Readfn)

#define _ARR_DENSE(e)       (!_SPS_REGION(_ARR_DECL_REG(e)))
#define _ARR_SPARSE(e)      (_SPS_REGION(_ARR_DECL_REG(e)))

#define _ARR_LABEL(e)       ((e)->array_label)

/* functional macros */

#define _T_GOOD_ARR(e) (e!=NULL)

#define _ARR_ADJX(e,d) (_ARR_OFF(e,d)*_ARR_BLK(e,d))
#define _ARR_ADJ1(e) (_ARR_ADJX(e,0))
#define _ARR_ADJ2(e) (_ARR_ADJ1(e)+_ARR_ADJX(e,1))
#define _ARR_ADJ3(e) (_ARR_ADJ2(e)+_ARR_ADJX(e,2))
#define _ARR_ADJ4(e) (_ARR_ADJ3(e)+_ARR_ADJX(e,3))
#define _ARR_ADJ5(e) (_ARR_ADJ4(e)+_ARR_ADJX(e,4))
#define _ARR_ADJ6(e) (_ARR_ADJ5(e)+_ARR_ADJX(e,5))

#define _ARR_COMP_ORIG(e,d) (((char *)(_ARR_DATA(e)))-(_ARR_ADJ##d(e)))

#define _ARR_SPS_ZERO_IDENT(e) \
  { \
    int i; \
    char *ptr; \
    ptr = _ARR_ORIGIN(e); \
    for (i=0;i<_ARR_ELEMSIZE(e);i++) { \
      *ptr = 0; \
      ptr++; \
    } \
  }
    
#define _ARR_FLUFF_UP(arr, dim) ((arr)->fluff.up[dim])
#define _ARR_FLUFF_DOWN(arr, dim) ((arr)->fluff.down[dim])
#define _ARR_FLUFF_WRAP(arr) ((arr)->fluff.wrap)

/* changing the array info structure will require changes to flood.c,
   promote.c, and index.c, temp_ens.c */

typedef struct _si {
  int offset;
  int stride;
} _str_info;

typedef struct __fluff_info {
  int up[_MAXRANK];
  int down[_MAXRANK];
  int wrap;
} _fluff_info;

struct __arr_info {
  char *origin;
  int elemsize;
  int blocksize[_MAXRANK];

  char *(* accessor)(_array,_vector);

  char *data;
  
  _str_info s[_MAXRANK];

  int numdims;
  _region_nc decl_regptr;

  unsigned long size;

  _fluff_info fluff;

  char *strbasetype;

  char* array_label;

  _filepointfn Writefn;
  _filepointfn Readfn;

  void (*copyfn)(_region, _array, _array);

  _MD_ENSEMBLE_INFO
};

#define _ARR_ACTION_GRID(a) (a->grid)
#define _ARR_ACTION_DIST(a) (a->dist)
#define _ARR_ACTION_REG(a) (a->reg)
#define _ARR_ACTION_ARR(a) (a->arr)
#define _ARR_ACTION_PRESERVE(a) (a->preserve)
#define _ARR_ACTION_NEXT(a) (a->next_array_action)

struct __array_action_list_info {
  _grid_nc grid;                        /* new grid                     */
  _distribution_nc dist;                /* new distribution             */
  _region_nc reg;                       /* new region                   */
  _array_nc arr;                        /* affected array               */
  int preserve;                         /* preserving assignment?       */
  _array_action_list next_array_action; /* pointer to next array action */
};

extern _array_action_list _aal;

#define _ACTION_LIST _aal

_PRIV_DECL(extern, _vector, _zindex);
typedef _array _IndexType[_MAXRANK];
_PRIV_DECL(extern, _IndexType, _Index);

#define _RMSCurrentRegion   (_RMStack.reg)
#define _RMSCurrentRegion1  (_RMStack.reg)
#define _RMSCurrentRegion2  (_RMStack.reg)
#define _RMSCurrentRegion3  (_RMStack.reg)
#define _RMSCurrentRegion4  (_RMStack.reg)
#define _RMSCurrentRegion5  (_RMStack.reg)
#define _RMSCurrentRegion6  (_RMStack.reg)
/* MAXRANK */

#define _RMSCurrentMask     (_RMStack.mask)
#define _RMSCurrentWithFlag (_RMStack.withflag)

typedef struct _rmsf {
  _region_nc reg;
  _array_pnc mask;
  int withflag;
} _rmsframe;

extern _rmsframe _RMStack;

#define _PROCESS_ENSEMBLES() _ProcessEnsembles()

#endif
