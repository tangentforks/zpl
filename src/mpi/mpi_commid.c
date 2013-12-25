/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <mpi.h>
#include "md_zinc.h"
#include "mpi_commid.h"
#include "mpi_zlib.h"
#include "grid.h"

static int dyncommid = _NUM_RESERVED_IDS;
static int maxcommid = 32767;

void _InitCommID() {
  int *tagub;
  int flag;

  MPI_Attr_get(_GRID_WORLD(_DefaultGrid),MPI_TAG_UB,&tagub,&flag);
  if (flag) {
    maxcommid = *tagub;
  }
}


int _GetDynamicCommID(void) {
  dyncommid++;
  if (dyncommid >= maxcommid) {
    dyncommid = _NUM_RESERVED_IDS;
  }
  return dyncommid;
}


/* FUNCTION: _SetDynamicCommID - set dynamic comm ID to dcid and return
 *           old value
 */

int _SetDynamicCommID(int dcid) {
  int tmp = dyncommid;

  dyncommid = dcid;

  return (tmp);
}
