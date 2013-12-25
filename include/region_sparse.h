/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _REGION_SPARSE_H_
#define _REGION_SPARSE_H_

/* macros for indexing into a region's sparse fields */

#define _SPS_REG_FLUFF_LO(reg,dim) ((reg)->spsfluff[dim].lo)
#define _SPS_REG_FLUFF_HI(reg,dim) ((reg)->spsfluff[dim].hi)
#define _SPS_REGION(reg)       ((reg)->sparse_region)
#define _SPS_DIR(reg,dim,lohi) ((reg)->directory[dim][lohi])
#define _SPS_CORNER_BUFF(reg)  ((reg)->corner)
#define _SPS_CORNER(reg,int)   ((reg)->corner[int])
#define _SPS_ORIGIN(reg)       (_SPS_CORNER(reg,0))
#define _SPS_NUMVALS(reg)      ((reg)->numspsvals)
#define _SPS_NUMNODES(reg)     ((reg)->numspsnodes)
#define _SPS_BUFF_ID(reg)      ((reg)->ID)
#define _SPS_BUFF_NEXT(reg,d)  ((reg)->next[d])
#define _SPS_BUFF_IND(reg,d)   ((reg)->index[d])
#define _SPS_BUFF_PREV(reg,d)  ((reg)->prev[d])
#define _SPS_BUFF(reg)         ((reg)->spsbuff)
#define _SPS_IMPL_BOUNDS(reg)  ((reg)->impl_sps)
#define _SPS_MEMREQ(reg)       ((reg)->spsmemreqs)


/* These seem strange, but it's because u is evaluated after this macro
   but before _IACCESS_xD... */
#define _SPS_DIR_ACCESS_1D(arr,ix) _IACCESS_0D(arr)
#define _SPS_DIR_ACCESS_2D(arr,u)  _IACCESS_1D(arr,u)
#define _SPS_DIR_ACCESS_3D(arr,u)  _IACCESS_2D(arr,u)
#define _SPS_DIR_ACCESS_4D(arr,u)  _IACCESS_3D(arr,u)
#define _SPS_DIR_ACCESS_5D(arr,u)  _IACCESS_4D(arr,u)
#define _SPS_DIR_ACCESS_6D(arr,u)  _IACCESS_5D(arr,u)




/* macros for indexing into _SPS_ORIGIN per dimension -- add or | together */

#define _GO_UP(dim) 0
#define _GO_DN(dim) (1<<(dim))

#define _SPS_IN_HEAD(dim) (1<<(dim))
#define _SPS_IN_BODY(dim) (0)

#define _SPS_CLASSIFY(dim) \
  (_SPS_INIT_IN_HEADER(dim) ? _SPS_IN_HEAD(dim) : _SPS_IN_BODY(dim))

#define _SPS_CLASSIFY_1D _SPS_CLASSIFY(0)
#define _SPS_CLASSIFY_2D _SPS_CLASSIFY_1D | _SPS_CLASSIFY(1)
#define _SPS_CLASSIFY_3D _SPS_CLASSIFY_2D | _SPS_CLASSIFY(2)
#define _SPS_CLASSIFY_4D _SPS_CLASSIFY_3D | _SPS_CLASSIFY(3)
#define _SPS_CLASSIFY_5D _SPS_CLASSIFY_4D | _SPS_CLASSIFY(4)
#define _SPS_CLASSIFY_6D _SPS_CLASSIFY_5D | _SPS_CLASSIFY(5)


#define _SPS_GEN_IND_1_0 0

#define _SPS_GEN_IND_2_0 _I(1)
#define _SPS_GEN_IND_2_1 _I(0)

#define _SPS_GEN_IND_3_2 _I(0),_I(1)
#define _SPS_GEN_IND_3_1 _I(0),_I(2)
#define _SPS_GEN_IND_3_0 _I(1),_I(2)

