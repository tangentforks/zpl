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





#ifndef GENLIST_H
#define GENLIST_H

#include "struct.h"

struct genlist_struct {
  union {
    statement_t	    *stmt_p;      
    symboltable_t   *ident;    
    expr_t          *expr_p;
    void            *generic_p;
    outdep_t        *outdep_p;
    indep_t         *indep_p;
    int             intval;
  } u;
  struct genlist_struct		*next_p;	
};


#define G_STATEMENT(x)		((x)->u.stmt_p)
#define G_IDENT(x)              ((x)->u.ident)
#define G_EXPR(x)               ((x)->u.expr_p)
#define G_NEXT(x)		((x)->next_p)
#define G_GENERIC(x)            ((x)->u.generic_p)
#define G_OUTDEP(x)		((x)->u.outdep_p)
#define G_INDEP(x)		((x)->u.indep_p)
#define G_ATTYPE_DEF(x)         ((x)->u.intval)
#define G_ATTYPE(x)             ((attype_t)((x)->u.intval))
#endif

