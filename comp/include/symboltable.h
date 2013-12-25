/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __UTIL_SYMBOLTABLE_H_
#define __UTIL_SYMBOLTABLE_H_

#include "symtab.h"

int symtab_is_global(symboltable_t *);     /* tells if it's global (0/1) */
int symtab_is_static(symboltable_t *);     /* tells if it's static (0/1) */
int symtab_is_param(symboltable_t*);       /* tells if it's a param (0/1) */

int symtab_is_qreg(symboltable_t *);       /* tells if it's ["]  (0/1) */
int symtab_is_grid_scalar(symboltable_t*); /* tells if it's [::*] (0/rank) */
int symtab_is_qmask(symboltable_t *);      /* tells if it's [w/ "] (0/rank) */
int symtab_is_indexi(symboltable_t *);        /* tells if it's Indexi (0/i) */
int symtab_is_ia_index(symboltable_t *);      /* tells if it's [] index (0/1) */

int symtab_is_fixed(symboltable_t *);      /* is this a standard context
					      function that can't be 
					      reordered? */

expr_t *symtab_find_reg(symboltable_t *); /* finds region (NULL,pst) */

int symtab_rank(symboltable_t *pst);           /* tells rank of pst (0/rank) */

int symtab_var_has_lvalue(symboltable_t *pst); /* tells if variable is lvalue */

int symtab_dir_c_legal_init(symboltable_t*); /* is the dir a legal C init? */

int symtab_is_sparse_reg(symboltable_t*);

#endif
