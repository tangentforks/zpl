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


#ifndef	TREEMAC_H
#define	TREEMAC_H

#include "struct.h"
#include "macros.h"
#include "genlist.h"

#define NLOOP_INSIDE  1
#define NLOOP_OUTSIDE 2
#define NLOOP_ABSENT  3

#define ST_TYPE(x)	((x)->st_type)

#define T_MODULE(x)     ((x)->module_p)
#define T_FCNLS(x)	((x)->fcnls_p)
#define T_FCN(x)	((x)->function_p)
#define T_SEMANTICS(x)  ((x)->semantics)
#define T_INVOCATIONS(x) ((x)->invocations)
#define T_PARAMLS(x)	((x)->paramls_p)
#define T_CGNODE(x)	((x)->cgnode_p)
#define	T_LOOPS(x)	((x)->loops_p)
#define T_G_REFS(x)	((x)->global_refs)
#define T_G_MODS(x)	((x)->global_mods)
#define T_G_USES(x)	((x)->global_uses)
#define T_IS_SUMMARIZED(x)	((x)->is_summed)
#define T_BLOCK(x)	((x)->flow)

#define T_LO_COMMID(x)  ((x)->lo_comm_id)
#define T_CGPROPS(x)	((x)->cgposition)

#define CG_ROOT 1
#define CG_LEAF 2
#define CG_CYCLE 4
#define CG_VISITED 8

/* SUMMARY of T_CGPROPS() meaning
 * 0
 * 1 CG_ROOT
 * 2           CG_LEAF
 * 3 CG_ROOT | CG_LEAF
 * 4                     CG_CYCLE
 * 5 CG_ROOT |           CG_CYCLE
 * 6           CG_LEAF | CG_CYCLE
 * 7 CG_ROOT | CG_LEAF | CG_CYCLE
 * 8                                CG_VISITED
 * 9 CG_ROOT |                      CG_VISITED
 * a           CG_LEAF |            CG_VISITED
 * b CG_ROOT | CG_LEAF |            CG_VISITED
 * c                     CG_CYCLE | CG_VISITED
 * d CG_ROOT |           CG_CYCLE | CG_VISITED
 * e           CG_LEAF | CG_CYCLE | CG_VISITED
 * f CG_ROOT | CG_LEAF | CG_CYCLE | CG_VISITED
 */

#define T_CG_ROOT(x)		((T_CGPROPS(x)) & CG_ROOT)
#define T_CG_LEAF(x)		((T_CGPROPS(x)) & CG_LEAF)
#define T_CG_CYCLE(x)		((T_CGPROPS(x)) & CG_CYCLE)
#define T_CG_VISITED(x)		((T_CGPROPS(x)) & CG_VISITED)

#define T_SET_CG_ROOT(x)	(T_CGPROPS(x)) |= CG_ROOT
#define T_CLEAR_CG_ROOT(x)	(T_CGPROPS(x)) &= ~CG_ROOT
#define T_SET_CG_LEAF(x)	(T_CGPROPS(x)) |= CG_LEAF
#define T_CLEAR_CG_LEAF(x)	(T_CGPROPS(x)) &= ~CG_LEAF
#define T_SET_CG_CYCLE(x)	(T_CGPROPS(x)) |= CG_CYCLE
#define T_CLEAR_CG_CYCLE(x)	(T_CGPROPS(x)) &= ~CG_CYCLE
#define T_SET_CG_VISITED(x)	(T_CGPROPS(x)) |= CG_VISITED
#define T_CLEAR_CG_VISITED(x)	(T_CGPROPS(x)) &= ~CG_VISITED

#define T_TYPE(x)         ((x)->type)
#define T_SUBTYPE(x)      ((x)->subtype)
#define	T_TYPEINFO(x)     ((x)->typeinfo)
#define T_TYPEINFO_REG(x) ((x)->typeinfo_reg)

