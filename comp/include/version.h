/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef VERSION_H
#define VERSION_H

#include <stdio.h>

#ifndef RELEASE_VER
#define RELEASE_VER 1
#endif

/* see version_num.h for version number stuff */

#define COPYRIGHT "Copyright (c) 1996 - 2004 -- University of Washington\n"

#define BANNER "\
ZPL Research Group\n\
Department of Computer Science and Engineering\n\
University of Washington\n\
Box 352350\n\
Seattle, Washington 98195-2350  USA\n\
zpl-info@cs.washington.edu\n"

void version_fprint(FILE*);
char* version_string(void);

#endif
