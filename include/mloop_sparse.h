/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _MLOOP_SPARSE_H
#define _MLOOP_SPARSE_H

/* sparse MLOOP variables */

#define _SPS_REGDIR(reg)       _RegDir_##reg
#define _SPS_IND(reg,dim)      _i##reg##_##dim
#define _SPS_WLK(reg,dim)      _RegWalker##reg##dim
#define _SPS_WLK_V(walker,dim) (walker[dim])

#define _SPS_VAL(arr)     _Val_##arr
#define _SPS_PTR(arr)     _Ptr_##arr
#define _SPS_ORIG(arr)    _Orig_##arr


/* setting up sparse array variables */

#define _SPS_DECL_INIT_ARR_R(type,arr) \
  register type _SPS_VAL(arr)
#define _SPS_DECL_INIT_ARR_W(type,arr) \
  register type* _SPS_PTR(arr)

#define _SPS_DECL_INIT_ARR_ORIG_R(type,arr) \
  const type* const _SPS_ORIG(arr) = (type*)(arr->origin)
#define _SPS_DECL_INIT_ARR_ORIG_W(type,arr) \
  type* const _SPS_ORIG(arr) = (type*)(arr->origin)


/* setting up sparse region variables */

#define _SPS_DECL_INIT_REG_MAIN(regname,dim) \
  int _SPS_WLK(regname,dim)

#define _SPS_DECL_INIT_REG_MAIN_INNER(regname,dim) \
  int _SPS_WLK(regname,dim); _SPS_DECL_INNER_STOP(regname)

#define _SPS_DECL_INIT_REG(regname,innerdim) \
  _SPS_DECL_INIT_REG_MAIN(regname,innerdim); \
  int _SPS_IND(regname,innerdim)

#define _SPS_DECL_INIT_REG_INNER(regname,innerdim) \
  _SPS_DECL_INIT_REG_MAIN(regname,innerdim); \
  int _SPS_IND(regname,innerdim); _SPS_DECL_INNER_STOP(regname)


/* some other stuff */

#define _SET_SPS_IND(reg,regdef,dim) \
  _SPS_IND(reg,dim) = _SPSNODE_INDEX(regdef,_SPS_WLK(reg,dim),dim)

#define _SET_SPS_IND_MAIN(reg,dim) \
  _I(dim) = _SPSNODE_INDEX(reg,_SPS_WLK(reg,dim),dim)



#define _SPS_START_UP(reg,basereg,nd,innerdim) \
  _SPS_WLK(reg,innerdim) = _SPS_STRT(basereg,nd,innerdim); \
  _SET_SPS_IND(reg,reg,innerdim)
#define _SPS_START_UP_INNER(reg,basereg,nd,innerdim) \
  _SPS_WLK(reg,innerdim) = _SPS_STRT(basereg,nd,innerdim); \
  _SPS_INNER_STOP(reg) = _SPSNODE_PREV(reg,_SPS_WLK(reg,innerdim)+1,innerdim); \
  _SPS_WLK(reg,innerdim) = _SPSNODE_NEXT(reg,_SPS_WLK(reg,innerdim),innerdim)-1; \
  _SPS_IND(reg,innerdim) = INT_MIN;
#define _SPS_START_DN(reg,basereg,nd,innerdim) \
  _SPS_WLK(reg,innerdim) = _SPS_STOP(basereg,nd,innerdim); \
  _SET_SPS_IND(reg,reg,innerdim)

#define _SPS_START_UP_OFF(reg,basereg,nd,innerdim,off) \
  _SPS_WLK(reg,innerdim) = _SPS_STRT_OFF(basereg,nd,innerdim,off); \
  _SET_SPS_IND(reg,reg,innerdim)
#define _SPS_START_DN_OFF(reg,basereg,nd,innerdim,off) \
  _SPS_WLK(reg,innerdim) = _SPS_STOP_OFF(basereg,nd,innerdim); \
  _SET_SPS_IND(reg,reg,innerdim)



/* macros to bump a sparse iteration */

#define _SPS_BUMP_UP(reg,regdef,d) _SPS_WLK(reg,d) = _SPSNODE_NEXT(regdef,_SPS_WLK(reg,d),d)
#define _SPS_BUMP_DN(reg,regdef,d) _SPS_WLK(reg,d) = _SPSNODE_PREV(regdef,_SPS_WLK(reg,d),d)