#define T_LINENO(x)	((x)->lineno)
#define T_FILENAME(x)	((x)->filename_p)
#define	T_LABEL(x)	((x)->label_p)

#define T_CMPD(x)	((x)->u.compound_p)
#define T_EXPR(x)	((x)->u.expr_p)
#define T_IF(x)		((x)->u.if_p)
#define T_LOOP(x)	((x)->u.loop_p)
#define T_RETURN(x)	((x)->u.return_p)
#define T_IO(x)		((x)->u.io_p)
#define T_HALT(x)       ((x)->u.halt_p)
#define T_END(x)	((x)->u.end_p)

#define T_WRAP(x)	((x)->u.wrap_p)		/*GHF these are new: */
#define T_REGION(x)	((x)->u.region_p)
#define T_MLOOP(x)	((x)->u.mloop_p)
#define T_NLOOP(x)	((x)->u.nloop_p)
#define T_COMM(x)	((x)->u.comm_p)
#define T_ZPROF(x)	((x)->u.zprof_p)
#define T_ZPROFTYPE(x)	(T_ZPROF(x)->type)
#define T_COMMSTAT(x)	(T_ZPROF(x)->u.commstat)

#define T_PARENT(x)	((x)->parent_p)
#define T_PARFCN(x)	((x)->parfcn_p)
#define T_NEXT(x)	((x)->next_p)
#define T_PREV(x)	((x)->prev_p)
#define T_NEXT_LEX(x)	((x)->next_lex_p)
#define T_PREV_LEX(x)	((x)->prev_lex_p)
#define T_DECL(x)	((x)->decl_p)
#define T_STLS(x)	((x)->stmtls_p)
#define T_FIRSTDO(x)	((x)->firstdo_p)
#define T_IN(x)		((x)->inset_p)
#define T_OUT(x)	((x)->outset_p)
#define T_LIVE(x)	((x)->live_p)
#ifndef BLAH
#define T_HIER(x)	((x)->hier_p)
#endif
#define T_FUNC_CALLS(x) ((x)->func_calls_p)
#define T_PARALLEL(x)   ((x)->parallel)
#define T_REACHABLE(x)  ((x)->reachable)

#define T_UNC_STMT(x,y) ((x)->unc_stmt[y])   /* BLC uncovered statements */
#define T_UNC_FUNC(x,y) ((x)->unc_fn[y])     /* BLC uncovered funcalls */
#define T_NUM_PARAMS(x) ((x)->numparams)
#define T_CALLINFO(x)   ((x)->callinfo)

#define T_OVERLOAD_PREV(x) ((x)->overload_prev)  /* overloaded function */
#define T_OVERLOAD_NEXT(x) ((x)->overload_next)  /* overloaded function */

#define T_IRON_COMMID(x) ((x)->iron_cid)

#define T_SCOPE(x) ((x)->scope)
#define T_OUTDEP(x) ((x)->outdep_p)
#define T_INDEP(x) ((x)->indep_p)
#define T_IN_MLOOP(x) ((x)->mloop_p)
#define T_VISITED(x) ((x)->visited)

#define T_MEANING(x)    ((x)->meaning_p)

#define T_PRE(x)        ((x)->pre)
#define T_POST(x)       ((x)->post)

#define T_IS_SHARD(x)   ((x)->shard)

#define T_PROHIBITIONS(x) ((x)->nogen)


#define T_CMPD_DECL(x)	(T_DECL(T_CMPD(x)))
#define T_CMPD_STLS(x)	(T_STLS(T_CMPD(x)))


#define T_OPLS(x)	((x)->expr_p)
#define T_IDENT(x)	((x)->symtab_p)
#define T_REGMASK(x)    ((x)->reg_p)
#define T_REGMASK2(x)   ((x)->reg2_p)
#define T_STMT(x)	((x)->stmt_p)


