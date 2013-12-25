/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/* FILE: idfa_traverse.c
 * DATE: 15 September 1996
 * CREATOR: echris
 */

#include <stdio.h>
#include "../include/struct.h"
#include "../include/error.h"
#include "../include/macros.h"
#include "../include/parsetree.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/global.h"
#include "../include/stmtutil.h"
#include "../include/db.h"
#include "../include/glist.h"
#include "../include/glistmac.h"
#include "../include/buildzplstmt.h"
#include "../include/idfa_traverse.h"
#include "../include/statement.h"
#include "../include/rmstack.h"


/* globals from global.h */

int no_iterate = 0;


/* note: we must call the STMT_FN on the region scope statement because
 * it may contain a mask */

static void *idfa_region(statement_t *stmt, void *indata, idfa_t *idfa_dat,
			 int final)
{
  void *outdata;
  region_t* region;

  region = T_REGION(stmt);
  INT_COND_FATAL((region!=NULL), stmt, "Null region in idfa_region()");

  if (IDFA_DIR(idfa_dat) == FORWARD) {
    indata = (*IDFA_STMT_FN(idfa_dat))(stmt, indata, final);
  }

  RMSPushScope(region);
  outdata = idfa_stmtls(T_BODY(region), indata, idfa_dat, final);
  RMSPopScope(region);

  if (IDFA_DIR(idfa_dat) == REVERSE) {
    outdata = (*IDFA_STMT_FN(idfa_dat))(stmt, outdata, final);
  }

  return (outdata);
}


static void *idfa_mloop(statement_t *stmt, void *indata, idfa_t *idfa_dat,
			int final)
{
  void *outdata;
  mloop_t* mloop = T_MLOOP(stmt);

  INT_COND_FATAL(mloop != NULL, NULL, "Null loop in idfa_mloop()");

  outdata = idfa_stmtls(T_BODY(mloop), indata, idfa_dat, final);

  return (outdata);
}


static void *idfa_nloop(statement_t *stmt, void *indata, idfa_t *idfa_dat,
			int final)
{
  void *outdata;
  nloop_t* nloop = T_NLOOP(stmt);

  INT_COND_FATAL((nloop!=NULL), NULL, "Null loop in idfa_nloop()");

  if (IDFA_DIR(idfa_dat) == FORWARD) {
    indata = (*IDFA_STMT_FN(idfa_dat))(stmt, indata, final);
  }
  
  outdata = idfa_stmtls(T_NLOOP_BODY(nloop), indata, idfa_dat, final);

  if (IDFA_DIR(idfa_dat) == REVERSE) {
    outdata = (*IDFA_STMT_FN(idfa_dat))(stmt, outdata, final);
  }

  return (outdata);
}


static void *idfa_if(statement_t *stmt,	if_t *ifstmt, void *indata,
		     idfa_t *idfa_dat, int final)
{
  void *outdatathen, *outdataelse, *outdata;
  void* indata_copy;

  INT_COND_FATAL((ifstmt!=NULL), stmt, "Null ifstmt in idfa_if()");

  /* don't traverse body of shard */
  if (stmt_is_shard(stmt)) {
    return((*IDFA_STMT_FN(idfa_dat))(stmt, indata, final));
  }

  if (IDFA_DIR(idfa_dat) == FORWARD) {
    indata = (*IDFA_STMT_FN(idfa_dat))(stmt, indata, final);
  }

  indata_copy = (*IDFA_COPY_FN(idfa_dat))(indata);
  outdatathen = idfa_stmtls(T_THEN(ifstmt), indata, idfa_dat, final);
  outdataelse = idfa_stmtls(T_ELSE(ifstmt), indata_copy, idfa_dat, final);
  outdata = (*IDFA_MERGE_FN(idfa_dat))(outdatathen, outdataelse);

  if (IDFA_DIR(idfa_dat) == REVERSE) {
    outdata = (*IDFA_STMT_FN(idfa_dat))(stmt, outdata, final);
  }

  return (outdata);
}


