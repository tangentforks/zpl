/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "zlib.h"
#include "md_zlib.h"
#include "zerror.h"


static void resize(_index_list l) {
  l->capacity *= 2;
  l->data = (int*)_zrealloc_reg(l->data,l->capacity*l->numdims*sizeof(int),
				   "index list for ",l->reg);
}


void _add_sparse_index(_index_list l,int i[]) {
  const int numdims = l->numdims;
  int dim;
  int* indptr;

  for (dim=0; dim<numdims; dim++) {
    if ((i[dim] < l->minind[dim]) || (i[dim] > l->maxind[dim])) {
      /*
      printf("[%d] returning for dim %d: %d\n",_INDEX,dim,i[dim]);
      */
      return;
    }
  }

  if (l->numelms == l->capacity) {
    resize(l);
  }

  indptr=&(l->data[l->numelms*numdims]);
  for (dim=0; dim<numdims; dim++) {
    *indptr = i[dim];
    indptr++;
  }

  l->numelms++;
}


static void _compress_ind_list(_index_list inds) {
  int numdims = inds->numdims;
  int numelms = inds->numelms;
  int i;
  int* writeind;
  int* readind;
  int same;
  int eliminated=0;
  int dim;

  writeind = &(inds->data[numdims]);
  readind = writeind+numdims;

  /*
  printf("[%d] numelems before compressing = %d\n",_INDEX,numelms);
  */

  for (i=2; i<numelms; i++) {
    /*
    printf("writing: [%d,%d]\n",writeind[0],writeind[1]);
    printf("reading: [%d,%d]\n",readind[0],readind[1]);
    */
    same = 1;
    for (dim=0; dim<numdims; dim++) {
      if (writeind[dim] != readind[dim]) {
	same = 0;
	break;
      }
    }

    if (same) {
      eliminated++;
    } else {
      /*
      printf("not the same... updating writeind\n");
      */
      writeind+=numdims;
      if (writeind != readind) {
	/*
	printf("and copying\n");
	*/
	for (dim=0; dim<numdims; dim++) {
	  writeind[dim] = readind[dim];
	}
      }
    }

    readind+=numdims;
  }
  inds->numelms -= eliminated;

  /*
  printf("[%d] numelems after compressing = %d, eliminated = %d\n",_INDEX,
	 inds->numelms,eliminated);
  */

  /*
  printf("After compressing:\n");
  printf("=================\n");
  _print_index_list(inds);
  printf("=================\n");
  */
}


