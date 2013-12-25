/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 ***
 *** FILE: usage.h
 *** AUTHOR: Sung-Eun Choi (sungeun@cs.washington.edu)
 ***
 ***/

#ifndef __USAGE_H_
#define __USAGE_H_

typedef enum {
  FROM_ZC=0,
  FROM_RZC,
  FROM_ZC0,
  FROM_RZC0
} usage_t;

extern void use_usage(usage_t);
extern void usage(usage_t);
extern void print_version(void);

#endif /* __USAGE_H_ */
