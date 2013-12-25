/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __PRIV_ACCESS_H_
#define __PRIV_ACCESS_H_

#ifndef _PRIV_TID_DECL
#define _PRIV_TID_DECL
#endif

#ifndef _PRIV_TID
#define _PRIV_TID
#endif

#ifndef _PRIV_ONCE_BEGIN
#define _PRIV_ONCE_BEGIN
#endif
#ifndef _PRIV_ONCE_END
#define _PRIV_ONCE_END
#endif

#ifndef _PRIV_ACCESS
#define _PRIV_ACCESS(x) x
#endif

#ifndef _PRIV_DECL
#define _PRIV_DECL(p_extern, p_type, p_var) p_extern p_type p_var
#endif

#ifndef _PRIV_ALLOC
#define _PRIV_ALLOC(p_type, p_name) ;
#endif

#ifndef _ENS_DECL
#define _ENS_DECL(flags, p_type1, p_type2, p_name) \
flags p_type1 _D_##p_name; \
flags p_type2 p_name=(&_D_##p_name)
#endif

#ifndef _ENS_DUAL_DECL
#define _ENS_DUAL_DECL(flags, p_type1, p_type2, p_name) \
flags p_type1 _D_##p_name; \
flags p_type2 p_name
#endif

#ifndef _REG_DECL
#define _REG_DECL(f,t1,t2,n) _ENS_DECL(f,t1,t2,n)
#endif

#ifndef _REG_DUAL_DECL
#define _REG_DUAL_DECL(f, t1, t2, n) _ENS_DUAL_DECL(f,t1,t2,n)
#endif

#ifndef _DIST_DECL
#define _DIST_DECL(f,t1,t2,n) _ENS_DECL(f,t1,t2,n)
#endif

#ifndef _DIST_DUAL_DECL
#define _DIST_DUAL_DECL(f, t1, t2, n) _ENS_DUAL_DECL(f,t1,t2,n)
#endif

#ifndef _GRID_DECL
#define _GRID_DECL(f,t1,t2,n) _ENS_DECL(f,t1,t2,n)
#endif

#ifndef _GRID_DUAL_DECL
#define _GRID_DUAL_DECL(f, t1, t2, n) _ENS_DUAL_DECL(f,t1,t2,n)
#endif

#define _DECL_ARR(type, id, s1) \
  type id[s1]

#define _INIT_1_ELTS(id, v0) \
  (id)[0] = (v0)

#define _INIT_2_ELTS(id, v0, v1) \
  (id)[1] = (v1); \
  _INIT_1_ELTS(id, v0)

#define _INIT_3_ELTS(id, v0, v1, v2) \
  (id)[2] = (v2); \
  _INIT_2_ELTS(id, v0, v1)

#define _INIT_4_ELTS(id, v0, v1, v2, v3) \
  (id)[3] = (v3); \
  _INIT_3_ELTS(id, v0, v1, v2)

#define _INIT_5_ELTS(id, v0, v1, v2, v3, v4) \
  (id)[4] = (v4); \
  _INIT_4_ELTS(id, v0, v1, v2, v3)

#define _INIT_6_ELTS(id, v0, v1, v2, v3, v4, v5) \
  (id)[5] = (v5); \
  _INIT_5_ELTS(id, v0, v1, v2, v3, v4)

#endif
