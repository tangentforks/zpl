/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MPI_SCANRED_H_
#define __MPI_SCANRED_H_

#include "mpi.h"

#ifndef _DEFINE_MPI_SR
#define _DEFINE_MPI_SR extern
#endif

_DEFINE_MPI_SR MPI_Datatype _mpi_fcomplex;
_DEFINE_MPI_SR MPI_Datatype _mpi_dcomplex;
_DEFINE_MPI_SR MPI_Datatype _mpi_qcomplex;

_DEFINE_MPI_SR MPI_Op _fcomplexOps[3];
_DEFINE_MPI_SR MPI_Op _dcomplexOps[3];
_DEFINE_MPI_SR MPI_Op _qcomplexOps[3];

/* hack! cough! wheeze!  -- copied from comm.h before it disappeared */
#define _PORT_SCAN  9
#define _WPROC_COMMID(x, p) (((p) << 8) | (x))

#endif
