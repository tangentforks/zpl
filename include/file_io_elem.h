/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __FILE_IO_ELEM_H_
#define __FILE_IO_ELEM_H_

#include "file_io_base.h"
#include "quad.h"

/* default fprintf/fscanf control strings */

#define _Z_W_bool   "%5s "
#define _Z_W_char   "%c"
#define _Z_W_sbyte  "%d "
#define _Z_W_ubyte  "%u "
#define _Z_W_short  "%9hd "
#define _Z_W_ushort "%9hu "
#define _Z_W_int    "%9d "
#define _Z_W_uint   "%9u "
#define _Z_W_long   "%9ld "
#define _Z_W_ulong  "%9lu "
#define _Z_W_float  "%9.4f "
#define _Z_W_double "%9.4f "
#define _Z_W_string "%s "
#define _Z_W_record "<don't know how to print records>"
#define _Z_W_array  "<don't know how to print arrays>"
#define _Z_W_fcomplex "%9.4f + %9.4fi"
#define _Z_W_dcomplex "%9.4f + %9.4fi"
#define _Z_W_reg     "%d"

#define _Z_R_bool   "%5s"
#define _Z_R_char   "%c"
#define _Z_R_sbyte  "%d"
#define _Z_R_ubyte  "%u"
#define _Z_R_short  "%hd"
#define _Z_R_ushort "%hu"
#define _Z_R_int    "%d"
#define _Z_R_uint   "%u"
#define _Z_R_long   "%ld"
#define _Z_R_ulong  "%lu"
#define _Z_R_float  "%f"
#define _Z_R_double "%lf"
#define _Z_R_string "%s"
#define _Z_R_record ""
#define _Z_R_array  ""
#define _Z_R_fcomplex "%f + %fi"
#define _Z_R_dcomplex "%lf + %lfi"
#define _Z_R_reg     "%d"

#define _StreamOut_sbyte _StreamOut_char
#define _StreamOut_ubyte _StreamOut_uchar
#define _StreamOut_ushort _StreamOut_short
#define _StreamOut_uint _StreamOut_int
#define _StreamOut_ulong _StreamOut_long
#define _StreamOut_record NULL
#define _StreamOut_array  NULL

#define _StreamIn_sbyte _StreamIn_char
#define _StreamIn_ubyte _StreamIn_uchar
#define _StreamIn_ushort _StreamIn_short
#define _StreamIn_uint _StreamIn_int
#define _StreamIn_ulong _StreamIn_long
#define _StreamIn_record NULL
#define _StreamIn_array  NULL

#endif
