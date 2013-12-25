/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "md_zinc.h"
#include "ensemble.h"
#include "file_io.h"
#include "stand_inc.h"
#include "zlib.h"

#define _VOIDPTR_HIT ((void*)0xffff)
#define _VOIDPTR_MIS ((void*)NULL)


_PRIV_DECL(static, unsigned int, globelemsize);
_PRIV_DECL(static, int, glb_numdims);


static _arr_info __dummy_arr = {
  NULL,
  0,
  {0,0,0,0,0,0},
  NULL,
  NULL,
  {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  0,
  NULL,
  0,
  {{0,0,0,0,0,0},{0,0,0,0,0,0},0},
  NULL,
  NULL,
  NULL,
  NULL
};
static _array_fnc _dummy_arr = &__dummy_arr;
static _region_nc glb_reg;
static int glb_firsttime;


void _InitEnsIO() {
  _PRIV_TID_DECL;

  _PRIV_ALLOC(int, _io_in_ens);
  _PRIV_ACCESS(_io_in_ens) = 0;

  _PRIV_ALLOC(unsigned int, globelemsize);
  _PRIV_ACCESS(globelemsize) = 0;
}


static void StreamElemBinOut(_zfile outfile,void *elem,char *control) {
  _PRIV_TID_DECL;
  fwrite(elem,_PRIV_ACCESS(globelemsize),1,outfile->fptr);
}


static void StreamElemBinIn(_zfile infile,void *elem,char *control) {
  _PRIV_TID_DECL;
  fread(elem,_PRIV_ACCESS(globelemsize),1,infile->fptr);
}


static void fzgetpos(_zfile stream) {
  long newpos;

  if (stream!=zout && stream!=zin && stream!=zerr) {
    newpos = ftell(stream->fptr);

    if (newpos != -1) {
      stream->pos = newpos;
    }
  }
}


static void fzsetpos(_zfile stream) {
  int status;

  if (stream!=zout && stream!=zin && stream!=zerr) {
    status = fseek(stream->fptr,stream->pos,SEEK_SET);

    if (status == -1) {
      stream->pos = 0;
    }
  }
}


static void _PassPosNextProc(_region reg,_zfile file,int felloff[]) {
  int i;
  int index;
  int loc[_MAXRANK];
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  
  if (_PROCESSORS != 1) {
    fzgetpos(file);

    for (i=0; i<_MAXDIM; i++) {
      loc[i] = _GRID_LOC(grid, i);
    }
    for (i=_MAXDIM-1; i>=0; i--) {
      if (felloff[i]) {
	if (_GRID_LOC(grid, i) == _MAX_PROC(reg,i)) {
	  loc[i] = _BEG_LOC(reg,i);
	} else {
	  if (_FLOODED_DIM(reg,i) == _DIM_RGRID) {
	    loc[i] = _GRID_LOC(grid, i)+1;
	  } else {
	    loc[i] = _DIST_TO_PROC(unknown, dist, _REG_MYHI(reg,i)+_REG_STRIDE(reg,i), i);
	  }
	  break;
	}
      }
    }
    index = _GRID_TO_PROC(grid, _MAXDIM,loc);
#ifdef PRINT_TOKENS
    printf("[%d] passing token to %d\n",_INDEX,index);
#endif
    _SendTokenWithValue(index,&(file->pos));
  }
}


static void _PassPosFirstProc(_region reg,_zfile file) {
  int numdims;
  int firstloc[_MAXRANK];
  int i;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);
  
  if (_PROCESSORS != 1) {
    fzgetpos(file);
    numdims = _NUMDIMS(reg);
    for (i=0; i<numdims; i++) {
      firstloc[i] = _MIN_PROC(reg,i);
    }
    _SendTokenWithValue(_GRID_TO_PROC(grid, numdims,firstloc),&(file->pos));
  }
}


static void _WaitForPos(_zfile file) {
  _PRIV_TID_DECL;
  if (_PROCESSORS != 1) {
    _RecvTokenWithValue(&(file->pos));
    fzsetpos(file);
  }
}


static void _UpdateFilePosition(_grid grid, _zfile file,int rootproc) {
  _PRIV_TID_DECL;
  if (_PROCESSORS!=1) {
    if (rootproc == _INDEX) {
      fzgetpos(file);
    }
    _BroadcastSimple(grid, &(file->pos),sizeof(file->pos),rootproc,-1);
    if (rootproc != _INDEX) {
      fzsetpos(file);
    } 
  } else {
    fzgetpos(file);
  }
}


