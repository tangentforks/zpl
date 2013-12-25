/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MD_ECC_H_
#define __MD_ECC_H_
#define ECC_COMMINFO

typedef struct {
  MPI_Request wid[MAXNODES][MAXSTEPS+1]; 
} ECC_CommInfo;

#endif
