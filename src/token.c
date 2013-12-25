/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>

#include "md_zinc.h"
#include "md_zlib.h"
#include "stand_inc.h"
#include "zlib.h"


/* send the print token to the specified processor */

void _SendToken(int next) {
  long junkval;

  _SendTokenWithValue(next,&junkval);
}


/* receive the token.  Block until someone sends it to us, if it hasn't
   already been sent. */

void _RecvToken() {
  long unused;

  _RecvTokenWithValue(&unused);
}


void _PassToken() {
  _PRIV_TID_DECL;
  _SendToken(_NEXTPROC);
}

void _PassTokenSameSlice(_grid grid, int slicenum) {
  _PRIV_TID_DECL;
  _SendToken(_NextProcSameSlice(grid, slicenum));
}

