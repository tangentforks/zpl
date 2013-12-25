/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ACCESS_H_
#define __ACCESS_H_

#include <math.h>
#include "cached_access.h"
#include "ensemble.h"
#include "zplglobal.h"
#include "zmacros.h"

#define _AD(E,i,d) ((((i)-_ARR_OFF(E,d))*_ARR_BLK(E,d))/_ARR_STR(E,d))


#define _ACCESS_1D(e,i0) (((char *)(e->data)) + _AD(e,i0,0))

#define _ACCESS_2D(e,i0,i1) (((char *)(e->data)) + _AD(e,i0,0) + _AD(e,i1,1))

#define _ACCESS_3D(e,i0,i1,i2) (((char *)(e->data))+ _AD(e,i0,0) +\
				_AD(e,i1,1) + _AD(e,i2,2))

#define _ACCESS_4D(e,i0,i1,i2,i3) (((char *)(e->data))+ _AD(e,i0,0) +\
				   _AD(e,i1,1) + _AD(e,i2,2) + _AD(e,i3,3))

#define _ACCESS_5D(e,i0,i1,i2,i3,i4) (((char *)(e->data))+ _AD(e,i0,0) +\
				      _AD(e,i1,1) + _AD(e,i2,2) +\
				      _AD(e,i3,3) + _AD(e,i4,4))


#define _ACCESS_6D(e,i0,i1,i2,i3,i4,i5) (((char *)(e->data))+ _AD(e,i0,0) +\
					 _AD(e,i1,1) + _AD(e,i2,2) +\
					 _AD(e,i3,3) + _AD(e,i4,4) +\
					 _AD(e,i5,5))


#define _N_ACCESS_1D(t,e) (*(t *)_ACCESS_1D(e,_i0))
#define _N_ACCESS_2D(t,e) (*(t *)_ACCESS_2D(e,_i0,_i1))
#define _N_ACCESS_3D(t,e) (*(t *)_ACCESS_3D(e,_i0,_i1,_i2))
#define _N_ACCESS_4D(t,e) (*(t *)_ACCESS_4D(e,_i0,_i1,_i2,_i3))
#define _N_ACCESS_5D(t,e) (*(t *)_ACCESS_5D(e,_i0,_i1,_i2,_i3,_i4))
#define _N_ACCESS_6D(t,e) (*(t *)_ACCESS_6D(e,_i0,_i1,_i2,_i3,_i4,_i5))



#define _FAD(E,i,d) ((i)*_ARR_BLK(E,d))


#define _F_ACCESS_1D(e,i0) (((char *)(e->origin)) + _FAD(e,i0,0))

#define _F_ACCESS_2D(e,i0,i1) (((char *)(e->origin)) + _FAD(e,i0,0) +\
			       _FAD(e,i1,1))

#define _F_ACCESS_3D(e,i0,i1,i2) (((char *)(e->origin))+ _FAD(e,i0,0) +\
				  _FAD(e,i1,1) + _FAD(e,i2,2))

#define _F_ACCESS_4D(e,i0,i1,i2,i3) (((char *)(e->origin))+ _FAD(e,i0,0) +\
				     _FAD(e,i1,1) + _FAD(e,i2,2) +\
				     _FAD(e,i3,3))

#define _F_ACCESS_5D(e,i0,i1,i2,i3,i4) (((char *)(e->origin))+ _FAD(e,i0,0) +\
					_FAD(e,i1,1) + _FAD(e,i2,2) +\
					_FAD(e,i3,3) + _FAD(e,i4,4))


#define _F_ACCESS_6D(e,i0,i1,i2,i3,i4,i5) (((char *)(e->origin))+\
					   _FAD(e,i0,0) + _FAD(e,i1,1) +\
					   _FAD(e,i2,2) + _FAD(e,i3,3) +\
					   _FAD(e,i4,4) + _FAD(e,i5,5))

#define _ACCESS_SPS_IDENT(e)    (_ARR_ORIGIN(e))
#define _ACCESS_SPS(e,i)        _ARR_ORIGIN(e)+(i)
#define _ACCESS_SPS_TYPE(t,e,i) (char *)(((t)_ARR_ORIGIN(e))+(i))


/* _ACCESS_xD_DISTANCE macros 
 * These macros are useful in determining the distance (in bytes) between
 * two elements in an ensemble.  
 * _ACCESS_2D_DISTANCE(e, i0hi, i1hi, i0lo, i1lo) ==
 *    _ACCESS_2D(e, i0hi, i1hi) - _ACCESS_2D(e, i0lo, i1lo)
 */