#define _SPS_BUMP_UP_INNER(reg,d) \
  _SPS_WLK(reg,d)++


#define _SPS_BUMP_UP_V(reg,wlk,d) \
  _SPS_WLK_V(wlk,d) = _SPSNODE_NEXT(reg,_SPS_WLK_V(wlk,d),d)

#define _SPS_BUMP_UP_V_INNER(reg,wlk,d) \
  _SPS_WLK_V(wlk,d)++;


/* macros to catch a sparse region iteration up to the controlling region */

#define _SPS_CATCH_UP(reg,regdef,dim) \
  while (_SPS_IND(reg,dim) < _I(dim)) { \
    _SPS_BUMP_UP(reg,regdef,dim); \
    _SET_SPS_IND(reg,regdef,dim); \
  }
#define _SPS_CATCH_DN(reg,regdef,dim) \
  while (_SPS_IND(reg,dim) > _I(dim)) { \
    _SPS_BUMP_DN(reg,regdef,dim); \
    _SET_SPS_IND(reg,regdef,dim); \
  }

#define _SPS_CATCH_UP_INNER(reg,regdef,dim) \
/*   printf("BLC-A:%d\n",_SPS_WLK(reg,dim)); */ \
  while ((_SPS_IND(reg,dim) < _I(dim)) && \
	 (_SPS_WLK(reg,dim) < _SPS_INNER_STOP(reg))) { \
/*  printf("%d <? %d, %d <= %d\n",_SPS_IND(reg,dim),_I(dim), \
    _SPS_WLK(reg,dim),(1+_SPSNODE_PREV(reg,_SPS_STOP(reg,2,dim),dim)));*/ \
/*   printf("BLC-B:%d\n",_SPS_WLK(reg,dim)); */ \
    _SPS_BUMP_UP_INNER(reg,dim); \
    _SET_SPS_IND(reg,regdef,dim); \
  }

#define _SPS_CATCH_UP_MAIN(reg,dim) \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,dim),dim) < _ILO(dim)) { \
    _SPS_BUMP_UP(reg,reg,dim); \
  }
#define _SPS_CATCH_DN_MAIN(reg,dim) \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,dim) > _IHI(dim)) { \
    _SPS_BUMP_DN(reg,dim); \
  }
#define _SPS_CATCH_UP_MAIN_INNER(reg,dim) \
/*  printf("C2:%d,%d\n",_SPS_WLK(reg,0),_SPS_WLK(reg,1)); */ \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,dim),dim) < _ILO(dim)) { \
    _SPS_BUMP_UP_INNER(reg,dim); \
/*     printf("C3:%d,%d\n",_SPS_WLK(reg,0),_SPS_WLK(reg,1)); */ \
  }

#define _SPS_CATCH_UP_V(reg,wlk,lo,dim) \
  while (_SPSNODE_INDEX(reg,_SPS_WLK_V(wlk,dim),dim) < lo[dim]) { \
    _SPS_BUMP_UP_V(reg,wlk,dim); \
  }

#define _SPS_CATCH_UP_V_INNER(reg,wlk,lo,dim,stop) \
  while (_SPSNODE_INDEX(reg,_SPS_WLK_V(wlk,dim),dim) < lo[dim] && \
         _SPS_WLK_V(wlk,dim) < stop) { \
    _SPS_BUMP_UP_V_INNER(reg,wlk,dim); \
  }



#define _SPS_CATCH_UP_OFF(reg,regdef,dim,off) \
  while (_SPS_IND(reg,dim) < (_I(dim) + (off))) { \
    _SPS_BUMP_UP(reg,regdef,dim); \
    _SET_SPS_IND(reg,regdef,dim); \
  }
#define _SPS_CATCH_DN_OFF(reg,regdef,dim,off) \
  while (_SPS_IND(reg,dim) > (_I(dim) + (off))) { \
    _SPS_BUMP_DN(reg,regdef,dim); \
    _SET_SPS_IND(reg,regdef,dim); \
  }
#define _SPS_CATCH_UP_OFF_INNER(reg,regdef,dim,off) \
  while (_SPS_IND(reg,dim) < (_I(dim) + (off)) && \
         (_SPS_WLK(reg,dim) < _SPS_INNER_STOP(reg))) { \
    _SPS_BUMP_UP_INNER(reg,dim); \
    _SET_SPS_IND(reg,regdef,dim); \
  }