static void _sort_single_dim(_index_list inds,int numdims,int dim,
			     int loind,int hiind) {
  int numbuckets;
  int minind;
  int indnum;
  int* indptr;
  int lastind;
  int i;
  int sorted;
  int currind;
  static int bucketsize[_MAXRANK] = {0,0,0,0,0,0};
  static int* buckets[_MAXRANK] = {NULL,NULL,NULL,NULL,NULL,NULL};
  static int* slot = NULL;
  static int slotsize=0;
  int skipping;
  int srcloc;
  int targloc;
  int lotargloc;
  int tempind[2][_MAXRANK];
  int tempnum=0;
  int lo;
  int hi;

  /*
  printf("Sorting dim %d (%d..%d)\n",dim,loind,hiind);
  */

  /* allocate buckets for this dim */
  minind = inds->minind[dim];
  numbuckets = inds->maxind[dim]-minind+1;
  if (numbuckets > bucketsize[dim]) {
    bucketsize[dim] = numbuckets;
    buckets[dim] = (int*)_zrealloc(buckets[dim],numbuckets*sizeof(int),
				      "buckets for sorting inds");
  }

  /* loop over indices for this dimension */
  indnum=loind;
  indptr = &(inds->data[loind*numdims+dim]);
  lastind = *indptr;

  /* zero out buckets for this dim */
  for (i=0; i<numbuckets; i++) {
    buckets[dim][i] = 0;
  }
      
  /* iterate over indices in this bucket */
  sorted = 1;
  while (indnum < hiind) {
    /*
      printf("    doing index %d\n",indnum);
    */
    currind = *indptr;
    if (currind < lastind) {
      sorted = 0;
    }
    buckets[dim][currind - minind]++;
	
    lastind = currind;
    indptr += numdims;
    indnum++;
  }

  buckets[dim][0]+=loind; /* make this a loind-based system to skip the start */
  /*
    printf("bucket[0] = %d\n",buckets[dim][0]);
  */
  for (i=1; i<numbuckets; i++) {
    /* scan buckets */
    buckets[dim][i] += buckets[dim][i-1];
    /*
      printf("bucket[%d] = %d\n",i,buckets[dim][i]);
    */
  }

  if (sorted) {
    /*
      printf("Already sorted!\n");
    */
  } else {
    if (numbuckets+1 > slotsize) {
      slotsize = numbuckets+1;
      slot = (int*)_zrealloc(slot,slotsize*sizeof(int),
				"slots for sorting inds");
    }

    slot[0] = loind;
    /*
      printf("slot[0] = 1;\n");
    */
    for (i=1; i<numbuckets; i++) {
      slot[i] = buckets[dim][i-1];
      /*
	printf("slot[%d] = %d\n",i,slot[i]);
      */
    }
    
    /* move things to their proper place */
    indnum = loind;
    indptr = &(inds->data[loind*numdims+dim]);
    while (indnum < hiind) {
      skipping = 1;
      do {
	currind = *indptr;
	
	srcloc = indnum;
	targloc = slot[currind-minind];
	lotargloc = buckets[dim][(currind-minind)-1];
	
	if ((srcloc == lotargloc) &&
	    (targloc == buckets[dim][currind-minind])) {
	  /*
	    printf("Skipping this bucket, it's already sorted\n");
	  */
	  if (targloc != srcloc) {
	    indnum = targloc;
	    indptr += ((indnum-srcloc)*numdims);
	  } else {
	    /*
	      printf("here's this funny case\n");
	    */
	    indnum++;
	    indptr += numdims;
	  }
	} else {
	  skipping = 0;
	}
      } while (skipping && (indnum < hiind));
      
      
      if (indnum < hiind) {
	/*
	  printf("I'm trying to sort index %d at %d\n",currind,srcloc);
	
	  printf("Is it between %d and %d?\n",lotargloc,targloc);
	*/
	if (srcloc >= lotargloc && srcloc <= targloc) {
	  if (srcloc == targloc) {
	    /*
	      printf("It's in a good spot!\n");
	    */
	    slot[currind-minind]++;
	  }
	} else {
	  /*
	    printf("I'll be moving it from %d to %d\n",srcloc,targloc);
	  */
	  
	  for (i=dim; i<numdims; i++) {
	    tempind[tempnum][i] = indptr[i-dim];
	  }
	  
	  do {
	    tempnum = !tempnum;
	    for (i=dim; i<numdims; i++) {
	      tempind[tempnum][i] = inds->data[targloc*numdims+i];
	    }
	    tempnum = !tempnum;
	    for (i=dim; i<numdims; i++) {
	      inds->data[targloc*numdims+i] = tempind[tempnum][i];
	    }
	    slot[currind-minind]++;
	    
	    tempnum = !tempnum;
	    currind = tempind[tempnum][dim];
	    targloc = slot[currind-minind];
	    /*	    
	      printf("Now I have to move the value it replaced (%d) to %d\n",
	      currind,targloc);
	    */
	  } while (targloc != srcloc);
	  
	  for (i=dim; i<numdims; i++) {
	    indptr[i-dim] = tempind[tempnum][i];
	  }
	  slot[currind-minind]++;
	}
	
	indptr += numdims;
	indnum++;
      }
    }
  }

  /*
  if (dim != 0) {
    int j;
    printf("here's the state:\n");
    indptr = &(inds->data[loind*numdims]);
    for (i=loind; i<hiind; i++) {
      printf("[%d",*(indptr++));
      for (j=1; j<numdims; j++) {
	printf(",%d",*(indptr++));
      }
      printf("]\n");
    }
  }
  */

  if (dim != numdims-1) {
    lo = loind;
    for (i=0; i<numbuckets; i++) {
      hi = buckets[dim][i];
      if (lo != hi) {
	_sort_single_dim(inds,numdims,dim+1,lo,hi);
      }
      lo = hi;
    }
  }

  if (dim == 0) {
    if (slot != NULL) {
      _zfree(slot,"slots for sorting inds");
      slot = NULL;
      slotsize = 0;
    }
    for (i=0; i<numdims; i++) {
      if (buckets[i] != NULL) {
	_zfree(buckets[i],"buckets for sorting inds");
	buckets[i] = NULL;
	bucketsize[i] = 0;
      }
    }
  }

}