#define _ACCESS_1D_DISTANCE(e, i0lo, i0hi)              \
        (((i0hi) - (i0lo)) * e->blocksize[0]/_ARR_STR(e,0))

#define _ACCESS_2D_DISTANCE(e, i0lo, i1lo, i0hi, i1hi)  \
          ((((i0hi) - (i0lo)) * e->blocksize[0]/_ARR_STR(e,0)) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]/_ARR_STR(e,1)))
  

#define _ACCESS_3D_DISTANCE(e, i0lo, i1lo, i2lo, i0hi, i1hi, i2hi)  \
          ((((i0hi) - (i0lo)) * e->blocksize[0]/_ARR_STR(e,0)) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]/_ARR_STR(e,1)) \
        + (((i2hi) - (i2lo)) * e->blocksize[2]/_ARR_STR(e,2)) \
	   )

#define _ACCESS_4D_DISTANCE(e, i0lo, i1lo, i2lo, i3lo, i0hi, i1hi, i2hi, i3hi) \
          ((((i0hi) - (i0lo)) * e->blocksize[0]/_ARR_STR(e,0)) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]/_ARR_STR(e,1)) \
        + (((i2hi) - (i2lo)) * e->blocksize[2]/_ARR_STR(e,2)) \
        + (((i3hi) - (i3lo)) * e->blocksize[3]/_ARR_STR(e,3)) \
	   )


#define _ACCESS_5D_DISTANCE(e, i0lo, i1lo, i2lo, i3lo, i4lo, i0hi, i1hi, i2hi, i3hi, i4hi) \
          ((((i0hi) - (i0lo)) * e->blocksize[0]/_ARR_STR(e,0)) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]/_ARR_STR(e,1)) \
        + (((i2hi) - (i2lo)) * e->blocksize[2]/_ARR_STR(e,2)) \
        + (((i3hi) - (i3lo)) * e->blocksize[3]/_ARR_STR(e,3)) \
        + (((i4hi) - (i4lo)) * e->blocksize[4]/_ARR_STR(e,4)) \
	   )

#define _ACCESS_6D_DISTANCE(e, i0lo, i1lo, i2lo, i3lo, i4lo, i5lo, i0hi, i1hi, i2hi, i3hi, i4hi, i5hi) \
          ((((i0hi) - (i0lo)) * e->blocksize[0]/_ARR_STR(e,0)) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]/_ARR_STR(e,1)) \
        + (((i2hi) - (i2lo)) * e->blocksize[2]/_ARR_STR(e,2)) \
        + (((i3hi) - (i3lo)) * e->blocksize[3]/_ARR_STR(e,3)) \
        + (((i4hi) - (i4lo)) * e->blocksize[4]/_ARR_STR(e,4)) \
        + (((i5hi) - (i5lo)) * e->blocksize[5]/_ARR_STR(e,5)) \
	   )


#define _F_ACCESS_1D_DISTANCE(e, i0lo, i0hi)              \
        (((i0hi) - (i0lo)) * e->blocksize[0])

#define _F_ACCESS_2D_DISTANCE(e, i0lo, i1lo, i0hi, i1hi)  \
          ((((i0hi) - (i0lo)) * e->blocksize[0]) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]))
  

#define _F_ACCESS_3D_DISTANCE(e, i0lo, i1lo, i2lo, i0hi, i1hi, i2hi)  \
          ((((i0hi) - (i0lo)) * e->blocksize[0]) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]) \
        + (((i2hi) - (i2lo)) * e->blocksize[2]) \
	   )

#define _F_ACCESS_4D_DISTANCE(e, i0lo, i1lo, i2lo, i3lo, i0hi, i1hi, i2hi, i3hi) \
          ((((i0hi) - (i0lo)) * e->blocksize[0]) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]) \
        + (((i2hi) - (i2lo)) * e->blocksize[2]) \
        + (((i3hi) - (i3lo)) * e->blocksize[3]) \
	   )


#define _F_ACCESS_5D_DISTANCE(e, i0lo, i1lo, i2lo, i3lo, i4lo, i0hi, i1hi, i2hi, i3hi, i4hi) \
          ((((i0hi) - (i0lo)) * e->blocksize[0]) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]) \
        + (((i2hi) - (i2lo)) * e->blocksize[2]) \
        + (((i3hi) - (i3lo)) * e->blocksize[3]) \
        + (((i4hi) - (i4lo)) * e->blocksize[4]) \
	   )