#define _SPS_CATCH_DN_OFF_INNER(reg,regdef,dim,off) \
  while (_SPS_IND(reg,dim) > (_I(dim) + (off))) { \
    _SPS_BUMP_DN_INNER(reg,dim); \
    _SET_SPS_IND(reg,regdef,dim); \
  }


/* macros to protect the catching up of a sparse region iteration */

#define _SPS_NEEDS_CATCHING_UP(reg,dim) (_SPS_IND(reg,dim) <= _I(dim))
#define _SPS_NEEDS_CATCHING_DN(reg,dim) (_SPS_IND(reg,dim) >= _I(dim))
#define _SPS_NEEDS_CATCHING_UP_OFF(reg,dim,off) \
  (_SPS_IND(reg,dim) <= (_I(dim) + (off)))
#define _SPS_NEEDS_CATCHING_DN_OFF(reg,dim,off) \
  (_SPS_IND(reg,dim) >= (_I(dim) + (off)))


/* some other stuff */

#define _SPS_LEGAL_UP(reg,dim) (_SPS_IND(reg,dim) <= _IHI(dim))
#define _SPS_LEGAL_DN(reg,dim) (_SPS_IND(reg,dim) >= _ILO(dim))

#define _SPS_NOT_USED_UP(reg,dim) (_I(dim) < _SPS_IND(reg,dim))
#define _SPS_USED_UP(reg,dim)     (_SPS_IND(reg,dim) <= _I(dim))

#define _STR_MLOOP_UP_SPS(dim,reg,cond) \
  for (; _I(dim)<=_IHI(dim) cond; _STR_BUMP_I_UP(dim))

#define _STR_MLOOP_UP_SPS_DONE(dim,reg) _STR_MLOOP_UP_SPS(dim,reg, )

#define _SPS_SET_VAL(arr,reg,dim) \
  _SPS_VAL(arr) = *(_SPS_ORIG(arr) + ((_SPS_IND(reg,dim) == _I(dim)) ? \
                                      _SPSNODE_ID(reg,_SPS_WLK(reg,dim)) : 0))

#define _SPS_HIT(reg,dim) (_SPS_IND(reg,dim) == _I(dim))
#define _IF_SPS_HIT(reg,dim) if _SPS_HIT(reg,dim)

#define _SPS_HIT_OFF(reg,dim,off) (_SPS_IND(reg,dim) == _I(dim) + (off))
#define _IF_SPS_HIT_OFF(reg,dim,off) if _SPS_HIT_OFF(reg,dim,off)

#define _SPSID_VAL(reg,regdef,dim) \
  (_SPS_HIT(reg,dim) ? _SPSNODE_ID(regdef,_SPS_WLK(reg,dim)) : 0)
#define _SPSID_VAL_MAIN(reg,dim) \
  (_SPSNODE_ID(reg,_SPS_WLK(reg,dim)))
#define _SPSID_VAL_OFF(reg,regdef,dim,off) \
  (_SPS_HIT_OFF(reg,dim,off) ? _SPSNODE_ID(regdef,_SPS_WLK(reg,dim)) : 0)


/* Sparse MLOOP preparation */

#define _PREP_SPS_MLOOP_UP(reg,nd,d) \
  _SPS_WLK(reg,d) = _SPS_STRT(reg,nd,d); \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d) < _ILO(d)) { \
    _SPS_BUMP_UP(reg,reg,d); \
  }

#define _PREP_SPS_MLOOP_UP_INNER(reg,nd,d) \
  _SPS_WLK(reg,d) = _SPS_STRT(reg,nd,d); \
  _SPS_INNER_STOP(reg) = _SPSNODE_PREV(reg,_SPS_WLK(reg,d)+1,d); \
  _SPS_WLK(reg,d) = _SPSNODE_NEXT(reg,_SPS_WLK(reg,d),d); \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d) < _ILO(d)) { \
    _SPS_BUMP_UP_INNER(reg,d); \
  }

#define _PREP_SPS_MLOOP_DN(reg,nd,d) \
  _SPS_WLK(reg,d) = _SPS_STOP(reg,nd,d); \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d) > _IHI(d)) { \
    _SPS_BUMP_DN(reg,reg,d); \
  }

#define _PREP_SPS_MLOOP_DN_INNER(reg,nd,d) \
  _SPS_WLK(reg,d) = _SPS_STOP(reg,nd,d); \
  while (_SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d) > _IHI(d)) { \
    _SPS_BUMP_DN_INNER(reg,d); \
  }


