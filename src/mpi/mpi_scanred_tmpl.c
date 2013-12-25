/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"
#include "mpi.h"
#include "mpi_scanred.h"
#include "mpi_zlib.h"

#define _SR(t)    _Scan##t
#define SR(t)     _SR(t)
#define SCAN_ROUT SR(TYPE)

#define _RR(t)   _Reduce##t
#define RR(t)    _RR(t)
#define RED_ROUT RR(TYPE)

#define _MT__zquad   MPI_LONG_DOUBLE
#define _MT_double   MPI_DOUBLE
#define _MT_float    MPI_FLOAT
#define _MT_int      MPI_INT
#define _MT_long     MPI_LONG
#define _MT__uint    MPI_UNSIGNED
#define _MT__ulong   MPI_UNSIGNED_LONG
#define _MT_fcomplex _mpi_fcomplex
#define _MT_dcomplex _mpi_dcomplex
#define _MT_qcomplex _mpi_qcomplex

#define _MPI_TYPE(t) _MT_##t
#define MPI_TYPE(t)  _MPI_TYPE(t)

#define _OPVECT(t) _##t##Ops
#define OPVECT(t) _OPVECT(t)

extern MPI_Comm MPI_COMM_ROW;
extern MPI_Comm MPI_COMM_COL;


#ifndef COMPLEX_TYPE
static MPI_Op opvect[] = {(MPI_Op)0,MPI_SUM,MPI_PROD,MPI_MAX,MPI_MIN,MPI_LAND,
			    MPI_LOR,MPI_BAND,MPI_BOR,MPI_BXOR};
#else
#define opvect OPVECT(TYPE)
#endif


void SCAN_ROUT(TYPE input[], TYPE output[], _region R, int dim, int numelms,
		   int dir,int op,TYPE ident) {
  int i,recv_proc,send_proc;
  int off[_MAXRANK] = {0,0,0,0,0,0};
  int read=0,combine=0,write=0,init=0;
  int loc[_MAXRANK];
  _distribution dist = _REG_DIST(R);
  _grid grid = _DIST_GRID(dist);

  if (_REG_I_OWN(R)) {
    
    if (dir != 1 && dir != -1) {
      printf("Whoops: brad dir mis-assumption\n");
      return;
    }

    off[dim] = 1 * dir;
    
    for (i=0; i<_MAXDIM; i++) {
      loc[i] = _GRID_LOC(grid, i)-off[i];
    }
    recv_proc = _GRID_TO_PROC(grid, _MAXDIM,loc);

    for (i=0; i<_MAXDIM; i++) {
      loc[i] = _GRID_LOC(grid, i)+off[i];
    }
    send_proc = _GRID_TO_PROC(grid, _MAXDIM,loc);
    
    if (!_DIM_DIST(R,dim) || (_FLOODED_DIM(R,dim) == _DIM_FLOODED)) {
      init=1;
    }
    else if (_FLOODED_DIM(R,dim) == _DIM_RGRID) {
      if (_GRID_LOC(grid, dim) == _GRID_SIZE(grid, dim)-1) {
	if (dir >= 0) {
	  read=1; combine=1;
	}
	else {
	  write=1; init=1;
	}
      }
      else if (_GRID_LOC(grid, dim) == 0) {
	if (dir >= 0) {
	  write=1; init=1;
	}
	else {
	  read=1; combine=1;
	}
      }
      else {
	write=1; read=1; combine=1;
      }
    }
    else if (_REG_MYLO(R, dim) == _REG_GLOB_LO(R, dim)) {
      if (dir>=0) {
	write=1;
	init=1;
      } else {
	read=1;
	combine=1;
      }
    } else if (_REG_MYHI(R, dim) == _REG_GLOB_HI(R, dim)) {
      if (dir>=0) {
	read=1;
	combine=1;
      } else {
	write=1;
	init=1;
      }
    } else {
      read=1;
      combine=1;
      write=1;
    }
  
    if (read) {
      _ReadPort(recv_proc, _PORT_SCAN, _WPROC_COMMID(100, recv_proc),
		output, numelms*sizeof(TYPE));
    }
    if (combine) {
      for (i=0; i<numelms; i++) {
	DO_OP(op,input[i],output[i])
      }
    }
    if (write) {
      _WritePort(send_proc, _PORT_SCAN, _WPROC_COMMID(100, _INDEX), input,
		 numelms*sizeof(TYPE));
    }
    if (init) {
      for (i=0; i<numelms; i++) {
	output[i] = ident;
      }
    }
  }
}



void RED_ROUT(_grid grid, TYPE* data,int numelms,int op,int redtype,int destproc,
	      int bcast) {

  MPI_Comm communicator;
#ifndef _NO_COPY_REQUIRED
  TYPE* data_mirror = (TYPE*)_zmalloc(numelms*sizeof(TYPE),
					"mpi reduction copybuff");
  int i;

  for (i=0; i<numelms; i++) {
    data_mirror[i] = data[i];
  }
#define data_in data_mirror
#else
#define data_in data
#endif

  if (_GRID_SLICE(grid, redtype) == MPI_COMM_NULL) {
    int color;
    int test;

    color = _GetSliceColor(grid, redtype);
    MPI_Comm_split(_GRID_WORLD(grid), color, _MyLocInSlice(grid, redtype), &(_GRID_SLICE(grid, redtype)));
    MPI_Comm_rank(_GRID_SLICE(grid, redtype), &test);
    if (test != _MyLocInSlice(grid, redtype)) {
      fprintf(stderr,"[%d/%d] pos didn't match in slice (%d != %d)\n",
	      _INDEX,_PROCESSORS,test,_MyLocInSlice(grid, redtype));
    }
  }
  communicator = _GRID_SLICE(grid, redtype);
  if (bcast) {
    MPI_Allreduce((void *)data_in,(void *)data,numelms,MPI_TYPE(TYPE),
		  opvect[op],communicator);
  } else {
    MPI_Reduce((void *)data_in,(void *)data,numelms,MPI_TYPE(TYPE),opvect[op],
	       destproc,communicator);
  }

#ifndef _NO_COPY_REQUIRED
  _zfree(data_mirror,"mpi reduction copybuff");
#endif
}
