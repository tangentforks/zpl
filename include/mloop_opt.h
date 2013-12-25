/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _MLOOP_OPT_H_
#define _MLOOP_OPT_H_

#define _DECL_INIT_XD_CONST(reg,d) \
  const int _I(d) = _REG_MYLO(reg,d)
#define _STR_DECL_INIT_XD_CONST(reg,d) \
  const int _I(d) = _REG_MYLO(reg,d)


/* recursive declarations for flat (F) / normal (N) dimensions of an MLOOP */

#define _DECL_INIT_1D_N(reg) _DECL_INIT_XD(reg,0)
#define _DECL_INIT_1D_F(reg) _DECL_INIT_XD_CONST(reg,0)

#define _DECL_INIT_2D_N(t0,reg) \
  _DECL_INIT_1D_##t0(reg); _DECL_INIT_XD(reg,1)
#define _DECL_INIT_2D_F(t0,reg) \
  _DECL_INIT_1D_##t0(reg); _DECL_INIT_XD_CONST(reg,1)

#define _DECL_INIT_3D_N(t0,t1,reg) \
  _DECL_INIT_2D_##t1(t0,reg); _DECL_INIT_XD(reg,2)
#define _DECL_INIT_3D_F(t0,t1,reg) \
  _DECL_INIT_2D_##t1(t0,reg); _DECL_INIT_XD_CONST(reg,2)

#define _DECL_INIT_4D_N(t0,t1,t2,reg) \
  _DECL_INIT_3D_##t2(t0,t1,reg); _DECL_INIT_XD(reg,3)
#define _DECL_INIT_4D_F(t0,t1,t2,reg) \
  _DECL_INIT_3D_##t2(t0,t1,reg); _DECL_INIT_XD_CONST(reg,3)

#define _DECL_INIT_5D_N(t0,t1,t2,t3,reg) \
  _DECL_INIT_4D_##t3(t0,t1,t2,reg); _DECL_INIT_XD(reg,4)
#define _DECL_INIT_5D_F(t0,t1,t2,t3,reg) \
  _DECL_INIT_4D_##t3(t0,t1,t2,reg); _DECL_INIT_XD_CONST(reg,4)

#define _DECL_INIT_6D_N(t0,t1,t2,t3,t4,reg) \
  _DECL_INIT_5D_##t4(t0,t1,t2,t3,reg); _DECL_INIT_XD(reg,5)
#define _DECL_INIT_6D_F(t0,t1,t2,t3,t4,reg) \
  _DECL_INIT_5D_##t4(t0,t1,t2,t3,reg); _DECL_INIT_XD_CONST(reg,5)



/* wrappers for the above MLOOP declarations */

#define _DECL_INIT_1D_OPT(t0,reg) \
  _DECL_INIT_1D_##t0(reg)

#define _DECL_INIT_2D_OPT(t0,t1,reg) \
  _DECL_INIT_2D_##t1(t0,reg)

#define _DECL_INIT_3D_OPT(t0,t1,t2,reg) \
  _DECL_INIT_3D_##t2(t0,t1,reg)

#define _DECL_INIT_4D_OPT(t0,t1,t2,t3,reg) \
  _DECL_INIT_4D_##t3(t0,t1,t2,reg)

#define _DECL_INIT_5D_OPT(t0,t1,t2,t3,t4,reg) \
  _DECL_INIT_5D_##t4(t0,t1,t2,t3,reg)

#define _DECL_INIT_6D_OPT(t0,t1,t2,t3,t4,t5,reg) \
  _DECL_INIT_6D_##t5(t0,t1,t2,t3,t4,reg)




/* recursive declarations for flat (F) / normal (N) dimensions of an MLOOP */

#define _STR_DECL_INIT_1D_N(reg) _STR_DECL_INIT_XD(reg,0)
#define _STR_DECL_INIT_1D_F(reg) _STR_DECL_INIT_XD_CONST(reg,0)

#define _STR_DECL_INIT_2D_N(t0,reg) \
  _STR_DECL_INIT_1D_##t0(reg); _STR_DECL_INIT_XD(reg,1)
#define _STR_DECL_INIT_2D_F(t0,reg) \
  _STR_DECL_INIT_1D_##t0(reg); _STR_DECL_INIT_XD_CONST(reg,1)

#define _STR_DECL_INIT_3D_N(t0,t1,reg) \
  _STR_DECL_INIT_2D_##t1(t0,reg); _STR_DECL_INIT_XD(reg,2)
