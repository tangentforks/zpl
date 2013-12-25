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

#ifndef _PARSETREE_H
#define _PARSETREE_H

#include "symtab.h"
#include "struct.h"
#include "iostmt.h"
#include "const.h"
#include "masktype.h"
#include "glistmac.h"

#define NORMAL_MLOOP_FLAG            0x01
#define UNROLLED_MLOOP_FLAG          0x02
#define UNRLD_FIXUP_MLOOP_FLAG       0x04
#define TILED_OUTER_MLOOP_FLAG       0x08
#define TILED_INNER_MLOOP_FLAG       0x10
#define TILED_FIXUP_MLOOP_FLAG       0x20
#define UNRLD_TILED_INNER_MLOOP_FLAG 0x40


#define MASK_INIT  1
#define MASK_STOP  2
#define MASK_STEP  4
#define VERY_SIMPLE 7

#define L_DO_UP    1		/* iterate upward in the FOR loop */
#define L_DO_DOWN  2		/* iterate downward in the FOR loop */

typedef enum {
  SEM_NONE,        /* 0 */
  SEM_FLOW_SINK,   /* 1 */
  SEM_LOCK,        /* 2 */
  SEM_UNLOCK,      /* 3 */
  SEM_BARRIER,     /* 4 */
  SEM_FORK         /* 5 */
} semtype_t;

typedef enum {		/*** for IRONMAN comm ***/
  C_NEW=0,
  C_DR,
  C_SR,
  C_DN,
  C_SV,
  C_OLD,

  C_WRAP_PRERECV,
  C_WRAP_SEND,
  C_WRAP_RECV,
  C_WRAP_POSTSEND,

  C_IM_LAST
} commtype_t;

typedef enum {
  P_SCATTER,
  P_GATHER,
  P_PERMUTE
} permutetype_t;

typedef enum {
  N_NORMAL,
  N_SPS_INIT_COUNT_PRE,
  N_SPS_INIT_COUNT,
  N_SPS_INIT_COUNT_POST,
  N_SPS_INIT_LINK_PRE,
  N_SPS_INIT_LINK,
  N_SPS_INIT_LINK_POST
} nulltype_t;

typedef enum {
  C_NORMAL,
  C_ELSIF
} condtype_t;

typedef enum {
  AT_NORMAL,
  AT_PRIME,
  AT_WRAP,
  AT_RANDACC  /* the BIAT is randomly accessed, must use access macros */
} attype_t;

typedef enum {
  ARRAY_IND
} arrayref_t;

typedef enum {
  PLUS,
  MINUS,
  TIMES,
  DIVIDE,
  MOD,
  MIN,
  MAX,
  BAND,
  BOR,
  BXOR,
  AND,
  OR,
  USER
} binop_t;

#define MLOOP_NORMAL 0x01
#define MLOOP_MSCAN  0x02
#define MLOOP_FUSED  0x80

#define MLOOP_IS_NORMAL(stmt) (T_SUBTYPE(stmt)&MLOOP_NORMAL)
#define MLOOP_IS_MSCAN(stmt)  (T_SUBTYPE(stmt)&MLOOP_MSCAN)
#define MLOOP_IS_FUSED(stmt)  (T_SUBTYPE(stmt)&MLOOP_FUSED)

#define MLOOP_FUSED_SUBTYPE(s1,s2) (T_SUBTYPE(s1) | T_SUBTYPE(s2) | MLOOP_FUSED)


/*** SEE include/Cname.h FOR A BETTER IDEA OF WHAT THE STATEMENTS ARE ***/
/*** The character array StmtName[] should correspond directly to this ***/
/*** ANY CHANGES MADE HERE MUST BE MADE THERE ALSO ***/
typedef	enum	{
	S_NULL=0,
	S_EXIT,
	S_COMM,
	S_COMPOUND,
	S_MSCAN,
	S_EXPR,
	S_IF,
	S_LOOP,
	S_RETURN,
	S_IO,
	S_END,
	S_REGION_SCOPE,
	S_MLOOP,
	S_NLOOP,
	S_WRAP,
	S_REFLECT,
	S_HALT,
	S_CONTINUE,
	S_ZPROF              /* MDD */
} stmnttype;


