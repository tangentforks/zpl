/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __BUILD2_H_
#define __BUILD2_H_

extern symboltable_t *combine(symboltable_t*,char*);
extern symboltable_t *build_int(int);
extern symboltable_t *build_real(double);
extern symboltable_t *build_char(char);
extern symboltable_t *build_string(void);
extern symboltable_t *check_var(char*);

#endif
