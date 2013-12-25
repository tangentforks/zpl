/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***************************************************************************
	Center for Supercomputing Research and Development
	at the University of Illinois at Urbana-Champaign
		305 Talbot Laboratory
		104 South Wright Street
		Urbana, Illinois  61801-2932 USA

Write to the above address for information on the CSRD Software Catalog,
or send e-mail to:  softcat@csrd.uiuc.edu

Copyright (c) 1991 by the Board of Trustees of the University of Illinois
*unpublished*.  All rights reserved.  This proprietary created material is
provided under a license agreement which restricts its use and protects its
confidentiality.  No publication or waiver of confidentiality is implied by
this transfer under the license.
***************************************************************************/


#ifndef	SETMAC_H
#define	SETMAC_H

#include "struct.h"

#define SET_TYPE(x)		((x)->type)
#define SET_DIR(x)		((x)->dir)
#define SET_DIR_IDENT(x)	(DEP_DIR_IDENT(SET_DIR(x)))
#define SET_AT_DIR(x)		(DEP_AT_DIR(SET_DIR(x)))
#define SET_VAR(x)		((x)->var)
#define SET_NEXT(x)		((x)->next_s)
#define SET_IMOD(x)     	((x)->imod)
#define SET_EXPRS(x)		((x)->exprs)

#endif
