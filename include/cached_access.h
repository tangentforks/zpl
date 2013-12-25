/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _CACHED_ACCESS_H_
#define _CACHED_ACCESS_H_

#define _F_ACCESS_BASE(n) _origin_##n
#define _ACCESS_BASE(n) _data_##n

/* Mechanisms for caching the data/origin pointers */

#define _DECL_INIT_HOISTED_CHAR_PTRS(n,e) \
  char* restrict const _ACCESS_BASE(n) = (e)->data; \
  char* restrict const _F_ACCESS_BASE(n) = (e)->origin

#define _DECL_INIT_HOISTED_TYPE_PTRS(t,n,e) \
  t* restrict const _ACCESS_BASE(n) = (t*)((e)->data); \
  t* restrict const _F_ACCESS_BASE(n) = (t*)((e)->origin)

#define _DECL_HOISTED_CHAR_PTRS(s,e) \
  char* s restrict _ACCESS_BASE(e); \
  char* s restrict _F_ACCESS_BASE(e)

#define _DECL_HOISTED_TYPE_PTRS(t,s,e) \
  t* s restrict _ACCESS_BASE(e); \
  t* s restrict _F_ACCESS_BASE(e)

#define _INIT_HOISTED_CHAR_PTR(e) \
  _ACCESS_BASE(e) = e->data; \
  _F_ACCESS_BASE(e) = e->origin

#define _INIT_HOISTED_TYPE_PTR(t,e) \
  _ACCESS_BASE(e) = (t*)e->data; \
  _F_ACCESS_BASE(e) = (t*)e->origin;

/* select the appropriate mechanism */

#ifdef _HOIST_ACCESS_POINTERS_TO_MLOOP
#ifdef _USE_CHARSTAR_PTRS
#define _DECL_INIT_HOISTED_PTRS(t,n,e) _DECL_INIT_HOISTED_CHAR_PTRS(n,e)
#define _DECL_HOISTED_PTRS(t,s,e) _DECL_HOISTED_CHAR_PTRS(s,e)
#define _INIT_HOISTED_PTR(t,e) _INIT_HOISTED_CHAR_PTR(e)
#else
#define _DECL_INIT_HOISTED_PTRS(t,n,e) _DECL_INIT_HOISTED_TYPE_PTRS(t,n,e)
#define _DECL_HOISTED_PTRS(t,s,e) _DECL_HOISTED_TYPE_PTRS(t,s,e)
#define _INIT_HOISTED_PTR(t,e) _INIT_HOISTED_TYPE_PTR(t,e)
#endif
#else

#ifdef _HOIST_ACCESS_POINTERS_TO_SUBPROG

#ifdef _USE_CHARSTAR_PTRS
#define _DECL_INIT_HOISTED_PTRS(t,n,e) _DECL_INIT_HOISTED_CHAR_PTRS(n,e)
#define _DECL_HOISTED_PTRS(t,s,e) _DECL_HOISTED_CHAR_PTRS(s,e)
#define _INIT_HOISTED_PTR(t,e) _INIT_HOISTED_CHAR_PTR(e)
#else
#define _DECL_INIT_HOISTED_PTRS(t,n,e) _DECL_INIT_HOISTED_TYPE_PTRS(t,n,e)
#define _DECL_HOISTED_PTRS(t,s,e) _DECL_HOISTED_TYPE_PTRS(t,s,e)
#define _INIT_HOISTED_PTR(t,e) _INIT_HOISTED_TYPE_PTR(t,e)
#endif

#endif

#endif


#define _SAFE_DECL_INIT_HOISTED_PTRS(t,n,e) \
  _DECL_HOISTED_PTRS(t, , e); \
  if (_T_GOOD_ARR(e)) { \
    _INIT_HOISTED_PTR(t, e); \
  }


/* Mechanisms for caching the multipliers */

#define _DECL_INIT_HOISTED_CHAR_MULTS_XD(n,e,x) const int _blk_##n##_##x = _ARR_BLK(e,x)
#define _DECL_INIT_HOISTED_TYPE_MULTS_XD(t,n,e,x) _DECL_INIT_HOISTED_CHAR_MULTS_XD(n,e,x)/sizeof(t)

/* select the appropriate mechanism */

#ifdef _HOIST_ACCESS_MULTS
#ifdef _USE_CHARSTAR_PTRS
#define _DECL_INIT_HOISTED_MULTS_XD(t,n,e,x) _DECL_INIT_HOISTED_CHAR_MULTS_XD(n,e,x)
#else
#define _DECL_INIT_HOISTED_MULTS_XD(t,n,e,x) _DECL_INIT_HOISTED_TYPE_MULTS_XD(t,n,e,x)
#endif
#else
#define _DECL_INIT_HOISTED_MULTS_XD(t,n,e,x)
#endif

#define _DECL_INIT_HOISTED_MULTS_1D(t,n,e) _DECL_INIT_HOISTED_MULTS_XD(t,n,e,0)
#define _DECL_INIT_HOISTED_MULTS_2D(t,n,e) _DECL_INIT_HOISTED_MULTS_1D(t,n,e); _DECL_INIT_HOISTED_MULTS_XD(t,n,e,1)
#define _DECL_INIT_HOISTED_MULTS_3D(t,n,e) _DECL_INIT_HOISTED_MULTS_2D(t,n,e); _DECL_INIT_HOISTED_MULTS_XD(t,n,e,2)
#define _DECL_INIT_HOISTED_MULTS_4D(t,n,e) _DECL_INIT_HOISTED_MULTS_3D(t,n,e); _DECL_INIT_HOISTED_MULTS_XD(t,n,e,3)
#define _DECL_INIT_HOISTED_MULTS_5D(t,n,e) _DECL_INIT_HOISTED_MULTS_4D(t,n,e); _DECL_INIT_HOISTED_MULTS_XD(t,n,e,4)
#define _DECL_INIT_HOISTED_MULTS_6D(t,n,e) _DECL_INIT_HOISTED_MULTS_5D(t,n,e); _DECL_INIT_HOISTED_MULTS_XD(t,n,e,5)




