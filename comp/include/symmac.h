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



#ifndef	SYMMAC_H
#define	SYMMAC_H

#include "symtab.h"
#include "symbol.h"


#define	STRUCTTYPE(x)	(x)->st_type
#define	S_NEXT(x)	(x)->next


#define	HE_IDENT(x)	(x)->ident
#define HE_SYM(x)	(x)->pst
#define HE_LAST(x)	(x)->last
#define HE_NEXT(x)      (x)->he_next


#define	S_LEVEL(x)	(x)->level
#define S_PHE(x)	(x)->phe	
#define S_IDENTLHS(x)	(x)->ident
#define	S_IDENT(x)	(x)->ident
#define	S_PARENT(x)	(x)->parent
#define	S_SIBPREV(x)	(x)->sib_prev
#define	S_SIBLING(x)	(x)->sib_next
#define	S_CLASS(x)	(x)->sclass
	
#define S_INLIST(x)	(x)->inlist 
#define S_OUTLIST(x)	(x)->outlist

#define S_STRIP(x)	((x) + 1)


#define	S_INACTIVE	0	
#define	S_ACTIVE	1	
#define	S_FG_ACTIVE(x)	(x)->flags.active_bit

#define	S_FG_SHADOW(x)	(x)->flags.shadow_bit
#define	S_FG_TYPE(x)	(x)->flags.type_bit
#define	S_FG_DELETE(x)	(x)->flags.delete_bit
#define	S_FG_ENUM(x)	(x)->flags.equiv_bit	
#define S_FG_EQUIV(x)	(x)->flags.equiv_bit
#define	S_FG_PARAM(x)	(x)->flags.param_bit
#define	S_FG_1(x)	(x)->flags.flag_1
#define	S_IS_PROGRAM(x)	(x)->flags.flag_1
#define	S_FG_2(x)	(x)->flags.flag_2
#define	S_IS_SUBRTN(x)	(x)->flags.flag_2
#define	S_FG_3(x)	(x)->flags.flag_3
#define	S_IS_FUNC(x)	(x)->flags.flag_3
#define	S_FG_4(x)	(x)->flags.flag_4
#define	S_IS_BLOCK(x)	(x)->flags.flag_4
#define	S_FG_5(x)	(x)->flags.flag_5
#define	S_IS_STFUNC(x)	(x)->flags.flag_5
#define S_IS_USED(x)	(x)->flags.used
#define	S_FG_6(x)	(x)->flags.future_6
#define	S_FG_7(x)	(x)->flags.future_7
#define	S_FG_8(x)	(x)->flags.future_8
#define	S_IS_INT(x)	(x)->flags.future_8
#define	S_FG_9(x)	(x)->flags.future_9
#define S_NO_SETUP(x)   (x)->flags.future_9
#define	S_FG_0(x)	(x)->flags.future_0
#define	S_FG_OTHER(x)	(x)->flags.future_0

#define	S_STYPE(x)	(x)->storagetype
#define S_INIT(x)	(x)->init_list
#define	S_SUBCLASS(x)	(x)->sub_class

/*GHF For S_VARIABLE declarations only:
 *
 * S_VAR_STYPE() has the SC_CONFIG bit set for config variables.
 *
 * For ensembles only:
 * int * S_VAR_LO_FLUFF() == minimum fluff used in each dimension
 * int * S_VAR_HI_FLUFF() == maximum fluff used in each dimension
 * int * S_VAR_LO_IREG() == minimum fluff used in each dimension
 * int * S_VAR_HI_IREG() == maximum fluff used in each dimension
 *     If these are NULL, then 0 fluff is used.
 *     If non-null, they point to an array of MAXRANK integers.
 *
 */
#define	S_IS_ENSEMBLE(x)     (D_IS_ENSEMBLE(S_DTYPE(x)))
#define	S_VAR_INIT(x)	(x)->init_list
#define	S_VAR_VCLASS(x)	(x)->sub_class

#define LO 0
#define HI 1

