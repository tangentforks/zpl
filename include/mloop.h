/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MLOOP_H_
#define __MLOOP_H_

#include "md_zinc.h"
#ifndef _MD_ML_PRE
#define _MD_ML_PRE
#endif

/* new access macros and decl/init code macros */
/* The are macros of the following forms:
 *   _DECL_dD()
 *   _INIT_dD(reg)
 *   _DECL_ENS_dD(e)
 *   _INIT_ENS_dD(e)
 *   _NEW_ACCESS_dD(e, ....)
 */

/* variables used by MLOOPs */

#define _I(d)       _i##d
#define _ILO(d)     _i##d##Lo
#define _IHI(d)     _i##d##Hi
#define _ISTR(d)    _i##d##Str
#define _ITMP(d)    _i##d##Tmp
#define _IUNRHI(d)  _i##d##UnrHi
#define _IUNRLO(d)  _i##d##UnrLo
#define _ITSTOP(d)  _i##d##TStop
#define _ITLO(d)    _i##d##TLo
#define _ITHI(d)    _i##d##THi
#define _ITBUMP(d)  _i##d##TBump
#define _ITGSTOP(d) _i##d##TGStop

/* ways of bumping the indices */

#define _BUMP_I_UP(d) (_I(d)++)
#define _BUMP_I_DN(d) (_I(d)--)

#define _STR_BUMP_I_UP(d) (_I(d)+=_ISTR(d))
#define _STR_BUMP_I_DN(d) (_I(d)-=_ISTR(d))


/* declarations and initializations of these variables */

#define _DECL_XD(d) _I(d),_ILO(d),_IHI(d)

#define _DECL_1D() int _DECL_XD(0)
#define _DECL_2D() _DECL_1D(),_DECL_XD(1)
#define _DECL_3D() _DECL_2D(),_DECL_XD(2)
#define _DECL_4D() _DECL_3D(),_DECL_XD(3)
#define _DECL_5D() _DECL_4D(),_DECL_XD(4)
#define _DECL_6D() _DECL_5D(),_DECL_XD(5)

#define _STR_DECL_XD(d) _DECL_XD(d),_ISTR(d)

#define _STR_DECL_1D() int _STR_DECL_XD(0)
#define _STR_DECL_2D() _STR_DECL_1D(),_STR_DECL_XD(1)
#define _STR_DECL_3D() _STR_DECL_2D(),_STR_DECL_XD(2)
#define _STR_DECL_4D() _STR_DECL_3D(),_STR_DECL_XD(3)
#define _STR_DECL_5D() _STR_DECL_4D(),_STR_DECL_XD(4)
#define _STR_DECL_6D() _STR_DECL_5D(),_STR_DECL_XD(5)

#define _INIT_XD_LO(reg,d) _ILO(d)=_REG_MYLO(reg,d)
#define _INIT_XD_HI(reg,d) _IHI(d)=_REG_MYHI(reg,d)
#define _INIT_XD_STR(reg,d) _ISTR(d)=_REG_STRIDE(reg,d)

#define _INIT_XD_UNR_UP(reg,d,ur) _IUNRHI(d)=_IHI(d)-(ur-1)
#define _INIT_XD_UNR_DN(reg,d,ur) _IUNRLO(d)=_ILO(d)+(ur-1)
#define _INIT_XD_UNR_STR_UP(reg,d,ur) _IUNRHI(d)=_IHI(d)-((ur-1)*_ISTR(d))
#define _INIT_XD_UNR_STR_DN(reg,d,ur) _IUNRLO(d)=_ILO(d)+((ur-1)*_ISTR(d))

#define _INIT_XD(reg,d) _INIT_XD_LO(reg,d);_INIT_XD_HI(reg,d)