/* Sparse MLOOPs */

#define _SPS_MLOOP_UP(reg,d) \
  for (; (_I(d) = _SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d)) <= _IHI(d); \
       _SPS_BUMP_UP(reg,reg,d))

#define _SPS_MLOOP_DN(reg,d) \
  for (; (_I(d) = _SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d)) >= _ILO(d); \
       _SPS_BUMP_DN(reg,reg,d))

#define _SPS_MLOOP_UP_INNER(reg,d) \
  for (; \
	 /* printf("walked to %d/%d (%d/%d)\n", _SPS_WLK(reg,d), _SPS_INNER_STOP(reg), _I(d), _IHI(d)), */ \
((_SPS_WLK(reg,d) <= _SPS_INNER_STOP(reg)) && ((_I(d) = _SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d)) <= _IHI(d)) \
); _SPS_BUMP_UP_INNER(reg,d))

#define _SPS_CLEAN_MLOOP_UP_INNER(reg,d) \
  for (; ( \
          (_SPS_WLK(reg,d) <= _SPS_INNER_STOP(reg)) && \
          ((_I(d) = _SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d)),1)\
         ); \
       _SPS_BUMP_UP_INNER(reg,d))

/*  for (; ((_I(d) = _SPSNODE_INDEX(reg,_SPS_WLK(reg,d),d)),\
          (_SPS_WLK(reg,d) <= _SPS_INNER_STOP(reg))); _SPS_BUMP_UP_INNER(reg,d))
*/


/* vectorized sparse MLOOPs */

#define _SPS_MLOOP_UP_V(reg,walker,nd,lo,hi) \
  _SPS_WALK_SETUP_MAIN_V(walker,reg,0); \
  _SPS_BUMP_UP_V(reg,walker,0); \
  { \
    int _i; \
    int _running = 1; \
    int _toohigh; \
    int _stopnode; \
\
    for (_i=0; _i<nd; _i++) { \
      if (_i) { \
        _SPS_WLK_V(walker,_i) = _SPS_WLK_V(walker,_i-1); \
      } \
      if (_i == nd-1) { \
        if (nd > 1) { \
          _stopnode = _SPSNODE_PREV(reg,_SPS_WLK_V(walker,_i)+1,_i); \
          /* printf("[%d] A: stopping at %d\n",_INDEX,_stopnode); */ \
        } else { \
          _stopnode = _SPS_CORNER(reg,1); \
         /* printf("[%d] A: stopping at %d\n",_INDEX,_stopnode); */ \
        } \
        /* advance into the internal nodes */ \
        _SPS_BUMP_UP_V(reg,walker,_i); \
        _SPS_CATCH_UP_V_INNER(reg,walker,lo,_i,_stopnode); \
      } else { \
        _SPS_CATCH_UP_V(reg,walker,lo,_i); \
      } \
      _toohigh = (_i == nd-1) && ((_SPS_WLK_V(walker,_i) > _stopnode) || \
                 (_SPSNODE_INDEX(reg,_SPS_WLK_V(walker,_i),_i) < lo[_i])); \
      if (_SPSNODE_INDEX(reg,_SPS_WLK_V(walker,_i),_i) > hi[_i] || _toohigh) { \
        if (_i == 0) { \
          _running = 0; \
          break; \
        } else { \
          _SPS_BUMP_UP_V(reg,walker,_i-1); \
          _i -= 2; \
        } \
      } \
    } \
/*    printf("[%d] walkers B: %d, %d\n",_INDEX,_SPS_WLK_V(walker,0),_SPS_WLK_V(walker,1)); */ \
    while (_running)

#define _ADVANCE_SPS_MLOOP_UP_V(reg,walker,nd,lo,hi) \
  { \
    int foundgoodindex; \
    int _stopnow = 0; \
/*    printf("[%d] walkers C: %d, %d\n",_INDEX,_SPS_WLK_V(walker,0),_SPS_WLK_V(walker,1)); */ \
    do { \
      for (_i = nd-1; _i>=0; _i--) { \
        _stopnow = 0; \
        if (_i == nd-1) { \
          _SPS_BUMP_UP_V_INNER(reg,walker,_i); \
          if (_SPS_WLK_V(walker,_i) > _stopnode) { \
	    _stopnow = 1; \
          } \
        } else { \
          _SPS_BUMP_UP_V(reg,walker,_i); \
        } \
        if (_SPSNODE_INDEX(reg,_SPS_WLK_V(walker,_i),_i) <= hi[_i] && _stopnow==0) { \
          break; \
        } else { \
          if (_i == 0) { \
            _running = 0; \
            break; \
          } \
        } \
      } \
      foundgoodindex = !_stopnow; \
      if (_running) { \
        for (_i++; _i < nd; _i++) { \
          _SPS_WLK_V(walker,_i) = _SPS_WLK_V(walker,_i-1); \
          if ((_i == nd-1) && (nd > 1)) { \
             _stopnode = _SPSNODE_PREV(reg,_SPS_WLK_V(walker,_i)+1,_i); \
/*             printf("[%d] B: stopping at %d\n",_INDEX,_stopnode); */ \
             _SPS_BUMP_UP_V(reg,walker,_i); \
             _SPS_CATCH_UP_V_INNER(reg,walker,lo,_i,_stopnode); \
          } else { \
            _SPS_CATCH_UP_V(reg,walker,lo,_i); \
          } \
          if (_SPSNODE_INDEX(reg,_SPS_WLK_V(walker,_i),_i) > hi[_i] || \
              _SPSNODE_INDEX(reg,_SPS_WLK_V(walker,_i),_i) < lo[_i] || \
              _SPS_WLK_V(walker,nd-1) > _stopnode) { \
            foundgoodindex = 0; \
          } \
        } \
      } else { \
        break; \
      } \
    } while (foundgoodindex == 0); \
     if (foundgoodindex && _running) { \
     /* printf("[%d] [%d,%d]\n",_INDEX,_SPSNODE_INDEX(reg,_SPS_WLK_V(walker,1),0), \
	_SPSNODE_INDEX(reg,_SPS_WLK_V(walker,1),1)); */ \
     } \
  }

#define _END_SPS_MLOOP_UP_V() \
  }



