/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "md_zinc.h"
#include "pvm3.h"
#include "pvm_zlib.h"
#include "stand_inc.h"
#include "ironman_utils.h"


extern int *_tids;
extern int *_rowtids;
extern int *_coltids;


void _BroadcastSimpleToSlice(_grid grid, void *ptr,unsigned int size,int involved,
			     int rootproc,int commid,int slicenum) {
  int dyn_commid;

  if (_GRID_DISTRIBUTED(grid, slicenum)) {
    dyn_commid = _GetDynamicCommID();
    if (involved) {
      if (_MyLocInSlice(grid, slicenum) == rootproc) {
	pvm_initsend(PvmDataRaw);
	pvm_pkbyte((char *)ptr,size,1);
	pvm_bcast(_GetSliceName(grid, slicenum,_groupname),dyn_commid);
      } else {
	pvm_recv(-1,dyn_commid);
	pvm_upkbyte((char *)ptr,size,1);
      }
    }
  }
}


void _BroadcastSimple(_grid grid, void *ptr,unsigned int size,int rootproc,int commid) {
  _BroadcastSimpleToSlice(grid, ptr,size,1,rootproc,commid,0);
}