/*** SEE include/Cname.h FOR A BETTER IDEA OF WHAT THE EXPRESSIONS ARE ***/
/*** The character array OpName[] should correspond directly to this ***/
/*** SEE util/expr.c for the relative precedence levels ***/
/*** The priority array expr_precedence[] should correspond directly to 
     this, as should expr_associative[] and expr_commutative[]***/
/*** ANY CHANGES MADE HERE MUST BE MADE THERE ALSO ***/
typedef	enum	{
  NULLEXPR,
  SBE,
  VARIABLE,
  CONSTANT,
  SIZE,
  GRID,
  DISTRIBUTION,
  UNEGATIVE,
  UPOSITIVE,
  NUDGE,
  REDUCE,
  SCAN,
  FLOOD,
  PERMUTE,
  UCOMPLEMENT,
  BIAT,
  BIPLUS,
  BIMINUS,
  BITIMES,
  BIDIVIDE,
  BIMOD,
  BIEXP,
  BIOP_GETS,
  BIGREAT_THAN,
  BILESS_THAN,
  BIG_THAN_EQ,
  BIL_THAN_EQ,
  BIEQUAL,
  BINOT_EQUAL,
  BILOG_AND,
  BILOG_OR,
  BIASSIGNMENT,
  BIDOT,
  BIPREP,
  BIWITH,
  ARRAY_REF,
  FUNCTION
} exprtype;

typedef	enum	{
  L_DO,
  L_WHILE_DO,
  L_REPEAT_UNTIL
} looptype;


typedef enum	{
  E_ENDDO,
  E_ENDIF
} endtype;

typedef enum {
  ZPROF_COMMSTAT
} zproftype;

typedef enum {
  START_COMM_TIMING,
  START_COMM,
  END_COMM,
  END_COMM_TIMING,
  PRINT_COMM_TIMING
} commstattype;

struct	module_struct	{
	structtype	st_type;	
	symboltable_t	*decl_p;	
	function_t	*fcnls_p;	
	module_t	*next_p;
};

struct	function_struct	{
	structtype	st_type;	
	char		tag1, tag2;
	symboltable_t	*function_p;	
	semtype_t       semantics;
	symboltable_t	*decl_p;	
	genlist_t	*paramls_p;	
	genlist_t       *glob_alias_p;
	genlist_t       *invocations;
	statement_t	*stmtls_p;
	statement_t	*firstdo_p;	
	function_t	*prev_p;
	function_t	*next_p;
	callgraph_t	*cgnode_p;	
	genlist_t	*loops_p;	
	reflist_t	*global_refs;
	summarylist_t	*global_mods;
	summarylist_t	*global_uses;
	int		is_summed;	
	genlist_t	*flow;		
#ifndef BLAH
 	genlist_t	*loopnodes;
#endif
	int             parallel;
	module_t        *module_p;
  unsigned int    reachable;           /* reachable from main */
  statement_t     *unc_stmt[MAXRANK+1];  /* BLC point to first unc stmt */
  expr_t          *unc_fn[MAXRANK+1];  /* BLC point to first unc fncall */
  int             lo_comm_id;    /* BLC: used in I-P ironman commid */
  short int	cgposition;	/* ECL: position in call graph */
  int             numparams;      /* BLC: number of parameters */
  callsite_t      *callinfo;      /* BLC: info about calls */

  function_t *overload_prev; /* pointer to other overloaded definitions */
  function_t *overload_next
; /* pointer to other overloaded definitions */
};


struct	statement_struct	{
	structtype	st_type;	
	char		tag1, tag2, tag3, tag4;
	int		*user1_p;	
	int		*user2_p;	
	int             *user3_p;
	int             *user4_p;

