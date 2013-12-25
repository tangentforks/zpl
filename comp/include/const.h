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





#ifndef CONST_H
#define CONST_H

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif
#ifndef MAYBE
#define	MAYBE		-1
#endif

#define	MAXBUFF		4096
#define	MAXHASH		1001
#define	MAXLEVEL	128
#define	MAXEXPO		5	
#define	MAXITER		4		
#define	MAXIDENT	26	
#define	FPPMAXTOK	5000	

/* Maximum # of dimensions in a region. */
#define MAXRANK		6
#define GRIDDIMS        2


#define F77		0	
#define KNRC		1	

#endif