#define T_IFCOND(x)	((x)->cond_p)
#define T_PDP(x)        ((x)->u.if_then_else.PDP)
#define T_THEN(x)	((x)->u.if_then_else.then_p)
#define T_ELSE(x)	((x)->u.if_then_else.else_p)

#define T_BODY(x)	((x)->body_p)
#define T_IVAR(x)	((x)->u.fixed_iteration_loop.var_p)
#define T_ENDDO(x)	((x)->u.fixed_iteration_loop.enddo_p)
#define T_START(x)	((x)->u.fixed_iteration_loop.start_p)
#define T_STOP(x)	((x)->u.fixed_iteration_loop.stop_p)
#define T_STEP(x)	((x)->u.fixed_iteration_loop.step_p)
#define T_UPDOWN(x)	((x)->u.fixed_iteration_loop.up_down)
#define T_START_VAL(x)	((x)->u.fixed_iteration_loop.start_val)
#define T_STOP_VAL(x)	((x)->u.fixed_iteration_loop.stop_val)
#define T_STEP_VAL(x)	((x)->u.fixed_iteration_loop.step_val)
#define T_PDECL(x)	((x)->u.fixed_iteration_loop.decl_p)
#define T_PINIT(x)	((x)->u.fixed_iteration_loop.initstmt_p)
#define T_PPOST(x)	((x)->u.fixed_iteration_loop.poststmt_p)
#define T_LNEXTDO(x)	((x)->u.fixed_iteration_loop.nextdo_p)

#define T_NEXTDO(x)	((x==NULL)?NULL: T_LNEXTDO(T_LOOP(x)))

#define T_LOOPCOND(x)   ((x)->u.generic_loop.cond_p)

/*GHF for WRAP statements */
#define T_WRAP_RANK(x)  (x)->rank
#define T_WRAP_SEND(x)  (x)->send
#define T_WRAP_CID(x)   (x)->cid

#define T_SBE(x)        ((x)->sbe_info)

#define T_FREE(x)       ((x)->free)

#define T_INSTANCE_NUM(x) ((x)->instance_num)

#define T_DUMMY(x)       ((x)->dummy)   /* is this a dummy expression? */

/*
 * T_GET_NUDGE : returns the nudge of {nudge|uat} X in dimension D
 * T_SET_NUDGE : adds N to the nudge of {nudge|uat} X in dimension D
 * T_IS_NUDGED : returns 1 iff the {nudge|uat} X is nudged in any dimension
 */
#define T_GET_NUDGE(x,d) (get_nudge ((x), (d)))
#define T_SET_NUDGE(x,d,n) (set_nudge ((x), (d), (n)))
#define T_IS_NUDGED(x) (expr_nudged (x))

#define T_NUDGE(x) ((x)->nudge)

#define T_RANDACC(x) ((x)->maplist)

#define T_MAP(x)	((x)->maplist)    /*GHF list of symbols */
#define T_MAPSIZE(x)    ((x)->mapsize)  /* length of that list */
#define T_RED_RANK(x)   ((x)->mapsize)  /* rank of reduction after red2mloops */

#define T_MLOOP_RANK(x)	((x)->rank)         /*GHF rank of region */
#define T_MLOOP_REG(x)	((x)->loop_region)  /*GHF pointer to (looping) region */
#define T_MLOOP_WITH(x)	((x)->with)         /*derrick * with or without mask */
#define T_MLOOP_MASK(x)	((x)->loop_mask)    /*derrick * mask */
#define T_MLOOP_BODY(x) ((x)->body_p)       /*MDD list of statements     */
/* T_MLOOP_ORDER(mloop_t x, int y) returns an int n, where valid dimension n 
   will be the yth dimension to be looped over in mloop x.
   Possible return values for n are: 0,1, . . .(MAXRANK-1). 
   Possible values for y are : 1,2,3. . . MAXRANK.
   Thus T_MLOOP_ORDER(x, 3) --> 0 means that the third dimension of mloop x
   to loop over will be dimension 0. REA*/
