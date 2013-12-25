/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __SEQ_MD_IRONMAN_H_
#define __SEQ_MD_IRONMAN_H_

typedef int IM_info;

#define IM_Initialize(v,w,x,y,z)
#define IM_New(g,x,y) (IM_info *)((x!=0)?_RT_ANY_FATAL0("Trying to do communication in sequential version -- contact zpl-info@cs.washington.edu"),NULL:NULL)
#define IM_SR(g,x)
#define IM_DR(g,x)
#define IM_SV(g,x)
#define IM_DN(g,x)
#define IM_Old(x)

#define _MD_GRID_INFO

#define _MD_DISTRIBUTION_INFO

#define _MD_REGION_INFO

#define _MD_ENSEMBLE_INFO


#endif
