/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __REGION_H_
#define __REGION_H_

#include "struct.h"
#include "registry.h"
#include "zplglobal.h"
#include "type.h"
#include "grid.h"
#include "distribution.h"
#include "region_sparse.h"
#include "iarray.h"
#include "distribution.h"

#define _LOHI (2)
#define _LO   (0)
#define _HI   (1)

#define _UNINITIALIZED (-2)
#define _NO_PROCESSOR  (-1)
#define _ILLEGAL_PROC  (-1)

#define _NUMDIMS(reg) ((reg)->numdims)

/* these are the first and last procs that own the bounding box
   should perhaps collapse them into one (this one) */
#define _INV_BEG_OF_REG(dist, reg, dim) ((_DIST_MYLO(dist, dim) <= _REG_LO(reg, dim)) &&\
                                        (_REG_LO(reg, dim) <= _DIST_MYHI(dist, dim)))
#define _INV_END_OF_REG(dist, reg, dim) ((_DIST_MYLO(dist, dim) <= _REG_HI(reg, dim)) &&\
                                        (_REG_HI(reg, dim) <= _DIST_MYHI(dist, dim)))

/* not the logical reverse of the above -- still owns reg */
#define _AFTER_BEG_OF_REG(grid, reg, dim) (_GRID_LOC(grid, dim) > _MIN_PROC(reg, dim))
#define _BEFOR_END_OF_REG(grid, reg, dim) (_GRID_LOC(grid, dim) < _MAX_PROC(reg, dim))

#define _DIM_DIST(reg,dim) (_MIN_PROC(reg,dim) != _MAX_PROC(reg,dim))

#define _TOP_ROW(reg)      ((reg)->loprocloc[0])
#define _BOT_ROW(reg)      ((reg)->hiprocloc[0])
#define _LFT_COL(reg)      ((reg)->loprocloc[1])
#define _RGT_COL(reg)      ((reg)->hiprocloc[1])
#define _BEG_LOC(reg,d)    ((reg)->loprocloc[d])
#define _END_LOC(reg,d)    ((reg)->hiprocloc[d])
#define _MIN_PROC(reg,dim) ((reg)->loprocloc[dim])
#define _MAX_PROC(reg,dim) ((reg)->hiprocloc[dim])

#define _REG_LOPROCLOC(reg) ((reg)->loprocloc)
#define _REG_HIPROCLOC(reg) ((reg)->hiprocloc)

#define _FLOODED_DIM(r,d) ((r)->flooddim[(d)])

#define _DIM_NORMAL  0
#define _DIM_FLOODED 1
#define _DIM_RGRID   2

#define _REG_GLOB_LO(reg,dim)     ((reg)->glbbounds[dim].lo)
#define _REG_GLOB_HI(reg,dim)     ((reg)->glbbounds[dim].hi)

#define _REG_INIT_BIT 01
#define _REG_SETUP_BIT 02

#define _REG_CLEAR_FLAG(reg)      ((reg)->regflag = 0)

#define _REG_GET_INIT(reg)        ((reg)->regflag & _REG_INIT_BIT)
#define _REG_SET_INIT_ON(reg)     ((reg)->regflag |= _REG_INIT_BIT)
#define _REG_SET_INIT_OFF(reg)    ((reg)->regflag &= ~_REG_INIT_BIT)

#define _REG_GET_SETUP(reg)        ((reg)->regflag & _REG_SETUP_BIT)
#define _REG_SET_SETUP_ON(reg)     ((reg)->regflag |= _REG_SETUP_BIT)
#define _REG_SET_SETUP_OFF(reg)    ((reg)->regflag &= ~_REG_SETUP_BIT)

#define _REG_I_OWN(reg)           ((reg)->iown)
#define _REG_MYLO(reg,dim)        ((reg)->locbounds[dim].lo)
#define _REG_MYHI(reg,dim)        ((reg)->locbounds[dim].hi)
#define _REG_STRIDE(reg,dim)      ((reg)->locbounds[dim].step)

#define _REG_LO(reg,dim)          ((reg)->tuple[dim].lo)
#define _REG_HI(reg,dim)          ((reg)->tuple[dim].hi)
#define _REG_ALIGN(reg,dim)       ((reg)->tuple[dim].step)

#define reglo(r, d) (_REG_MYLO(r, d-1))
#define reghi(r, d) (_REG_MYHI(r, d-1))