static int _already_sorted(_index_list inds,int numdims,int dim,
			   int loind,int hiind) {
  int i;
  int j;
  int loval;
  int hival;

  for (i=loind; i<hiind-1; i++) {
    for (j=0; j<numdims; j++) {
      loval = inds->data[i*numdims + j];
      hival = inds->data[(i+1)*numdims + j];
      if (loval < hival) {
	break;
      } else if (loval == hival) {
      } else {
	/*
	printf("dim %d out of order for elems %d and %d: %d > %d\n",j,i,i+1,
	       loval, hival);
	*/
	return 0;
      }
    }
  }
  /*
  printf("List already sorted.\n");
  */
  return 1;
}


void _sort_ind_list(_index_list inds) {
  int numdims = inds->numdims;

  /*
    printf("Prior to sorting:\n");
    printf("=================\n");
    _print_index_list(inds);
    printf("=================\n");
  */

  if (!_already_sorted(inds,numdims,0,1,inds->numelms)) {
    _sort_single_dim(inds,numdims,0,1,inds->numelms);
  }
  
  _compress_ind_list(inds);
}



_index_list _new_index_list_withcap(int numdims,_region reg,int cap) {
  int i;
  _index_list l;

  l = (_index_list)_zmalloc_reg(sizeof(_index_list_info),
				  "index list structure for ",reg);
  l->numdims = numdims;
  l->capacity = cap;
  l->numelms = 0;
  l->data = (int*)_zmalloc_reg(l->capacity*numdims*sizeof(int),
				 "index list for ",reg);
  for (i=0; i<numdims; i++) {
    l->minind[i] = (_REG_MYLO(reg,i)-_REG_STRIDE(reg,i))+_SPS_REG_FLUFF_LO(reg,i);
    l->maxind[i] = _REG_MYHI(reg,i)+_REG_STRIDE(reg,i)+_SPS_REG_FLUFF_HI(reg,i);
  }
  l->reg = reg;
  l->headers = NULL;

  /* insert sentinel value */
  _add_sparse_index(l,l->minind);

  return l;
}


_index_list _new_index_list(int numdims,_region reg) {
  return _new_index_list_withcap(numdims,reg,64);
}



void _print_index_list(_index_list l) {
  int i;
  int j;
  int* intptr;
  int numdims = l->numdims;

  _ResetToken();
  if (_INDEX != 0) {
    _RecvToken();
  }
  if (_PROCESSORS != 1) {
    printf("Processor %d's contribution:\n",_INDEX);
  }

  intptr = l->data+numdims;
  for (i=1; i<l->numelms; i++) {
    printf("[");
    for (j=0; j<numdims; j++) {
      if (j) {
	printf(",");
      }
      printf("%2d",*intptr);
      intptr++;
    }
    printf("]\n");
  }
  printf("---------- %d elements\n\n",l->numelms);
  fflush(stdout);

  if (_INDEX != _PROCESSORS-1) {
    _SendToken(_INDEX+1);
  }
  _BarrierSynch();
}


void _old_index_list(_index_list l) {
  if (l->headers) {
    _zfree_reg(l->headers->data,"index list for ",l->reg);
    _zfree_reg(l->headers,"index list structure for ",l->reg);
  }
  _zfree_reg(l->data,"index list for ",l->reg);
  _zfree_reg(l,"index list structure for ",l->reg);
}

void _ReallocSparseArray(_array_fnc arr) {
  /*  printf("[%d] reallocating sparse array\n",_INDEX);*/
  _MemLogMark("Start of ReallocSparseArray()");
  _zfree(_ARR_DATA(arr),"resising sparse array");
  _ARR_DATA_SIZE(arr)=_ARR_BLK(arr,1)*_SPS_NUMVALS(_ARR_DECL_REG(arr));
  _ARR_DATA(arr) = (char*)_zmalloc(_ARR_DATA_SIZE(arr),
				     "resizing sparse array");
  _ARR_ORIGIN(arr)=_ARR_DATA(arr);
  _ARR_SPS_ZERO_IDENT(arr);
  _MemLogMark("End of ReallocSparseArray()");
}
