/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"

static int numdims;
static int capacity;
static int* indbuff[_MAXRANK];
static int numheaders;


static int* _DirAccess(_region reg,const int dim,const int index[],int lohi) {
  _ind_array arr = _SPS_DIR(reg,dim,lohi);
  char* loc = _IARR_DATA(arr);
  int readdim=0;
  int i;
  
  for (i=0; i<numdims; i++) {
    if (i != dim) {
      loc += ((((index[i])-_IARR_OFF(arr,readdim))*_IARR_BLK(arr,readdim)) /
	      _IARR_STR(arr,readdim));
      readdim++;
    }
  }

  return (int*)loc;
}


static void _SparseAllocDenseDirectory(_region_fnc reg) {
  _loc_arr_info* dnsbuff;
  _loc_arr_info* currdns;
  int dim;
  int lohi;
  int dirsize;
  int readdim;
  int writedim;
  int i;

  dnsbuff = (_loc_arr_info*)_zmalloc_reg(2*numdims*sizeof(_loc_arr_info),
					   "sps dns directory for ",reg);
  currdns = dnsbuff;
  for (dim=0; dim<numdims; dim++) { 
    for (lohi=0; lohi<2; lohi++) {
      _SPS_DIR(reg,dim,lohi) = currdns; 
      _IARR_NUMDIMS(currdns) = numdims-1; 

      dirsize = sizeof(int); 
      writedim = numdims-2; 
      for (readdim=numdims-1; readdim>=0; readdim--) { 
	if (dim != readdim) {
	  _IARR_OFF(currdns,writedim)=(_REG_MYLO(reg,readdim)-_REG_STRIDE(reg,readdim))+_SPS_REG_FLUFF_LO(reg,readdim);
	  _IARR_STR(currdns,writedim) = _REG_STRIDE(reg,readdim); 
	  _IARR_BLK(currdns,writedim) = dirsize; 
	  writedim--; 
	  dirsize *= (_REG_NUMELMS(reg,readdim)+2+
		      (_SPS_REG_FLUFF_HI(reg,readdim)-_SPS_REG_FLUFF_LO(reg,readdim)));
	}
      }
      _IARR_SIZE(currdns) = dirsize; 
      dirsize /= sizeof(int); 
      _IARR_DATA(currdns) = (char*)_zmalloc_reg(dirsize*sizeof(int),
						  "dns dir data for ",reg); 
      if (lohi == _LO) {
	for (i=0; i<dirsize; i++) { 
	  ((int*)_IARR_DATA(currdns))[i] = _EMPTY_STRT; 
	}
      } else {
	for (i=0; i<dirsize; i++) { 
	  ((int*)_IARR_DATA(currdns))[i] = _EMPTY_STOP; 
	}
      }
      
      currdns++;
    }
  }
}


static void _AddIndex(_region reg,const int index[]) {
  int i;

  if ((numheaders + 1) > capacity) {
    capacity = capacity * 2;
    for (i=0; i<numdims; i++) {
      indbuff[i] = (int*)_zrealloc_reg(indbuff[i],capacity*sizeof(int),
					  "header node indices for ",reg);
    }
  }
  for (i=0; i<numdims; i++) {
    indbuff[i][numheaders] = index[i];
  }

  /*
  switch (numdims) {
  case 1:
    printf("Adding [%d]\n",indbuff[0][numheaders]);
    break;
  case 2:
    printf("Adding [%d,%d]\n",indbuff[0][numheaders],
	   indbuff[1][numheaders]);
    break;
  case 3:
    printf("Adding [%d,%d,%d] and [%d,%d,%d]\n",indbuff[0][numheaders],
	   indbuff[1][numheaders],
	   indbuff[2][numheaders]);
    break;
  }
  */
  
  numheaders++;
}




static void _AddCorners(_region_fnc reg,int* minind,int* maxind) {
  int dim;
  int numcorners = 1 << numdims;
  int corner;
  int cornercopy;
  int index[_MAXRANK];
  int* dirloc;
  int lohi;

  _SPS_CORNER_BUFF(reg) = _zmalloc_reg(numcorners*sizeof(int),
					 "sps corners for ",reg);
  for (corner=0; corner<numcorners; corner++) {
    cornercopy = corner;
    for (dim=0; dim<numdims; dim++) {
      if (cornercopy & 0x1) {
	index[dim] = maxind[dim];
      } else {
	index[dim] = minind[dim];
      }
      cornercopy = cornercopy >> 1;
    }

    /* add into directories */
    cornercopy = corner;
    for (dim=0; dim<numdims; dim++) {
      if (cornercopy & 0x1) {
	lohi = _HI;
      } else {
	lohi = _LO;
      }
      dirloc = _DirAccess(reg,dim,index,lohi);
      *dirloc = numheaders;
      cornercopy = cornercopy >> 1;
    }
    
    /* add into corner pointers */
    _SPS_CORNER(reg,corner) = numheaders;

    _AddIndex(reg,index);
  }
}