#define _INIT_1D(reg) _INIT_XD(reg,0)
#define _INIT_2D(reg) _INIT_1D(reg);_INIT_XD(reg,1)
#define _INIT_3D(reg) _INIT_2D(reg);_INIT_XD(reg,2)
#define _INIT_4D(reg) _INIT_3D(reg);_INIT_XD(reg,3)
#define _INIT_5D(reg) _INIT_4D(reg);_INIT_XD(reg,4)
#define _INIT_6D(reg) _INIT_5D(reg);_INIT_XD(reg,5)

#define _STR_INIT_XD(reg,d) _INIT_XD(reg,d);_INIT_XD_STR(reg,d)

#define _STR_INIT_1D(reg) _STR_INIT_XD(reg,0)
#define _STR_INIT_2D(reg) _STR_INIT_1D(reg);_STR_INIT_XD(reg,1)
#define _STR_INIT_3D(reg) _STR_INIT_2D(reg);_STR_INIT_XD(reg,2)
#define _STR_INIT_4D(reg) _STR_INIT_3D(reg);_STR_INIT_XD(reg,3)
#define _STR_INIT_5D(reg) _STR_INIT_4D(reg);_STR_INIT_XD(reg,4)
#define _STR_INIT_6D(reg) _STR_INIT_5D(reg);_STR_INIT_XD(reg,5)

#define _DECL_INIT_XD_LO(reg,d) const int _INIT_XD_LO(reg,d)
#define _DECL_INIT_XD_HI(reg,d) const int _INIT_XD_HI(reg,d)
#define _DECL_INIT_XD_STR(reg,d) const int _INIT_XD_STR(reg,d)

#define _DECL_INIT_XD_UNR_UP(reg,d,ur) const int _INIT_XD_UNR_UP(reg,d,ur)
#define _DECL_INIT_XD_UNR_DN(reg,d,ur) const int _INIT_XD_UNR_DN(reg,d,ur)
#define _DECL_INIT_XD_UNR_STR_UP(reg,d,ur) \
  const int _INIT_XD_UNR_STR_UP(reg,d,ur)
#define _DECL_INIT_XD_UNR_STR_DN(reg,d,ur) \
  const int _INIT_XD_UNR_STR_DN(reg,d,ur)

#define _DECL_XD_TIL(d) int _ITLO(d),_ITHI(d)
#define _DECL_INIT_XD_TIL(reg,d,t) \
  _DECL_XD_TIL(d)
#define _DECL_INIT_XD_TIL_STR(reg,d,t) \
  _DECL_XD_TIL(d); \
  const int _ITBUMP(d) = _REG_STRIDE(reg,d)*(t)

#define _NARROW_TILE_WIDTH(d,t)     ((_IHI(d)-_ILO(d)+1)%t)
#define _NARROW_TILE_WIDTH_STR(d,t) ((((_IHI(d)-_ILO(d))/_ISTR(d)+1)%t)*_ISTR(d))

#define _DECL_INIT_XD_TIL_UP(reg,d,t) \
  _DECL_INIT_XD_TIL(reg,d,t); \
  const int _ITSTOP(d)=_IHI(d)-_NARROW_TILE_WIDTH(d,t)
#define _DECL_INIT_XD_TIL_DN(reg,d,t) \
  _DECL_INIT_XD_TIL(reg,d,t); \
  const int _ITSTOP(d)=_ILO(d)+_NARROW_TILE_WIDTH(d,t)
#define _DECL_INIT_XD_TIL_STR_UP(reg,d,t) \
  _DECL_INIT_XD_TIL_STR(reg,d,t); \
  const int _ITSTOP(d)=_IHI(d)-_NARROW_TILE_WIDTH_STR(d,t)
#define _DECL_INIT_XD_TIL_STR_DN(reg,d,t) \
  _DECL_INIT_XD_TIL_STR(reg,d,t); \
  const int _ITSTOP(d)=_ILO(d)+_NARROW_TILE_WIDTH_STR(d,t)

#define _DECL_INIT_XD(reg,d) \
  int _I(d); \
  _DECL_INIT_XD_LO(reg,d); \
  _DECL_INIT_XD_HI(reg,d)

