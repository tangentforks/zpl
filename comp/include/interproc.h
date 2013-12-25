/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __INTERPROC_H_
#define __INTERPROC_H_

#include "glist.h"
#include "struct.h"

/* definitions */
#define IPUP 0
#define IPDN 1

/* prototypes */
int ip_analyze(module_t *mod, 
	       int dir, 
	       void (*usr_init_wl)(module_t *mod),
	       int (*ip_fn)(function_t *fn, glist funls_p),
	       void (*wl_fn)(function_t *fn));

int ip_wl_append(function_t *fn, function_t *force_fn);
void add_callers(function_t *fn);
void add_callees(function_t *fn);

#endif /* __INTERPROC_H_ */

