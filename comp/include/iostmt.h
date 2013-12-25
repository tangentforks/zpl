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

#ifndef IOSTMT_H
#define IOSTMT_H

#include "../include/struct.h"

typedef enum	{
	IO_READ,
	IO_WRITE,
	IO_WRITELN,
	IO_BREAD,
	IO_BWRITE,
	IO_HALT,
	IO_HALTLN
} iotype;			/*GHF 3 kinds of IO. */ /* BLC 5 with binary */

#define IO_IS_READ(io) (IO_TYPE(io) == IO_READ || IO_TYPE(io) == IO_BREAD)
#define IO_IS_BIN(io)  (IO_TYPE(io) == IO_BREAD || IO_TYPE(io) == IO_BWRITE)

struct io_struct {
	structtype	st_type;
	iotype		type;		/*GHF IO_READ,IO_WRITE, or IO_WRITELN */
	expr_t		*expr;	        /*GHF parameter to R or W */
	expr_t   	*file;
	expr_t		*control;	/*linc control string for formatting */
};

#define IO_TYPE(x)	((x)->type)
#define IO_NEXT(x)	((x)->next)

#define IO_EXPR1(x)	((x)->expr)	/*GHF 1 parameter to read or write */
#define IO_FILE(x)	((x)->file)
#define IO_CONTROL(x)	((x)->control)

#define IOP_EXP(x)	((x)->u.exp)
#define IOP_SYM(x)	((x)->u.sym)

#endif
