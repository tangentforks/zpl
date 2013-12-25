/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _GLISTMAC_H__
#define _GLISTMAC_H__

#include "inout.h"
#include "depgraph.h"
#include "glist.h"

/* This header file's name no longer makes sense but I'm too lazy to think
   of a good new one.  ---BLC */

/* expression list macros and structures built on generic lists */

/* an elist is an expression list (see use in ccodegen/Cgen.c) */

#define EXPR_DENSE       0 /* expression is dense */
#define EXPR_SPARSE      1 /* expression is sparse */
#define EXPR_SPARSE_DIFF 2 /* expression is sparse, but different than MLOOP */

#define ELIST_DATA(el)       (expr_t *)(GLIST_DATA(&((el)->basic)))
#define ELIST_NEXT(el)       ((elist)(GLIST_NEXT(&((el)->basic))))
#define ELIST_DIR(el)        (T_IDENT(ELIST_DATA(el)))
#define ELIST_INDEX(el)      ((el)->index)
#define ELIST_ARR_COUNTS(el) ((el)->arrgood)
#define ELIST_DIRLIST(el)    ((el)->dirlist)
#define ELIST_DENSITY(el)    ((el)->density)
#define ELIST_WRITTEN(el)    ((el)->written)
#define ELIST_ATNODE(el)     ((el)->atnode)

#define elist_create(expr)   (elist)glist_create(expr,ELIST_NODE_SIZE)
#define elist_append(l,expr) glist_append((glist)l,expr,ELIST_NODE_SIZE)

typedef struct no {
  gnode_t basic;
  int index;
  int arrgood;
  glist dirlist;
  int density;
  int written;
  expr_t* atnode;
} enode_t, *elist;


/* an rlist is a region list (used in ccodegen/Mgen.c, Wgen.c) */


#define DIRLIST_DIR(dl)     (expr_t*)(GLIST_DATA(&((dl)->basic)))
#define DIRLIST_NEXT(dl)    (dlist)(GLIST_NEXT(&((dl)->basic)))
#define DIRLIST_EXPRLS(dl)  ((dl)->exprs)

#define dlist_create(dir)   (dlist)glist_create(dir,DIRLIST_NODE_SIZE);
#define dlist_append(l,dir) (dlist)glist_append((glist)l,dir,DIRLIST_NODE_SIZE)

typedef struct dlist {
  gnode_t basic;
  glist exprs;
} dnode_t, *dlist;


#define RLIST_DATA(rl)    (expr_t *)(GLIST_DATA(&((rl)->basic)))
#define RLIST_NEXT(rl)    ((rlist)(GLIST_NEXT(&((rl)->basic))))
#define RLIST_DIRLIST(rl) ((rl)->dirlist)

#define rlist_create(reg)   (rlist)glist_create(reg,RLIST_NODE_SIZE)
#define rlist_append(l,reg) (rlist)glist_append((glist)l,reg,RLIST_NODE_SIZE)

typedef struct rno {
  gnode_t basic;
  dlist dirlist;
} rnode_t, *rlist;

#define RLIST_NODE_SIZE (sizeof(rnode_t))


#define DIRLIST_NODE_SIZE (sizeof(dnode_t))

/* a dvlist is a list of distance vectors */
typedef struct dvnode_struct {
  gnode_t basic;
  distvect_t dv;
} dvnode_t;

#define ELIST_NODE_SIZE (sizeof(enode_t))
#define DVLIST_NODE_SIZE (sizeof(dvnode_t))


#define DVLIST_DATA(dvl) (GLIST_DATA(&(dvl)->basic))
#define DVLIST_NEXT(dvl) ((dvlist_t)(GLIST_NEXT(&((dvl)->basic))))
#define DVLIST_NEXT_LHS(dvl) (GLIST_NEXT(&((dvl)->basic)))
#define DVLIST_DV(dvl)   ((dvl)->dv)

#endif