#define _REG_REGISTER(reg, arr) _REG_ARRLIST(reg) = _REGISTER(_REG_ARRLIST(reg), (void*)(arr))
#define _REG_UNREGISTER(reg, arr) _REG_ARRLIST(reg) = _UNREGISTER(_REG_ARRLIST(reg), (void*)(arr))
#define _REG_ARRLIST(reg) ((reg)->arrays)
#define _REG_ARRLIST_ARR(ralist) ((_array_fnc)_REGISTRY_PTR(ralist))
#define _REG_ARRLIST_NEXT(ralist) (_REGISTRY_NEXT(ralist))

#define _REG_REGISTER_REG(reg, derived) _REG_REGLIST(reg) = _REGISTER(_REG_REGLIST(reg), (void*)(derived))
#define _REG_UNREGISTER_REG(reg, derived) _REG_REGLIST(reg) = _UNREGISTER(_REG_REGLIST(reg), (void*)(derived))
#define _REG_REGLIST(reg) ((reg)->regions)
#define _REG_REGLIST_REG(rrlist) ((_region)_REGISTRY_PTR(rrlist))
#define _REG_REGLIST_NEXT(rrlist) (_REGISTRY_NEXT(rrlist))

#define _REG_PREP(reg) ((reg)->prep_info.prep)
#define _REG_PREP_DIR(reg) ((reg)->prep_info.dir)
#define _REG_PREP_BASEREG(reg) ((reg)->prep_info.base)

#define CurrentRegion (_RMStack.reg)
#define _CurrentRegion (_RMStack.reg)

#define _REG_LABEL(reg)            ((reg)->region_label)


#define _SNAP_POS(val,str) ((int)(val)%((int)(str)))
#define _SNAP_NEG(val,str) (((int)(val)+(((int)(str))*((-(int)(val)/((int)(str)))+1)))%((int)(str)))
#define _SNAP_POSNEG(val,str) (((val)>=0)?_SNAP_POS(val,str):_SNAP_NEG(val,str))

#define _SNAP_LO_TO_ALIGN(lo,str) ((str)==1?(lo):_SNAP_POSNEG(lo,str))

/* macros to set the stuff above */

#define _SET_GB_LOC(reg,dim,lo,hi) \
  _REG_LO(reg,dim)=lo;\
  _REG_HI(reg,dim)=hi

#define _SET_GB_GLOB(reg,dim) \
  _REG_GLOB_LO(reg,dim)=_SNAP_UP(_REG_LO(reg,dim),reg,dim); \
  _REG_GLOB_HI(reg,dim)=_SNAP_DN(_REG_HI(reg,dim),reg,dim)

#define _SET_GLOB_BOUNDS(reg,dim,lo,hi,str) \
  _SET_GB_LOC(reg,dim,lo,hi); \
  _REG_ALIGN(reg,dim)=_SNAP_LO_TO_ALIGN(lo,str); \
  _REG_STRIDE(reg,dim)=(int)(str); \
  _FLOODED_DIM(reg,dim)=_DIM_NORMAL; \
  _SET_GB_GLOB(reg,dim)

#define _INHERIT_GLOB_BOUNDS(reg1,reg2,dim) \
  _REG_LO(reg1,dim) = _REG_LO(reg2,dim); \
  _REG_HI(reg1,dim) = _REG_HI(reg2,dim); \
  _REG_ALIGN(reg1,dim) = _REG_ALIGN(reg2,dim); \
  _REG_STRIDE(reg1,dim) = _REG_STRIDE(reg2,dim); \
  _FLOODED_DIM(reg1,dim) = _FLOODED_DIM(reg2,dim); \
  _REG_GLOB_LO(reg1,dim) = _REG_GLOB_LO(reg2,dim); \
  _REG_GLOB_HI(reg1,dim) = _REG_GLOB_HI(reg2,dim)

#define _SET_GLOB_BOUNDS_FLOOD(reg,dim,lo,hi,fd) \
  _SET_GB_LOC(reg,dim,lo,hi); \
  _REG_ALIGN(reg,dim)=0; \
  _REG_STRIDE(reg,dim)=1; \
  _FLOODED_DIM(reg,dim)=fd; \
  _SET_GB_GLOB(reg,dim)


#define _EXPAND_DIST_BOUNDS(reg,nd) \
  { \
    int _i; \
    for (_i=0;_i<nd;_i++) { \
      if (!_FLOODED_DIM(reg,_i)) { \
        if (_REG_LO(reg,_i) < _BND_LO(_invisibleBounds[_i])) { \
          _BND_LO(_invisibleBounds[_i]) = _REG_LO(reg, _i); \
        } \
        if (_REG_HI(reg,_i) > _BND_HI(_invisibleBounds[_i])) { \
          _BND_HI(_invisibleBounds[_i]) = _REG_HI(reg, _i); \
        } \
      } \
    } \
  }