#define _DECL_INIT_1D(reg) _DECL_INIT_XD(reg,0)
#define _DECL_INIT_2D(reg) _DECL_INIT_1D(reg);_DECL_INIT_XD(reg,1)
#define _DECL_INIT_3D(reg) _DECL_INIT_2D(reg);_DECL_INIT_XD(reg,2)
#define _DECL_INIT_4D(reg) _DECL_INIT_3D(reg);_DECL_INIT_XD(reg,3)
#define _DECL_INIT_5D(reg) _DECL_INIT_4D(reg);_DECL_INIT_XD(reg,4)
#define _DECL_INIT_6D(reg) _DECL_INIT_5D(reg);_DECL_INIT_XD(reg,5)

#define _STR_DECL_INIT_XD(reg,d) _DECL_INIT_XD(reg,d);_DECL_INIT_XD_STR(reg,d)

#define _STR_DECL_INIT_1D(reg) _STR_DECL_INIT_XD(reg,0)
#define _STR_DECL_INIT_2D(reg) _STR_DECL_INIT_1D(reg);_STR_DECL_INIT_XD(reg,1)
#define _STR_DECL_INIT_3D(reg) _STR_DECL_INIT_2D(reg);_STR_DECL_INIT_XD(reg,2)
#define _STR_DECL_INIT_4D(reg) _STR_DECL_INIT_3D(reg);_STR_DECL_INIT_XD(reg,3)
#define _STR_DECL_INIT_5D(reg) _STR_DECL_INIT_4D(reg);_STR_DECL_INIT_XD(reg,4)
#define _STR_DECL_INIT_6D(reg) _STR_DECL_INIT_5D(reg);_STR_DECL_INIT_XD(reg,5)

#define _FLF_DECL_INIT_XD_LO(newreg,reg,d) _DECL_INIT_XD_LO(reg,d)+_SPS_REG_FLUFF_LO(newreg,d)
#define _FLF_DECL_INIT_XD_HI(newreg,reg,d) _DECL_INIT_XD_HI(reg,d)+_SPS_REG_FLUFF_HI(newreg,d)

#define _FLF_DECL_INIT_XD(newreg,reg,d) \
  int _I(d); \
  _FLF_DECL_INIT_XD_LO(newreg,reg,d); \
  _FLF_DECL_INIT_XD_HI(newreg,reg,d)

#define _FLF_DECL_INIT_1D(newreg,reg) _FLF_DECL_INIT_XD(newreg,reg,0)
#define _FLF_DECL_INIT_2D(newreg,reg) _FLF_DECL_INIT_1D(newreg,reg);_FLF_DECL_INIT_XD(newreg,reg,1)
#define _FLF_DECL_INIT_3D(newreg,reg) _FLF_DECL_INIT_2D(newreg,reg);_FLF_DECL_INIT_XD(newreg,reg,2)
#define _FLF_DECL_INIT_4D(newreg,reg) _FLF_DECL_INIT_3D(newreg,reg);_FLF_DECL_INIT_XD(newreg,reg,3)
#define _FLF_DECL_INIT_5D(newreg,reg) _FLF_DECL_INIT_4D(newreg,reg);_FLF_DECL_INIT_XD(newreg,reg,4)
#define _FLF_DECL_INIT_6D(newreg,reg) _FLF_DECL_INIT_5D(newreg,reg);_FLF_DECL_INIT_XD(newreg,reg,5)

#define _STR_FLF_DECL_INIT_XD(newreg,reg,d) \
  _FLF_DECL_INIT_XD(newreg,reg,d); \
  _DECL_INIT_XD_STR(reg,d)  

