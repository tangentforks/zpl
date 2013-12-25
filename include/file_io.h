/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __FILE_IO_H_
#define __FILE_IO_H_

#include <stdio.h>
#include "region.h"
#include "ensemble.h"
#include "printfun.h"
#include "md_zinc.h"
#include "priv_access.h"

#include "file_io_base.h"
#include "file_io_elem.h"
#include "file_io_scalar.h"

extern _zfile zout;
extern _zfile zin;
extern _zfile zerr;
extern _zfile znull;

#define _ZIO_R 0
#define _ZIO_W 1

#define file_read  "r"
#define file_write "w"

_PRIV_DECL(extern, int, _io_in_ens);

#endif
