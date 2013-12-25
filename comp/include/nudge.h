/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __NUDGE_H_
#define __NUDGE_H_

int expr_nudged (expr_t*);          /* is the BIAT nudged at all?            */

int set_nudge (expr_t*, int, int); /* sets a nudge of a given dimension */
int get_nudge (expr_t*, int);

#endif
