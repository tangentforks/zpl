/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* FILE: idfa_traverse.h
 * DATE: 15 September 1996
 * CREATOR: echris
 */

#ifndef __IDFA_TRAVERSE_H_
#define __IDFA_TRAVERSE_H_

#define FORWARD 1
#define REVERSE 0

#define IDFA_STMT_FN(idfa) ((idfa)->stmt_fn)
#define IDFA_MERGE_FN(idfa) ((idfa)->merge_fn)
#define IDFA_FREE_FN(idfa) ((idfa)->free_fn)
#define IDFA_EQUAL_FN(idfa) ((idfa)->eq_fn)
#define IDFA_COPY_FN(idfa) ((idfa)->copy_fn)
#define IDFA_DIR(idfa) ((idfa)->dir)

typedef struct idfa_struct {
  void *(*stmt_fn)(statement_t *, void *, int);
  void *(*merge_fn)(void *, void *);
  void *(*free_fn)(void *);
  int (*eq_fn)(void *, void *);
  void *(*copy_fn)(void *);
  int dir;
} idfa_t;

void *idfa_stmtls(statement_t *stmtls,  void *indata, idfa_t *idfa_dat, 
		  int final);

#endif