#define _STR_FLF_DECL_INIT_1D(newreg,reg) _STR_FLF_DECL_INIT_XD(newreg,reg,0)
#define _STR_FLF_DECL_INIT_2D(newreg,reg) _STR_FLF_DECL_INIT_1D(newreg,reg);_STR_FLF_DECL_INIT_XD(newreg,reg,1)
#define _STR_FLF_DECL_INIT_3D(newreg,reg) _STR_FLF_DECL_INIT_2D(newreg,reg);_STR_FLF_DECL_INIT_XD(newreg,reg,2)
#define _STR_FLF_DECL_INIT_4D(newreg,reg) _STR_FLF_DECL_INIT_3D(newreg,reg);_STR_FLF_DECL_INIT_XD(newreg,reg,3)
#define _STR_FLF_DECL_INIT_5D(newreg,reg) _STR_FLF_DECL_INIT_4D(newreg,reg);_STR_FLF_DECL_INIT_XD(newreg,reg,4)
#define _STR_FLF_DECL_INIT_6D(newreg,reg) _STR_FLF_DECL_INIT_5D(newreg,reg);_STR_FLF_DECL_INIT_XD(newreg,reg,5)

#define _DECL_GTILE_STOP_XD(d) _ITGSTOP(d)

#define _DECL_GTILE_STOP_1D() int _DECL_GTILE_STOP_XD(0)
#define _DECL_GTILE_STOP_2D() _DECL_GTILE_STOP_1D(),_DECL_GTILE_STOP_XD(1)
#define _DECL_GTILE_STOP_3D() _DECL_GTILE_STOP_2D(),_DECL_GTILE_STOP_XD(2)
#define _DECL_GTILE_STOP_4D() _DECL_GTILE_STOP_3D(),_DECL_GTILE_STOP_XD(3)
#define _DECL_GTILE_STOP_5D() _DECL_GTILE_STOP_4D(),_DECL_GTILE_STOP_XD(4)
#define _DECL_GTILE_STOP_6D() _DECL_GTILE_STOP_5D(),_DECL_GTILE_STOP_XD(5)

/* values for initializing walkers */

#define _WALK_UP(d) _ILO(d)
#define _WALK_DN(d) _IHI(d)

#define _WALK_TILE_UP_UP(d,t) _ILO(d)
#define _WALK_TILE_UP_DN(d,t) _ILO(d)+(t-1)
#define _WALK_TILE_DN_DN(d,t) _IHI(d)
#define _WALK_TILE_DN_UP(d,t) _IHI(d)-(t-1)

#define _STR_WALK_UP(d) _ILO(d)
#define _STR_WALK_DN(d) _IHI(d)

#define _STR_WALK_TILE_UP_UP(d,t) _ILO(d)
#define _STR_WALK_TILE_UP_DN(d,t) _ILO(d)+(_ITBUMP(d)-_ISTR(d))
#define _STR_WALK_TILE_DN_DN(d,t) _IHI(d)
#define _STR_WALK_TILE_DN_UP(d,t) _IHI(d)-(_ITBUMP(d)-_ISTR(d))


/* traditional MLOOP headers */

#define _MLOOP_UP(reg,d) \
  _MD_ML_PRE for (_I(d)=_ILO(d);_I(d)<=_IHI(d);_BUMP_I_UP(d))
#define _MLOOP_DN(reg,d) \
  _MD_ML_PRE for (_I(d)=_IHI(d);_I(d)>=_ILO(d);_BUMP_I_DN(d))

#define _STR_MLOOP_UP(reg,d) \
  _MD_ML_PRE for (_I(d)=_ILO(d);_I(d)<=_IHI(d);_STR_BUMP_I_UP(d))
#define _STR_MLOOP_DN(reg,d) \
  _MD_ML_PRE for (_I(d)=_IHI(d);_I(d)>=_ILO(d);_STR_BUMP_I_DN(d))


/* unrolled MLOOP headers */

#ifndef _NEVER_UNROLL

#define _UNROLLED_MLOOP_UP(reg,d) \
  _MD_ML_PRE for (_I(d)=_ILO(d);_I(d)<=_IUNRHI(d);_BUMP_I_UP(d))
#define _UNROLLED_MLOOP_DN(reg,d) \
  _MD_ML_PRE for (_I(d)=_IHI(d);_I(d)>=_IUNRLO(d);_BUMP_I_DN(d))