/* I have a hunch that this isn't good enough -- that indices have to be
   checked against the stride to see if they hit (for a strided subset
   of a denser sparse region, e.g.), but this will serve as a placeholder
   for the time being -BLC */

#define _STR_SPS_MLOOP_UP(reg,d) _SPS_MLOOP_UP(reg,d)
#define _STR_SPS_MLOOP_DN(reg,d) _SPS_MLOOP_DN(reg,d)

#define _STR_SPS_MLOOP_UP_INNER(reg,d) _SPS_MLOOP_UP_INNER(reg,d)
#define _STR_SPS_MLOOP_DN_INNER(reg,d) _SPS_MLOOP_DN_INNER(reg,d)




/* The following is new stuff for iterating over a region's sparse
   directory rather than its dense one. */


/* for declaring a sparse walker and index per dimension */

#define _SPS_DECL_REGWLKX(reg,dim) int _SPS_WLK(reg,dim)
#define _SPS_DECL_REGINDX(reg,dim) int _SPS_IND(reg,dim)

#define _SPS_DECL_REGWLK1(reg) _SPS_DECL_REGWLKX(reg,0)
#define _SPS_DECL_REGWLK2(reg) _SPS_DECL_REGWLK1(reg); _SPS_DECL_REGWLKX(reg,1)
#define _SPS_DECL_REGWLK3(reg) _SPS_DECL_REGWLK2(reg); _SPS_DECL_REGWLKX(reg,2)
#define _SPS_DECL_REGWLK4(reg) _SPS_DECL_REGWLK3(reg); _SPS_DECL_REGWLKX(reg,3)
#define _SPS_DECL_REGWLK5(reg) _SPS_DECL_REGWLK4(reg); _SPS_DECL_REGWLKX(reg,4)
#define _SPS_DECL_REGWLK6(reg) _SPS_DECL_REGWLK5(reg); _SPS_DECL_REGWLKX(reg,5)

#define _SPS_DECL_REGIND1(reg) _SPS_DECL_REGINDX(reg,0)
#define _SPS_DECL_REGIND2(reg) _SPS_DECL_REGIND1(reg); _SPS_DECL_REGINDX(reg,1)
#define _SPS_DECL_REGIND3(reg) _SPS_DECL_REGIND2(reg); _SPS_DECL_REGINDX(reg,2)
#define _SPS_DECL_REGIND4(reg) _SPS_DECL_REGIND3(reg); _SPS_DECL_REGINDX(reg,3)
#define _SPS_DECL_REGIND5(reg) _SPS_DECL_REGIND4(reg); _SPS_DECL_REGINDX(reg,4)
#define _SPS_DECL_REGIND6(reg) _SPS_DECL_REGIND5(reg); _SPS_DECL_REGINDX(reg,5)