#define S_FLUFF(x,d,lh) ((x)->fluff[d][lh])
#define S_FLUFF_LO(x,d) ((x)->fluff[d][LO])    /* rea */
#define S_FLUFF_HI(x,d) ((x)->fluff[d][HI])    /* rea */
#define S_UNK_FLUFF_LO(x,d) ((x)->unknown_fluff[d][LO])
#define S_UNK_FLUFF_HI(x,d) ((x)->unknown_fluff[d][HI])
#define S_UNK_FLUFF(x,d,lh) ((x)->unknown_fluff[d][lh])
#define S_WRAPFLUFF(x,d,lh) ((x)->wrap_fluff[d][lh])
#define S_WRAPFLUFF_LO(x,d) ((x)->wrap_fluff[d][LO])
#define S_WRAPFLUFF_HI(x,d) ((x)->wrap_fluff[d][HI])
#define S_MASK(x)	  ((x)->mask) /* wdw*/
/*** true if statement is in standard context ***/
#define S_STD_CONTEXT(x)  ((x)->std_context)	/*** sungeun ***/
#define S_LINENO(x)  	  ((x)->lineno)
#define S_FILENAME(x)	  ((x)->filename_p)
#define S_SETUP(x)        ((x)->setup)
#define S_ANON(x)         ((x)->anon)


#define S_ACTUALS(x)    ((x)->actuals)


/*GHF For S_REGION declarations only: 
 *
 * regionclass S_REG_CLASS() = which kind of region declaration:
 *
 *    case OF_REGION:		e.g.  region East = [east of R];
 *	symboltable_t * S_REG_DIR() = direction declaration for "east"
 *	symboltable_t * S_REG_REG() = region declaration for "R"
 *
 *    case AT_REGION:		e.g.  region East = [R@east];
 *	symboltable_t * S_REG_DIR() = direction declaration for "east"
 *	symboltable_t * S_REG_REG() = region declaration for "R"
 *
 *    case SIMPLE_REGION:	e.g.  region R = [1..N, 1..N];
 * 	datatype_t * d = S_REG_TYPE() = just like an array declaration
 *	int D_REG_NUM(d) = # of dimensions
 * 	dimension_t * D_REG_DIM(d) = list of dimensions, just like an array decl
 *
 * At some point during compilation, the S_DTYPE() could be filled in for
 * regions derived with "of".
 */
#define S_REG_MASK(x)   ((x)->mask_expr)         /* for regions only */
#define S_SPARSITY(x)   ((x)->mask_expr)         /* want to switch to this */
#define S_REG_WITH(x)   ((x)->mask_with)

#define S_NUM_INSTANCES(x) ((x)->numinstances)
#define S_INSTANCE_LOOP(x) ((x)->inst_loop_num)
#define S_PROHIBITIONS(x)  ((x)->nogen)

#define D_REG_NUM(x)	((x)->u.region.num_dim)
#define D_REG_DIM_TYPE_V(x) ((x)->u.region.dimtype)
#define D_REG_DIM_TYPE(x,d) ((x)->u.region.dimtype[d])
#define D_REG_DIST(x)    ((x)->u.region.distribution)
#define D_REG_GRID(x)    (D_DIST_GRID(D_REG_DIST(x)))
#define D_REG_NEED_IND(x,d)       ((x)->u.region.need_ind[d])
#define D_REG_NEED_NEXT(x,d)      ((x)->u.region.need_next[d])
#define D_REG_NEED_PREV(x,d)      ((x)->u.region.need_prev[d])
#define D_REG_NEED_DENSE_DIR(x,d) ((x)->u.region.need_dense_dir[d])

#define D_DIST_GRID(x)     ((x)->u.distribution.grid)
#define D_DIST_NUM(x)     ((x)->u.distribution.num_dim)
#define D_DIST_DIMS_V(x)  ((x)->u.distribution.dims)
#define D_DIST_DIMS(x, d) ((x)->u.distribution.dims[d])

#define D_GRID_NUM(x)     ((x)->u.grid.num_dim)
#define D_GRID_DIMS_V(x)  ((x)->u.grid.griddims)
#define D_GRID_DIMS(x, d) ((x)->u.grid.griddims[d])

#define	S_DTYPE(x)	(x)->type