#define _UNROLLED_STR_MLOOP_UP(reg,d) \
  _MD_ML_PRE for (_I(d)=_ILO(d);_I(d)<=_IUNRHI(d);_STR_BUMP_I_UP(d))
#define _UNROLLED_STR_MLOOP_DN(reg,d) \
  _MD_ML_PRE for (_I(d)=_IHI(d);_I(d)>=_IUNRLO(d);_STR_BUMP_I_DN(d))

#define _CONT_MLOOP_UP(reg,d) for (;_I(d)<=_IHI(d);_BUMP_I_UP(d))
#define _CONT_MLOOP_DN(reg,d) for (;_I(d)>=_ILO(d);_BUMP_I_DN(d))

#define _CONT_STR_MLOOP_UP(reg,d) for (;_I(d)<=_IHI(d);_STR_BUMP_I_UP(d))
#define _CONT_STR_MLOOP_DN(reg,d) for (;_I(d)>=_ILO(d);_STR_BUMP_I_DN(d))

#else

#define _UNROLLED_MLOOP_UP(reg,d) \
  if (0)
#define _UNROLLED_MLOOP_DN(reg,d) \
  if (0)

#define _UNROLLED_STR_MLOOP_UP(reg,d) \
  if (0)
#define _UNROLLED_STR_MLOOP_DN(reg,d) \
  if (0)

#define _CONT_MLOOP_UP(reg,d) \
  _MD_ML_PRE for (_I(d) = _ILO(d);_I(d)<=_IHI(d);_BUMP_I_UP(d))
#define _CONT_MLOOP_DN(reg,d) \
  _MD_ML_PRE for (_I(d) = _IHI(d);_I(d)>=_ILO(d);_BUMP_I_DN(d))

#define _CONT_STR_MLOOP_UP(reg,d) \
  _MD_ML_PRE for (_I(d) = _ILO(d);_I(d)<=_IHI(d);_STR_BUMP_I_UP(d))
#define _CONT_STR_MLOOP_DN(reg,d) \
  _MD_ML_PRE for (_I(d) = _IHI(d);_I(d)>=_ILO(d);_STR_BUMP_I_DN(d))

#endif

/* generic tiled MLOOP headers */

#define _G_TILED_OUTER_MLOOP_UP_FOR_UP(reg,d,t) \
  for (_ITLO(d)=_ILO(d),_ITHI(d)=_ITLO(d)+(t); \
       _ITLO(d)<=_ITSTOP(d)+((t)); \
       _ITLO(d)=_ITHI(d),_ITHI(d)+=(t))
#define _G_TILED_OUTER_MLOOP_DN_FOR_DN(reg,d,t) \
  for (_ITHI(d)=_IHI(d),_ITLO(d)=_ITHI(d)-(t); \
       _ITHI(d)>=_ITSTOP(d)-((t)); \
       _ITHI(d)=_ITLO(d),_ITLO(d)-=(t))

#define _G_TILED_INNER_MLOOP_UP(reg,d,t) \
  for (_I(d)=_ITLO(d); _I(d)<_ITGSTOP(d); _BUMP_I_UP(d))
#define _G_TILED_INNER_MLOOP_DN(reg,d,t) \
  for (_I(d)=_ITHI(d); _I(d)>_ITGSTOP(d); _BUMP_I_DN(d))

/* dir1_FOR_dir2 indicates that the tiles are visited in dir1 and the inner
   loop will be in dir2 */

#define _TILED_OUTER_MLOOP_UP_FOR_UP(reg,d,t) \
  for (_ITLO(d)=_ILO(d),_ITHI(d)=_ITLO(d)+(t); \
       _ITLO(d)<=_ITSTOP(d); \
       _ITLO(d)=_ITHI(d),_ITHI(d)+=(t))
#define _TILED_OUTER_MLOOP_UP_FOR_DN(reg,d,t) \
  for (_ITLO(d)=_ILO(d)-1,_ITHI(d)=_ITLO(d)+(t); \
       _ITLO(d)<_ITSTOP(d); \
       _ITLO(d)=_ITHI(d),_ITHI(d)+=(t))