static void _InsertHeaders(_region_fnc reg,int* minind,int* maxind,int headnum,
			   int writepat,int topbit,int topdim,int dim) {
  int i;
  int loindex[_MAXRANK];
  int hiindex[_MAXRANK];
  int* loloc;
  int* hiloc;

  /*
  printf("    requested to insert using %d by modifying dim %d\n",writepat,
	 dim);
  */
  for (i=0; i<numdims; i++) {
    if (i == dim) {
      loindex[i] = minind[i];
      hiindex[i] = maxind[i];
    } else {
      loindex[i] = indbuff[i][headnum];
      hiindex[i] = loindex[i];
    }
  }
  loloc = _DirAccess(reg,dim,loindex,_LO);
  hiloc = _DirAccess(reg,dim,hiindex,_HI);
  if (*loloc == _EMPTY_STRT) {
    /*
    printf("    inserting into directory for dim %d\n",topdim);
    */
    *loloc = numheaders;
    *hiloc = numheaders+1;
    
    while (topdim < numdims) {
      if ((topdim != dim) && (topbit & writepat)) {
	if (loindex[topdim] == minind[i]) {
	  /*
	  printf("    inserting into lo directory for dim %d\n",topdim);
	  */
	  loloc = _DirAccess(reg,topdim,loindex,_LO);
	  *loloc = numheaders;
	  hiloc = _DirAccess(reg,topdim,hiindex,_LO);
	  *hiloc = numheaders+1;
	} else {
	  /*
	  printf("    inserting into hi directory for dim %d\n",topdim);
	  */
	  loloc = _DirAccess(reg,topdim,loindex,_HI);
	  *loloc = numheaders;
	  hiloc = _DirAccess(reg,topdim,hiindex,_HI);
	  *hiloc = numheaders+1;
	}
      }
      topdim++;
      topbit = topbit << 1;
    }
    
    _AddIndex(reg,loindex);
    _AddIndex(reg,hiindex);
  } else {
    /*
    printf("    seen this one before...\n");
    */
  }
}


static void _SparseSetupHeaderNodes(_region_fnc reg,_index_list inds) {
  int numvals;
  int val;
  int dim;
  int i;
  int* loloc;
  int* hiloc;
  int* intptr;
  int* indptr;

  numvals = inds->numelms;
  numheaders = 0;

  capacity = 64;
  for (dim=0; dim<numdims; dim++) {
    indbuff[dim] = (int*)_zmalloc_reg(capacity*sizeof(int),
					"header node indices for ",reg);
  }

  if (numdims == 1) {
    return;
  }
  for (dim=0; dim<numdims; dim++) {
    intptr = inds->data;
    for (val = 1; val < numvals; val++) {
      intptr += numdims;
      /*      printf("Considering [%d,%d]\n",*intptr,*(intptr+1));*/
      loloc = _DirAccess(reg,dim,intptr,_LO);
      hiloc = _DirAccess(reg,dim,intptr,_HI);
      if (*loloc == _EMPTY_STRT) {
	*loloc = numheaders;
	*hiloc = numheaders+1;
	if (numheaders+2 > capacity) {
	  capacity *= 2;
	  for (i=0; i<numdims; i++) {
	    indbuff[i] = (int*)_zrealloc_reg(indbuff[i],capacity*sizeof(int),
						"header node indices for ",reg);
	  }
	}
	indptr = intptr;
	for (i=0; i<numdims; i++) {
	  if (i != dim) {
	    indbuff[i][numheaders] = *indptr;
	    indbuff[i][numheaders+1] = *indptr;
	  } else {
	    indbuff[i][numheaders] = inds->minind[i];
	    indbuff[i][numheaders+1] = inds->maxind[i];
	  }
	  indptr++;
	}

	numheaders += 2;
      }
    }
  }
}


