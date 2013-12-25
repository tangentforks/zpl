/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _X86_LINUX_MPI_MD_ZINC_H_
#define _X86_LINUX_MPI_MD_ZINC_H_

#define restrict __restrict__

#define _QUAD_SUPPORTED

#ifdef stdout
#undef stdout
#endif

#ifdef stdin
#undef stdin
#endif

#ifdef stderr
#undef stderr
#endif

#include "../md_zinc.h"

#endif
