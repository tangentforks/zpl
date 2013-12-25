/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _ACCESS_OPT_H
#define _ACCESS_OPT_H

/* here are the 1-6D ACCESS macro guts -- MAXRANK dependent. */

#define _ACCESS_1D_OPT_N(e,i0) \
  _ACCESS_1D(e,i0)
#define _ACCESS_1D_OPT_F(e,i0) \
  (char*)((e)->data)

#define _ACCESS_2D_OPT_N(t0,e,i0,i1) \
  _ACCESS_1D_OPT_##t0(e,i0) + _AD(e,i1,1)
#define _ACCESS_2D_OPT_F(t0,e,i0,i1) \
  _ACCESS_1D_OPT_##t0(e,i0)

#define _ACCESS_3D_OPT_N(t0,t1,e,i0,i1,i2) \
  _ACCESS_2D_OPT_##t1(t0,e,i0,i1) + _AD(e,i2,2)
#define _ACCESS_3D_OPT_F(t0,t1,e,i0,i1,i2) \
  _ACCESS_2D_OPT_##t1(t0,e,i0,i1)

#define _ACCESS_4D_OPT_N(t0,t1,t2,e,i0,i1,i2,i3) \
  _ACCESS_3D_OPT_##t2(t0,t1,e,i0,i1,i2) + _AD(e,i3,3)
#define _ACCESS_4D_OPT_F(t0,t1,t2,e,i0,i1,i2,i3) \
  _ACCESS_3D_OPT_##t2(t0,t1,e,i0,i1,i2)

#define _ACCESS_5D_OPT_N(t0,t1,t2,t3,e,i0,i1,i2,i3,i4) \
  _ACCESS_4D_OPT_##t3(t0,t1,t2,e,i0,i1,i2,i3) + _AD(e,i4,4)
#define _ACCESS_5D_OPT_F(t0,t1,t2,t3,e,i0,i1,i2,i3,i4) \
  _ACCESS_4D_OPT_##t3(t0,t1,t2,e,i0,i1,i2,i3)

#define _ACCESS_6D_OPT_N(t0,t1,t2,t3,t4,e,i0,i1,i2,i3,i4,i5) \
  _ACCESS_5D_OPT_##t4(t0,t1,t2,t3,e,i0,i1,i2,i3,i4) + _AD(e,i5,5)
#define _ACCESS_6D_OPT_F(t0,t1,t2,t3,t4,e,i0,i1,i2,i3,i4,i5) \
  _ACCESS_5D_OPT_##t4(t0,t1,t2,t3,e,i0,i1,i2,i3,i4)


/* Here are the top-level access macros -- MAXRANK dependent.  
   t0-t5 are the symbols F or N, indicating whether each dimension is
   Flat (flood, rgrid, singleton) or Normal (1..n) */

