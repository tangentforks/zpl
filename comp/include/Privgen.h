/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __PRIVGEN_H_
#define __PRIVGEN_H_

#include <stdio.h>
#include "struct.h"

void priv_access_begin(FILE *);
void priv_access_end(FILE *);
void priv_named_access(FILE *, char *);

void priv_decl_begin(FILE *);
void priv_decl_end(FILE *);

void priv_reg_access(FILE *, expr_t *);

void priv_alloc_begin(FILE *);
void priv_alloc_end(FILE *);
void priv_reg_alloc(FILE *, symboltable_t*);

void priv_tid_decl(FILE *);

#endif
