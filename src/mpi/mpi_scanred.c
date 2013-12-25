/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"
#include "mpi.h"
#define _DEFINE_MPI_SR
#include "mpi_scanred.h"
#include "mpi_zlib.h"

static void (*mpi_scanred_userfn)(void*,void*);
static int data_multiple;
static MPI_Op userfnop;

static void AddFComplex(void *in,void *inout,int *len,MPI_Datatype *dptr) {
  int i;

  for (i=0;i<*len;i++) {
    _ADD_STMT_fcomplex((*(fcomplex *)inout),(*(fcomplex *)in));
    in = ((fcomplex*)in)+1;
    inout = ((fcomplex*)inout)+1;
  }
}


static void AddDComplex(void *in,void *inout,int *len,MPI_Datatype *dptr) {
  int i;

  for (i=0;i<*len;i++) {
    _ADD_STMT_dcomplex((*(dcomplex *)inout),(*(dcomplex *)in));
    in = ((dcomplex*)in)+1;
    inout = ((dcomplex*)inout)+1;
  }
}


static void AddQComplex(void *in,void *inout,int *len,MPI_Datatype *dptr) {
  int i;

  for (i=0;i<*len;i++) {
    _ADD_STMT_qcomplex((*(qcomplex *)inout),(*(qcomplex *)in));
    in = ((qcomplex*)in)+1;
    inout = ((qcomplex*)inout)+1;
  }
}


static void MultFComplex(void *in,void *inout,int *len,MPI_Datatype *dptr) {
  int i;

  for (i=0;i<*len;i++) {
    _MULT_STMT_fcomplex((*(fcomplex *)inout),(*(fcomplex *)in));
    in = ((fcomplex*)in)+1;
    inout = ((fcomplex*)inout)+1;
  }
}


static void MultDComplex(void *in,void *inout,int *len,MPI_Datatype *dptr) {
  int i;

  for (i=0;i<*len;i++) {
    _MULT_STMT_dcomplex((*(dcomplex *)inout),(*(dcomplex *)in));
    in = ((dcomplex*)in)+1;
    inout = ((dcomplex*)inout)+1;
  }
}


static void MultQComplex(void *in,void *inout,int *len,MPI_Datatype *dptr) {
  int i;

  for (i=0;i<*len;i++) {
    _MULT_STMT_qcomplex((*(qcomplex *)inout),(*(qcomplex *)in));
    in = ((qcomplex*)in)+1;
    inout = ((qcomplex*)inout)+1;
  }
}

static void MPIUserFunc(void* in, void* inout, int* len, MPI_Datatype* dptr) {
  int honest_len;

  honest_len = *len / data_multiple;
  if (*len % data_multiple != 0) {
    _RT_ALL_FATAL0("mpi user reduction error - data not fully delivered to user function");
    exit(1);
  }
  if (honest_len != 1) {
    _RT_ALL_FATAL0("mpi user reduction error - arrays not handled");
    exit(1);
  }
  mpi_scanred_userfn(in, inout);
}

void _InitScanRed() {
  MPI_Type_contiguous(2,MPI_FLOAT,&_mpi_fcomplex);
  MPI_Type_commit(&_mpi_fcomplex);
  MPI_Op_create(AddFComplex,1,&_fcomplexOps[_OP_ADD]);
  MPI_Op_create(MultFComplex,1,&_fcomplexOps[_OP_MULT]);

  MPI_Type_contiguous(2,MPI_DOUBLE,&_mpi_dcomplex);
  MPI_Type_commit(&_mpi_dcomplex);
  MPI_Op_create(AddDComplex,1,&_dcomplexOps[_OP_ADD]);
  MPI_Op_create(MultDComplex,1,&_dcomplexOps[_OP_MULT]);

  MPI_Type_contiguous(2,MPI_LONG_DOUBLE,&_mpi_qcomplex);
  MPI_Type_commit(&_mpi_qcomplex);
  MPI_Op_create(AddQComplex,1,&_qcomplexOps[_OP_ADD]);
  MPI_Op_create(MultQComplex,1,&_qcomplexOps[_OP_MULT]);

  MPI_Op_create(MPIUserFunc,1,&userfnop);
}

void _Reduceuser(_grid grid, void* data,int numelms,int redtype,int destproc, int bcast,
                 void (*userfn)(void*,void*), size_t size) {

  MPI_Comm communicator;
#ifndef _NO_COPY_REQUIRED
  char* data_mirror;
  int i;
#define data_in data_mirror
#else
#define data_in data
#endif

  data_multiple = size / sizeof(char);
  if (size % sizeof(char) != 0) {
    _RT_ALL_FATAL0("mpi user reduction error - datatype size must be multiple of char size");
    exit(1);
  }
  mpi_scanred_userfn = userfn;

#ifndef _NO_COPY_REQUIRED
  data_mirror = (char*)_zmalloc(data_multiple*numelms*sizeof(char), "mpi reduction copybuff");
  for (i = 0; i < data_multiple*numelms; i++) {
	data_mirror[i] = ((char*)data)[i];
  }
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
    MPI_Allreduce((void *)data_in,(void *)data,data_multiple*numelms,MPI_CHAR,
		  userfnop,communicator);
  } else {
    MPI_Reduce((void *)data_in,(void *)data,data_multiple*numelms,MPI_CHAR,userfnop,
	       destproc,communicator);
  }

#ifndef _NO_COPY_REQUIRED
  /*  _zfree(data_mirror,"mpi reduction copybuff");*/
#endif
}

/** inserted to deal with mac os x link bug in which zmath.o is not included even though we use some variables from it; we need to use a function **/
void __never_called_macosxbugquestion(void) {
  bpop(2);
}