#define _F_ACCESS_6D_DISTANCE(e, i0lo, i1lo, i2lo, i3lo, i4lo, i5lo, i0hi, i1hi, i2hi, i3hi, i4hi, i5hi) \
          ((((i0hi) - (i0lo)) * e->blocksize[0]) \
        + (((i1hi) - (i1lo)) * e->blocksize[1]) \
        + (((i2hi) - (i2lo)) * e->blocksize[2]) \
        + (((i3hi) - (i3lo)) * e->blocksize[3]) \
        + (((i4hi) - (i4lo)) * e->blocksize[4]) \
        + (((i5hi) - (i5lo)) * e->blocksize[5]) \
	   )

#define _STR_OFFSET(e,dim,off) (((off)*_ARR_BLK(e,dim))/_ARR_STR(e,dim))
#define _OFFSET(e,dim,off) ((off)*_ARR_BLK(e,dim))

#define _DECL_ARR_1D(e) char * _##e##_origin; \
                        int    _##e##_block0

#define _DECL_ARR_2D(e) char * _##e##_origin; \
                        int    _##e##_block0, _##e##_block1

#define _DECL_ARR_3D(e) char * _##e##_origin; \
                        int    _##e##_block0, _##e##_block1, _##e##_block2

#define _DECL_ARR_4D(e) char * _##e##_origin; \
                        int    _##e##_block0, _##e##_block1, _##e##_block2, \
                               _##e##_block3

#define _DECL_ARR_5D(e) char * _##e##_origin; \
                        int    _##e##_block0, _##e##_block1, _##e##_block2, \
                               _##e##_block3, _##e##_block4

#define _DECL_ARR_6D(e) char * _##e##_origin; \
                        int    _##e##_block0, _##e##_block1, _##e##_block2, \
                               _##e##_block3, _##e##_block4, _##e##_block4

#define _INIT_ARR_1D(e,v) _##v##_origin = (char *)(e->origin); \
                          _##v##_block0 = (e->blocksize[0])
#define _INIT_ARR_2D(e,v) _##v##_origin = (char *)(e->origin); \
                          _##v##_block0 = (e->blocksize[0]); \
                          _##v##_block1 = (e->blocksize[1])
#define _INIT_ARR_3D(e,v) _##v##_origin = (char *)(e->origin); \
                          _##v##_block0 = (e->blocksize[0]); \
                          _##v##_block1 = (e->blocksize[1]); \
                          _##v##_block2 = (e->blocksize[2])
#define _INIT_ARR_4D(e,v) _##v##_origin = (char *)(e->origin); \
                          _##v##_block0 = (e->blocksize[0]); \
                          _##v##_block1 = (e->blocksize[1]); \
                          _##v##_block2 = (e->blocksize[2]); \
                          _##v##_block3 = (e->blocksize[3])
#define _INIT_ARR_5D(e,v) _##v##_origin = (char *)(e->origin); \
                          _##v##_block0 = (e->blocksize[0]); \
                          _##v##_block1 = (e->blocksize[1]); \
                          _##v##_block2 = (e->blocksize[2]); \
                          _##v##_block3 = (e->blocksize[3]); \
                          _##v##_block4 = (e->blocksize[4])
#define _INIT_ARR_6D(e,v) _##v##_origin = (char *)(e->origin); \
                          _##v##_block0 = (e->blocksize[0]); \
                          _##v##_block1 = (e->blocksize[1]); \
                          _##v##_block2 = (e->blocksize[2]); \
                          _##v##_block3 = (e->blocksize[3]); \
                          _##v##_block4 = (e->blocksize[4]); \
                          _##v##_block5 = (e->blocksize[5])

#define _NEW_ACCESS_1D(e,i0) (((char *)(_##e##_origin))+( \
	       ((i0) * _##e##_block0) \
	       ))


#define _NEW_ACCESS_2D(e,i0,i1) (((char *)(_##e##_origin))+( \
               ((i0) * _##e##_block0) \
              +((i1) * _##e##_block1) \
               ))


