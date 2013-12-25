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



#ifndef IN_H
#define IN_H

#include "inout.h"

typedef enum {
	VAR,  		
	COMPL,		
	ALL		
} settype;

#define UNIV_DEPTH MAXINT
struct set_struct {
	settype			type;
	dir_depend		dir;
	symboltable_t 		*var;
	genlist_t		*exprs;
	struct set_struct	*next_s;
        unsigned short          imod;
};


set_t *alloc_set(settype);
void free_set(set_t *);
set_t *s_union(set_t *, set_t *);
void set_dir(expr_t *, dir_depend *);
set_t *s_copy(set_t *);
void p_set(FILE *, set_t *);
void p_setelement(FILE *, set_t *);

void addinset(FILE *, set_t *, statement_t *);
void addoutset(FILE *, set_t *, statement_t *);

#endif
