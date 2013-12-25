/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MD_ZINC_H_
#define __MD_ZINC_H_

#define _MD_SYM_ALLOC

#define FALSE 0
#define TRUE  1

#define _RedBcastSimple(a,b,c,d)
#define _RedBcastRow(a,b,c,d,e)
#define _RedBcastCol(a,b,c,d,e)
#define _RedBcastUnknown(a,b,c,d,e,f)
#define _MD_REDBCAST_DEFINED

/* stub this; shmem doesn't use dynamic commids */
#define _SetDynamicCommID(x)

#endif