#define _TILED_OUTER_MLOOP_DN_FOR_UP(reg,d,t) \
  for (_ITHI(d)=_IHI(d)+1,_ITLO(d)=_ITHI(d)-(t); \
       _ITHI(d)>_ITSTOP(d); \
       _ITHI(d)=_ITLO(d),_ITLO(d)-=(t))
#define _TILED_OUTER_MLOOP_DN_FOR_DN(reg,d,t) \
  for (_ITHI(d)=_IHI(d),_ITLO(d)=_ITHI(d)-(t); \
       _ITHI(d)>=_ITSTOP(d); \
       _ITHI(d)=_ITLO(d),_ITLO(d)-=(t))

/* inner loops are the same regardless of outer loop direction */

#define _TILED_INNER_MLOOP_UP(reg,d,t) \
  for (_I(d)=_ITLO(d); _I(d)<_ITHI(d); _BUMP_I_UP(d))
#define _TILED_INNER_MLOOP_DN(reg,d,t) \
  for (_I(d)=_ITHI(d); _I(d)>_ITLO(d); _BUMP_I_DN(d))

/* continuation loops depend on both outer and inner directions */

#define _TILED_CONT_MLOOP_UP_IN_UP(reg,d,t) \
  for (_I(d)=_ITLO(d); _I(d)<=_IHI(d); _BUMP_I_UP(d))
#define _TILED_CONT_MLOOP_UP_IN_DN(reg,d,t) \
  for (_I(d)=_ILO(d); _I(d)<_ITHI(d); _BUMP_I_UP(d))

#define _TILED_CONT_MLOOP_DN_IN_UP(reg,d,t) \
  for (_I(d)=_IHI(d); _I(d)>_ITLO(d); _BUMP_I_DN(d))
#define _TILED_CONT_MLOOP_DN_IN_DN(reg,d,t) \
  for (_I(d)=_ITHI(d); _I(d)>=_ILO(d); _BUMP_I_DN(d))


/* the strided versions of the same... */

#define _TILED_OUTER_STR_MLOOP_UP_FOR_UP(reg,d,t) \
  for (_ITLO(d)=_ILO(d),_ITHI(d)=_ITLO(d)+_ITBUMP(d); \
       _ITLO(d)<=_ITSTOP(d); \
       _ITLO(d)=_ITHI(d),_ITHI(d)+=_ITBUMP(d))
#define _TILED_OUTER_STR_MLOOP_UP_FOR_DN(reg,d,t) \
  for (_ITLO(d)=_ILO(d)-_ISTR(d),_ITHI(d)=_ITLO(d)+_ITBUMP(d); \
       _ITLO(d)<_ITSTOP(d); \
       _ITLO(d)=_ITHI(d),_ITHI(d)+=_ITBUMP(d))

#define _TILED_OUTER_STR_MLOOP_DN_FOR_UP(reg,d,t) \
  for (_ITHI(d)=_IHI(d)+_ISTR(d),_ITLO(d)=_ITHI(d)-_ITBUMP(d); \
       _ITHI(d)>_ITSTOP(d); \
       _ITHI(d)=_ITLO(d),_ITLO(d)-=_ITBUMP(d))
#define _TILED_OUTER_STR_MLOOP_DN_FOR_DN(reg,d,t) \
  for (_ITHI(d)=_IHI(d),_ITLO(d)=_ITHI(d)-_ITBUMP(d); \
       _ITHI(d)>=_ITSTOP(d); \
       _ITHI(d)=_ITLO(d),_ITLO(d)-=_ITBUMP(d))

#define _TILED_INNER_STR_MLOOP_UP(reg,d,t) \
  for (_I(d)=_ITLO(d); _I(d)<_ITHI(d); _STR_BUMP_I_UP(d))