/* This indices thing (to count from zero or one) is tricky. REA*/
#define T_MLOOP_ORDER(x, y)    ((x)->order[(y)-1]) 
#define T_MLOOP_ORDER_V(x)     ((x)->order)
#define T_MLOOP_FLAT_V(x)      ((x)->flat)
#define T_MLOOP_FLAT(x,y)        ((x)->flat[(y)-1])

/* T_MLOOP_DIRECTION(mloop_t x, int y) returns an int n, where n
   indicates that looping along dimension y for mloop x will be:
   ascending(+)                     for n = +1,    (low to hi)
   descending(-)                    for n = -1,    (hi to low)
   Possible values for y are : 0,1, . . .(MAXRANK-1).
   Currently 1 and -1 are the only expected return values for 
   a valid dimension y.  REA*/
#define T_MLOOP_DIRECTION(x, y)  ((x)->direction[y])
#define T_MLOOP_DIRECTION_V(x)   ((x)->direction)

/* The TILEd version of these macros determine the inter-tile iteration 
   order */
#define T_MLOOP_TILE_ORDER(x,y)	 ((x)->tile_order[y-1])
#define T_MLOOP_TILE_ORDER_V(x)	 ((x)->tile_order)
#define T_MLOOP_TILE_DIRECTION(x,y)	((x)->tile_direction[y])
#define T_MLOOP_TILE_DIRECTION_V(x)	((x)->tile_direction)

#define T_MLOOP_TILE_NOPEEL(x)	((x)->tile_nopeel)

#define T_MLOOP_CONSTR(x) ((x)->constraints) /*echris distance vector constr */

/*
 * T_ADD_MLOOP_* : adds a variable, prestatement or poststatement S to
 *                 loop D of mloop M where the outside of the entire
 *                 mloop is given by D == -1. Variables are prepended,
 *                 statements are appended.
 */
#define T_ADD_MLOOP_VAR(m,s,d) \
  { \
    genlist_t* _glnode = alloc_gen(); \
    G_IDENT(_glnode) = s; \
    T_MLOOP_VARS(m,d) = cat_genlist_ls(T_MLOOP_VARS(m,d),_glnode); \
  }

#define T_ADD_MLOOP_PREPRE(m,s,d) ADD_TO_END (T_MLOOP_PREPRE((m),(d)),      \
                                              T_NEXT, statement_t, (s))
#define T_ADD_MLOOP_PRE(m,s,d) ADD_TO_END (T_MLOOP_PRE((m),(d)),      \
                                           T_NEXT, statement_t, (s))
#define T_ADD_MLOOP_POST(m,s,d) ADD_TO_END (T_MLOOP_POST((m),(d)),    \
                                            T_NEXT, statement_t, (s))
#define T_ADD_MLOOP_POSTPOST(m,s,d) ADD_TO_END (T_MLOOP_POSTPOST((m),(d)),    \
                                                T_NEXT, statement_t, (s))

#define T_MLOOP_VARS(m,d)     ((m)->vars[(d)+2])
#define T_MLOOP_PREPRE(m,d)   ((m)->prepre_stmts[(d)+1])
#define T_MLOOP_PRE(m,d)      ((m)->pre_stmts[(d)+1])
#define T_MLOOP_POST(m,d)     ((m)->post_stmts[(d)+1])
#define T_MLOOP_POSTPOST(m,d) ((m)->postpost_stmts[(d)+1])
#define T_MLOOP_VARS_V(m)     ((m)->vars)
#define T_MLOOP_PREPRE_V(m)   ((m)->prepre_stmts)
#define T_MLOOP_PRE_V(m)      ((m)->pre_stmts)
#define T_MLOOP_POST_V(m)     ((m)->post_stmts)
#define T_MLOOP_POSTPOST_V(m) ((m)->postpost_stmts)
#define T_MLOOP_REPLS(m)      ((m)->replacements)
#define T_MLOOP_UNROLL(m,d)   ((m)->unroll[d])
#define T_MLOOP_UNROLL_V(m)   ((m)->unroll)
#define T_MLOOP_TILE(m,d)     ((m)->tile[d])
#define T_MLOOP_TILE_V(m)     ((m)->tile)
#define T_MLOOP_TILE_EXPR(m,d)((m)->tile_expr[d])
#define T_MLOOP_TILE_EXPR_V(m)((m)->tile_expr)