#define _SPS_STRTSTOP_HELP(reg,nd,dim,lohi) (*((int*)(_SPS_DIR_ACCESS_##nd##D(_SPS_DIR(reg,dim,lohi),_SPS_GEN_IND_##nd##_##dim))))

#define _SPS_STRT(reg,nd,dim) (_SPS_STRTSTOP_HELP(reg,nd,dim,_LO))
#define _SPS_STOP(reg,nd,dim) (_SPS_STRTSTOP_HELP(reg,nd,dim,_HI))

#define _SPS_GEN_IND_2_0 _I(1)
#define _SPS_GEN_IND_2_1 _I(0)

#define _SPS_GEN_IND_3_2 _I(0),_I(1)
#define _SPS_GEN_IND_3_1 _I(0),_I(2)
#define _SPS_GEN_IND_3_0 _I(1),_I(2)



#define _SPS_GEN_IND_2_0_OFF(off) (_SPS_GEN_IND_2_0+(off))
#define _SPS_GEN_IND_2_1_OFF(off) (_SPS_GEN_IND_2_1+(off))

#define _SPS_STRTSTOP_HELP_OFF(reg,nd,dim,lohi,off) (*((int*)(_SPS_DIR_ACCESS_##nd##D(_SPS_DIR(reg,dim,lohi),_SPS_GEN_IND_##nd##_##dim##_OFF(off)))))

#define _SPS_STRT_OFF(reg,nd,dim,off) (_SPS_STRTSTOP_HELP_OFF(reg,nd,dim,_LO,off))
#define _SPS_STOP_OFF(reg,nd,dim,off) (_SPS_STRTSTOP_HELP_OFF(reg,nd,dim,_HI,off))

#define _SPS_GEN_IND_V_1_0(ind) 0

#define _SPS_GEN_IND_V_2_0(ind) ind[1]
#define _SPS_GEN_IND_V_2_1(ind) ind[0]

#define _SPS_GEN_IND_V_3_2(ind) ind[0],ind[1]
#define _SPS_GEN_IND_V_3_1(ind) ind[0],ind[2]
#define _SPS_GEN_IND_V_3_0(ind) ind[1],ind[2]

#define _SPS_GEN_IND_V_4_3(ind) ind[0],ind[1],ind[2]
#define _SPS_GEN_IND_V_4_2(ind) ind[0],ind[1],ind[3]
#define _SPS_GEN_IND_V_4_1(ind) ind[0],ind[2],ind[3]
#define _SPS_GEN_IND_V_4_0(ind) ind[1],ind[2],ind[3]

#define _SPS_GEN_IND_V_5_4(ind) ind[0],ind[1],ind[2],ind[3]
#define _SPS_GEN_IND_V_5_3(ind) ind[0],ind[1],ind[2],ind[4]
#define _SPS_GEN_IND_V_5_2(ind) ind[0],ind[1],ind[3],ind[4]
#define _SPS_GEN_IND_V_5_1(ind) ind[0],ind[2],ind[3],ind[4]
#define _SPS_GEN_IND_V_5_0(ind) ind[1],ind[2],ind[3],ind[4]

#define _SPS_GEN_IND_V_6_5(ind) ind[0],ind[1],ind[2],ind[3],ind[4]
#define _SPS_GEN_IND_V_6_4(ind) ind[0],ind[1],ind[2],ind[3],ind[5]
#define _SPS_GEN_IND_V_6_3(ind) ind[0],ind[1],ind[2],ind[4],ind[5]
#define _SPS_GEN_IND_V_6_2(ind) ind[0],ind[1],ind[3],ind[4],ind[5]
#define _SPS_GEN_IND_V_6_1(ind) ind[0],ind[2],ind[3],ind[4],ind[5]
#define _SPS_GEN_IND_V_6_0(ind) ind[1],ind[2],ind[3],ind[4],ind[5]

