/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __BB_TRAVERSE_H_
#define __BB_TRAVERSE_H_

#include "struct.h"

#define BBTR_NONE    0x00
#define BBTR_IO      0x01
#define BBTR_NN_COMM 0x02
#define BBTR_WRAP    0x04
#define BBTR_REFLECT 0x08

void set_last_next(statement_t *, statement_t *);
statement_t *bb_traversal_stmtls(statement_t *,statement_t *(*)(statement_t *),
				 int);
void bb_traversal(module_t *mod, statement_t *(*)(statement_t *), int);

#endif