#define T_MLOOP_WBLIST(m)     ((m)->wblist)
#define T_MLOOP_REGLIST(m)    ((m)->reglist)

#define T_NLOOP_BODY(x)	((x)->body_p)
#define T_NLOOP_DEPTH(x) ((x)->depth)
#define T_NLOOP_DIMS(x)	((x)->num_dims)
#define T_NLOOP_DIMLIST(x)((x)->dim_list)
#define T_NLOOP_MLOOP_PROXIMITY(x) ((x)->mloop_proximity)
#define T_ORDER_P(x)	((x)->order)
#define T_STRIDE_P(x)	((x)->stride)
#define T_DIRECTION_P(x)((x)->direction)

/* WDW: New macros for merged region/mask node */
#define T_REGION_SYM(x)	 (x)->region
#define T_MASK_EXPR(x)	 (x)->mask
#define T_MASK_BIT(x)	 (x)->with

/*** sungeun ***/
#define T_NLOOP_ORDER(x, y)    ((x)->order[y-1]) 

/*** sungeun ***/
#define T_NLOOP_STRIDE(x, y)   ((x)->stride[y])

/*** sungeun ***/
#define T_NLOOP_DIRECTION(x, y)  ((x)->direction[y])
   
#define T_COMM_TYPE(x)		(x)->u.comm_p->type
#define T_COMM_ID(x)		(x)->u.comm_p->communication_id
#define T_COMM_REG(x)		(x)->u.comm_p->region
#define T_COMM_INFO(x)		(x)->u.comm_p->comm_info
#define T_COMM_LHSCOMM(x)	(x)->u.comm_p->lhscomm
#define T_COMM_PREV(x)		(x)->u.comm_p->prev_comm
#define T_COMM_NEXT(x)		(x)->u.comm_p->next_comm
#define T_COMM_INFO_STRUCT(x)	(x)->comm_info
#define T_COMM_INFO_DIR(x)	(x)->directions
#define T_COMM_INFO_DIRTYPES(x) (x)->dirtypes
#define T_COMM_INFO_ENS(x)	(x)->ensemble
#define T_COMM_INFO_NEXT(x)	(x)->next_c

#define T_F_ASN_STMT(x)	((x)->label_p)	/*** is this used? ***/
#define T_F_ASN_VAR(x)	((x)->var_p)	/*** is this used? ***/

#define T_CONSTLS(x)	((x)->constls_p)

#define T_FLAG1(x)	((x)->flags.flag_1)
#define T_FLAG2(x)	((x)->flags.flag_2)
#define T_FLAG3(x)	((x)->flags.flag_3)
#define T_FLAG4(x)	((x)->flags.flag_4)

#define T_PROMOTED_FUNC(x) ((x)->flags.flag_3)

#define T_SET_FLAG(x)	((x) = TRUE)
#define T_CLR_FLAG(x)	((x) = FALSE)

#define T_IS_UNARY(x)	(((int)UNEGATIVE<= (int)(x)) && ((int)(x) <= (int)UCOMPLEMENT))
#define T_IS_BINARY(x)	(((int)BIAT <= (int)(x)) && ((int)(x) <= (int)BIASSIGNMENT))
#define T_IS_NARY(x)	(((int)ARRAY_REF <= (int)(x)) && ((int)(x) <= (int)FUNCTION))
#define T_IS_ASSIGNOP(x) ((x) == BIOP_GETS || (x) == BIASSIGNMENT)
#define T_IS_CCOMM(x)	((x) >= REDUCE && (x) <= PERMUTE)

