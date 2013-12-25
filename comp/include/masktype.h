/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _MASKTYPE_H_
#define _MASKTYPE_H_

#define MASK_IS_NEGATIVE(x) ((x) & 0x1)
#define MASK_IS_SPARSE(x)   ((x) & 0x2)

#define SPARSIFY_MASK(x)    ((x) | 0x2)

typedef enum {
  MASK_NONE = -1,
  MASK_WITHOUT = 0,
  MASK_WITH,
  MASK_SPARSE_WITH,
  MASK_SPARSE_WITHOUT
} masktype;

#endif