#define _SPS_STRTSTOP_V(reg,nd,dim,lohi,ind) \
  (*((int*)(_SPS_DIR_ACCESS_##nd##D(_SPS_DIR(reg,dim,lohi),\
                       _SPS_GEN_IND_V_##nd##_##dim(ind)))))

#define _SPS_STRT_V(reg,nd,dim,ind) \
  (_SPS_STRTSTOP_V(reg,nd,dim,_LO,ind))

#define _SPS_STOP_V(reg,nd,dim,ind) \
  (_SPS_STRTSTOP_V(reg,nd,dim,_HI,ind))


#define _SPS_STRT_V_GEN(reg,nd,ind) ((nd == 1) ? _SPS_STRT_V(reg,1,0,ind) :\
                                    ((nd == 2) ? _SPS_STRT_V(reg,2,1,ind) : \
                                    ((nd == 3) ? _SPS_STRT_V(reg,3,2,ind) : \
                                    ((nd == 4) ? _SPS_STRT_V(reg,4,3,ind) : \
                                    ((nd == 5) ? _SPS_STRT_V(reg,5,4,ind) : \
                                                 _SPS_STRT_V(reg,6,5,ind))))))

#define _SPS_STOP_V_GEN(reg,nd,ind) ((nd == 1) ? _SPS_STOP_V(reg,1,0,ind) :\
                                    ((nd == 2) ? _SPS_STOP_V(reg,2,1,ind) : \
                                    ((nd == 3) ? _SPS_STOP_V(reg,3,2,ind) : \
                                    ((nd == 4) ? _SPS_STOP_V(reg,4,3,ind) : \
                                    ((nd == 5) ? _SPS_STOP_V(reg,5,4,ind) : \
                                                 _SPS_STOP_V(reg,6,5,ind))))))

/* that was MAXRANK dependent */
                             

#define _SPS_INUSE (-1)


/* macros for getting at the fields of a sparse region's node */

#define _NULL_NODE   0
#define _EMPTY_STRT -2
#define _EMPTY_STOP -1

#define _SPSNODE_ID(reg,node) (node)
#define _SPSNODE_INDEX(reg,node,dim) (_SPS_BUFF_IND(reg,dim)[node])
#define _SPSNODE_PREV(reg,node,dim)  (_SPS_BUFF_PREV(reg,dim)[node])
#define _SPSNODE_NEXT(reg,node,dim)  (_SPS_BUFF_NEXT(reg,dim)[node])

#define _SPSNODE_PREV_SAFE(reg,node,dim) \
  ((_SPS_MEMREQ(reg)[dim].prev  || node <= 0) ? _SPSNODE_PREV(reg,node,dim) : _NULL_NODE)
#define _SPSNODE_INDEX_SAFE(reg,node,dim) \
  ((_SPS_MEMREQ(reg)[dim].ind || node <= 0) ? _SPSNODE_INDEX(reg,node,dim) : -777)
#define _SPSNODE_NEXT_SAFE(reg,node,dim) \
  ((_SPS_MEMREQ(reg)[dim].next || node <= 0) ? _SPSNODE_NEXT(reg,node,dim) : _NULL_NODE)

#define _SPSNODE_INDEX_SAFE_SET(reg,node,dim,val) \
  ((_SPS_MEMREQ(reg)[dim].ind || node <= 0) ? _SPSNODE_INDEX(reg,node,dim) = val : val)
#define _SPSNODE_PREV_SAFE_SET(reg,node,dim,val) \
  ((_SPS_MEMREQ(reg)[dim].prev || node <= 0) ? _SPSNODE_PREV(reg,node,dim) = val : val)
#define _SPSNODE_NEXT_SAFE_SET(reg,node,dim,val) \
  ((_SPS_MEMREQ(reg)[dim].next || node <= 0) ? _SPSNODE_NEXT(reg,node,dim) = val : val)

#define _SPSNODE_EMPTYID -2
#define _SPSNODE_DUMMYID -1

#define _SPS_FLUFF_BOUNDS(reg) \
  { \
    int _i; \
    for (_i=0; _i<_NUMDIMS(reg); _i++) { \
      _SPS_REG_FLUFF_LO(reg,_i) -= _REG_MYLO(reg,_i); \
      _SPS_REG_FLUFF_HI(reg,_i) -= _REG_MYHI(reg,_i); \
    } \
  }

#define _SPS_ALLOC_STRUCTURE(reg) \
  { \
    const int _numdims = _NUMDIMS(reg); \
    int _dim; \
    int _readdim; \
    int _writedim; \
    int _size; \
    int _arrnum = 0; \
    int _lohi; \
    int _i; \
    _loc_arr_info* arrbuff = (_loc_arr_info*)_zmalloc_reg(2*_numdims,\
                                                      sizeof(_loc_arr_info),\
                                                      "sps dns directory for ",\
                                                      reg);\
    for (_dim=0; _dim<_numdims; _dim++) { \
      for (_lohi=0; _lohi<2; _lohi++) { \
        _SPS_DIR(reg,_dim,_lohi) = &(arrbuff[_arrnum]); \
        _IARR_NUMDIMS(_SPS_DIR(reg,_dim,_lohi)) = _numdims-1; \
        _arrnum++; \
        _size = sizeof(int); \
        _writedim = _numdims-2; \
        for (_readdim=_numdims-1; _readdim>=0; _readdim--) { \
          if (_dim != _readdim) { \
            _IARR_OFF(_SPS_DIR(reg,_dim,_lohi),_writedim) = \
              _REG_MYLO(_SPS_IMPL_BOUNDS(reg),_readdim); \
            _IARR_STR(_SPS_DIR(reg,_dim,_lohi),_writedim) = \
              _REG_STRIDE(reg,_readdim); \
	    _IARR_BLK(_SPS_DIR(reg,_dim,_lohi),_writedim) = _size; \
            _writedim--; \
            _size *= _REG_NUMELMS(_SPS_IMPL_BOUNDS(reg),_readdim); \
          } \
	} \
	_IARR_SIZE(_SPS_DIR(reg,_dim,_lohi)) = _size; \
	_size /= sizeof(int); \
        _IARR_DATA(_SPS_DIR(reg,_dim,_lohi)) = \
          (char*)_zmalloc_reg(_size,sizeof(int),"dns dir data for ",reg); \
        if (_lohi == 0) { \
  	  for (_i=0; _i<_size; _i++) { \
            ((int*)_IARR_DATA(_SPS_DIR(reg,_dim,_lohi)))[_i] = _emptystrt; \
          } \
        } else { \
  	  for (_i=0; _i<_size; _i++) { \
            ((int*)_IARR_DATA(_SPS_DIR(reg,_dim,_lohi)))[_i] = _emptystop; \
          } \
        } \
      } \
    } \
  }

#define _SPS_RESET_STRUCTURE(reg) \
  { \
    const int _numdims = _NUMDIMS(reg); \
    int _dim; \
    int _readdim; \
    int _writedim; \
    int _size; \
    int _arrnum = 0; \
    int _lohi; \
    int _i; \
    for (_dim=0; _dim<_numdims; _dim++) { \
      for (_lohi=0; _lohi<2; _lohi++) { \
        _size = sizeof(int); \
        _writedim = _numdims-2; \
        for (_readdim=_numdims-1; _readdim>=0; _readdim--) { \
          if (_dim != _readdim) { \
            _writedim--; \
            _size *= _REG_NUMELMS(_SPS_IMPL_BOUNDS(reg),_readdim); \
          } \
	} \
	_size /= sizeof(int); \
        if (_lohi == 0) { \
  	  for (_i=0; _i<_size; _i++) { \
            ((int*)_IARR_DATA(_SPS_DIR(reg,_dim,_lohi)))[_i] = _emptystrt; \
          } \
        } else { \
  	  for (_i=0; _i<_size; _i++) { \
            ((int*)_IARR_DATA(_SPS_DIR(reg,_dim,_lohi)))[_i] = _emptystop; \
          } \
        } \
      } \
    } \
  }

      
#define _SPS_SET_DIM_INUSE(reg,nd,dim) \
  _SPS_STRT(reg,nd,dim) = _SPS_INUSE

#define _SPS_COUNT_DIM(reg,nd,dim) \
  if (_SPS_STRT(reg,nd,dim) != _SPS_INUSE) { \
    _numelms+=2; \
    _SPS_SET_DIM_INUSE(reg,nd,dim); \
  }

#define _SPS_COUNT_DIM_WEIGHTED(reg,nd,dim,w) \
  if (_SPS_STRT(reg,nd,dim) != _SPS_INUSE) { \
    _numelms+=w; \
    _SPS_SET_DIM_INUSE(reg,nd,dim); \
  }

#define _SPS_IS_INUSE(reg,nd,dim) \
  (_SPS_STRT(reg,nd,dim) != _NULL_NODE && \
   _SPS_STRT(reg,nd,dim) != _emptystrt)

#define _SPS_UNUSED(reg,nd,dim) \
  _SPS_STRT(reg,nd,dim) == _NULL_NODE

#define _SPS_INHERIT_STRUCTURE(reg,basereg,nd) \
  { \
    int _i; \
    _SPS_MEMREQ(reg) = _SPS_MEMREQ(basereg); \
    for (_i=0; _i<nd; _i++) { \
      _SPS_DIR(reg,_i,_LO) = _SPS_DIR(basereg,_i,_LO); \
      _SPS_DIR(reg,_i,_HI) = _SPS_DIR(basereg,_i,_HI); \
      _SPS_BUFF_IND(reg,_i) = _SPS_BUFF_IND(basereg,_i); \
      _SPS_BUFF_NEXT(reg,_i) = _SPS_BUFF_NEXT(basereg,_i); \
      _SPS_BUFF_PREV(reg,_i) = _SPS_BUFF_PREV(basereg,_i); \
    } \
    _SPS_CORNER_BUFF(reg) = _SPS_CORNER_BUFF(basereg); \
  }

/* default number of elements = 2 for an empty dim, 1 for ident, and 2^d
   for corners */

#define _SPS_INIT_NUMELMS(d) 2+1+(0x1<<d)


/* this is overkill -- not true for fluff, e.g...  */
#define _SPSN_GI_XD(reg,node,ind,d) \
_SPSNODE_INDEX(reg,node,d) = ((ind[d] < _ILO(d)) ? (_on_boundary=1),INT_MIN : \
                              (ind[d] > _IHI(d)) ? (_on_boundary=1),INT_MAX : \
                               ind[d])

#define _SPSNODE_GRAB_INDEX_1D(reg,node,ind) _SPSN_GI_XD(reg,node,ind,0)
#define _SPSNODE_GRAB_INDEX_2D(reg,node,ind) _SPSNODE_GRAB_INDEX_1D(reg,node,ind); _SPSN_GI_XD(reg,node,ind,1)
#define _SPSNODE_GRAB_INDEX_3D(reg,node,ind) _SPSNODE_GRAB_INDEX_2D(reg,node,ind); _SPSN_GI_XD(reg,node,ind,2)
#define _SPSNODE_GRAB_INDEX_4D(reg,node,ind) _SPSNODE_GRAB_INDEX_3D(reg,node,ind); _SPSN_GI_XD(reg,node,ind,3)
#define _SPSNODE_GRAB_INDEX_5D(reg,node,ind) _SPSNODE_GRAB_INDEX_4D(reg,node,ind); _SPSN_GI_XD(reg,node,ind,4)
#define _SPSNODE_GRAB_INDEX_6D(reg,node,ind) _SPSNODE_GRAB_INDEX_5D(reg,node,ind); _SPSN_GI_XD(reg,node,ind,5)

#define _SPS_ADD_EMPTY(reg,nd,dim) \
  _SPS_STRT(reg,nd,dim) = _emptystrt; \
  _SPS_STOP(reg,nd,dim) = _emptystop

#endif
