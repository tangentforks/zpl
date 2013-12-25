/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"


void _SpsCopyShiftStructure(_region_fnc reg,_region basereg,_direction dir) {
  int numdims;
  int numnodes;
  int numvals;
  int numheaders;
  int i;
  int j;
  int k;
  int readdim;
  _ind_array_nc dst_dir;
  _ind_array_pnc src_dir;
  int srcindex;
  int numtoalloc;
  int looplimit;

  numdims = _NUMDIMS(reg);
  numnodes = _SPS_NUMNODES(basereg);
  numvals = _SPS_NUMVALS(basereg);
  numheaders = numnodes-numvals;
  _SPS_NUMVALS(reg) = numvals;
  _SPS_NUMNODES(reg) = numnodes;
  _SPS_MEMREQ(reg) = _SPS_MEMREQ(basereg);
  _SPS_CORNER_BUFF(reg) = _SPS_CORNER_BUFF(basereg);

  for (i=0; i<numdims; i++) {
    _SPS_BUFF_NEXT(reg,i) = _SPS_BUFF_NEXT(basereg,i);
    _SPS_BUFF_PREV(reg,i) = _SPS_BUFF_PREV(basereg,i);
    if (dir[i] == 0) {
      _SPS_BUFF_IND(reg,i) = _SPS_BUFF_IND(basereg,i);
      _SPS_DIR(reg,i,_LO) = _SPS_DIR(basereg,i,_LO);
      _SPS_DIR(reg,i,_HI) = _SPS_DIR(basereg,i,_HI);
    } else {
      if (_SPS_MEMREQ(basereg)[i].ind) {
	numtoalloc = numnodes;
	looplimit = numvals;
      } else {
	numtoalloc = numheaders+1; /* include origin */
	looplimit = 1;
      }
      _SPS_BUFF_IND(reg,i) = (int*)_zmalloc(numtoalloc*sizeof(int),
					      "shifted sparse indices");
      _SPS_BUFF_IND(reg,i) += numheaders; /* include origin */
      for (j=-numheaders; j<looplimit; j++) {
	srcindex = _SPS_BUFF_IND(basereg,i)[j];
	if (((j >= -2) && (j <= 0)) ||
	    ((j < -2) &&
	     (((dir[i] < 0) && (_SPSNODE_NEXT(basereg,j,i) == 0)) ||
	      ((dir[i] > 0) && (_SPSNODE_PREV(basereg,j,i) == 0))))) {
	  /* don't shift dummy nodes, identity node, or nodes on the
	     extreme side of a region (as doing so will move dummy
	     nodes into the actual data of a region incorrectly...)
	     running apps_sparse/2d/brad/test_prep5.z on > 2 procs
	     demonstrates this. */
	  _SPS_BUFF_IND(reg,i)[j] = srcindex;
	} else {
	  /* all other nodes should be shifted by the offset */
	  _SPS_BUFF_IND(reg,i)[j] = srcindex + dir[i];
	}
      }

      for (j=_LO; j<=_HI; j++) {
	dst_dir = (_loc_arr_info*)_zmalloc(sizeof(_loc_arr_info),
					     "shifted sparse directory");
	_SPS_DIR(reg,i,j) = dst_dir;
	src_dir = _SPS_DIR(basereg,i,j);
	_IARR_SIZE(dst_dir) = _IARR_SIZE(src_dir);
	_IARR_NUMDIMS(dst_dir) = _IARR_NUMDIMS(src_dir);
	_IARR_DATA(dst_dir) = _IARR_DATA(src_dir);
	readdim = 0;
	for (k=0; k<numdims-1; k++) {
	  _IARR_OFF(dst_dir,k) = _IARR_OFF(src_dir,k)+dir[readdim];
	  _IARR_BLK(dst_dir,k) = _IARR_BLK(src_dir,k);
	  _IARR_STR(dst_dir,k) = _IARR_STR(src_dir,k);
	  readdim++;
	  if (k == i) {
	    readdim++;
	  }
	}
      }
    }
  }
}


static void print_index(_region reg,int ind,int dim) {
  int lo = _REG_MYLO(reg,dim);
  int hi = _REG_MYHI(reg,dim);

  if (ind <= lo) {
    printf("START");
  } else if (ind >= hi) {
    printf("STOP");
  } else {
    printf("%d",ind);
  }
}