static void _CompressHeaders(_region_fnc reg,int* minind,int* maxind) {
  int readpat;
  int writepat;
  int bit;
  int topbit;
  int topdim;
  int i;
  int dim;
  int maxbit;
  int fullpat;

  maxbit = 0x1 << (numdims-1);
  fullpat = (0x1 << numdims)-1;

  if (numdims > 2) { /* no headers need compressing for 1D/2D arrays */
    for (i=0; i<numheaders; i++) {
      /*
      printf("header %d = [%d,%d,%d]\n",i,indbuff[0][i],indbuff[1][i],
	     indbuff[2][i]);
      */

      /* classify the pattern that we're reading */
      bit = 0x1;
      readpat = 0;
      topbit = 0;
      for (dim=0; dim<numdims; dim++) {
	if ((indbuff[dim][i] == minind[dim]) || 
	    (indbuff[dim][i] == maxind[dim])) {
	  readpat |= bit;
	  if (topbit == 0) {
	    topbit = bit;
	    topdim = dim;
	  }
	}
	bit = bit << 1;
      }
      /*
	printf("  pattern = %d\n",readpat);
	printf("  topbit  = %d\n",topbit);
      */
      
      bit = topbit;
      dim = topdim;
      while (bit != maxbit) {
	bit = bit << 1;
	dim++;
	
	writepat = bit | readpat;
	if ((writepat != readpat) && (writepat != fullpat)) {
	  _InsertHeaders(reg,minind,maxind,i,writepat,topbit,topdim,dim);
	}
      }
    }
  }

  _AddCorners(reg,minind,maxind);

  /*
  for (i=0; i<numheaders; i++) {
    printf("header %d = [%d,%d,%d]\n",i,indbuff[0][i],indbuff[1][i],
	   indbuff[2][i]);
  }
  */

  /*  _RT_ALL_FATAL0("Brad: Implement CompressHeaders\n");*/
}


static void _AllocateNodes(_region_fnc reg,_index_list inds) {
  int numvals;
  int numnodes;
  int i;
  int j;
  int numtoalloc;

  /*  _print_index_list(inds);*/

  numheaders += 2;  /* memory for blank row/col */
  numvals = inds->numelms;
  numnodes = numvals + numheaders;

  /*
  printf("[%d] numvals = %d\n",_INDEX,numvals); 
  printf("[%d] numnodes = %d\n",_INDEX,numnodes);
  */

  _SPS_NUMVALS(reg) = numvals;
  _SPS_NUMNODES(reg) = numnodes;

  for (i=0; i<numdims; i++) {
    if (_SPS_MEMREQ(reg)[i].ind) {
      numtoalloc = numnodes;
    } else {
      numtoalloc = numheaders+1; /* plus origin */
    }
    indbuff[i] = (int*)_zrealloc_reg(indbuff[i],numtoalloc*sizeof(int),
					"header node indices for ",reg);
    _SPS_BUFF_IND(reg,i) = indbuff[i];

    if (_SPS_MEMREQ(reg)[i].next) {
      numtoalloc = numnodes;
    } else {
      numtoalloc = numheaders+1; /* plus origin */
    }
    _SPS_BUFF_NEXT(reg,i) = (int*)_zmalloc_reg(numtoalloc*sizeof(int),
						 "sps next ptrs for ",reg);
    for (j=0; j<numtoalloc; j++) {
      _SPS_BUFF_NEXT(reg,i)[j] = _NULL_NODE;
    }


    if (_SPS_MEMREQ(reg)[i].prev) {
      numtoalloc = numnodes;
    } else {
      numtoalloc = numheaders+1; /* plus origin */
    }
    _SPS_BUFF_PREV(reg,i) = (int*)_zmalloc_reg(numtoalloc*sizeof(int),
						 "sps prev ptrs for ",reg);
    for (j=0; j<numtoalloc; j++) {
      _SPS_BUFF_PREV(reg,i)[j] = _NULL_NODE;
    }

    _SPS_BUFF_IND(reg,i) += numheaders;
    _SPS_BUFF_NEXT(reg,i) += numheaders;
    _SPS_BUFF_PREV(reg,i) += numheaders;

    _SPSNODE_INDEX(reg,_NULL_NODE,i) = 0;
    _SPSNODE_PREV(reg,_NULL_NODE,i) = _NULL_NODE;
    _SPSNODE_NEXT(reg,_NULL_NODE,i) = _NULL_NODE;

    _SPSNODE_INDEX(reg,_EMPTY_STRT,i) = INT_MAX;
    _SPSNODE_PREV(reg,_EMPTY_STRT,i) = _NULL_NODE;
    _SPSNODE_NEXT(reg,_EMPTY_STRT,i) = _EMPTY_STOP;

    _SPSNODE_INDEX(reg,_EMPTY_STOP,i) = INT_MIN;
    _SPSNODE_PREV(reg,_EMPTY_STOP,i) = _EMPTY_STRT;
    _SPSNODE_NEXT(reg,_EMPTY_STOP,i) = _NULL_NODE;
  }
}