#define _TILED_INNER_STR_MLOOP_DN(reg,d,t) \
  for (_I(d)=_ITHI(d)-_ISTR(d); _I(d)>=_ITLO(d); _STR_BUMP_I_DN(d))


#define _TILED_CONT_STR_MLOOP_UP_IN_UP(reg,d,t) \
  for (_I(d)=_ITLO(d); _I(d)<=_IHI(d); _STR_BUMP_I_UP(d))
#define _TILED_CONT_STR_MLOOP_UP_IN_DN(reg,d,t) \
  for (_I(d)=_ILO(d); _I(d)<_ITHI(d); _STR_BUMP_I_UP(d))

#define _TILED_CONT_STR_MLOOP_DN_IN_UP(reg,d,t) \
  for (_I(d)=_IHI(d); _I(d)>_ITLO(d); _STR_BUMP_I_DN(d))
#define _TILED_CONT_STR_MLOOP_DN_IN_DN(reg,d,t) \
  for (_I(d)=_ITHI(d); _I(d)>=_ILO(d); _STR_BUMP_I_DN(d))


/* unrolled tiled MLOOP headers */

#define _UNROLLED_TILED_INNER_MLOOP_UP(reg,d,t) _I(d)=_ITLO(d);
#define _UNROLLED_TILED_INNER_MLOOP_DN(reg,d,t) _I(d)=_ITHI(d);
#define _UNROLLED_TILED_INNER_STR_MLOOP_UP(reg,d,t) _I(d)=_ITLO(d);
#define _UNROLLED_TILED_INNER_STR_MLOOP_DN(reg,d,t) _I(d)=_ITHI(d);


/* Maria's old tiled MLOOP headers -- can be removed soon */

#define _TILED_MLOOP_UP(reg,d,t) for (_I(d)=_ILO(d);_I(d)+t-1<=_IHI(d);)
#define _TILED_MLOOP_DN(reg,d,t) for (_I(d)=_IHI(d);_I(d)+t-1>=_ILO(d);)
#define _TILE_FIXLOOP_UP(d,t) for (_ITMP(d)=0;_I(d)<=_IHI(d)&&_ITMP(d)<t;_BUMP_I_UP(d),_ITMP(d)++)
#define _TILE_FIXLOOP_DN(d,t) for (_ITMP(d)=0;_I(d)>=_ILO(d)&&_ITMP(d)<t;_BUMP_I_DN(d),_ITMP(d)++)

#define _BIG_BUMPER(d,arr) (_IHI(d)-_ILO(d)+1)*_Bumper_##arr##_##d
#define _BUMPER_NTR_INIT(d0,d1,arr,t) ((_IHI(d1)-_ILO(d1)+1-t)*_Bumper_##arr##_##d1+_Bumper_##arr##_##d0)
#define _BUMPER_NNTR_INIT(d0,d1,arr,t) ((t>(_IHI(d1)-_ILO(d1)+1))?(_Bumper_##arr##_##d0):(_Bumper_##arr##_##d0+(_IHI(d1)-_ILO(d1)+1-((_IHI(d1)-_ILO(d1)+1)%t))*_Bumper_##arr##_##d1))
#define _BUMPER_NT_INIT(d0,d1,arr,t) (-t*_Bumper_##arr##_##d0-t*(_IHI(d1)-_ILO(d1))*_Bumper_##arr##_##d1)
#define _BUMPER_NST_INIT(d0,d1,arr,t) ((t>(_IHI(d1)-_ILO(d1)+1))?(-(_IHI(d0)-_ILO(d0)+1)*(_Bumper_##arr##_##d0+(_IHI(d1)-_ILO(d1)+1-t)*_Bumper_##arr##_##d1)):(-((_IHI(d0)-_ILO(d0)+1)%t)*_Bumper_##arr##_##d0-(((_IHI(d0)-_ILO(d0)+1)%t)*(_IHI(d1)-_ILO(d1)+1)-t)*_Bumper_##arr##_##d1))


#define _STR_LOOP_NEST_BEGIN(reg,d) { \
  _STR_DECL_##d##D(); \
  _STR_INIT_##d##D(reg);

