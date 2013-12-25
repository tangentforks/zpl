/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ZC_PROTO_H_
#define __ZC_PROTO_H_

/* This file prototypes things that the zc compiler generates */

/* .c prototypes */

void _zplmain(void);


/* _cfg.c prototypes */

void _AssignToConfig(char *,char *);
void _InitExprConfigs(void);
void _PrintConfigs(void);


/* dir.c prototypes */

void _InitAllDirections(void);

/* _ens.c prototypes */

void _InitGlobalEnsembles(void);
void _DestroyGlobalEnsembles(void);


/* _reg.c prototypes */

void _InitAllRegions(void);

#define _null_array NULL
#define _null_region NULL
#define _null_distribution NULL

#endif