#define _ACCESS_1D_OPT(t0,e,i0) \
  (_ACCESS_1D_OPT_##t0(e,i0))

#define _ACCESS_2D_OPT(t0,t1,e,i0,i1) \
  (_ACCESS_2D_OPT_##t1(t0,e,i0,i1))

#define _ACCESS_3D_OPT(t0,t1,t2,e,i0,i1,i2) \
  (_ACCESS_3D_OPT_##t2(t0,t1,e,i0,i1,i2))

#define _ACCESS_4D_OPT(t0,t1,t2,t3,e,i0,i1,i2,i3) \
  (_ACCESS_4D_OPT_##t3(t0,t1,t2,e,i0,i1,i2,i3))

#define _ACCESS_5D_OPT(t0,t1,t2,t3,t4,e,i0,i1,i2,i3,i4) \
  (_ACCESS_5D_OPT_##t4(t0,t1,t2,t3,e,i0,i1,i2,i3,i4))

#define _ACCESS_6D_OPT(t0,t1,t2,t3,t4,t5,e,i0,i1,i2,i3,i4,i5) \
  (_ACCESS_6D_OPT_##t5(t0,t1,t2,t3,t4,e,i0,i1,i2,i3,i4,i5))


/* here are the 1-6D _F_ACCESS macro guts -- MAXRANK dependent. */

#define _F_ACCESS_1D_OPT_N(e,i0) \
  _F_ACCESS_1D(e,i0)
#define _F_ACCESS_1D_OPT_F(e,i0) \
  (char*)((e)->origin)

#define _F_ACCESS_2D_OPT_N(t0,e,i0,i1) \
  _F_ACCESS_1D_OPT_##t0(e,i0) + _FAD(e,i1,1)
#define _F_ACCESS_2D_OPT_F(t0,e,i0,i1) \
  _F_ACCESS_1D_OPT_##t0(e,i0)

#define _F_ACCESS_3D_OPT_N(t0,t1,e,i0,i1,i2) \
  _F_ACCESS_2D_OPT_##t1(t0,e,i0,i1) + _FAD(e,i2,2)
#define _F_ACCESS_3D_OPT_F(t0,t1,e,i0,i1,i2) \
  _F_ACCESS_2D_OPT_##t1(t0,e,i0,i1)

#define _F_ACCESS_4D_OPT_N(t0,t1,t2,e,i0,i1,i2,i3) \
  _F_ACCESS_3D_OPT_##t2(t0,t1,e,i0,i1,i2) + _FAD(e,i3,3)
#define _F_ACCESS_4D_OPT_F(t0,t1,t2,e,i0,i1,i2,i3) \
  _F_ACCESS_3D_OPT_##t2(t0,t1,e,i0,i1,i2)

#define _F_ACCESS_5D_OPT_N(t0,t1,t2,t3,e,i0,i1,i2,i3,i4) \
  _F_ACCESS_4D_OPT_##t3(t0,t1,t2,e,i0,i1,i2,i3) + _FAD(e,i4,4)
#define _F_ACCESS_5D_OPT_F(t0,t1,t2,t3,e,i0,i1,i2,i3,i4) \
  _F_ACCESS_4D_OPT_##t3(t0,t1,t2,e,i0,i1,i2,i3)

#define _F_ACCESS_6D_OPT_N(t0,t1,t2,t3,t4,e,i0,i1,i2,i3,i4,i5) \
  _F_ACCESS_5D_OPT_##t4(t0,t1,t2,t3,e,i0,i1,i2,i3,i4) + _FAD(e,i5,5)
#define _F_ACCESS_6D_OPT_F(t0,t1,t2,t3,t4,e,i0,i1,i2,i3,i4,i5) \
  _F_ACCESS_5D_OPT_##t4(t0,t1,t2,t3,e,i0,i1,i2,i3,i4)


/* Here are the top-level _F_ACCESS macros -- MAXRANK dependent.  
   t0-t5 are the symbols F or N, indicating whether each dimension is
   Flat (flood, rgrid, singleton) or Normal (1..n) */

#define _F_ACCESS_1D_OPT(t0,e,i0) \
  (_F_ACCESS_1D_OPT_##t0(e,i0))

#define _F_ACCESS_2D_OPT(t0,t1,e,i0,i1) \
  (_F_ACCESS_2D_OPT_##t1(t0,e,i0,i1))

#define _F_ACCESS_3D_OPT(t0,t1,t2,e,i0,i1,i2) \
  (_F_ACCESS_3D_OPT_##t2(t0,t1,e,i0,i1,i2))

#define _F_ACCESS_4D_OPT(t0,t1,t2,t3,e,i0,i1,i2,i3) \
  (_F_ACCESS_4D_OPT_##t3(t0,t1,t2,e,i0,i1,i2,i3))

#define _F_ACCESS_5D_OPT(t0,t1,t2,t3,t4,e,i0,i1,i2,i3,i4) \
  (_F_ACCESS_5D_OPT_##t4(t0,t1,t2,t3,e,i0,i1,i2,i3,i4))

#define _F_ACCESS_6D_OPT(t0,t1,t2,t3,t4,t5,e,i0,i1,i2,i3,i4,i5) \
  (_F_ACCESS_6D_OPT_##t5(t0,t1,t2,t3,t4,e,i0,i1,i2,i3,i4,i5))


/* here are optimized DISTANCE macros for flat dimensions */

/* These are naive;  we can do better and do just below here
#define _DISTANCE_RESET_UP_FLAT(a,d)     _CALC_DISTANCE(1,a,d)
#define _DISTANCE_RESET_DN_FLAT(a,d)     _CALC_DISTANCE(-1,a,d)
*/

/* I no longer think these are correct: 
#define _DISTANCE_RESET_UP_FLAT(a,d)     _ABLK(a,d)
#define _DISTANCE_RESET_DN_FLAT(a,d)     (-_ABLK(a,d))
#define _DISTANCE_STR_RESET_UP_FLAT(a,d) _STR_CALC_DISTANCE(_ISTR(d),a,d)
#define _DISTANCE_STR_RESET_DN_FLAT(a,d) _STR_CALC_DISTANCE(-_ISTR(d),a,d)
*/

#define _DISTANCE_RESET_UP_FLAT(a,d)     0
#define _DISTANCE_RESET_DN_FLAT(a,d)     0
#define _DISTANCE_STR_RESET_UP_FLAT(a,d) 0
#define _DISTANCE_STR_RESET_DN_FLAT(a,d) 0

#define _DISTANCE_ADVNC_UP_FLAT(a,d)     0
#define _DISTANCE_ADVNC_DN_FLAT(a,d)     0
#define _DISTANCE_STR_ADVNC_UP_FLAT(a,d) 0
#define _DISTANCE_STR_ADVNC_DN_FLAT(a,d) 0





/* these are under development for Access macros whose fields have been
   tucked into scalars */

#define _ARR_BLK_SCALAR(e,d) e##_block##d

#define _FAD_B(E,i,d) ((i)*_ARR_BLK_SCALAR(E,d))

#define _F_ACCESS_2D_O_NF(e,i0,i1) (_SPS_ORIG(e) + _FAD(e,i0,0))
#define _F_ACCESS_2D_O_FN(e,i0,i1) (_SPS_ORIG(e) + _FAD(e,i1,1))

#define _F_ACCESS_2D_OB_NF(e,i0,i1) (_SPS_ORIG(e) + _FAD_B(e,i0,0))
#define _F_ACCESS_2D_OB_FN(e,i0,i1) (_SPS_ORIG(e) + _FAD_B(e,i1,1))


#endif
