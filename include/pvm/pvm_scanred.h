/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __T3D_SCANRED_H_
#define __T3D_SCANRED_H_

#ifndef DEFINE_T3D_SR
#define DEFINE_T3D_SR extern
#endif

void _InitScanRed(void);

#define _PVM_TYPE_float    PVM_FLOAT
#define _PVM_TYPE_double   PVM_DOUBLE
#define _PVM_TYPE_int      PVM_INT
#define _PVM_TYPE_long     PVM_LONG
#define _PVM_TYPE__uint    PVM_INT
#define _PVM_TYPE__ulong   PVM_LONG
#define _PVM_TYPE_fcomplex PVM_FLOAT
#define _PVM_TYPE_dcomplex PVM_DOUBLE

/* hack! cough! wheeze!  -- copied from comm.h before it disappeared */
#define _PORT_SCAN  9
#define _WPROC_COMMID(x, p) (((p) << 8) | (x))

#endif