#define _REG_DIST(reg) ((reg)->distribution)
#define _REG_GRID(reg) (_DIST_GRID((reg)->distribution))

#define _REG_NUMELMS(reg,dim)     (((_REG_MYHI(reg,dim)-_REG_MYLO(reg,dim))/\
				    _REG_STRIDE(reg,dim))+1)
#define _REG_GLB_NUMELMS(reg,dim) (((_REG_GLOB_HI(reg,dim)-\
				     _REG_GLOB_LO(reg,dim))/\
				    _REG_STRIDE(reg,dim))+1)
#define _REG_SPAN(reg,dim) (_REG_HI(reg,dim)-_REG_LO(reg,dim)+1)

#define _SNAP_UP(x,reg,dim) ((x)+(((x)<=_REG_ALIGN(reg,dim))?\
                             ((_REG_ALIGN(reg,dim)-(x))%_REG_STRIDE(reg,dim)):\
                             ((_REG_STRIDE(reg,dim)-(((x)-_REG_ALIGN(reg,dim))%_REG_STRIDE(reg,dim)))%_REG_STRIDE(reg,dim))))

#define _SNAP_DN(x,reg,dim) ((x)-(((x)>=_REG_ALIGN(reg,dim))?\
                             (((x)-_REG_ALIGN(reg,dim))%_REG_STRIDE(reg,dim)):\
                             ((_REG_STRIDE(reg,dim)-((_REG_ALIGN(reg,dim)-(x))%_REG_STRIDE(reg,dim)))%_REG_STRIDE(reg,dim))))

#define _SNAP_UP_OFF(x,reg,dim,off) ((x)+(((x)<=_REG_ALIGN(reg,dim))?\
                             (((off)+_REG_ALIGN(reg,dim)-(x))%_REG_STRIDE(reg,dim)):\
                             (((off)+_REG_STRIDE(reg,dim)-(((x)-_REG_ALIGN(reg,dim))%_REG_STRIDE(reg,dim)))%_REG_STRIDE(reg,dim))))

#define _SNAP_DN_OFF(x,reg,dim,off) ((x)-(((x)>=_REG_ALIGN(reg,dim))?\
                             ((((x)+(off))-_REG_ALIGN(reg,dim))%_REG_STRIDE(reg,dim)):\
                             (((off)+_REG_STRIDE(reg,dim)-((_REG_ALIGN(reg,dim)-(x))%_REG_STRIDE(reg,dim)))%_REG_STRIDE(reg,dim))))

#define _SNAP_UP_WRAP(x,reg,dim,reg2) _SNAP_UP_OFF(x,reg,dim,(_REG_HI(reg2,dim)-_REG_LO(reg2,dim)+1))

#define _SNAP_DN_WRAP(x,reg,dim,reg2) _SNAP_DN_OFF(x,reg,dim,(_REG_HI(reg2,dim)-_REG_LO(reg2,dim)+1))

#define _DISTRIBUTED(r,e,d) ((_REG_MYHI(r,d) != _REG_GLOB_HI(r,d)) || \
			     (_REG_MYLO(r,d) != _REG_GLOB_LO(r,d)))

#define _OWN_EXTREMEHI_SECTION_IN_DIM(r,e,d) \
                            ((_REG_MYHI(r,d) == _REG_GLOB_HI(r,d)))

#define _2D_OWN_REG(reg) (_REG_MYLO(reg,0) <= _REG_MYHI(reg,0) && \
		          _REG_MYLO(reg,1) <= _REG_MYHI(reg,1))

#define _T_AT(reg)    (_REG_I_OWN(reg))

#define _T_ATEND(reg,d) (_GRID_LOC(_DIST_GRID(_REG_DIST(reg)), d)==_END_LOC(reg,d))

#define _RECV 0
#define _SEND 1

#define _COMM_UNINITED(reg,dir) (_I_RECV_RHS(reg,dir) == _UNINITIALIZED)
#define _I_DO_RHS(reg,dir,sr) ((reg)->comm[dir][sr].ido)
#define _I_SEND_RHS(reg,dir) _I_DO_RHS(reg,dir,_SEND)
#define _I_RECV_RHS(reg,dir) _I_DO_RHS(reg,dir,_RECV)