static void *idfa_loop(statement_t *stmt, loop_t *loopstmt, void *indata,
		       idfa_t *idfa_dat, int final) {
  void *outdata, *backdata, *last_backdata, *propdata;
  statement_t *body;
  int lasttime, final2, iterations, lasttimep, bodyisloop;
  
  INT_COND_FATAL((loopstmt!=NULL), stmt, "Null loop in idfa_loop()");

  /* don't traverse body of shard */
  if (stmt_is_shard(stmt)) {
    return((*IDFA_STMT_FN(idfa_dat))(stmt, indata, final));
  }

  backdata = NULL;
  last_backdata = NULL;
  body = T_BODY(loopstmt);
  bodyisloop = (T_TYPE(body)==S_LOOP)?TRUE:FALSE;  /* used to prune its. */

  lasttime = lasttimep = no_iterate?TRUE:FALSE;
  if (bodyisloop) lasttime = TRUE;

  iterations = 0;

  DBS1(27, "Loop(%d):", T_LINENO(stmt));
  do {
  
    DBS2(27, "%d(%d) ", iterations, T_LINENO(stmt));

    if (iterations++ > 100) {
      INT_FATAL(stmt, "Infinite idfa loop!");
    }

    final2 = final && lasttime;

    last_backdata = (*IDFA_COPY_FN(idfa_dat))(backdata);
    propdata = (*IDFA_COPY_FN(idfa_dat))(indata);
    propdata = (*IDFA_MERGE_FN(idfa_dat))(backdata, propdata);

    if (IDFA_DIR(idfa_dat) == FORWARD) {
      propdata = (*IDFA_STMT_FN(idfa_dat))(stmt, propdata, final2);
    }

    propdata = idfa_stmtls(body, propdata, idfa_dat, final2);
    
    if (IDFA_DIR(idfa_dat) == REVERSE) {
      propdata = (*IDFA_STMT_FN(idfa_dat))(stmt, propdata, final2);
    }
    
    if (lasttime) lasttimep = TRUE;
    if (!no_iterate) {
      backdata = propdata;
      if ((*IDFA_EQUAL_FN(idfa_dat))(last_backdata, backdata)) {
	lasttime = TRUE;
      }
    }
  } while (!lasttimep);

  outdata = propdata;

  /* this merge is required, for non-repeat-until loops may not exec at all */
  if (T_TYPE(loopstmt) != L_REPEAT_UNTIL) {
    outdata = (*IDFA_MERGE_FN(idfa_dat))(outdata, indata);
  }

  return (outdata);
}


static void *idfa_stmt(statement_t *stmt,  void *indata, idfa_t *idfa_dat, 
		       int final)
{
  void *outdata = NULL;

  /* null stmt? yes, outdata = indata */
  if (!stmt) 
    return (indata);

  switch (T_TYPE(stmt)) {

  /* first, consider statements that do not contain other statements */
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_HALT:
  case S_CONTINUE:
  case S_WRAP:
  case S_REFLECT:
  case S_COMM:
  case S_EXPR:
  case S_RETURN:
  case S_IO:
  case S_ZPROF:
    INT_COND_FATAL((IDFA_STMT_FN(idfa_dat)!=NULL), stmt,
		   "NULL stmt_fn in idfa_stmt");
    outdata = (*IDFA_STMT_FN(idfa_dat))(stmt, indata, final);
    break;

  /* now, consider statements that may contain other statements */
  case S_REGION_SCOPE:
    outdata = idfa_region(stmt, indata, idfa_dat, final);
    break;
  case S_MLOOP:
    outdata = idfa_mloop(stmt, indata, idfa_dat, final);
    break;
  case S_NLOOP:
    outdata = idfa_nloop(stmt, indata, idfa_dat, final);
    break;
  case S_MSCAN: /* BLC -- same? */
  case S_COMPOUND:
    outdata = idfa_stmtls(T_CMPD_STLS(stmt), indata, idfa_dat, final);
    break;
  case S_IF:
    outdata = idfa_if(stmt, T_IF(stmt), indata, idfa_dat, final);
    break;
  case S_LOOP:
    outdata = idfa_loop(stmt, T_LOOP(stmt), indata, idfa_dat, final);
    break;

  default:
    INT_FATAL(stmt, "Bad statementtype (%d) in idfa_stmt()",T_TYPE(stmt));
  }

  return (outdata);
}


/* FN: idfa_stmtls
 * This function uses recursion to iterate down stmtls.  If direction is
 * FORWARD, then idfa_stmt() is called before the recursion.  
 * Otherwise, idfa_stmt() is a called after the recursion, thus
 * traversing stmtls in reverse.
 *
 * BLC -- rewrote iteratively 2/17
 *
 */

void *idfa_stmtls(statement_t *stmtls,  void *indata, idfa_t *idfa_dat, 
		  int final)
{
  statement_t	*next;
  void          *outdata;

  /* null stmtls? yes, outdata = indata */
  if (!stmtls) {
    return indata;
  }


  if (IDFA_DIR(idfa_dat) == FORWARD) {
    while (stmtls != NULL) {
      outdata = idfa_stmt(stmtls, indata, idfa_dat, final);
      indata = outdata;
      stmtls = T_NEXT(stmtls);
    }
  } else if (IDFA_DIR(idfa_dat) == REVERSE) {
    /* move stmtls to end of list */
    next = T_NEXT(stmtls);
    while (next != NULL) {
      stmtls = next;
      next = T_NEXT(stmtls);
    }

    /* traverse */
    while (stmtls != NULL) {
      DBS1(26,"Doing statement %d\n",T_LINENO(stmtls));
      outdata = idfa_stmt(stmtls, indata, idfa_dat, final);
      indata = outdata;
      stmtls = T_PREV(stmtls);
    }
  }

  return outdata;
}