#define	T_MAKE_LIST(x)	(T_NEXT(x) = NULL,T_PREV(x) = NULL,(x))

/* was: #define	T_ADD_LIST(x,l)	( T_MAKE_LIST(x) , \ */
/* on some compilers, this yielded a warning due to the useless (x) above */

#define T_ADD_LIST(x,l) ((T_NEXT(x) = NULL,T_PREV(x) = NULL),\
			 ((l) == NULL\
			  ? (x) \
			  : (T_PREV(l) = (x),T_NEXT(x) = (l), (x))))


#define T_SET_PARENT(l,p,t)	for ((t) = (l) ; (t) != NULL; (t) = T_NEXT(t)) { \
					T_PARENT(t) = (p); \
				}
#define T_LAST(ls,last)	for ( last = ls; T_NEXT(last) != NULL; last = T_NEXT(last)) ;

#define T_SET1(x)	((x)->tag1 = TRUE)
#define T_CLEAR1(x)	((x)->tag1 = FALSE)
#define T_IS_SET1(x)	((x)->tag1)
#define T_SET2(x)	((x)->tag2 = TRUE)
#define T_CLEAR2(x)	((x)->tag2 = FALSE)
#define T_IS_SET2(x)	((x)->tag2)
#define T_SET3(x)	((x)->tag3 = TRUE)
#define T_CLEAR3(x)	((x)->tag3 = FALSE)
#define T_IS_SET3(x)	((x)->tag3)
#define T_SET4(x)	((x)->tag4 = TRUE)
#define T_CLEAR4(x)	((x)->tag4 = FALSE)
#define T_IS_SET4(x)	((x)->tag4)

#define T_DIMS(x)	(x)->user1_p /*GHF for REDUCE/SCAN ops, dimensions to reduce */
#define T_RANK(x)       (x)->user2_p /*rea the rank of a scan or reduce op */

#define T_SETUP1(x,y)	((x)->user1_p = (int *)(y))
#define T_GETUP1(x,t)	((t)(x)->user1_p)
#define T_CMP_UP1(x,y)	((x)->user1_p == (int *)(y))


#define T_SETUP2(x,y)	((x)->user2_p = (int *)(y))
#define T_GETUP2(x,t)	((t)(x)->user2_p)
#define T_CMP_UP2(x,y)	((x)->user2_p == (int *)(y))


#define T_SETUP3(x,y)	((x)->user3_p = (int *)(y))
#define T_GETUP3(x,t)	((t)(x)->user3_p)
#define T_CMP_UP3(x,y)	((x)->user3_p == (int *)(y))


#define T_SETUP4(x,y)	((x)->user4_p = (int *)(y))
#define T_GETUP4(x,t)	((t)(x)->user4_p)
#define T_CMP_UP4(x,y)	((x)->user4_p == (int *)(y))

#define T_DATA_NLIST(x)	((x)->nlist_p)
#define T_DATA_CLIST(x)	((x)->clist_p)


#define LIST_STMT(x)	((x)->stmt_l)
#define LIST_NEXT(x)	((x)->next_l)


#define G_FUNCTION(x)           ((x)->u.function_p)
#define G_GENLIST(x)		((x)->u.genlist_p)
#define G_STATEMENT(x)		((x)->u.stmt_p)
#define G_EDGETYPE(x)		((x)->u.edge_type)
#define G_IDENT(x)              ((x)->u.ident)
#define G_SYM_VAL(x)		((x)->u.sym_exp_p)
#define G_STATUS(x)		((x)->u.status)
#define G_SET(x)		((x)->u.set_p)
#define G_EXP(x)		((x)->u.expr_p)

#define G_NEXT(x)		((x)->next_p)


#define T_ALIAS_SET(x)  ((x)->alias_p)
#endif