void _PrintSparse2DRegion(_region reg,int wholething) {
  int spsptr;
  int _I(0);
  int printed=0;
  int index;
  int id;
  int printnum=1;
  int outerdim=0;
  int innerdim=1;
  
  _ResetToken();
  if (_INDEX != 0) {
    _RecvToken();
  }
  if (_PROCESSORS != 1) {
    printf("Processor %d's contribution:\n",_INDEX);
  }
  for (_i0=_REG_MYLO(reg,outerdim); _i0<=_REG_MYHI(reg,outerdim); _i0++) {
    printed = 0;

    spsptr = _SPS_STRT(reg,2,1);
    index = _SPSNODE_INDEX(reg,spsptr,innerdim);
    while (spsptr != _NULL_NODE && index <= _REG_MYHI(reg,innerdim)) {
      index = _SPSNODE_INDEX(reg,spsptr,innerdim);
      if (((index >= _REG_MYLO(reg,innerdim)) && 
	   (index <= _REG_MYHI(reg,innerdim))) ||
	  wholething) {
	id = _SPSNODE_ID(reg,spsptr);
	if (id != _SPSNODE_EMPTYID) {
	  printf("[");
	  print_index(reg,_SPSNODE_INDEX(reg,spsptr,0),0);
	  printf(",");
	  print_index(reg,_SPSNODE_INDEX(reg,spsptr,1),1);
	  printf("]");
	  if (id != _SPSNODE_DUMMYID) {
	    printf(":%d",id);
	  }
	  if (printnum) {
	    printf("(%d)",spsptr);
	  }
	  printf(" ");
	  printed=1;
	}
      }

      spsptr++;
    }
    if (printed) {
      printf("\n");
    }      
  }
  printf("\n\n");
  fflush(stdout);
  if (_INDEX != _PROCESSORS-1) {
    _SendToken(_INDEX+1);
  }
  _BarrierSynch();
}


static void PrintSingleRange(int prev,int index,int next,int numdims) {
  printf("  ");
  if (prev == _NULL_NODE) {
    printf("NULL");
  } else {
    printf("%4d",prev);
  }
  printf(" <- ");
  if (index == INT_MIN) {
    printf("MIN");
  } else if (index == INT_MAX) {
    printf("MAX");
  } else if (index == -777) {
    printf("???");
  } else {
    printf("%3d",index);
  }
  printf(" -> ");
  if (next == _NULL_NODE) {
    printf("NULL");
  } else {
    printf("%4d",next);
  }
  printf("\n");
}


static void PrintSingleRangeInner(_region reg,int id,int index,int numdims) {
  int prev = id-1;
  int next = id+1;

  if (id <= 0) {
    prev = _SPSNODE_PREV_SAFE(reg,id,numdims-1);
    next = _SPSNODE_NEXT_SAFE(reg,id,numdims-1);
    PrintSingleRange(prev,index,next,numdims);
  } else {
    printf(" ... <- %3d -> ... \n",index);
  }
}


void _PrintSparseNodes(_region reg) {
  int i;
  int j;
  int numdims = _NUMDIMS(reg);
  
  _ResetToken();
  if (_INDEX != 0) {
    _RecvToken();
  }
  if (_PROCESSORS != 1) {
    printf("Processor %d's contribution:\n",_INDEX);
  }

  for (i=_SPS_NUMVALS(reg)-_SPS_NUMNODES(reg); i<_SPS_NUMVALS(reg); i++) {
    printf("---------------\n");
    printf("node %2d:\n",i);
    printf("  index = %d\n",_SPSNODE_ID(reg,i));
    for (j=0; j<numdims-1; j++) {
      PrintSingleRange(_SPSNODE_PREV_SAFE(reg,i,j),
		       _SPSNODE_INDEX_SAFE(reg,i,j),
		       _SPSNODE_NEXT_SAFE(reg,i,j),numdims);
    }
    PrintSingleRangeInner(reg,i,_SPSNODE_INDEX_SAFE(reg,i,numdims-1),numdims);
    printf("---------------\n\n");
  }

  printf("\n\n");
  fflush(stdout);
  if (_INDEX != _PROCESSORS-1) {
    _SendToken(_INDEX+1);
  }
  _BarrierSynch();
}