static void _SetFilePosition(_zfile file,_region reg,unsigned int elemsize,
			     int _zindex[],int final) {
  int j;
  int index;
  int numdims;
  int mult;
  long basepos;

  basepos = file->pos;
  index = basepos;

  numdims = _NUMDIMS(reg);
  mult = elemsize;
  for (j=numdims-1; j >= 0; j--) {
    index += mult * (_zindex[j]-_REG_GLOB_LO(reg,j));
    mult *= _REG_GLOB_HI(reg,j)-_REG_GLOB_LO(reg,j)+1;
  }

  if (final) {
    index += elemsize;
    file->pos = index;
    fzsetpos(file);
  } else {
    file->pos = index;
    fzsetpos(file);
    file->pos = basepos;
  }

#ifdef NODEF /* echris */
  printf("(");
  for (j=0; j<numdims; j++) {
    if (j<numdims-1) 
      printf("%d,", _zindex[j]);
    else
      printf("%d)%d ", _zindex[j], index);
  }
#endif
}


static void _StreamArrayTxtSps(_zfile file,_region reg,_array ens,int write,
			       _filepointfn pfun,char* control,int print,
			       int waitfortoken,int _zindex[],int linefeed,
			       int lfdim,int numdims,int onewide[],
			       int lastproc) {
  int regwalker[_MAXRANK];
  int hit;
  int dim;
  int j;
  int stop;

  regwalker[0] = _SPS_ORIGIN(reg);

  while (print) {
    if (waitfortoken) {
#ifdef PRINT_TOKENS
      printf("[%d] waiting for token\n",_INDEX);
#endif
      fflush(stdout);
      _WaitForPos(file);
      waitfortoken = 0;
    }

    hit = 1;
    for (dim=0; dim<numdims-1 && hit; dim++) {
      while (_SPSNODE_INDEX(reg,regwalker[dim],dim) < _zindex[dim]) {
	regwalker[dim] = _SPSNODE_NEXT(reg,regwalker[dim],dim);
      }
      if (_SPSNODE_INDEX(reg,regwalker[dim],dim) == _zindex[dim]) {
	regwalker[dim+1] = regwalker[dim];
      } else {
	hit = 0;
      }
    }

    if (hit) {
      dim = numdims-1;
      stop = _SPSNODE_PREV(reg,regwalker[dim]+1,dim);
      regwalker[dim] = _SPSNODE_NEXT(reg,regwalker[dim],dim);
      while (_SPSNODE_INDEX(reg,regwalker[dim],dim) < _REG_MYLO(reg,dim)) {
	regwalker[dim]++;
      }
      for ( ; 
	    ((regwalker[dim] <= stop) &&
	     ((_zindex[dim] = _SPSNODE_INDEX(reg,regwalker[dim], dim)) <= _REG_MYHI(reg,dim)));
	    regwalker[dim]++) {
#ifdef PRINT_TOKENS
	printf("[%d] printing [%d,%d,%d]: ",_INDEX,_zindex[0],_zindex[1],_zindex[2]);
#endif
	pfun(file,_ARR_ACCESS(ens,_zindex), control);
#ifdef PRINT_TOKENS
	printf("\n");
#endif
      }
    }

    if (linefeed) {
      fprintf(file->fptr,"\n");
    }

    {
      int felloffsomething = 0;
      int felloff[_MAXRANK] = {0,0,0,0,0,0};
      
      if (!onewide[numdims-1]) {
	felloffsomething = 1;
	felloff[numdims-1] = 1;
      }

      for (j=numdims-2;j>=0;j--) {
	_zindex[j] += _REG_STRIDE(reg,j);
	if ((_zindex[j] <= _REG_MYHI(reg,j)) && (_FLOODED_DIM(reg,j) != _DIM_RGRID)) {
	  break;
	} else {
	  if (j != 0) {
	    _zindex[j] = _REG_MYLO(reg,j);
	  }
	  if (!onewide[j]) {
	    felloffsomething = 1;
	    felloff[j] = 1;
	  }
	}
      }
      if (felloffsomething) {
	if (write) {
	  fflush(file->fptr);
	}
	if (_zindex[0] <= _REG_MYHI(reg,0) || _INDEX != lastproc) {
	  _PassPosNextProc(reg,file,felloff);
	  waitfortoken = 1;
	}
      }
      if (_zindex[0] > _REG_MYHI(reg,0)) {
	print = 0;
      }
    }
  }
}