#define S_IS_CONSTANT(x)  (S_STYPE(x) & SC_CONSTANT)

#define	S_CON_TYPE(x)	(x)->type
#define	S_CON_NAME(x)	((x)->init_list != NULL && !IN_BRA((x)->init_list) \
			 ? IN_VAL((x)->init_list) : NULL )

#define	S_FUN_TYPE(x)	D_FUN_TYPE((x)->type)
#define	S_FUN_BODY(x)	D_FUN_BODY((x)->type)

#define S_PAR_TYPE(x)	(x)->type
#define S_PAR_CLASS(x)	(x)->sub_class

#define S_STRUCT(x)	(x)->type

#define S_VARI(x)	(x)->type

#define S_COM_CLASS(x)	(x)->sub_class
#define S_COM_TYPE(x)	(x)->type
#define S_COM_BITLHS(x)	(x)->init_list
#define S_COM_BIT(x)	((x)->init_list==NULL ? 0:IN_BRA((x)->init_list))

#define S_EVALUE(x)	(x)->offset
#define S_EINIT(x)	(x)->sub_class

#define D_NAME(x)	(x)->pstName
#define	D_CLASS(x)	(x)->sclass
#define	D_IS_ENSEMBLE(x) ((x)->sclass == DT_ENSEMBLE)	/*GHF*/
/*** sungeun *** match any ensemble ***/
#define	D_IS_ANY_ENSEMBLE(x) (((x)->sclass == DT_ENSEMBLE) || \
			      ((x)->sclass == DT_GENERIC_ENSEMBLE))

#define D_ENS_REG(x)	(x)->u.ensemble.region
#define	D_ENS_TYPE(x)	(x)->u.ensemble.type
#define	D_ENS_NUM(x)	(x)->u.ensemble.num_dim
#define D_ENS_DNR(x)    (x)->u.ensemble.do_not_register
#define D_ENS_COPY(x)   (x)->u.ensemble.copyfn

#define D_DIR_NUM(x)    (x)->u.direction.num_dim
#define D_DIR_SIGN_V(x) (x)->u.direction.sign
#define D_DIR_SIGN(x,d) (x)->u.direction.sign[d]

#define	D_ARR_TYPE(x)	(x)->u.array.type
#define	D_ARR_NUM(x)	(x)->u.array.num_dim
#define	D_ARR_DIM(x)	(x)->u.array.dim_list
#define D_ARR_ATYPE(x)	(x)->u.array.array_type

#define D_FUN_BODY(x)	(x)->u.function.body
#define D_FUN_TYPE(x)	(x)->u.function.return_type

#define	D_STRUCT(x)	(x)->u.record.list
#define D_STAG(x)	(x)->u.record.tag


#define	DIM_TYPE(x)	(x)->dimtype
#define	DIM_LO(x)	(x)->lo_var
#define DIM_HI(x)       (x)->hi_var
#define DIM_NEXT(x)     (x)->nextdim


#define SYM_IDENT(x)	((x)->entry)
#define SYM_NEXT(x)     ((x)->next_p)

#define	VAR_TAG(x)	(x)->tag
#define	VAR_COMP(x)	(x)->component



#define	IN_BRA(x)	(x)->brace
#define	IN_REP(x)	(x)->repeat
#define	IN_VAL(x)	(x)->u.value
#define	IN_LIS(x)	(x)->u.list
#define	IN_IVAL(x)	(x)->u.ivalue


#define	DT_PDT(x)	(x)->pdt


#define	DC_LIST(x)	(x)->list
#define DC_SC(x)	(x)->psc

#define LU_INS(x)		lu_insert(x)

/* Value Macros */

#define V_DIR_VECT_V(x) (x)->u.direction.dir_vec
#define V_DIR_VECT(x,d) (x)->u.direction.dir_vec[d]

#define V_GRID_VECT_V(x) (x)->u.grid.grid_vec
#define V_GRID_VECT(x,d) (x)->u.grid.grid_vec[d]

#define V_DIST_SBE(x) (x)->u.distribution.sbe

#define V_REG_DIM(x) (x)->u.region.dim_list

#endif