static void _AdjustDirectories(_region reg) {
  int dim;
  int lohi;
  _ind_array_pnc arr;
  int numelems;
  int i;
  int numcorners = 1 << numdims;

  for (dim=0; dim<numdims; dim++) {
    for (lohi=0; lohi<2; lohi++) {
      arr = _SPS_DIR(reg,dim,lohi);
      numelems = _IARR_SIZE(arr)/sizeof(int);
      for (i=0; i<numelems; i++) {
	if (((int*)_IARR_DATA(arr))[i] >= 0) {
	  ((int*)_IARR_DATA(arr))[i] -= numheaders;
	}
      }
    }
  }

  for (i=0; i<numcorners; i++) {
    _SPS_CORNER(reg,i) -= numheaders;
  }
}


/* _LinkHeaders()...

   The idea behind this routine is that it will link all header nodes
   within each plane of the sparse directory by iterating over the
   dense directory.

   dim = the planes that are being linked (low and high dense
         directories)
   dim2 = the dimension currently being linked
   dim3 = the counter for incrementing dimensions in the MLOOP
*/

static void _LinkHeaders(_region reg) {
  int dim;
  _ind_array_pnc arr;
  int dim2;
  int i;
  int ind[_MAXRANK];
  int* loloc;
  int* hiloc;
  int* stoploc;
  int step;
  int currlonode;
  int nextlonode;
  int currhinode;
  int nexthinode;
  int blkdim;
  int dim3;
  int done;
  int breaking;

  /* 1D sparsity is a very special case... */
  if (numdims == 1) {
    _SPSNODE_NEXT(reg,-4,0) = -3;
    _SPSNODE_PREV(reg,-3,0) = -4;
    return;
  }

  /* iterate over the directories for dim "dim" */
  for (dim=0; dim<numdims; dim++) {
    arr = _SPS_DIR(reg,dim,_LO);
    blkdim = 0;

    /* setup links for dim "dim2" */
    for (dim2=0; dim2<numdims; dim2++) {
      if (dim != dim2) {
	for (i=0; i<numdims; i++) {
	  ind[i] = (_REG_MYLO(reg,i)-_REG_STRIDE(reg,i)) + _SPS_REG_FLUFF_LO(reg,i);
	}
	done = 0;

	/* Here's the beginning of the MLOOP over the plane that
	   we're dealing with */
	while (!done) {

	  /* get the lo/hi locations to iterate */
	  loloc = _DirAccess(reg,dim,ind,_LO);
	  hiloc = _DirAccess(reg,dim,ind,_HI);

	  ind[dim2] = _REG_MYHI(reg,dim2)+_REG_STRIDE(reg,dim2)+_SPS_REG_FLUFF_HI(reg,dim2);
	  stoploc = _DirAccess(reg,dim,ind,_LO);

	  /*
	  printf("dim: %d/%d, lo,hi,stop = %d,%d,%d\n",dim,dim2,
		 *loloc,*hiloc,*stoploc);
	  */
	  step = _IARR_BLK(arr,blkdim)/sizeof(int);
	  
	  currlonode = *loloc;
	  currhinode = *hiloc;

	  if (currlonode != _EMPTY_STRT) {
	    _SPSNODE_PREV(reg,currlonode,dim2) = _NULL_NODE;
	    _SPSNODE_PREV(reg,currhinode,dim2) = _NULL_NODE;

	    /* walk along dim2, linking as we go */
	    do {
	      loloc += step;
	      hiloc += step;
	      /*
		printf("lo/hi loc = %d,%d\n",*loloc,*hiloc);
	      */
	      nextlonode = *loloc;
	      nexthinode = *hiloc;
	      if (nextlonode != _EMPTY_STRT) {
		/*
		  printf("  linking %d to %d\n",currlonode,nextlonode);
		*/
		_SPSNODE_NEXT(reg,currlonode,dim2) = nextlonode;
		_SPSNODE_PREV(reg,nextlonode,dim2) = currlonode;
		currlonode = nextlonode;
		
		/*
		  printf("  linking %d to %d\n",currhinode,nexthinode);
		*/
		_SPSNODE_NEXT(reg,currhinode,dim2) = nexthinode;
		_SPSNODE_PREV(reg,nexthinode,dim2) = currhinode;
		currhinode = nexthinode;
	      }
	    } while (loloc != stoploc);
	    _SPSNODE_NEXT(reg,currlonode,dim2) = _NULL_NODE;
	    _SPSNODE_NEXT(reg,currhinode,dim2) = _NULL_NODE;
	  }


	  /* update the ind[] value for the next iteration of the
             MLOOP */
	  ind[dim2] = (_REG_MYLO(reg,dim2)-_REG_STRIDE(reg,dim2)) + 
	    _SPS_REG_FLUFF_LO(reg,dim2);
	  breaking = 0;
	  for (dim3=numdims-1; dim3>=0; dim3--) {
	    if (dim3 != dim && dim3 != dim2) {
	      ind[dim3] += _REG_STRIDE(reg,dim3);
	      if (ind[dim3] <= (_REG_MYHI(reg,dim3)+_REG_STRIDE(reg,dim3)+
				_SPS_REG_FLUFF_HI(reg,dim3))) {
		breaking = 1;
		break;
	      } else {
		ind[dim3] = (_REG_MYLO(reg,dim3)-_REG_STRIDE(reg,dim3))+
		  _SPS_REG_FLUFF_LO(reg,dim3);
	      }
	    }
	  }
	  if (!breaking) {
	    done = 1;
	  }
	}

	blkdim++;
      }
    }
  }
}