#define _STR_LOOP_NEST_END(reg,d) }

#define _STR_LOOP_NEST_1D(reg) \
  _STR_MLOOP_UP(0)

#define _STR_LOOP_NEST_2D(reg) _STR_LOOP_NEST_1D(reg) \
  _STR_MLOOP_UP(1)

#define _STR_LOOP_NEST_3D(reg) _STR_LOOP_NEST_2D(reg) \
  _STR_MLOOP_UP(2)

#define _STR_LOOP_NEST_4D(reg) _STR_LOOP_NEST_3D(reg) \
  _STR_MLOOP_UP(3)

#define _STR_LOOP_NEST_5D(reg) _STR_LOOP_NEST_4D(reg) \
  _STR_MLOOP_UP(4)

#define _STR_LOOP_NEST_6D(reg) _STR_LOOP_NEST_5D(reg) \
  _STR_MLOOP_UP(5)

#define _RMSM (_RMSCurrentMask)
#define _RMSF (_RMSCurrentWithFlag)

#define _TEST_GENERIC_MASK(d) (!_RMSM ||\
			       ((_N_ACCESS_##d##D(boolean,_RMSM) &&\
				 _RMSF) ||\
				(!_N_ACCESS_##d##D(boolean,_RMSM) &&\
				 !_RMSF)))

#define _DECL_INIT_XD_LO_PAN_N(reg,d) const int _ILO(d)=_REG_MYLO(reg,d)+_lob[d]
#define _DECL_INIT_XD_HI_PAN_N(reg,d) const int _IHI(d)=_REG_MYLO(reg,d)-1

#define _DECL_INIT_XD_LO_PAN_0(reg,d) _DECL_INIT_XD_LO(reg,d)
#define _DECL_INIT_XD_HI_PAN_0(reg,d) _DECL_INIT_XD_HI(reg,d)

#define _DECL_INIT_XD_LO_PAN_1(reg,d) const int _ILO(d)=_REG_MYHI(reg,d)+1
#define _DECL_INIT_XD_HI_PAN_1(reg,d) const int _IHI(d)=_REG_MYHI(reg,d)+_hib[d]

#define _DECL_INIT_XD_PAN_N(reg,d) \
  int _I(d); \
  _DECL_INIT_XD_LO_PAN_N(reg,d); \
  _DECL_INIT_XD_HI_PAN_N(reg,d)

#define _DECL_INIT_XD_PAN_0(reg,d) \
  int _I(d); \
  _DECL_INIT_XD_LO_PAN_0(reg,d); \
  _DECL_INIT_XD_HI_PAN_0(reg,d)

#define _DECL_INIT_XD_PAN_1(reg,d) \
  int _I(d); \
  _DECL_INIT_XD_LO_PAN_1(reg,d); \
  _DECL_INIT_XD_HI_PAN_1(reg,d)


#define _DECL_INIT_1D_PAN(reg,p1) _DECL_INIT_XD_PAN_##p1(reg,0)
#define _DECL_INIT_2D_PAN(reg,p1,p2) _DECL_INIT_1D_PAN(reg,p1);_DECL_INIT_XD_PAN_##p2(reg,1)
#define _DECL_INIT_3D_PAN(reg,p1,p2) _DECL_INIT_2D_PAN(reg,p1,p2);_DECL_INIT_XD(reg,2)
#define _DECL_INIT_4D_PAN(reg,p1,p2) _DECL_INIT_3D_PAN(reg,p1,p2);_DECL_INIT_XD(reg,3)
#define _DECL_INIT_5D_PAN(reg,p1,p2) _DECL_INIT_4D_PAN(reg,p1,p2);_DECL_INIT_XD(reg,4)
#define _DECL_INIT_6D_PAN(reg,p1,p2) _DECL_INIT_5D_PAN(reg,p1,p2);_DECL_INIT_XD(reg,5)

#include "mloop_opt.h"

#endif
