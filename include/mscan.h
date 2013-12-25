/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MSCAN_H_
#define __MSCAN_H_

#define _MSCAN_INIT() _SetDynamicCommID(_dcid = _SetDynamicCommID(0))
#define _MSCAN_POST() _SetDynamicCommID(_dcid)

#define _MSCAN_REGINIT_DIM_UP(R,dim) _MSCAN_REGINIT_DIM(_MSCANREG,R,dim,_i##dim##TLo,_i##dim##THi-1)
#define _MSCAN_REGINIT_DIM_DN(R,dim) _MSCAN_REGINIT_DIM(_MSCANREG,R,dim,_i##dim##TLo+1,_i##dim##THi)

/* inlined code v. function call is significantly faster */
#define _MSCAN_REGINIT_DIM(R1,R2,dim,lo,hi)\
  _REG_DIST(R1) = _REG_DIST(R2); \
  if ((lo > _REG_MYHI(_PRIV_ACCESS(R2),dim)) || \
      (hi < _REG_MYLO(_PRIV_ACCESS(R2),dim))) {\
      _SET_GLOB_BOUNDS(_PRIV_ACCESS(R1), dim, 0, -1, 1);\
  } else{\
      _SET_GLOB_BOUNDS(_PRIV_ACCESS(R1), dim, \
		       ((lo<=_REG_MYLO(_PRIV_ACCESS(_PRIV_ACCESS(R2)),dim))?\
			_REG_GLOB_LO(_PRIV_ACCESS(_PRIV_ACCESS(R2)),dim):lo),\
		       ((hi>=_REG_MYHI(_PRIV_ACCESS(_PRIV_ACCESS(R2)),dim))?\
			_REG_GLOB_HI(_PRIV_ACCESS(_PRIV_ACCESS(R2)),dim):hi),\
		       1);\
  }

#define _MSCAN_REGINIT(R,dims) {_NUMDIMS(R) = dims; _InitDynamicRegion(R);}

#endif /* __MSCAN_H_ */
