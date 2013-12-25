/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../include/zutil.h"

/* greatest common divisor */
int gcd (int a, int b)
{
  return (b == 0) ? a : gcd (b, a % b);
}

/* least common multiple */
int lcm (int a, int b)
{
  return (a == 0 || b == 0) ? 0 : (a * b) / gcd (a, b);
}

/* return variable size string */
char* vstr(char* fmt, ...) {
  int n, size = 128;
  char* str;
  va_list ap;

  if ((str = malloc(size)) == NULL) {
    return NULL;
  }
  while (1) {
    va_start(ap, fmt);
    n = vsnprintf(str, size, fmt, ap);
    va_end(ap);
    if (n > -1 && n < size) {
      return str;
    }
    if (n > -1) {
      size = n+1;
    }
    else {
      size *= 2;
    }
    if ((str = realloc(str, size)) == NULL) {
      return NULL;
    }
  }
}
