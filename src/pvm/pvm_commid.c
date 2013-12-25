/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "md_zlib.h"
#include "pvm_zlib.h"

static int dyncommid = _NUM_RESERVED_IDS;


void _InitCommID() {
}


int _GetDynamicCommID(void) {
  dyncommid++;
  if (dyncommid < 0) {
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