static void _StreamArrayTxtDns(_zfile file,_region reg,_array ens,int write,
			       _filepointfn pfun,char* control,int print,
			       int waitfortoken,int _zindex[],int linefeed,
			       int lfdim,int numdims,int onewide[],
			       int lastproc) {
  int j;

  while (print) {
    if (waitfortoken) {
#ifdef PRINT_TOKENS
      printf("[%d] waiting for token\n",_INDEX);
#endif
      fflush(stdout);
      _WaitForPos(file);
      waitfortoken = 0;
    }

#ifdef PRINT_TOKENS
    printf("[%d] printing [%d,%d,%d]: ",_INDEX,_zindex[0],_zindex[1],_zindex[2]);
#endif
    pfun(file,_ARR_ACCESS(ens,_zindex), control);
#ifdef PRINT_TOKENS
    printf("\n");
#endif

    if (linefeed && _zindex[lfdim]==_REG_MYHI(reg,lfdim)) {
      fprintf(file->fptr,"\n");
    }

    {
      int felloffsomething = 0;
      int felloff[_MAXRANK] = {0,0,0,0,0,0};

      for (j=numdims-1;j>=0;j--) {
	_zindex[j] += _REG_STRIDE(reg,j);
	if ((_zindex[j] <= _REG_MYHI(reg,j)) && (_FLOODED_DIM(reg,j) != _DIM_RGRID)) {
	  break;
	} else {
	  if (j != 0) {
	    _zindex[j] = _REG_MYLO(reg,j);
	  }
	  if (!onewide[j]) {
	    felloffsomething = 1;
	    felloff[j] = 1;
	  }
	}
      }
      if (felloffsomething) {
	if (write) {
	  fflush(file->fptr);
	}
	if (_zindex[0] <= _REG_MYHI(reg,0) || _INDEX != lastproc) {
	  _PassPosNextProc(reg,file,felloff);
	  waitfortoken = 1;
	}
      }
      if (_zindex[0] > _REG_MYHI(reg,0)) {
	print = 0;
      }
    }
  }
}


void _StreamEnsembleTxt(_zfile file,_region reg,_array ens,int write,
			_filepointfn pfun,char *control) {
  int j;
  int numdims;
  int print=1;
  int waitfortoken;
  int onewide[_MAXRANK];
  int lfdim;
  int linefeed;
  int botloc[_MAXRANK];
  int firstproc;
  int lastproc;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  numdims=_NUMDIMS(reg);
  firstproc = _GRID_TO_PROC(grid, numdims, _REG_LOPROCLOC(reg));
  lastproc = _GRID_TO_PROC(grid, numdims, _REG_HIPROCLOC(reg));

  lfdim = numdims-1;
  while (lfdim>0 && (_REG_GLOB_LO(reg,lfdim)==_REG_GLOB_HI(reg,lfdim) ||
		     _FLOODED_DIM(reg,lfdim)==_DIM_FLOODED)) {
    lfdim--;
  }

  linefeed = write && (_GRID_LOC(grid, lfdim) == _MAX_PROC(reg, lfdim) || 
		       _FLOODED_DIM(reg,lfdim)==_DIM_FLOODED) &&
    (pfun != StreamElemBinOut);

  _PRIV_ACCESS(_io_in_ens) = 1;
  if (write) {
    if (_ARR_W(ens) != NULL) {
      pfun = _ARR_W(ens);
    }
  } else {
    if (_ARR_R(ens) != NULL) {
      pfun = _ARR_R(ens);
    }
  }
  if (pfun==NULL) {
    _RT_ALL_FATAL0("Can't do I/O on an ensemble of non-scalars\n"
		   "without an associated function.");
  }

  for (j=0;j<numdims;j++) {
    if ((_REG_MYLO(reg,j)>_REG_MYHI(reg,j)) ||
	((_FLOODED_DIM(reg,j)==_DIM_FLOODED)&&
	 (_GRID_LOC(grid, j)!=_MIN_PROC(reg,j)))) {
      print=0;
    } else {
      _zindex[j]=_REG_MYLO(reg,j);
    }
  }

  _ResetToken();
  _UpdateFilePosition(grid, file, 0);

  waitfortoken = (_INDEX != 0);
  if (_INDEX == 0) {
    if (firstproc == _INDEX) {
      waitfortoken = 0;
    } else {
      _PassPosFirstProc(reg,file);
    }
  } else {
    waitfortoken = 1;
  }

  for (j=0; j<numdims; j++) {
    onewide[j] = ((_BEG_LOC(reg,j) == _END_LOC(reg,j)) || 
		  (_FLOODED_DIM(reg,j)==_DIM_FLOODED));
  }

  if (print) {
    if (_SPS_REGION(reg)) {
      _StreamArrayTxtSps(file,reg,ens,write,pfun,control,print,waitfortoken,
			 _zindex,linefeed,lfdim,numdims,onewide,lastproc);
    } else {
      _StreamArrayTxtDns(file,reg,ens,write,pfun,control,print,waitfortoken,
			 _zindex,linefeed,lfdim,numdims,onewide,lastproc);
    }
  }

#ifdef PRINT_TOKENS
  printf("[%d] DONE!!\n",_INDEX);
#endif
  if (write && _INDEX != lastproc) {
    fflush(file->fptr);
  }

  for (j=0; j<numdims; j++) {
    botloc[j] = _MAX_PROC(reg,j);
  }
  _UpdateFilePosition(grid, file,_GRID_TO_PROC(grid, numdims,botloc));

  _PRIV_ACCESS(_io_in_ens)=0;
  _BarrierSynch();
}