#define _AT_RHS_SR_LO(reg,dir,dim,sr) ((reg)->comm[dir][sr].bounds[dim].lo)
#define _AT_RHS_SR_HI(reg,dir,dim,sr) ((reg)->comm[dir][sr].bounds[dim].hi)
#define _AT_RHS_SEND_LO(reg,dir,dim) _AT_RHS_SR_LO(reg,dir,dim,_SEND)
#define _AT_RHS_SEND_HI(reg,dir,dim) _AT_RHS_SR_HI(reg,dir,dim,_SEND)
#define _AT_RHS_RECV_LO(reg,dir,dim) _AT_RHS_SR_LO(reg,dir,dim,_RECV)
#define _AT_RHS_RECV_HI(reg,dir,dim) _AT_RHS_SR_HI(reg,dir,dim,_RECV)

#define _SBND_LO(sbnd) ((sbnd).lo)
#define _SBND_HI(sbnd) ((sbnd).hi)
#define _SBND_STEP(sbnd) ((sbnd).step)

typedef struct __sbound {
  int lo;
  int hi;
  int step;
} _sbound;

typedef struct __spsmemreqs {
  int ind;
  int next;
  int prev;
  int dense;
} _spsmemreqs;

typedef enum __prep_type {_NONE, _OF, _IN, _AT, _BY, _PLUS, _MINUS} _prep_type;

typedef struct __prep_info {
  _prep_type prep;
  _direction_pnc dir;
  _region_pnc base;
} _prep_info;

struct __reg_info {
  int regflag;
  int iown;
  _sbound locbounds[_MAXRANK];
  _bound glbbounds[_MAXRANK];

  int numdims;

  _sbound tuple[_MAXRANK];
  int loprocloc[_MAXRANK];
  int hiprocloc[_MAXRANK];

  _prep_info prep_info;

  int flooddim[_MAXRANK];

  char* region_label;

  _bound spsfluff[_MAXRANK];
  int sparse_region;
  int numspsvals;
  int numspsnodes;
  _ind_array_nc directory[_MAXRANK][_LOHI];
  int* corner;
  int* next[_MAXRANK];
  int* index[_MAXRANK];
  int* prev[_MAXRANK];
  _spsmemreqs* spsmemreqs;
  _distribution_nc distribution;
  _registry* arrays;
  _registry* regions; /* derived regions, e.g. east of R is derived from R */
  _MD_REGION_INFO
};

static _reg_info _AutoRedReg;
static _region_fnc _AutoRedReg1 = &_AutoRedReg;
static _region_fnc _AutoRedReg2 = &_AutoRedReg;
static _region_fnc _AutoRedReg3 = &_AutoRedReg;
static _region_fnc _AutoRedReg4 = &_AutoRedReg;
static _region_fnc _AutoRedReg5 = &_AutoRedReg;
static _region_fnc _AutoRedReg6 = &_AutoRedReg;

/*** wrap reflect ***/

struct __of_info {
  int local;
  int iwrap[2];
  _bound wrapsend[_MAXRANK];
  _bound reflectsrc[_MAXRANK];
  int reflectstep[_MAXRANK];
} _of_info;

#define _WRAP_LOCAL(reg)           (_of_info.local)
#define _I_WRAP(reg,sr)            (_of_info.iwrap[sr])
#define _I_RECV_WRAP(reg)          (_of_info.iwrap[_RECV])
#define _I_SEND_WRAP(reg)          (_of_info.iwrap[_SEND])
#define _WRAP_SEND_LO(reg,dim)     (_of_info.wrapsend[dim].lo)
#define _WRAP_SEND_HI(reg,dim)     (_of_info.wrapsend[dim].hi)
#define _WRAP_RECV_LO(reg,dim)     (_REG_MYLO(reg,dim))
#define _WRAP_RECV_HI(reg,dim)     (_REG_MYHI(reg,dim))
#define _I_REFLECT(reg)            (_I_RECV_WRAP(reg))
#define _REFLECT_SRC_STEP(reg,dim) (_of_info.reflectstep[dim])
#define _REFLECT_DST_STEP(reg,dim) (_REG_STRIDE(reg,dim))
#define _REFLECT_SRC_LO(reg,dim)   (_of_info.reflectsrc[dim].lo)
#define _REFLECT_SRC_HI(reg,dim)   (_of_info.reflectsrc[dim].hi)
#define _REFLECT_DST_LO(reg,dim)   (_REG_MYLO(reg,dim))
#define _REFLECT_DST_HI(reg,dim)   (_REG_MYHI(reg,dim))

#define _UPDATE_REGION(R1, R2, preserve) _UpdateRegion(R1, R2, preserve)
#define _COPY_REGION(R1, R2, process_arrays) _CopyRegion(R1, R2, process_arrays)

#endif
