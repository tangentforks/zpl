/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***************************************************************************
	Center for Supercomputing Research and Development
	at the University of Illinois at Urbana-Champaign
		305 Talbot Laboratory
		104 South Wright Street
		Urbana, Illinois  61801-2932 USA

Write to the above address for information on the CSRD Software Catalog,
or send e-mail to:  softcat@csrd.uiuc.edu

Copyright (c) 1991 by the Board of Trustees of the University of Illinois
*unpublished*.  All rights reserved.  This proprietary created material is
provided under a license agreement which restricts its use and protects its
confidentiality.  No publication or waiver of confidentiality is implied by
this transfer under the license.
***************************************************************************/

#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include "struct.h"

struct callgraph_struct {
  int		tag;
  symboltable_t	*function_p;	
  calledge_t	*call_list;	
  calledge_t    *caller_list;
};

/*
 * for call_list items:
 *     expr: statement issuing the call
 *     sink: called function
 * for caller_list items (in_edges) :
 *     expr: statement issuing the call
 *     sink: function issuing the call
 */
struct calledge_struct {
  expr_t       	*expr;	
  symboltable_t	*sink;		
  calledge_t	*next_p;	
  calledge_t    *same;    /* MDD: pointer to respective in/out edge    */
};

#define CG_NEXT(x)	(T_NEXT(x))

#define CG_FLAG(x)	((x)->tag)

#define CG_FCN(x)	((x)->function_p)

#define CG_EDGE(x)	((x)->call_list)

#define CG_IN_EDGE(x)   ((x)->caller_list)

#define CG_CALL(x)	((x)->expr)

#define CG_SINK(x)	((x)->sink)

#define CG_NODE(x)	((S_FUN_BODY(x) == NULL) ? NULL : T_CGNODE(S_FUN_BODY(x)))

#define CG_EDGE_SAME(x)  ((x)->same) /* MDD */

#endif