#define _STR_DECL_INIT_3D_F(t0,t1,reg) \
  _STR_DECL_INIT_2D_##t1(t0,reg); _STR_DECL_INIT_XD_CONST(reg,2)

#define _STR_DECL_INIT_4D_N(t0,t1,t2,reg) \
  _STR_DECL_INIT_3D_##t2(t0,t1,reg); _STR_DECL_INIT_XD(reg,3)
#define _STR_DECL_INIT_4D_F(t0,t1,t2,reg) \
  _STR_DECL_INIT_3D_##t2(t0,t1,reg); _STR_DECL_INIT_XD_CONST(reg,3)

#define _STR_DECL_INIT_5D_N(t0,t1,t2,t3,reg) \
  _STR_DECL_INIT_4D_##t3(t0,t1,t2,reg); _STR_DECL_INIT_XD(reg,4)
#define _STR_DECL_INIT_5D_F(t0,t1,t2,t3,reg) \
  _STR_DECL_INIT_4D_##t3(t0,t1,t2,reg); _STR_DECL_INIT_XD_CONST(reg,4)

#define _STR_DECL_INIT_6D_N(t0,t1,t2,t3,t4,reg) \
  _STR_DECL_INIT_5D_##t4(t0,t1,t2,t3,reg); _STR_DECL_INIT_XD(reg,5)
#define _STR_DECL_INIT_6D_F(t0,t1,t2,t3,t4,reg) \
  _STR_DECL_INIT_5D_##t4(t0,t1,t2,t3,reg); _STR_DECL_INIT_XD_CONST(reg,5)


/* wrappers for the above MLOOP declarations */

#define _STR_DECL_INIT_1D_OPT(t0,reg) \
  _STR_DECL_INIT_1D_##t0(reg)

#define _STR_DECL_INIT_2D_OPT(t0,t1,reg) \
  _STR_DECL_INIT_2D_##t1(t0,reg)

#define _STR_DECL_INIT_3D_OPT(t0,t1,t2,reg) \
  _STR_DECL_INIT_3D_##t2(t0,t1,reg)

#define _STR_DECL_INIT_4D_OPT(t0,t1,t2,t3,reg) \
  _STR_DECL_INIT_4D_##t3(t0,t1,t2,reg)

#define _STR_DECL_INIT_5D_OPT(t0,t1,t2,t3,t4,reg) \
  _STR_DECL_INIT_5D_##t4(t0,t1,t2,t3,reg)

#define _STR_DECL_INIT_6D_OPT(t0,t1,t2,t3,t4,t5,reg) \
  _STR_DECL_INIT_6D_##t5(t0,t1,t2,t3,t4,reg)


/* Here are the flat versions of these MLOOP directional things */

#define _WALK_UP_FLAT(d) _I(d)
#define _WALK_DN_FLAT(d) _I(d)

#define _MLOOP_UP_FLAT(reg,dim)
#define _MLOOP_DN_FLAT(reg,dim)
#define _STR_MLOOP_UP_FLAT(reg,dim)
#define _STR_MLOOP_DN_FLAT(reg,dim)



/* Here are the flat sparse MLOOPs */

#define _SPS_MLOOP_UPDN_FLAT(reg,d) \
  if (_SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d) == _I(d))


#define _SPS_CATCH_UP_MAIN_FLAT(reg,dim) \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,dim),dim) < _I(dim)) { \
    _SPS_BUMP_UP(reg,reg,dim); \
  }

#define _SPS_MAIN_MLOOP_UP_FLAT(reg,d) \
  _SPS_CATCH_UP_MAIN_FLAT(reg,d); \
  _SPS_MLOOP_UPDN_FLAT(reg,d)

#define _SPS_WALK_SETUP_MAIN_FLAT(reg,regdef,dim,corner) \
  _SPS_WLK(reg,dim) = _SPS_CORNER(reg,corner)


#define _SPS_CATCH_UP_MAIN_INNER_FLAT(reg,dim) \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,dim),dim) < _I(dim)) { \
    _SPS_BUMP_UP_INNER(reg,dim); \
  }


#define _SPS_MLOOP_UP_INNER_FLAT(reg,dim) \
  if (_SPSNODE_INDEX(reg,_SPS_WLK(reg,dim),dim) == _I(dim))


#define _SPS_MAIN_MLOOP_UP_INNER_FLAT(reg,dim) \
  _SPS_CATCH_UP_MAIN_INNER_FLAT(reg,dim); \
  _SPS_MLOOP_UP_INNER_FLAT(reg,dim)

#endif