#define _NEW_ACCESS_3D(e,i0,i1,i2) (((char *)(_##e##_origin))+( \
               ((i0) * _##e##_block0) \
              +((i1) * _##e##_block1) \
              +((i2) * _##e##_block2) \
               ))


#define _NEW_ACCESS_4D(e,i0,i1,i2,i3) (((char *)(_##e##_origin))+( \
               ((i0) * _##e##_block0) \
              +((i1) * _##e##_block1) \
              +((i2) * _##e##_block2) \
              +((i3) * _##e##_block3) \
               ))


#define _NEW_ACCESS_5D(e,i0,i1,i2,i3,i4) (((char *)(_##e##_origin))+( \
               ((i0) * _##e##_block0) \
              +((i1) * _##e##_block1) \
              +((i2) * _##e##_block2) \
              +((i3) * _##e##_block3) \
              +((i4) * _##e##_block4) \
               ))



#define _NEW_ACCESS_6D(e,i0,i1,i2,i3,i4,i5) (((char *)(_##e##_origin))+( \
               ((i0) * _##e##_block0) \
              +((i1) * _##e##_block1) \
              +((i2) * _##e##_block2) \
              +((i3) * _##e##_block3) \
              +((i4) * _##e##_block4) \
              +((i5) * _##e##_block5) \
               ))

/* newest ones */

#define _CALC_DISTANCE(exp,a,d)     (exp)*_ABLK(a,d)
#define _STR_CALC_DISTANCE(exp,a,d) (((exp)*_ABLK(a,d))/_ASTR(a,d))

#define _ABLK(a,d) _ARR_BLK(a,d)
#define _ASTR(a,d) _ARR_STR(a,d)

#define _DISTANCE_ADVNC_UP(a,d)       _ABLK(a,d)
#define _DISTANCE_ADVNC_DN(a,d)       -_ABLK(a,d)
#define _DISTANCE_STR_ADVNC_UP(a,d)   _STR_CALC_DISTANCE( _ISTR(d),a,d)
#define _DISTANCE_STR_ADVNC_DN(a,d)   _STR_CALC_DISTANCE(-_ISTR(d),a,d)

#define _DISTANCE_RESET_UP(a,d)       _CALC_DISTANCE(_ILO(d)-(_IHI(d)+1),a,d)
#define _DISTANCE_RESET_DN(a,d)       _CALC_DISTANCE(_IHI(d)-(_ILO(d)-1),a,d)
#define _DISTANCE_STR_RESET_UP(a,d)   _STR_CALC_DISTANCE(_ILO(d)-(_IHI(d)+_ISTR(d)),a,d)
#define _DISTANCE_STR_RESET_DN(a,d)   _STR_CALC_DISTANCE(_IHI(d)-(_ILO(d)-_ISTR(d)),a,d)

#define _DISTANCE_TILED_UP(a,d,t)     _CALC_DISTANCE(t,a,d)
#define _DISTANCE_TILED_DN(a,d,t)     _CALC_DISTANCE(-(t),a,d)
#define _DISTANCE_STR_TILED_UP(a,d,t) _STR_CALC_DISTANCE(_ITBUMP(d),a,d)
#define _DISTANCE_STR_TILED_DN(a,d,t) _STR_CALC_DISTANCE(-_ITBUMP(d),a,d)

#define _DISTANCE_SKINY_UP(a,d)       _CALC_DISTANCE(_ITSTOP(d)-_IHI(d),a,d)
#define _DISTANCE_SKINY_DN(a,d)       _CALC_DISTANCE(_ITSTOP(d)-_ILO(d),a,d)
#define _DISTANCE_STR_SKINY_UP(a,d)   _STR_CALC_DISTANCE(_ITSTOP(d)-_IHI(d),a,d)
#define _DISTANCE_STR_SKINY_DN(a,d)   _STR_CALC_DISTANCE(_ITSTOP(d)-_ILO(d),a,d)

#define _DISTANCE_EOTIL_UP(a,d)       _CALC_DISTANCE(_ILO(d)-(_ITSTOP(d)+1),a,d)
#define _DISTANCE_EOTIL_DN(a,d)       _CALC_DISTANCE(_IHI(d)-(_ITSTOP(d)-1),a,d)
#define _DISTANCE_STR_EOTIL_UP(a,d)   _STR_CALC_DISTANCE(_ILO(d)-(_ITSTOP(d)+_ISTR(d)),a,d)
#define _DISTANCE_STR_EOTIL_DN(a,d)   _STR_CALC_DISTANCE(_IHI(d)-(_ITSTOP(d)-_ISTR(d)),a,d)


#include "access_opt.h"

#endif