static void _InsertIndices(_region reg,_index_list inds) {
  int numvals;
  int* intptr;
  int i;
  int j;
  int tail;
  int head;
  
  numvals = inds->numelms;
  intptr = inds->data;
  for (i=1; i<numvals; i++) {
    intptr += numdims;
    for (j=0; j<numdims; j++) {
      tail = *(_DirAccess(reg,j,intptr,_HI));
      if (_SPSNODE_PREV_SAFE(reg,tail,j) == _NULL_NODE) {
	head = *(_DirAccess(reg,j,intptr,_LO));
      } else {
	head = _SPSNODE_PREV(reg,tail,j);
      }
      _SPSNODE_NEXT_SAFE_SET(reg,head,j,i);
      _SPSNODE_PREV_SAFE_SET(reg,i,j,head);
      _SPSNODE_PREV_SAFE_SET(reg,tail,j,i);
      _SPSNODE_NEXT_SAFE_SET(reg,i,j,tail);
      _SPSNODE_INDEX_SAFE_SET(reg,i,j,intptr[j]);
    }
  }
}


void _SparseBuildDirectory(_region_fnc reg,_index_list inds) {
  numdims = _NUMDIMS(reg);

  _sort_ind_list(inds);

  /*  printf("[%d] allocating sparse directory\n",_INDEX);*/
  _SparseAllocDenseDirectory(reg);
  /*  printf("[%d] setting up header nodes\n",_INDEX);*/
  _SparseSetupHeaderNodes(reg,inds);
  /*  printf("[%d] compressing headers\n",_INDEX);*/
  _CompressHeaders(reg,inds->minind,inds->maxind);
  /*  printf("[%d] allocating nodes\n",_INDEX);*/
  _AllocateNodes(reg,inds);
  /*  printf("[%d] adjusting directories\n",_INDEX);*/
  _AdjustDirectories(reg);
  /*  printf("[%d] linking headers\n",_INDEX);*/
  _LinkHeaders(reg);

  /* _PrintSparseNodes(reg);*/

  /*  printf("[%d] inserting indices\n",_INDEX);*/
  _InsertIndices(reg,inds);

  /*
  _PrintSparseNodes(reg);
  */

  /* if all we've got is the identity, then we ain't got much... :) */
  if (_SPS_NUMVALS(reg) == 1) {
    _REG_I_OWN(reg) = 0;
  }

  /*
    printf("[%d] numvals = %d\n",_INDEX,_SPS_NUMVALS(reg));
    printf("[%d] numelems = %d\n",_INDEX,_SPS_NUMNODES(reg));
  */
}
