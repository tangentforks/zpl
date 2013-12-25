/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ZPL_STRING_H_
#define __ZPL_STRING_H_

#include "type.h"

/* small char array used to cast a char as a string */
static char char_as_string[2] = {0, 0};

/* char array used to return strings from functions */
static _zstring _zretstring;

#define _ZPL_NULLSTR ""
#define _ZPL_STRLEN 256
#define _ZPL_STRCPY(x,y) strncpy(x,y,_ZPL_STRLEN)
#define _ZPL_STRCMP(x,y) strncmp(x,y,_ZPL_STRLEN)
#define _ZPL_STRCAT(x,y) strncat(x,y,_ZPL_STRLEN)
#define _ZPL_CHR2STR(x) (char_as_string[0] = (x), char_as_string)
#endif