#define _SPS_INNER_STOP(reg) _Stop##reg
#define _SPS_DECL_INNER_STOP(reg) int _SPS_INNER_STOP(reg)

#define _SPS_DECL_INIT_REG_SPS(reg,nd) \
  _SPS_DECL_REGWLK##nd(reg); _SPS_DECL_REGIND##nd(reg)

#define _SPS_DECL_INIT_REG_SPS_INNER(reg,nd) \
  _SPS_DECL_INIT_REG_SPS(reg,nd); _SPS_DECL_INNER_STOP(reg)

#define _SPS_DECL_INIT_REG_MAIN_SPS(reg,nd) \
  _SPS_DECL_REGWLK##nd(reg)

#define _SPS_DECL_INIT_REG_MAIN_SPS_INNER(reg,nd) \
  _SPS_DECL_INIT_REG_MAIN_SPS(reg,nd); _SPS_DECL_INNER_STOP(reg)

#define _UP0 0x00
#define _UP1 0x00
#define _UP2 0x00
#define _UP3 0x00
#define _UP4 0x00
#define _UP5 0x00

#define _DN0 0x01
#define _DN1 0x02
#define _DN2 0x04
#define _DN3 0x08
#define _DN4 0x10
#define _DN5 0x20


#define _SPS_WALK_SETUP(reg,regdef,dim,corner) \
  _SPS_WLK(reg,dim) = _SPS_CORNER(regdef,corner); \
  _SET_SPS_IND(reg,regdef,dim)

#define _SPS_WALK_SETUP_MAIN(reg,regdef,dim,corner) \
  _SPS_WLK(reg,dim) = _SPS_CORNER(reg,corner)


#define _SPS_WALK_SETUP_1D_UP(reg,regdef,dim,corner) \
  _SPS_WLK(reg,dim) = _SPS_CORNER(reg,corner); \
  _SPS_INNER_STOP(reg) = _SPSNODE_PREV(reg,_SPS_WLK(reg,0)+1,0); \
  /* we set the walker and index lower than starting here because the loops \
     assume that the walker is always < the current index or that it's \
     already set to the identity */ \
  _SPS_WLK(reg,0) = _SPSNODE_NEXT(reg,_SPS_WLK(reg,0),0)-1; \
  _SPS_IND(reg,0) = INT_MIN
  

#define _SPS_WALK_SETUP_MAIN_1D_UP(reg,regdef,dim,corner) \
  _SPS_WLK(reg,dim) = _SPS_CORNER(reg,corner); \
  _SPS_INNER_STOP(reg) = _SPSNODE_PREV(reg,_SPS_WLK(reg,0)+1,0); \
  _SPS_WLK(reg,0) = _SPSNODE_NEXT(reg,_SPS_WLK(reg,0),0); \
  _SET_SPS_IND_MAIN(reg,dim)


#define _SPS_WALK_SETUP_MAIN_V(walker,reg,dim) \
  _SPS_WLK_V(walker,dim) = _SPS_ORIGIN(reg)
  

#define _SPS_COPY_WALKER(reg,regdef,dim,next) \
  _SPS_WLK(reg,next) = _SPS_WLK(reg,dim); \
  _SET_SPS_IND(reg,regdef,next)

#define _SPS_COPY_WALKER_MAIN(reg,dim,next) \
  _SPS_WLK(reg,next) = _SPS_WLK(reg,dim)

#define _SPS_COPY_WALKER_INNER_UP(reg,regdef,dim,next) \
  _SPS_WLK(reg,next) = _SPS_WLK(reg,dim); \
  _SPS_INNER_STOP(reg) = _SPSNODE_PREV(regdef,_SPS_WLK(reg,next)+1,next); \
  _SPS_WLK(reg,next) = _SPSNODE_NEXT(regdef,_SPS_WLK(reg,dim),next)-1; \
  _SPS_IND(reg,next) = INT_MIN

