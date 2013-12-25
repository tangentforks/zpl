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



#ifndef CGLOBAL_H
#define CGLOBAL_H

#include "struct.h"
#include "symtab.h"

extern int enum_value;
extern symboltable_t *yystmtlab;
extern symboltable_t *currlabel;
extern symboltable_t *last_ident;

extern datatype_t *pdtVOID;
extern datatype_t *pdtLONG, *pdtSHORT, *pdtDOUBLE, *pdtQUAD, *pdtUNSIGNED_INT;
extern datatype_t *pdtGENERIC, *pdtGENERIC_ENSEMBLE;

#endif