void _StreamEnsembleBin(_zfile file,_region reg,_array ens,int write,
			unsigned int elemsize) {
  int j;
  int numdims;
  int lo[_MAXRANK];
  int hi[_MAXRANK];
  size_t size;
  int stride;
  unsigned int origelemsize;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  if (file == zin || file == zout || file == zerr ||
      (write && _ARR_W(ens) != NULL) || (!write && _ARR_R(ens) != NULL) ||
      _SPS_REGION(reg)) {
    _PRIV_ACCESS(globelemsize) = elemsize;
    if (file == zin || (!write && (_ARR_R(ens) != NULL || _SPS_REGION(reg)))) {
      _StreamEnsembleTxt(file,reg,ens,write,StreamElemBinIn,NULL);
    } else {
      _StreamEnsembleTxt(file,reg,ens,write,StreamElemBinOut,NULL);
    }
    return;
  }
  
  origelemsize = elemsize;

  numdims = _NUMDIMS(reg);  

  _UpdateFilePosition(grid, file, 0);

  if (_REG_I_OWN(reg)) {

    for (j=0;j<numdims;j++) {
      lo[j] = _REG_MYLO(reg,j);
      hi[j] = _REG_MYHI(reg,j);
      _zindex[j] = lo[j];
    }

    for (j=numdims-1;j>=0;j--) {
      _zindex[j] += _REG_STRIDE(reg,j);
      stride = _GenAccessDistance(ens,lo,_zindex);
      _zindex[j] = _REG_MYLO(reg,j);
      if (stride == elemsize &&
	  (j == numdims-1 || 
	   ((_REG_MYLO(reg,j+1) == _REG_GLOB_LO(reg,j+1)) && 
	    (_REG_MYHI(reg,j+1) == _REG_GLOB_HI(reg,j+1))))) {
	elemsize *= _REG_NUMELMS(reg,j);
	hi[j] = lo[j];
      } else {
	break;
      }
    }

    /* this code does not contain the inner loop optimization */
    while (1) {
      _SetFilePosition(file,reg,origelemsize,_zindex,0);

      if (write) {
	size = fwrite(_GenAccess(ens,_zindex),elemsize,1,file->fptr);
	if (size == 0) {
	  _RT_ANY_WARN1("Error in writing binary data (%d)",errno);
	  break;
	}
      } else {
	size = fread(_GenAccess(ens,_zindex),elemsize,1,file->fptr);
	if (size == 0) {
	  _RT_ANY_WARN1("Error in reading binary data (%d)",errno);
	  break;
	}
      }
    
      _zindex[numdims-1]++;
      for (j = numdims-1;j>0;j--) {
	if (_zindex[j] > hi[j]) {
	  _zindex[j-1]++;
	  _zindex[j] = lo[j];
	} else {
	  break;
	}
      }
      if (_zindex[0] > hi[0]) {
	break;
      }
    }
  }
  /* leave the filepos just after the last element in the ensemble */
  for (j=0;j<numdims;j++) {
    _zindex[j] = _REG_GLOB_HI(reg,j);
  }
  _SetFilePosition(file,reg,origelemsize,_zindex,1);
}


static int* ReturnCoord(_array arr,_vector i) {
  return &(_zindex[0]);
}