	stmnttype	type;
	int             subtype;
	int		lineno;	
        char            *filename_p;
	symboltable_t	*label_p;
	union	{
		compound_t	*compound_p;	
		expr_t		*expr_p;	
						
		if_t		*if_p;		
		loop_t		*loop_p;	
		expr_t		*return_p;	
		io_t		*io_p;		
		statement_t     *halt_p;
		endtype		end_p;		

		wrap_t		*wrap_p;	/*GHF these ones are new */
		region_t	*region_p;
		mloop_t		*mloop_p;
		nloop_t		*nloop_p;
		comm_t		*comm_p;	/*** for Ironman comm ***/
		zprof_t         *zprof_p;       /*** for profiling ***/
	} u;
	semtype_t       semantics;
	statement_t	*parent_p;	
	function_t	*parfcn_p;
	statement_t	*prev_p;	
	statement_t	*next_p;	
	statement_t	*prev_lex_p;	
	statement_t	*next_lex_p;	
 	set_t		*inset_p;	
 	set_t		*outset_p;	
        glist	        live_p;
	genlist_t       *func_calls_p;
	int             parallel;
	unsigned int    iron_cid;
	statement_t	*scope;
	outdep_t	*outdep_p;
	indep_t		*indep_p;
	statement_t     *mloop_p;   /* mloop statement this stmt is in */
	int             visited; /*echris*/
	genlist_t       *pre;
	genlist_t       *post;
        int             shard;
        int             nogen;
};



struct	wrap_struct	{			/*GHF for Wrap and Reflect */
	expr_t	        *expr_p;		/* a list of ensemble vars 
						 (possibly including array 
						 accesses and records). */
	int             rank;                    /* rank of wrap.  This
						   is extracted from expr_p,
						   whether the region is
						   known at compiletime or not*/
	symboltable_t	*region;		/* of-region for all vars */
	int		send;			/* S_WRAP=> send/recv */
	int		cid;			/* S_WRAP=> uniq comm ID */
};

struct	region_struct	{			/*WDW: New region node*/
	expr_t          *region;		/* a region */
	expr_t		*mask;			/* a mask expression */
	masktype	with;			/* mask bit */
	statement_t	*body_p;		/* body of region*/
};


typedef struct _replacement {
  expr_t* origexpr;
  expr_t* newexpr;
  struct _replacement* next;
} replacement_t;


struct	mloop_struct	{			/*GHF for Multi-Loops */
  /* CORE MLOOP FIELDS */
  int		rank;			/* rank of MLOOP */
  expr_t	*loop_region;		/* a single region */
  int		with;			/* with or without mask */
  expr_t        *loop_mask;		/* mask */
  statement_t	*body_p;		/* compound statement list */
  int 		order[MAXRANK];         /* order of iteration */
  int           flat[MAXRANK];          /* is this dim flat? */
  int 		direction[MAXRANK];     /* iteration directions */
  int 		tile_order[MAXRANK];    /* order of inter-tile iteration */
  int 		tile_direction[MAXRANK];/* iteration inter-tile directions */
  int		tile_nopeel;		/* generate concise (no peel) tiles? */
  dvlist_t	constraints;		/* list of distance vectors */
  
  /* NEW GENERALIZED MLOOP FIELDS */
  genlist_t*      vars[MAXRANK+2];           /* vars local to scope/loop */
  statement_t*    prepre_stmts[MAXRANK+1];   /* statements before loop */
  statement_t*    pre_stmts[MAXRANK+1];      /* stmts at top of loop body */
  statement_t*    post_stmts[MAXRANK+1];     /* stmts at bot of loop body */
  statement_t*    postpost_stmts[MAXRANK+1]; /* statements after loop */
  replacement_t*  replacements;
  int             unroll[MAXRANK];
  int             tile[MAXRANK];
  expr_t*	  tile_expr[MAXRANK];
  
  /* computed during MLOOP generation */
  elist wblist;
  rlist reglist;
};