/* definition of normal access macros */

#ifndef _HOIST_ACCESS_POINTERS_TO_MLOOP
#ifndef _HOIST_ACCESS_POINTERS_TO_SUBPROG
#define _F_HOISTED_ACCESS_BASE(n,e) (e)->origin
#define _HOISTED_ACCESS_BASE(n,e) (e)->data
#else
#define _F_HOISTED_ACCESS_BASE(n,e) _origin_##e
#define _HOISTED_ACCESS_BASE(n,e) _data_##e
#endif
#else
#define _F_HOISTED_ACCESS_BASE(n,e) _origin_##n
#define _HOISTED_ACCESS_BASE(n,e) _data_##n
#endif


#define _ACCESS_BLK(e,d) _blk_##e##_##d

#ifdef _HOIST_ACCESS_MULTS
#define _FCAD(N,E,i,d) ((i)*_ACCESS_BLK(N,d))
#else
#define _FCAD(N,E,i,d) ((i)*_ARR_BLK(E,d))
#endif


#define _F_HOISTED_ACCESS_1D(n,e,i0) (_F_HOISTED_ACCESS_BASE(n,e) + _FCAD(n,e,i0,0))

#define _F_HOISTED_ACCESS_2D(n,e,i0,i1) (_F_HOISTED_ACCESS_BASE(n,e) + \
                               _FCAD(n,e,i0,0) + _FCAD(n,e,i1,1))

#define _F_HOISTED_ACCESS_3D(n,e,i0,i1,i2) (_F_HOISTED_ACCESS_BASE(n,e)+ \
                                _FCAD(n,e,i0,0) + _FCAD(n,e,i1,1) + _FCAD(n,e,i2,2))

#define _F_HOISTED_ACCESS_4D(n,e,i0,i1,i2,i3) (_F_HOISTED_ACCESS_BASE(n,e)+ \
                                     _FCAD(n,e,i0,0) + _FCAD(n,e,i1,1) + \
                                     _FCAD(n,e,i2,2) + _FCAD(n,e,i3,3))

#define _F_HOISTED_ACCESS_5D(n,e,i0,i1,i2,i3,i4) (_F_HOISTED_ACCESS_BASE(n,e)+ _FCAD(n,e,i0,0) +\
					_FCAD(n,e,i1,1) + _FCAD(n,e,i2,2) +\
					_FCAD(n,e,i3,3) + _FCAD(n,e,i4,4))


#define _F_HOISTED_ACCESS_6D(n,e,i0,i1,i2,i3,i4,i5) (_F_HOISTED_ACCESS_BASE(n,e)+\
					   _FCAD(n,e,i0,0) + _FCAD(n,e,i1,1) +\
					   _FCAD(n,e,i2,2) + _FCAD(n,e,i3,3) +\
					   _FCAD(n,e,i4,4) + _FCAD(n,e,i5,5))


#ifdef _HOIST_ACCESS_MULTS
#define _CAD(N,E,i,d) ((((i)-_ARR_OFF(E,d))*_ACCESS_BLK(N,d))/_ARR_STR(E,d))
#else
#define _CAD(N,E,i,d) ((((i)-_ARR_OFF(E,d))*_ARR_BLK(E,d))/_ARR_STR(E,d))
#endif

#define _HOISTED_ACCESS_1D(n,e,i0) (_HOISTED_ACCESS_BASE(n,e) + _CAD(n,e,i0,0))

#define _HOISTED_ACCESS_2D(n,e,i0,i1) (_HOISTED_ACCESS_BASE(n,e) + _CAD(n,e,i0,0) + _CAD(n,e,i1,1))

#define _HOISTED_ACCESS_3D(n,e,i0,i1,i2) (_HOISTED_ACCESS_BASE(n,e)+ _CAD(n,e,i0,0) +\
				_CAD(n,e,i1,1) + _CAD(n,e,i2,2))

#define _HOISTED_ACCESS_4D(n,e,i0,i1,i2,i3) (_HOISTED_ACCESS_BASE(n,e)+ _CAD(n,e,i0,0) +\
				   _CAD(n,e,i1,1) + _CAD(n,e,i2,2) + _CAD(n,e,i3,3))

#define _HOISTED_ACCESS_5D(n,e,i0,i1,i2,i3,i4) (_HOISTED_ACCESS_BASE(n,e)+ _CAD(n,e,i0,0) +\
				      _CAD(n,e,i1,1) + _CAD(n,e,i2,2) +\
				      _CAD(n,e,i3,3) + _CAD(n,e,i4,4))


#define _HOISTED_ACCESS_6D(n,e,i0,i1,i2,i3,i4,i5) (_HOISTED_ACCESS_BASE(n,e)+ _CAD(n,e,i0,0) +\
					 _CAD(n,e,i1,1) + _CAD(n,e,i2,2) +\
					 _CAD(n,e,i3,3) + _CAD(n,e,i4,4) +\
					 _CAD(n,e,i5,5))

#endif