#define _SPS_COPY_WALKER_MAIN_INNER_UP(reg,dim,next) \
  _SPS_WLK(reg,next) = _SPS_WLK(reg,dim); \
  _SPS_INNER_STOP(reg) = _SPSNODE_PREV(reg,_SPS_WLK(reg,next)+1,next); \
  _SPS_WLK(reg,next) = _SPSNODE_NEXT(reg,_SPS_WLK(reg,dim),next)

#define _SPS_MAXIFYX(reg,next) _SPS_IND(reg,next) = INT_MAX

#define _SPS_MAXIFY1(reg,nd1) \
  _SPS_MAXIFYX(reg,nd1)
#define _SPS_MAXIFY2(reg,nd1,nd2) \
  _SPS_MAXIFY1(reg,nd1); _SPS_MAXIFYX(reg,nd2)
#define _SPS_MAXIFY3(reg,nd1,nd2,nd3) \
  _SPS_MAXIFY2(reg,nd1,nd2); _SPS_MAXIFYX(reg,nd3)
#define _SPS_MAXIFY4(reg,nd1,nd2,nd3,nd4) \
  _SPS_MAXIFY3(reg,nd1,nd2,nd3); _SPS_MAXIFYX(reg,nd4)
#define _SPS_MAXIFY5(reg,nd1,nd2,nd3,nd4,nd5) \
  _SPS_MAXIFY4(reg,nd1,nd2,nd3,nd4); _SPS_MAXIFYX(reg,nd5)
#define _SPS_MAXIFY6(reg,nd1,nd2,nd3,nd4,nd5,nd6) \
  _SPS_MAXIFY5(reg,nd1,nd2,nd3,nd4,nd5); _SPS_MAXIFYX(reg,nd6)

#define _SPS_MINIFYX(reg,next) _SPS_IND(reg,next) = INT_MIN

#define _SPS_MINIFY1(reg,nd1) \
  _SPS_MINIFYX(reg,nd1)
#define _SPS_MINIFY2(reg,nd1,nd2) \
  _SPS_MINIFY1(reg,nd1); _SPS_MINIFYX(reg,nd2)
#define _SPS_MINIFY3(reg,nd1,nd2,nd3) \
  _SPS_MINIFY2(reg,nd1,nd2); _SPS_MINIFYX(reg,nd3)
#define _SPS_MINIFY4(reg,nd1,nd2,nd3,nd4) \
  _SPS_MINIFY3(reg,nd1,nd2,nd3); _SPS_MINIFYX(reg,nd4)
#define _SPS_MINIFY5(reg,nd1,nd2,nd3,nd4,nd5) \
  _SPS_MINIFY4(reg,nd1,nd2,nd3,nd4); _SPS_MINIFYX(reg,nd5)
#define _SPS_MINIFY6(reg,nd1,nd2,nd3,nd4,nd5,nd6) \
  _SPS_MINIFY5(reg,nd1,nd2,nd3,nd4,nd5); _SPS_MINIFYX(reg,nd6)

#define _SPS_MAIN_CLEAN_MLOOP_UP(reg,dim) \
  _SPS_BUMP_UP(reg,reg,dim); \
  _SPS_MLOOP_UP(reg,dim)

#define _SPS_MAIN_MLOOP_UP(reg,dim) \
  _SPS_CATCH_UP_MAIN(reg,dim); \
  _SPS_MLOOP_UP(reg,dim)

#define _SPS_SET_STOP(reg,dim) /* nothing here */


#define _SPS_MAIN_MLOOP_UP_INNER(reg,dim) \
  _SPS_CATCH_UP_MAIN_INNER(reg,dim); \
  _SPS_MLOOP_UP_INNER(reg,dim)

#define _SPS_MAIN_CLEAN_MLOOP_UP_INNER(reg,dim) \
  _SPS_CLEAN_MLOOP_UP_INNER(reg,dim)

#define _SPS_MAIN_MLOOP_UP_INNER_MATCH(reg,dim) \
  _SPS_MLOOP_UP_INNER(reg,dim)


/* I have a hunch that this isn't good enough -- that indices have to be
   checked against the stride to see if they hit (for a strided subset
   of a denser sparse region, e.g.), but this will serve as a placeholder
   for the time being -BLC */

#define _STR_SPS_MAIN_MLOOP_UP(reg,dim) _SPS_MAIN_MLOOP_UP(reg,dim)
#define _STR_SPS_MAIN_MLOOP_UP_INNER(reg,dim) _SPS_MAIN_MLOOP_UP_INNER(reg,dim)

#endif
