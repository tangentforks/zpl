/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _X86_LINUX_SEQ_MD_ZINC_H_
#define _X86_LINUX_SEQ_MD_ZINC_H_

#define restrict __restrict__

#define _QUAD_SUPPORTED
#include "../md_zinc.h"

#ifdef stdin
#undef stdin
#endif

#ifdef stdout
#undef stdout
#endif

#ifdef stderr
#undef stderr
#endif

#endif