static void StreamCoordOut(_zfile outfile,void* elem,char* control) {
  int i;

  fprintf(outfile->fptr," (%d",((int*)elem)[0]);
  for (i=1; i<glb_numdims; i++) {
    fprintf(outfile->fptr,",%d",((int*)elem)[i]);
  }
  fprintf(outfile->fptr,")");
}


void _StreamRegionTxt(_zfile file,_region_fnc reg,int write,char *control) {
  int numdims;
  int i;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  numdims = _NUMDIMS(reg);
  if (write) {
    if (_INDEX == 0) {
      fprintf(file->fptr,"[");
      for (i=0;i<numdims;i++) {
	if (i) {
	  fprintf(file->fptr,",");
	}
	fprintf(file->fptr,"<");
	fprintf(file->fptr,control,_REG_LO(reg,i));
	fprintf(file->fptr,",");
	fprintf(file->fptr,control,_REG_HI(reg,i));
	fprintf(file->fptr,",");
	fprintf(file->fptr,control,_REG_STRIDE(reg,i));
	fprintf(file->fptr,",");
	fprintf(file->fptr,control,_REG_ALIGN(reg,i));
	fprintf(file->fptr,">");
      }
      fprintf(file->fptr,"]");
    }
    _UpdateFilePosition(grid,file,0);
    if (_SPS_REGION(reg)) {
      if (_INDEX == 0) {
	fprintf(file->fptr,":");
	fflush(file->fptr);
      }
      glb_numdims = numdims;
      _ARR_SET_ACCESSOR(_dummy_arr,ReturnCoord);
      _StreamEnsembleTxt(file,reg,_dummy_arr,1,StreamCoordOut,control);
      if (_INDEX == 0) {
	_vector lowcoord;
	for (i=0; i<numdims; i++) {
	  lowcoord[i] = _REG_MYLO(reg,i)-1;
	}
	StreamCoordOut(file,lowcoord,control);
      }
      _UpdateFilePosition(grid,file,0);
    }
  } else {
    if (_INDEX == 0) {
      fscanf0(file->fptr,"[");
      for (i=0;i<numdims;i++) {
	if (i) {
	  fscanf0(file->fptr,",");
	}
	fscanf0(file->fptr,"<");
	fscanf(file->fptr,control,&(_REG_LO(reg,i)));
	fscanf0(file->fptr,",");
	fscanf(file->fptr,control,&(_REG_HI(reg,i)));
	fscanf0(file->fptr,",");
	fscanf(file->fptr,control,&(_REG_STRIDE(reg,i)));
	fscanf0(file->fptr,",");
	fscanf(file->fptr,control,&(_REG_ALIGN(reg,i)));
	fscanf0(file->fptr,">");
	_BroadcastSimple(grid, &(_REG_LO(reg,i)),sizeof(_REG_LO(reg,i)),0,0);
	_BroadcastSimple(grid, &(_REG_HI(reg,i)),sizeof(_REG_HI(reg,i)),0,0);
	_BroadcastSimple(grid, &(_REG_STRIDE(reg,i)),sizeof(_REG_STRIDE(reg,i)),0,0);
	_BroadcastSimple(grid, &(_REG_ALIGN(reg,i)),sizeof(_REG_ALIGN(reg,i)),0,0);
	_FLOODED_DIM(reg,i)=_DIM_NORMAL;
	_SET_GB_GLOB(reg,i);
      }
      fscanf0(file->fptr,"]");
    } else {
      for (i=0;i<numdims;i++) {
	_BroadcastSimple(grid, &(_REG_LO(reg,i)),sizeof(_REG_LO(reg,i)),0,0);
	_BroadcastSimple(grid, &(_REG_HI(reg,i)),sizeof(_REG_HI(reg,i)),0,0);
	_BroadcastSimple(grid, &(_REG_STRIDE(reg,i)),sizeof(_REG_STRIDE(reg,i)),0,0);
	_BroadcastSimple(grid, &(_REG_ALIGN(reg,i)),sizeof(_REG_ALIGN(reg,i)),0,0);
	_FLOODED_DIM(reg,i)=_DIM_NORMAL;
	_SET_GB_GLOB(reg,i);
      }
    }
    _UpdateFilePosition(grid,file,0);
    _REG_SET_SETUP_ON(reg);
    _InitRegion(reg, 0);
  }
  _BarrierSynch();
}


