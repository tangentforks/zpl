/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdlib.h>

#include "pvm3.h"

#include "md_zlib.h"
#include "pvm_zlib.h"

static int ptid;


void _InitHalt() {
  ptid = pvm_parent();
}


/* Halt all processes in this applications */

void _ZPL_halt(int linenum) {
  if (ptid == PvmNoParent) {
    exit(1);
  } else {
    pvm_initsend(PvmDataRaw);
    pvm_pkint(&linenum,1,1);
    pvm_send(ptid,_HALTTAG);
    while (1) {
    }
  }
}

