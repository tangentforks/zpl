/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _LIVE_H_
#define _LIVE_H_

extern void print_live_vars(statement_t *);
extern int num_live_vars(statement_t *);
extern int num_live_array_vars(statement_t *);
extern int is_live(statement_t *, symboltable_t *);
extern glist copy_live_var_list(glist);

#endif /*** _LIVE_H ***/