static void* Return2DHit(_array arr,_vector i) {
  static int regwalker[2];
  static int regstop;
  int numdims = 2;
  int hit;
  int dim;

  if (glb_firsttime == 1) {
    glb_firsttime = 0;
    regwalker[0] = _SPS_CORNER(glb_reg,0);
  }

  hit = 1;
  /*
  printf("\n[%d] looking for [%d,%d]\n",_INDEX,_zindex[0],_zindex[1]);
  printf("currently at [%d,%d], [%d,%d]\n",
	 _SPSNODE_INDEX(glb_reg,regwalker[0],0),
	 _SPSNODE_INDEX(glb_reg,regwalker[0],1),
	 _SPSNODE_INDEX(glb_reg,regwalker[1],0),
	 _SPSNODE_INDEX(glb_reg,regwalker[1],1));
  */
  for (dim=0; dim<numdims && hit; dim++) {
    if (_SPSNODE_INDEX(glb_reg,regwalker[dim],dim) != _zindex[dim]) {
      while (_SPSNODE_INDEX(glb_reg,regwalker[dim],dim) < _zindex[dim] && hit) {
	if (dim == numdims-1) {
	  if (regwalker[dim] == regstop) {
	    hit = 0;
	  } else {
	    regwalker[dim]++;
	  }
	} else {
	  regwalker[dim] = _SPSNODE_NEXT(glb_reg,regwalker[dim],dim);
	}
	/*
	printf("  now at [%d,%d]/[%d,%d] (%d)\n",
	       _SPSNODE_INDEX(glb_reg,regwalker[0],0),
	       _SPSNODE_INDEX(glb_reg,regwalker[0],1),
	       _SPSNODE_INDEX(glb_reg,regwalker[1],0),
	       _SPSNODE_INDEX(glb_reg,regwalker[1],1),
	       dim);
	*/
      }
      if (dim != numdims-1) {
	regwalker[dim+1] = _SPSNODE_NEXT(glb_reg,regwalker[dim],dim+1);
	if (dim == numdims-2) {
	  regstop = _SPSNODE_PREV(glb_reg,regwalker[dim]+1,dim+1);
	}
	/*	
	printf(" copying from %d to %d: [%d,%d]\n",dim,dim+1,
	       _SPSNODE_INDEX(glb_reg,regwalker[dim+1],0),
	       _SPSNODE_INDEX(glb_reg,regwalker[dim+1],1));
	*/
      }
      if (_SPSNODE_INDEX(glb_reg,regwalker[dim],dim) != _zindex[dim]) {
	hit = 0;
      }
    }
  }
  if (hit) {
    return _VOIDPTR_HIT;
  } else {
    return _VOIDPTR_MIS;
  }
}


static void StreamPBMOut(_zfile outfile,void* elem,char* control) {
  if (elem == _VOIDPTR_HIT) {
    fprintf(outfile->fptr,"1");
  } else {
    fprintf(outfile->fptr,"0");
  }
}


void _DrawRegionSparsity(_zfile file, _region_fnc reg) {
  int i;
  int j;
  _distribution dist = _REG_DIST(reg);
  _grid grid = _DIST_GRID(dist);

  if (_NUMDIMS(reg) != 2) {
    _RT_ALL_FATAL0("Currently can't draw the sparsity of anything other"
		   " than 2D regions");
  }
  if (_INDEX == 0) {
    fprintf(file->fptr,"P1\n");
    fprintf(file->fptr,"%d %d\n",_REG_GLB_NUMELMS(reg,0),
	    _REG_GLB_NUMELMS(reg,1));
  }
  _UpdateFilePosition(grid, file,0);
  if (_SPS_REGION(reg)) {
    _ARR_SET_ACCESSOR(_dummy_arr,Return2DHit);
    _SPS_REGION(reg) = 0;  /* pretend it's not sparse for the time being */
    glb_reg = reg;
    glb_firsttime = 1;
    _StreamEnsembleTxt(file,reg,_dummy_arr,1,StreamPBMOut,NULL);
    _SPS_REGION(reg) = 1;  /* we're done pretending */
  } else {
    if (_INDEX == 0) {
      for (i=0;i<_REG_GLB_NUMELMS(reg,0);i++) {
	for (j=0;j<_REG_GLB_NUMELMS(reg,1);i++) {
	  fprintf(file->fptr,"1");
	}
	fprintf(file->fptr,"\n");
      }
    }
    _UpdateFilePosition(grid, file,0);
  }
}