struct nloop_struct	{		/* sungeun */
  statement_t	*body_p;		/* statement list */

  int		depth;			/* static depth of nloop */
  int		num_dims;		/* number of dimensions */
  dimension_t	*dim_list;		/* list of dimensions */
  int		*order;			/* order of iteration */
  int		*direction;		/* iteration directions */
  int		*stride;		/* UNUSED *** stride for each it. */
  int		mloop_proximity;	/* proximity of nloop rel. to mloop */
};

struct comm_info_struct {
  genlist_t *directions;	/*** directions, e.g. east and east2 ***/
  genlist_t *dirtypes;          /*** types -- @ vs. @^... ***/
  expr_t *ensemble;		/*** ensemble expression ***/

  comm_info_t *next_c;
};

struct comm_struct {
  commtype_t type;
  int communication_id;

  expr_t *region;
  comm_info_t *comm_info;

  /*** for lhs @'s ***/
  /*** maybe move this into comm_info_struct if you ***/
  /***  eventually want to combine rhs @ and lhs @ ***/
  int lhscomm;

  /*** for chaining together a set of comms ***/
  statement_t *prev_comm;
  statement_t *next_comm;
};

struct zprof_struct {
  zproftype type;

  union {
    commstattype commstat;
  } u;
  
};

struct	compound_struct	{
	structtype	st_type;	
	char		tag1, tag2;

	symboltable_t	*decl_p;
	statement_t	*stmtls_p;
};


struct expr_struct {
  structtype	st_type;	
  char		tag1, tag2;
  int		*user1_p;	
  int		*user2_p;	
  int           dummy;      /* if it's a dummy, don't make things like Walkers, ... */

  struct {
    unsigned flag_1 : 1; /* used in parse pass to distinguish control strings */
    unsigned flag_2 : 1;
    unsigned flag_3 : 1; /* used to flag promoted functions, see T_PROMOTED_FUNC */
    unsigned flag_4 : 1;
  } flags;

  exprtype	 type;
  int            subtype;
  int            free;
  datatype_t*    typeinfo;
  expr_t*        typeinfo_reg;
  expr_t*        expr_p;
  
  symboltable_t* symtab_p;
  region_t*      reg_p;
  region_t*      reg2_p;
  expr_t*        maplist;
  int            mapsize;

  dimension_t*   sbe_info;

  int            nudge[MAXRANK]; /* nudge expression vector */

  int            instance_num;  /* inst. # of vars with mult. instances */
  
  statement_t*   stmt_p;
  expr_t*        parent_p;	
  expr_t*        prev_p;	
  expr_t*        next_p;

  expr_t*        meaning_p;
  int           need_ind[MAXRANK];   /* need indices? */
  int           need_next[MAXRANK];  /* for optimizing sparse region storage */
  int           need_prev[MAXRANK];
  int           need_dense_dir[MAXRANK];
};


struct	if_struct	{
	structtype	st_type;	
	char		tag1, tag2;

	expr_t		*cond_p;
	union	{
		struct	{	
			int             PDP;
			statement_t	*then_p;
			statement_t	*else_p;
		} if_then_else;

	} u;
};



struct	loop_struct	{
  structtype	st_type;	
  char		tag1, tag2;

  looptype	type;
  statement_t	*body_p;
  union	{
    struct	{	
      int		start_val; 
      int		stop_val; 
      int		step_val; 
      int             up_down;       /* iterate up or down? */
      expr_t     	*var_p;	
      symboltable_t	*enddo_p; 
      expr_t		*start_p;
      expr_t		*stop_p;
      expr_t		*step_p;	
      statement_t	*nextdo_p;	
      symboltable_t	*decl_p;	
      statement_t	*initstmt_p;	
      statement_t	*poststmt_p;	
    } fixed_iteration_loop;
    struct  {
      expr_t          *cond_p;
    } generic_loop;
  } u;
};

struct stmtlist_struct {
  statement_t *stmt_l;
  stmtlist_t  *next_l;
};

#endif

