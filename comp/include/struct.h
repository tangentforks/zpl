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

#ifndef STRUCT_H
#define STRUCT_H

typedef enum {
  MODULE_T = 17,
  FUNCTION_T,
  STATEMENT_T,
  COMPOUND_T,
  EXPR_T,
  IF_T,
  LOOP_T,
  CALLGRAPH_T,
  CALLEDGE_T,
  DIMENSION_T,
  INITIAL_T,
  SYMBOLTABLE_T,
  DATATYPE_T,
  ASSIGN_T,
  SET_T,
  EXPRLIST_T,
  STMTLIST_T,
  IO_T,
  SYMLIST_T,
  HASHENTRY_T,
  REFLIST_T,
  SUMMARYLIST_T,
  CCS_T,
  LIVE_T
} structtype;

typedef	struct	function_struct      	function_t;
typedef	struct	statement_struct	statement_t;
typedef	struct	expr_struct             expr_t;
typedef	struct	callgraph_struct	callgraph_t;
typedef	struct	calledge_struct	        calledge_t;
typedef	struct	symboltable_struct	symboltable_t;
typedef	struct	hashentry_struct	hashentry_t;
typedef	struct	symlist_struct		symlist_t;
typedef	struct	datatype_struct		datatype_t;
typedef	struct	module_struct		module_t;
typedef struct  set_struct		set_t;
typedef struct	exprlist_struct		exprlist_t;
typedef struct	stmtlist_struct		stmtlist_t;
typedef struct	io_struct		io_t;
typedef struct	genlist_struct		genlist_t;
typedef struct	reflist_struct		reflist_t;
typedef struct	summarylist_struct	summarylist_t;
typedef struct	outdep_struct		outdep_t;
typedef struct	indep_struct		indep_t;
typedef struct  dvnode_struct           *dvlist_t;
typedef struct  dep_info_struct         dep_info_t;
typedef struct  dep_struct              dep_t;
typedef struct  callsite_struct         callsite_t;
typedef struct  at_struct               at_t;
typedef	struct	compound_struct	compound_t;
typedef	struct	if_struct       if_t;
typedef	struct	loop_struct	loop_t;
typedef struct	region_struct	region_t;	/*GHF these ones are new */
typedef struct	mloop_struct	mloop_t;	/* Multi-Loop */
typedef struct	nloop_struct	nloop_t;	/* loops for indexed arrays */
typedef struct	wrap_struct	wrap_t;		/* for WRAP & REFLECT */
typedef struct  comm_info_struct comm_info_t;	/* for IRONMAN comm */
typedef struct  comm_struct     comm_t;		/* for IRONMAN comm */
typedef struct  zprof_struct    zprof_t;	/* for profile info */
#endif
