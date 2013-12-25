/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/buildstmt.h"
#include "../include/error.h"
#include "../include/parsetree.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/global.h"
#include "../include/stmtutil.h"
#include "../include/db.h"
#include "../include/glist.h"
#include "../include/glistmac.h"
#include "../include/expr.h"
#include "../include/datatype.h"
#include "../include/rmstack.h"
#include "../include/dimension.h"
#include "../include/statement.h"
#include "../include/symboltable.h"
#include "../include/Cgen.h"
#include "../include/traverse.h"
#include "../include/typeinfo.h"
#include "../include/passes.h"

#define _T_IS_INTEGER(x) ((x) == pdtINT || (x) == pdtSHORT || (x) == pdtLONG ||\
			  (x) == pdtSIGNED_BYTE || (x) == pdtUNSIGNED_BYTE || \
			  (x) == pdtUNSIGNED_INT || (x) == pdtUNSIGNED_LONG || \
			  (x) == pdtUNSIGNED_SHORT || (x) == pdtBOOLEAN)

/* typechecking globals */
static int shard_rank = 0;
static int free_control_flow = 0;
static int functionParam = 0;
static expr_t *LHS_ArrayRef = NULL;
static glist Seen_Array_Refs;
static int on_lhs=0;
static int did_flood_check=0;
static int in_reduce=0;
static int in_mscan=0;

/* typechecking prototypes */

static void typecheck_exprls(expr_t*);
static void typecheck_stmtls(statement_t*);

static void typecheck_preserve_destroy(expr_t*);


/** this should probably go elsewhere **/
/** okay = 1 if the transform works, initially it should be zero
    expr is an array reference, but shouldn't be
    unless reg is :: **/
static expr_t* randacc_transform(expr_t* reg, expr_t* expr, int* okay) {
  int i;
  int indices;
  int mapsize;
  expr_t* tmp;
  expr_t* map;

  /** let's start with just constants **/
  if (T_TYPE(reg) != CONSTANT) {
    return expr;
  }

  if (!T_TYPEINFO_REG(T_OPLS(expr))) {
    return expr;
  }

  indices = 0;
  for (i = 0; i < D_REG_NUM(T_TYPEINFO(reg)); i++) {
    if ((D_REG_DIM_TYPE(T_TYPEINFO(reg), i) == DIM_GRID) &&
	(D_REG_DIM_TYPE(T_TYPEINFO(T_TYPEINFO_REG(expr)), i) != DIM_GRID)) {
      indices++;
    }
  }
  mapsize = indices;
  for (tmp = T_NEXT(T_OPLS(expr)); tmp != NULL; tmp = T_NEXT(tmp)) {
    indices--;
  }

  if (indices != 0) {
    return expr;
  }

  *okay = 1;
  tmp = T_NEXT(T_OPLS(expr));
  map = NULL;
  for (i = 0; i < D_REG_NUM(T_TYPEINFO(reg)); i++) {
    if ((D_REG_DIM_TYPE(T_TYPEINFO(reg), i) == DIM_GRID) &&
	(D_REG_DIM_TYPE(T_TYPEINFO(T_TYPEINFO_REG(expr)), i) != DIM_GRID)) {
      map = cat_expr_ls(map, copy_expr(tmp));
      tmp = T_NEXT(tmp);
    }
    else {
      map = cat_expr_ls(map, copy_expr(build_typed_0ary_op(CONSTANT, pstINDEX[i+1])));
    }
  }
  expr = replace_expr(expr, copy_expr(T_OPLS(expr)));
  T_MAP(expr) = map;
  T_MAPSIZE(expr) = mapsize;
  return expr;
}


static void CheckFloodability(expr_t *expr,char *type,int lhs,int linenum) {
  int numdims;
  int i;
  expr_t *ensreg;
  int indexirank;
  expr_t* uat;

  uat = expr_find_at_up(expr);
  if (uat == NULL) {
    uat = expr_find_at(expr);
  }
  if (uat != NULL && T_SUBTYPE(uat) == AT_RANDACC) {
    return;
  }
  ensreg = T_TYPEINFO_REG(expr);
  if (ensreg == NULL || T_SUBTYPE(ensreg) == INTERNAL_REGION) {
    return;
  }
  numdims = D_REG_NUM(T_TYPEINFO(ensreg));
  indexirank = expr_is_indexi(expr);
  if (indexirank == 0) {
    for (i=0;i<numdims;i++) {
      if (D_REG_DIM_TYPE(T_TYPEINFO(ensreg), i) != DIM_INHERIT) {
	if (D_REG_DIM_TYPE(T_TYPEINFO(ensreg), i) == DIM_FLOOD) {
	  if (RMSRegDimFloodable(i,1)) {
	  } else {
	    if (lhs) {
	      USR_FATAL_CONT(T_STMT(expr), "Region/%s floodability mismatch "
			     "(dimension %d)",type,i+1);
	    }
	  }
	} else {
	  if (RMSRegDimFloodable(i,0)) {
	    USR_FATAL_CONT(T_STMT(expr), "Region/%s floodability mismatch "
			   "(dimension %d)",type,i+1);
	  } else {
	  }
	}
      }
    }
  } else {  /* Indexi variable -- this is hackish, as we don't have good
	     region information for Indexi */
    if (T_TYPE(T_STMT(expr)) == S_EXPR) {
      if (T_TYPE(T_EXPR(T_STMT(expr))) == FUNCTION) {
        if ((strcmp(S_IDENT(T_IDENT(T_OPLS(T_EXPR(T_STMT(expr))))), "_DESTROY") == 0) ||
	    (strcmp(S_IDENT(T_IDENT(T_OPLS(T_EXPR(T_STMT(expr))))), "_PRESERVE") == 0)) {
	  return;
	}
      }
    }

    for (i=0;i<numdims;i++) {
      if (i+1 != indexirank) {  /* floodable dimension */
	if (RMSRegDimFloodable(i,1)) {
	} else {
	  if (lhs) {
	    USR_FATAL_CONT(T_STMT(expr), "Region/%s floodability mismatch "
			   "(dimension %d)",type,i+1);
	  }
	}
      } else {  /* nonfloodable dimension */
	if (RMSRegDimFloodable(i,0)) {
	  USR_FATAL_CONT(T_STMT(expr), "Region/%s floodability mismatch "
			 "(dimension %d)",type,i+1);
	} else {
	}
      }
    }	
  }
}


/*
 *  typecheck_reg_limit
 *    Check upper or lower element of a region dimension expression
 *    to make sure valid
 */
#ifdef DEADCODE
static void typecheck_reg_limit(expr_t *limit_expr,char *region_name,
				int dyn_reg,statement_t *stmt) {
  if ( region_name == NULL ) {
    region_name = "unknown";
  }

  if (stmt == NULL) {
    stmt = T_STMT(limit_expr);
  }

  /*  
   *  Traverse through expression until get to an atom
   */
 
  if (!datatype_scalar(T_TYPEINFO(limit_expr)) || T_TYPEINFO_REG(limit_expr)) {
    if (dyn_reg || 
	(strncmp(region_name, "_AnonReg", strlen("_AnonReg")) == 0)) {
      USR_FATAL_CONT(stmt,"Non-scalar used in region definition");
    } else {
      USR_FATAL_CONT(stmt,"Non-scalar used in region definition for '%s'.",
		     region_name);
    }
  }
  /*
  if (!dyn_reg && !expr_rt_const(limit_expr)) {
    USR_FATAL_CONT(stmt,
		   "Use of non-runtime constant in region definition for '%s'.",
		   region_name);
		   }*/
}
#endif


/*
 *  typecheck_region_dim
 *    Check the dimensions of a region definition
 */

static void typecheck_region_dim(expr_t *stp,int dyn_reg,
				 statement_t *stmt) {
  datatype_t *type;
  /*  expr_t *up_var, *low_var;*/
  int numdims;
  int i;
  
  type = T_TYPEINFO( stp );
  if (type == NULL) {
    return;
  }

  numdims = D_REG_NUM(type);
  
  /*
   *  Step through each dimension definition of the region
   */
  for (i=0; i<numdims; i++) {
    switch (D_REG_DIM_TYPE(type, i)) {
    case DIM_RANGE:
      INT_FATAL(NULL, "Trying to check a region's dimension value");
      /*      up_var = DIM_HI( list_p );
      typecheck_reg_limit( up_var, S_IDENT(T_IDENT(stp)),dyn_reg,stmt);
      */
      /* fall through */
    case DIM_FLAT:
      /*
      low_var = DIM_LO( list_p );
      typecheck_reg_limit( low_var, S_IDENT(T_IDENT(stp)),dyn_reg,stmt);
      */
      break;
    case DIM_GRID:
    case DIM_FLOOD:
      break;
    case DIM_INHERIT:
      if (dyn_reg == 0) {
	USR_FATAL_CONT(stmt,"Can't use blank dimensions in definition of "
		       "static region %s", S_IDENT(T_IDENT(stp)));
      }
      break;
    default:
      INT_FATAL(stmt, "Unknown dimension type\n");
    }
  }
}


/*
 *  static int typecheck_basic()
 *  
 *  Return 1 if the passed type is a basic type of ZPL
 *  Return 0 otherwise
 */
static int typecheck_basic(datatype_t *tinfo ) {
  int retVal;

  if ( tinfo == NULL ) {
    return 0;
  }

   /*
    *  Recurse through ensemble and array types to find
    *  the base type for the array or ensemble.
    */
  if ( D_IS_ENSEMBLE(tinfo) ) {
    return typecheck_basic( D_ENS_TYPE(tinfo) );
  }
 
  if ( D_CLASS(tinfo) == DT_ARRAY ) {
    return typecheck_basic( D_ARR_TYPE(tinfo) );
  }
 
  retVal = 0;

  switch( D_CLASS(tinfo) ) {
  case DT_BOOLEAN:
    if ( strict_check ) 
      break;
  case DT_INTEGER:
  case DT_SHORT:
  case DT_LONG:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_LONG:
  case DT_REAL:
  case DT_DOUBLE:
  case DT_QUAD:
  case DT_CHAR:	
  case DT_UNSIGNED_BYTE:
  case DT_SIGNED_BYTE:
  case DT_VOID:
  case DT_GENERIC:
  case DT_GENERIC_ENSEMBLE:
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
    retVal = 1;
    break;
    
  default:
    retVal = 0;
    break;
  }

  return retVal;
}


static typeclass typeequiv_strict(datatype_t *tinfo ) {
  typeclass sclass, retclass;

  /*
   *  Get datatype class from datatype info
   *  and return the more general class which this 
   *  datatype fits into.
   *  Include DT_PROCEDURE so that passing a procedure to an external
   *    function will not be caught by typecheck.  If a procedure is
   *    passed to a ZPL function another part of the compiler catches
   *    this (the same applies to passing regions)
   */
  sclass = D_CLASS( tinfo );

  retclass = sclass;

  switch( sclass ) {
  case DT_FILE:
    retclass = DT_INTEGER;
  case DT_INTEGER:
    break;
  case DT_REAL:
    break;
  case DT_BOOLEAN:	
    break;
  case DT_CHAR:	
    break;
  case DT_SHORT:
    break;
  case DT_LONG:
    break;
  case DT_DOUBLE:
    break;
  case DT_QUAD:
    break;
  case DT_UNSIGNED_INT:
    break;
  case DT_UNSIGNED_SHORT:
    break;
  case DT_UNSIGNED_LONG:
    break;
  case DT_UNSIGNED_BYTE:
    break;
  case DT_SIGNED_BYTE:
    break;
  case DT_GENERIC:
    break;
  case DT_GENERIC_ENSEMBLE:
    break;
    
  case DT_VOID:
  case DT_PROCEDURE:
    retclass = DT_VOID;
    break;
    
  default:
    retclass = sclass;
    break;
  }

  return retclass;
}


static typeclass typeequiv(datatype_t *tinfo) {
  typeclass sclass, retclass;

  /*
   *  Get datatype class from datatype info
   *  and return the more general class which this 
   *  datatype fits into.
   *  Include DT_PROCEDURE so that passing a procedure to an external
   *    function will not be caught by typecheck.  If a procedure is
   *    passed to a ZPL function another part of the compiler catches
   *    this (the same applies to passing regions)
   */
  sclass = D_CLASS( tinfo );

  retclass = sclass;

  switch( sclass ) {
  case DT_REGION:
  case DT_FILE:
  case DT_INTEGER:
  case DT_REAL:
  case DT_BOOLEAN:	
  case DT_CHAR:	
  case DT_SHORT:
  case DT_LONG:
  case DT_DOUBLE:
  case DT_QUAD:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_LONG:
  case DT_UNSIGNED_BYTE:
  case DT_SIGNED_BYTE:
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
    retclass = DT_INTEGER;
    break;
  case DT_GENERIC:
    break;
  case DT_GENERIC_ENSEMBLE:
    break;
    
  case DT_VOID:
  case DT_PROCEDURE:
    retclass = DT_VOID;
    break;
    
  default:
    retclass = sclass;
    break;
  }
  
  return retclass;
}


/*
 *  typeequiv_literal_strict
 *     This function is idential in nature to typeequiv() except it is
 *     used in testing compatibility between literals on the RHS and 
 *     the object on the LHS.
 */
static typeclass typeequiv_literal_strict(datatype_t *tinfo ) {
  typeclass sclass, retclass;

  sclass = D_CLASS( tinfo );

  retclass = sclass;

  switch( sclass ) {
  case DT_FILE:
  case DT_INTEGER:
  case DT_SHORT:
  case DT_LONG:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_LONG:
    retclass = DT_INTEGER;
    break;
  case DT_REAL:
  case DT_DOUBLE:
  case DT_QUAD:
    retclass = DT_REAL;
    break;
  case DT_BOOLEAN:	
    break;
  case DT_CHAR:	
  case DT_UNSIGNED_BYTE:
  case DT_SIGNED_BYTE:
    retclass = DT_CHAR;
    break;
  case DT_GENERIC:
    break;
  case DT_GENERIC_ENSEMBLE:
    break;
    
  case DT_VOID:
  case DT_PROCEDURE:
    retclass = DT_VOID;
    break;
    
  default:
    retclass = sclass;
    break;
  }

  return retclass;
}


/*
 *  typeequiv_literal
 *     This function is idential in nature to typeequiv() except it is
 *     used in testing compatibility between literals on the RHS and 
 *     the object on the LHS.
 */
static typeclass typeequiv_literal(datatype_t *tinfo ) {
  typeclass sclass, retclass;

  sclass = D_CLASS( tinfo );

  retclass = sclass;

  switch( sclass ) {
  case DT_REGION:
  case DT_FILE:
  case DT_INTEGER:
  case DT_SHORT:
  case DT_LONG:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_LONG:
  case DT_REAL:
  case DT_DOUBLE:
  case DT_QUAD:
  case DT_BOOLEAN:	
  case DT_CHAR:	
  case DT_UNSIGNED_BYTE:
  case DT_SIGNED_BYTE:
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
    retclass = DT_INTEGER;
    break;
  case DT_GENERIC:
    break;
  case DT_GENERIC_ENSEMBLE:
    break;
    
  case DT_VOID:
  case DT_PROCEDURE:
    retclass = DT_VOID;
    break;
    
  default:
    retclass = sclass;
    break;
  }
  
  return retclass;
}


/*
 *  typecheck_compat( datatype_t expr_lhs, datatype-t expr_rhs, int isliteral,
 *     int strict_check )
 *
 *  Return zero if not compatible
 *  Otherwise compatible
 *  Note that order of parameters matters with this routine
 *
 *  There is a small hack here to separate the test of RHS literals and
 *  other RHS types.  This is necessary since literals follow different
 *  type rules than other objects.
 */
int typecheck_compat(datatype_t *expr_lhs,datatype_t *expr_rhs,int isliteral,
		     int strict_check,int assign) {
  typeclass class_lhs, class_rhs;

  if (expr_lhs == NULL || expr_rhs == NULL) {
    return 0;
  }

  if (D_NAME(expr_lhs) && D_NAME(expr_rhs) && 
      D_NAME(expr_lhs)==D_NAME(expr_rhs)) {
    return 1;
  }
  
  /*
   *  If the rhs is an array, then the lhs must be an array.
   *  Recurse to check the base type of the array
   *  The same goes for ensembles
   */
  if ( D_CLASS(expr_rhs) == DT_ARRAY ) {
    if ( D_CLASS(expr_lhs) == DT_ARRAY ) {
      return typecheck_compat( D_ARR_TYPE(expr_lhs),
			      D_ARR_TYPE(expr_rhs),
			      isliteral,
			      strict_check,assign);
    } else if (in_reduce) { /* in reductions, arrays and scalars can be mixed */
      return typecheck_compat(expr_lhs, D_ARR_TYPE(expr_rhs),isliteral,
			      strict_check,assign);
    } else {
      if (D_IS_ENSEMBLE(expr_lhs) &&
	  D_CLASS(D_ENS_TYPE(expr_lhs)) == DT_ARRAY ) {
	return typecheck_compat(D_ENS_TYPE(expr_lhs),
				expr_rhs,
				isliteral,
				strict_check,assign);
      } else {
	if ( D_CLASS(expr_lhs) != DT_GENERIC_ENSEMBLE ) {
	  return 0;
	}
	/* else a generic ensemble, then fall through */
      }
    }
  } else {
    if (D_CLASS(expr_lhs) == DT_ARRAY) {
      if (in_reduce) {
	return typecheck_compat(D_ARR_TYPE(expr_lhs),expr_rhs,isliteral,
				strict_check,assign);
      } else {
	return 0;
      }
    }
  }

  if ( D_IS_ENSEMBLE(expr_rhs) ) {
    if ( D_IS_ENSEMBLE(expr_lhs) ) {
      return typecheck_compat( D_ENS_TYPE(expr_lhs),
			      D_ENS_TYPE(expr_rhs),
			      isliteral,
			      strict_check,assign);
    } else {
      return 0;
    }
  }

  /* allow promotion of scalar types to arrays or ensembles*/
  if (assign && D_CLASS(expr_lhs) == DT_ARRAY) {
    return typecheck_compat( D_ARR_TYPE(expr_lhs),expr_rhs,isliteral,
			    strict_check,assign);
  }
  if ( D_IS_ENSEMBLE(expr_lhs) ) {
    return typecheck_compat( D_ENS_TYPE(expr_lhs), expr_rhs, isliteral,
	      strict_check,assign);
  }


  /*
   *  Check compatibility between types on RHS and LHS
   *  For literal check, both LHS and RHS need to call typeequiv_literal()
   *  such that the constants which are returned are in synch.
   */
  if ( !isliteral ) {
    if ( strict_check ) {
      class_lhs = typeequiv_strict( expr_lhs );
      class_rhs = typeequiv_strict( expr_rhs );
    } else {
      class_lhs = typeequiv( expr_lhs );
      class_rhs = typeequiv( expr_rhs );
    }
  } else {
    if ( strict_check ) {
      class_lhs = typeequiv_literal_strict( expr_lhs );
      class_rhs = typeequiv_literal_strict( expr_rhs );
    } else {
      class_lhs = typeequiv_literal( expr_lhs );
      class_rhs = typeequiv_literal( expr_rhs );
    }
  }
    

  /*
   *  If either LHS or RHS is generic, then set other side to generic
   *  Make sure though that the types are basic though to not allow
   *  things like structures to be passed as generics
   */
  if (class_lhs == DT_GENERIC || class_rhs == DT_GENERIC) {
    if ((typecheck_basic(expr_rhs) || S_CLASS(expr_rhs) == DT_SUBPROGRAM) &&
	typecheck_basic(expr_lhs)) {
      class_lhs = class_rhs = DT_GENERIC;
    } else {
      class_lhs = DT_VOID;
      class_rhs = DT_GENERIC;
    }
  }
	

  /*
   *  If either LHS or RHS is a generic ensemble, then set
   *  the other side to be a generic ensemble.  This is done
   *  so that any generic ensemble and any ensemble will appear
   *  to be compatible.
   *  a DT_GENERIC_ENSEMBLE is different than a a DT_GENERIC in that
   *  a DT_GENERIC_ENSEMBLE is an ensemble and the type of ensemble
   *  elements can be of ANY type.  A DT_GENERIC is a scalar type that
   *  can only be of specific types (the ones returning true by
   *  typecheck_basic()).
   */
  if ( class_lhs == DT_GENERIC_ENSEMBLE || class_rhs == DT_GENERIC_ENSEMBLE ) {
    class_lhs = class_rhs = DT_GENERIC_ENSEMBLE;
  }

  /*
   *  Check for structure name equivalence
   */
  if ( class_lhs == DT_STRUCTURE && class_rhs == DT_STRUCTURE && 
       expr_lhs != expr_rhs) {
    if ((D_NAME(expr_lhs)==NULL) || (D_NAME(expr_rhs)==NULL) ||
	(strcmp( S_IDENT(D_NAME(expr_lhs)), S_IDENT(D_NAME(expr_rhs))))) {
      /* structure names do not compare */
      /* make sure comparison fails */
      class_lhs = DT_STRUCTURE;
      class_rhs = DT_VOID;
    }
  }

  return (class_lhs == class_rhs);
}


/*
 *  typecheck_rank( datatype_t expr_lhs, datatype_t expr_rhs )
 * 
 *  Return zero if not same rank
 *  Otherwise compatible
 */
static int typecheck_rank(expr_t *reg_lhs, expr_t *reg_rhs ) {
  int rank_lhs, rank_rhs;

  /*
   *  Default ranks to zero (i.e. scalar)
   */
  rank_lhs = 0;
  rank_rhs = 0;

  /* if  either is an unresolved quote region */
  if (expr_is_qreg(reg_lhs) == -1 || expr_is_qreg(reg_rhs)) {
    return 1;  /* liberal */
  }

  /*
   *  Get rank of LHS and RHS ensembles
   *  rank values remaine zero if not ensembles or are generic ensembles
   */
  if ( reg_lhs != NULL && reg_rhs != NULL ) {
    if (D_CLASS(T_TYPEINFO(reg_lhs)) != DT_REGION ||
	D_CLASS(T_TYPEINFO(reg_rhs)) != DT_REGION) {
      INT_FATAL(NULL, "something wasn't a region");
    }
    rank_lhs = D_REG_NUM(T_TYPEINFO(reg_lhs));
    rank_rhs = D_REG_NUM(T_TYPEINFO(reg_rhs));
  }

  return ( rank_lhs == rank_rhs );
}


static void typecheck_array_bounds(expr_t *expr_rhs ) {
  expr_t *expr_lhs;
  expr_t *traverse_lhs, *traverse_rhs;
  expr_t *offset_lhs, *offset_rhs;
  dimension_t *dim_lhs, *dim_rhs;
  int i, num_dims, empty_paren_lhs, empty_paren_rhs;
  
  /*
   * pass in lhs array ref via global variable
   * done this way so that we can use the given traversal
   * routines
   */
  expr_lhs = LHS_ArrayRef;
  
  if ( T_TYPE(expr_lhs) != ARRAY_REF ) {
    INT_FATAL( T_STMT(expr_lhs), "LHS_ArrayRef is not an array ref." );
  }
  
  /* if we haven't found an array ref yet, then don't do any checking */
  if ( T_TYPE(expr_rhs) != ARRAY_REF ) {
    return;
  }
  
  if ( glist_find( Seen_Array_Refs, expr_rhs ) ) {
    return;
  }
    
  traverse_lhs = expr_lhs;
  traverse_rhs = expr_rhs;

  empty_paren_lhs = 0;
  empty_paren_rhs = 0;
  while ( traverse_lhs != NULL && traverse_rhs != NULL ) {
    /*
     *  Find next array ref
     *  Then incr. traverse_lhs to next expression.
     *  This next expression will contain the indexing information
     *  Match the indexing information for the lhs and rhs
     *  Decide what to do based on this information
     */
    if ( T_TYPE(traverse_lhs) != ARRAY_REF ) {
      while ( traverse_lhs != NULL &&
	     T_TYPE(traverse_lhs) != ARRAY_REF ) {
	traverse_lhs = T_OPLS(traverse_lhs);
      }
    }
    
    if ( T_TYPE(traverse_rhs) != ARRAY_REF ) {
      while ( traverse_rhs != NULL &&
	     T_TYPE(traverse_rhs) != ARRAY_REF ) {
	traverse_rhs = T_OPLS(traverse_rhs);
      }
    }
    
    /*
     *  Important that both of these test come after trying to
     *  find the array refs in the while loops above.  This is
     *  so that we can tell what happened when we leave this
     *  enclosing while loop
     */
    if ( traverse_rhs != NULL ) {
      Seen_Array_Refs = glist_prepend( Seen_Array_Refs,
				      traverse_rhs,
				      GLIST_NODE_SIZE );
      traverse_rhs = T_OPLS(traverse_rhs);
    }
    
    if ( traverse_lhs != NULL ) {
      traverse_lhs = T_OPLS(traverse_lhs);
    }
    
    /*  compute number of empty [] for lhs and rhs before
     *  there is a chance of continuing, since we need this
     *  information when the loop is complete.
     */
    if ( traverse_lhs != NULL && T_NEXT(traverse_lhs) == NULL ) {
      empty_paren_lhs++;
    }

    if ( traverse_rhs != NULL && T_NEXT(traverse_rhs) == NULL ) {
      empty_paren_rhs++;
    }

    /* got to end of array ref chain, stop */
    if ( traverse_lhs == NULL || traverse_rhs == NULL ) {
      continue;
    }
	
    /* get the array offsets from the AST */
    offset_lhs = T_NEXT(traverse_lhs);
    offset_rhs = T_NEXT(traverse_rhs);

    /*
     *  Possible cases:
     *     lhs[ ]  rhs[ ]        Check that both array index sets
     *                              compatible
     *     lhs[x]  rhs[ ]        Illegal, reverse promotion
     *     lhs[x]  rhs[x]        Legal
     *     lhs[ ]  rhs[x]        Legal - promotion
     */
    
    /* case of lhs[] and rhs[] */
    if ( offset_lhs == NULL && offset_rhs == NULL ) {
      /* if there are no dimensions specified in the []'s, then
       * check the dimensions stored in the symboltable
       */
      if (T_TYPEINFO(traverse_lhs) == NULL||T_TYPEINFO(traverse_rhs) == NULL) {
	INT_FATAL( T_STMT(expr_rhs), "NULL typeinfo for indexed array." );
      }
      
      num_dims = D_ARR_NUM(T_TYPEINFO(traverse_lhs));
      
      if ( num_dims != D_ARR_NUM(T_TYPEINFO(traverse_rhs) ) ) {
	USR_FATAL_CONT(T_STMT(traverse_lhs),
		       "Number of dimensions for left and right "
		       "hand side indexed arrays are not equal");
      }
      
      dim_lhs = D_ARR_DIM(T_TYPEINFO(traverse_lhs));
      dim_rhs = D_ARR_DIM(T_TYPEINFO(traverse_rhs));
      for ( i = 0; i < num_dims; i++ ) {
	if ( dim_lhs == NULL || dim_rhs == NULL ) {
	  INT_FATAL( T_STMT(expr_rhs), "NULL dimension in dimension list." );
	}
	
	if ( T_IDENT(DIM_HI(dim_lhs)) != NULL &&
	    T_IDENT(DIM_HI(dim_rhs)) != NULL &&
	    T_IDENT(DIM_HI(dim_lhs)) != T_IDENT(DIM_HI(dim_rhs)) ) {
	  /* JRS: could output more information here, like
	     the actual values which don't correspond */
	  USR_FATAL_CONT(T_STMT(traverse_lhs),
			 "High bound on dimension %d for left and "
			 "right hand side indexed arrays are not "
			 "equal", i+1);
	}
	
	if ( T_IDENT(DIM_LO(dim_lhs)) != NULL &&
	    T_IDENT(DIM_LO(dim_rhs)) != NULL &&
	    T_IDENT(DIM_LO(dim_lhs)) != T_IDENT(DIM_LO(dim_rhs)) ) {
	  /* JRS: could output more information here, like
	     the actual values which don't correspond */
	  USR_FATAL_CONT(T_STMT(traverse_lhs),
			 "Low bound on dimension %d for left and "
			  "right hand side indexed arrays are not "
			  "equal", i+1);
	}
	
	dim_lhs = DIM_NEXT(dim_lhs);
	dim_rhs = DIM_NEXT(dim_rhs);
      }
    }
    
    /* know rhs[x] at this point.  Make sure x is a valid type */
    if ( offset_rhs != NULL && T_TYPEINFO(offset_rhs) ) {
      switch( S_CLASS(T_TYPEINFO(offset_rhs)) ) {
      case DT_INTEGER:
      case DT_SIGNED_BYTE:
      case DT_SHORT:
      case DT_LONG:
      case DT_UNSIGNED_BYTE:
      case DT_UNSIGNED_INT:
      case DT_UNSIGNED_SHORT:
      case DT_UNSIGNED_LONG:
      case DT_CHAR:
      case DT_ENUM:
	break;
	
      default:
	{
	  symboltable_t *rhs_st;

	  rhs_st = expr_is_var(offset_rhs);
	  INT_COND_FATAL((rhs_st!=NULL), T_STMT(traverse_rhs),
			 "No VARIABLE node in expression");
	  USR_FATAL_CONT(T_STMT(traverse_rhs),
			 "'%s' has invalid type for indexing array",
			 S_IDENT(rhs_st));
	  break;
	}
      }
    }
    
    if ( offset_lhs != NULL && T_TYPEINFO(offset_lhs) ) {
      switch( S_CLASS(T_TYPEINFO(offset_lhs)) ) {
      case DT_INTEGER:
      case DT_SIGNED_BYTE:
      case DT_SHORT:
      case DT_LONG:
      case DT_UNSIGNED_BYTE:
      case DT_UNSIGNED_INT:
      case DT_UNSIGNED_SHORT:
      case DT_UNSIGNED_LONG:
      case DT_CHAR:
      case DT_ENUM:
	break;
	
      default:
	{
	  symboltable_t *lhs_st;

	  lhs_st = expr_is_var(offset_lhs);
	  INT_COND_FATAL((lhs_st!=NULL), T_STMT(traverse_lhs),
			 "No VARIABLE node in expression");
	  USR_FATAL_CONT(T_STMT(traverse_lhs),
			 "'%s' has invalid type for indexing array",
			 S_IDENT(lhs_st));
	  break;
	}
      }
    }
  }
  
  DB2( 1, "Empty [] difference: LHS %d, RHS %d\n",empty_paren_lhs, 
      empty_paren_rhs );
  if ( empty_paren_lhs >= empty_paren_rhs ) {
    return;
  }

  return;
}


/*
 *  typecheck_assign( datatype_t expr_lhs, datatype_t expr_rhs )
 *
 *  Return zero if not compatible assignment, (scalar := ens)
 *  Otherwise compatible
 */
static int typecheck_assign(expr_t *reg_lhs, expr_t *reg_rhs ) {
  /* if either region is a [::,::,...] scalar, it's going to be OK */
  if (expr_is_grid_scalar(reg_lhs) || expr_is_grid_scalar(reg_rhs)) {
    return 1;
  }

  /*
   *  scalar := ens
   *  ILLEGAL
   *  If LHS is a scalar and RHS is not a scalar, then not assign 
   *    compatible, return 0.
   */
  if ( reg_lhs == NULL && reg_rhs != NULL ) {
    return 0;
  }

  return 1;
}


/*
 *  typecheck_index_rank
 *    Make sure the rank of the LHS ensemble is greater than or equal to
 *    the max representable dimension of the RHS Indexi variable.
 *
 *    Return 0 only when above condition is guaranteed not to hold
 *	otherwise return 1.
 */

static int typecheck_index_rank(expr_t *reg_lhs,
				symboltable_t *symtab_rhs) {
  int i;
  /*
   *  Check that the dimension that a RHS Indexi variable represents
   *  is within the dimensionality of the ensemble the Indexi ensemble
   *  is being assigned into.
   */

  i = symtab_is_indexi(symtab_rhs);
  if (i) {
    if (reg_lhs != NULL) {
      if (D_REG_NUM(T_TYPEINFO(reg_lhs)) >= i) {
	/* If ranks match up, ok */
	return 1;
      } else {
	/* If ranks don't match up, error */
	if (expr_is_qreg(reg_lhs) == -1) { /* unresolved " region */
	  return 1;  /* liberal */
	} else {
	  return 0;
	}
      }
    } else {
      /* LHS != ensemble, RHS == Indexi, error */
      return 0;
    }
  }

  /* 
   * Not an index variable on RHS
   */
  return 1;
}
    

static void typecheck_expr(expr_t *expr) {
  int isliteral, indexRank, stmtRank;
  expr_t *e, *find_lhs_arrayref;
  
  if (expr == NULL) {
    return;
  }
  switch(T_TYPE(expr)) {
  case VARIABLE: 
      /* if we are dealing with an indexed array, make sure that we are
	 going to index into it, if not then an error */
    if (T_TYPEINFO(expr) && S_CLASS(T_TYPEINFO(expr)) == DT_ARRAY &&
	!functionParam && !in_reduce) {
      e = T_PARENT(expr);
      while ( e != NULL && T_TYPE(e) == BIAT )
	e = T_PARENT(e);

      if ( e == NULL || T_TYPE(e) != ARRAY_REF ) {
	
	if (T_STMT(expr) && (T_TYPE(T_STMT(expr)) != S_IO ||
			     T_TYPEINFO_REG(expr) == NULL)) {
	  USR_FATAL_CONT(T_STMT(expr), "Not indexing into array '%s'",
			 S_IDENT(T_IDENT(expr)));
	}
      }
    }
	      

    if (shard_rank > 0 || free_control_flow) {
      int rank = expr_rank(expr);

      if (rank != 0 && shard_rank && rank != shard_rank) {
	USR_FATAL_CONT(T_STMT(expr),
		       "Shattered control flow contains array with "
		       "nonconforming rank");
      }
    }

    /*
     *  If this is an Indexi variable, then make sure stmt rank large enough
     *  such that MLOOP will support this Indexi expression
     */
    indexRank = expr_contains_indexi(expr);
    if ( indexRank > 0 ) {
      stmtRank = stmt_rank( T_STMT(expr) );
      DB2( 1, "indexRank = %d, stmtRank = %d\n", indexRank, stmtRank );
      if ( stmtRank <= 0 || stmtRank < indexRank ) {
	if (functionParam == 1) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Rank of statement not large enough for Indexi "
			 "expression");
	}
      }
    }

    if (T_TYPEINFO_REG(expr) && !on_lhs && !did_flood_check) {
      CheckFloodability(expr,"RHS-expr",0,T_LINENO(T_STMT(expr)));
    }

    break;

  case CONSTANT: 
    break;
    
  case UCOMPLEMENT:
    if ( strict_check ) {
      if ( S_CLASS(T_TYPEINFO(expr)) != DT_BOOLEAN ) {
	USR_FATAL_CONT(T_STMT(expr),
		       "Applying complement operator to non-boolean value");
      }
    }
    /* fall through */
  case UNEGATIVE:
  case UPOSITIVE:
    break;
  case BIAT:
    if (T_TYPEINFO_REG(T_OPLS(expr)) == NULL) {
      USR_FATAL(T_STMT(expr), "@ operator applied to non-array expression");
    }

    if (!expr_is_lvalue(T_OPLS(expr))) {
      USR_FATAL(T_STMT(expr), "@ operator may not be applied to expressions");
    }

    if ((shard_rank || free_control_flow) && on_lhs) {
      USR_FATAL(T_STMT(expr),
		"Assigned array used with @ in shattered control flow");
    }

    if (!in_mscan && T_SUBTYPE(expr) == AT_PRIME) {
      USR_FATAL(T_STMT(expr), "Illegal to use ' outside of a scan statement");
    }

    if (T_SUBTYPE(expr) == AT_RANDACC) {
      if (T_MAPSIZE(expr) != D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(T_OPLS(expr))))) {
	USR_FATAL_CONT(T_STMT(expr),"Number of mapping arrays does not match statement dimension");
      }
    }
    typecheck_expr(T_OPLS(expr));

    /* done in parser */
    if ((D_CLASS(T_TYPEINFO(T_NEXT(T_OPLS(expr)))) != DT_DIRECTION)) {
      USR_FATAL_CONT(T_STMT(expr), "@ uses nondirection");
    }

    if (!expr_is_qmask(expr)) {
      if (T_TYPEINFO_REG(expr) &&
	  D_DIR_NUM(T_TYPEINFO(T_NEXT(T_OPLS(expr)))) != D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr)))) {
	USR_FATAL_CONT(T_STMT(expr), "@-operator rank mismatch");
      }
    }

    if (expr_find_at(T_OPLS(expr))) {
      USR_FATAL_CONT(T_STMT(expr), "Cannot apply two @'s to one expression");
    }

    break;
    
    /* scan/reduce */
  case REDUCE: 
    if (shard_rank || free_control_flow) {
      USR_FATAL_CONT(T_STMT(expr), "Reduce used in shattered control flow");
    }
    if (in_mscan) {
      USR_FATAL_CONT(T_STMT(expr), "Reduce used in interleave");
    }
    
    if (T_REGMASK(expr)) {
      RMSPushScope(T_REGMASK(expr));
    }

    in_reduce++;
    typecheck_expr(T_OPLS(expr));
    in_reduce++;

    if (T_REGMASK(expr)) {
      RMSPopScope(T_REGMASK(expr));
    }
    
    /*
     *  Make sure reduction operation operating on correct types
     */
    switch ( T_SUBTYPE(expr) ) {
    case BAND:
    case BOR:
    case BXOR:
      if (datatype_float(T_TYPEINFO(T_OPLS(expr)))) {
	USR_FATAL_CONT(T_STMT(expr),
		       "Can't reduce floating point values with this operator");
      }
    case AND:
    case OR:
      if ( strict_check ) {
	if ( S_CLASS(T_TYPEINFO(expr)) != DT_BOOLEAN ) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Applying and/or reduction on non-boolean array");
	}
	
	break;
      }
      /* fall through in the case of non-strict checking */

    case MIN:
    case MAX:
      if (datatype_complex(T_TYPEINFO(T_OPLS(expr)))) {
	USR_FATAL_CONT(T_STMT(expr),
		       "Can't reduce complex values with this operator");
      }
    case PLUS:     
    case TIMES:
      /*
       *  Check that the type of the reduction is a basic type
       */
      if ( !typecheck_basic( T_TYPEINFO(T_OPLS(expr)) ) ) {
	USR_FATAL_CONT(T_STMT(expr), "Reduction not on a valid type");
      }
      break;
      
    default:
      break;
    }
    
    break;
    
  case SCAN:
    if (shard_rank || free_control_flow) {
      USR_FATAL_CONT(T_STMT(expr), "Scan used in shattered control flow");
    }
    if (in_mscan) {
      USR_FATAL_CONT(T_STMT(expr), "Scan used in interleave");
    }
    
    typecheck_expr(T_OPLS(expr));

    {
      int opdims;
      int scandims;
      int i;

      if (T_DIMS(expr) != NULL) {
	opdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(T_OPLS(expr))));
	scandims = T_DIMS(expr)[MAXRANK];
	
	if (scandims > opdims) {
	  USR_FATAL_CONT(T_STMT(expr), "Scan rank exceeds argument rank");
	}
	for (i=0;i<scandims;i++) {
	  scandims = T_DIMS(expr)[i];
	  if (scandims > opdims) {
	    USR_FATAL_CONT(T_STMT(expr), "Scan dimension exceeds argument rank");
	  }
	}
      }
    }
    
    
    /*
     *  Make sure scan operation operating on correct types
     */
    switch ( T_SUBTYPE(expr) ) {
      
    case BAND:
    case BOR:
    case BXOR:
      if (datatype_float(T_TYPEINFO(T_OPLS(expr)))) {
	USR_FATAL_CONT(T_STMT(expr),
		       "Can't scan floating point values with this operator");
      }

    case AND:
    case OR:
      if ( strict_check ) {
	if ( S_CLASS(T_TYPEINFO(expr)) != DT_BOOLEAN ) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Applying and/or scan on non-boolean array");
	}
	break;
      }
    case MIN:
    case MAX:
      if (datatype_complex(T_TYPEINFO(T_OPLS(expr)))) {
	USR_FATAL_CONT(T_STMT(expr),
		       "Can't scan complex values with this operator");
      }

    case PLUS:     
    case TIMES:
      /*
       *  Check that the type of the scan is a basic type
       */
      if ( !typecheck_basic( T_TYPEINFO(T_OPLS(expr)) ) ) {
	USR_FATAL_CONT(T_STMT(expr), "Scan not on a valid type");
      }
      break;

    default:
      break;
    }

    break;

  case PERMUTE:
    {
      int mapdims=0;
      int opdims;
      expr_t *reg;
      
      mapdims = T_MAPSIZE(expr);
      if (T_TYPE(T_MAP(expr)) == VARIABLE && T_IDENT(T_MAP(expr)) == lu_pst("_rgridremap")) {
	  mapdims--;
      }
      reg = T_TYPEINFO_REG(T_OPLS(expr));
      if (reg) {
	opdims = D_REG_NUM(T_TYPEINFO(reg));
	
	if (opdims != mapdims) {
	  USR_FATAL_CONT(T_STMT(expr),"Number of mapping arrays does not match array rank");
	}
      }
    }
    break;
 
  case FLOOD:
    {
      int regdims;
      int opdims;
      expr_t *srcreg;

      if (shard_rank || free_control_flow) {
	USR_FATAL_CONT(T_STMT(expr), "Flood used in shattered control flow");
      }
      if (in_mscan) {
	USR_FATAL_CONT(T_STMT(expr), "Flood used in interleave");
      }

      srcreg = T_REGION_SYM(T_REGMASK(expr));
      regdims = D_REG_NUM(T_TYPEINFO(srcreg));

      RMSPushScope(T_REGMASK(expr));
      
      RMSLegalFlood(T_STMT(expr),regdims);

      typecheck_expr(T_OPLS(expr));

      RMSPopScope(T_REGMASK(expr));

      if (T_TYPEINFO_REG(T_OPLS(expr)) == NULL) {
	opdims = regdims;
      } else {
	opdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(T_OPLS(expr))));
      }
      if (regdims != opdims) {
	USR_FATAL_CONT(T_STMT(expr), "Flood uses a region of the wrong rank "
		       "(%d rather than %d)" ,regdims,opdims);
      }
    }

    /* don't need to check the arguments here.  Just need to make sure
       type matches destination. */
    
    break;
    
    /* n-ary operators */

  case ARRAY_REF:
    {
      int    dims = 0;
      expr_t *e;
      int reset_flood_check=0;

      if (on_lhs &&
	  !(expr_is_free(T_OPLS(expr)) || expr_parallel(T_OPLS(expr))) &&
	  expr_is_free(expr)) {
	USR_FATAL_CONT(T_STMT(expr), "Indexing of unfree indexed array by free illegal");
      }

      if (T_TYPEINFO_REG(expr) && !on_lhs && !did_flood_check) {
	CheckFloodability(expr,"RHS-expr",0,T_LINENO(T_STMT(expr)));
	did_flood_check=1;
	reset_flood_check=1;
      }
      
      typecheck_expr(T_OPLS(expr));
      
      if (reset_flood_check) {
	did_flood_check=0;
	reset_flood_check=0;
      }

      if (!T_OPLS(expr) ||
	  !T_TYPEINFO(T_OPLS(expr)) ||
	  (S_CLASS(T_TYPEINFO(T_OPLS(expr))) != DT_ARRAY)) {
	int i;
	int okay = 0; /* allow optional indexing into indexi */
	for (i = 0; i < MAXRANK; i++) {
	  if (T_IDENT(T_OPLS(expr)) == pstindex[i])
	    okay = 1;
	}

	if (!okay) {
	  /** allow indexing into arrays read over :: */
	  typecheck_exprls(T_NEXT(T_OPLS(expr)));
	  expr = randacc_transform(RMSCurrentRegion(), expr, &okay);
	  if (okay) break;
	}
	if (!okay) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Array indexing on nonindexed array or scalar");
	}
      } else {
	/* count number of dims in this array ref */
	e = T_OPLS(expr);
	while (T_NEXT(e)) {
	  e = T_NEXT(e);
	  dims++;
	}
	
	INT_COND_FATAL((T_TYPEINFO(T_OPLS(expr))!=NULL), T_STMT(expr),
		       "NULL typeinfo ptr");
	
	/*
	 *  dims = number of items in comma separated list within the []
	 *         of the array
	 *
l	   *  Check that dims equals the number of dimensions for the array
*    or that it equals zero, meaning all the dimensions of the array
*/
	if (dims != 0 && D_ARR_NUM(T_TYPEINFO(T_OPLS(expr))) != dims) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "%dD array referenced as a %dD array",
			 D_ARR_NUM(T_TYPEINFO(T_OPLS(expr))), dims);
	}
	
	typecheck_exprls(T_NEXT(T_OPLS(expr)));
      }


      if ((shard_rank || free_control_flow) && T_NEXT(T_OPLS(expr)) == NULL) {
	USR_FATAL_CONT(T_STMT(expr),
		       "Empty array reference in shattered control flow");
      }
      
      break;
    }
  case FUNCTION:
    {
      genlist_t *formals = NULL;
      symboltable_t* formal;
      expr_t        *actuals = NULL;
      int	    STRICT = FALSE;
      int           paramnum = 1;
      int           oldFP = 0;
      int           unc_promoted = 0;
      int           unc_not_promoted = 0;
      int           par_promoted = 0;
      int           scalar_out = 0;
      datatype_t    *dt;

      dt = S_DTYPE(T_IDENT(T_OPLS(expr)));
      if ( dt == NULL ) {
	USR_WARN( T_STMT(expr),
		 "Function '%s' contains no prototype",
		 S_IDENT(T_IDENT(T_OPLS(expr))));
      }
      
      /* no open/close in shard */
      if (shard_rank || free_control_flow || in_mscan) {
	if (T_OPLS(expr) &&
	    T_IDENT(T_OPLS(expr)) &&
	    (!strncmp(S_IDENT(T_IDENT(T_OPLS(expr))), "open", 5) ||
	     !strncmp(S_IDENT(T_IDENT(T_OPLS(expr))), "close", 6))) {
	  if (in_mscan) {
	    USR_FATAL_CONT(T_STMT(expr),
			   "May not open/close files in interleave");
	  }
	  else {
	    USR_FATAL_CONT(T_STMT(expr),
			   "May not open/close files in shattered control flow");
	  }
	}
      }

      /* the following will not work unless this pass is run after paranal */
      if (shard_rank || free_control_flow || in_mscan) {
	if (T_OPLS(expr) &&
	    T_IDENT(T_OPLS(expr)) &&
	    T_TYPE(T_IDENT(T_OPLS(expr))) &&
	    D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))) &&
	    T_PARALLEL(D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))))) {
	  if (in_mscan) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Parallel function call in interleave");
	  }
	  else {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Parallel function call in shattered control flow");
	  }
	} 
      }
      
      if (T_OPLS(expr)) {
	do {
	  actuals = T_NEXT(T_OPLS(expr));
	} while (actuals != NULL && T_TYPE(actuals) == NULLEXPR);
      }

      /* check rshift/lshift */
      if (T_OPLS(expr) && (T_IDENT(T_OPLS(expr)) == pstLSHIFT || 
			   T_IDENT(T_OPLS(expr)) == pstRSHIFT)) {
	int numargs = 0;
	
	while (actuals != NULL) {
	  numargs++;

	  if (!_T_IS_INTEGER(T_TYPEINFO(actuals))) {
	    if (numargs == 1) {
	      USR_FATAL_CONT(T_STMT(expr), "May only shift integer types");
	    } else if (numargs == 2) {
	      USR_FATAL_CONT(T_STMT(expr), "May only shift by an integer");
	    }
	  }
	  typecheck_expr(actuals);
	  actuals = T_NEXT(actuals);
	}

	if (numargs != 2) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Wrong number of arguments in shift function");
	}

	return;
      }

      /* check rshift/lshift */
      if (T_OPLS(expr) && (T_IDENT(T_OPLS(expr)) == pstBAND || 
			   T_IDENT(T_OPLS(expr)) == pstBOR || 
			   T_IDENT(T_OPLS(expr)) == pstBXOR)) {
	int numargs = 0;
	
	while (actuals != NULL) {
	  numargs++;

	  if (!_T_IS_INTEGER(T_TYPEINFO(actuals))) {
	    USR_FATAL_CONT(T_STMT(expr),
			   "bitwise operators require integer types");
	  }
	  typecheck_expr(actuals);

	  actuals = T_NEXT(actuals);
	}

	if (numargs != 2) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Wrong number of arguments in bit function");
	}

	return;
      }

      if (T_OPLS(expr) && (T_IDENT(T_OPLS(expr)) == pstBNOT)) {
	int numargs = 0;
	
	while (actuals != NULL) {
	  numargs++;

	  if (!_T_IS_INTEGER(T_TYPEINFO(actuals))) {
	    USR_FATAL_CONT(T_STMT(expr),
			   "bitwise operators require integer types");
	  }
	  typecheck_expr(actuals);

	  actuals = T_NEXT(actuals);
	}

	if (numargs != 1) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Wrong number of arguments in bit function");
	}

	return;
      }

      /* check strcpy/strcmp */
      if (T_OPLS(expr) && (T_IDENT(T_OPLS(expr)) == pstSTRNCPY ||
			   T_IDENT(T_OPLS(expr)) == pstSTRNCMP)) {
	int numargs = 0;
	int ensemblearg[2];
	
	while (actuals != NULL) {
	  if (numargs < 2) {
	    ensemblearg[numargs] = (T_TYPEINFO_REG(actuals) != NULL);
	  }
	  numargs++;

	  if (T_TYPEINFO(actuals) != pdtSTRING) {
	    if (T_IDENT(T_OPLS(expr)) == pstSTRNCPY) {
	      USR_FATAL_CONT(T_STMT(expr),
			     "Assigning non-string to string");
	    } else {
	      USR_FATAL_CONT(T_STMT(expr), "Comparing string with non-string");
	    }
	  }

	  typecheck_expr(actuals);

	  actuals = T_NEXT(actuals);
	}

	if (T_IDENT(T_OPLS(expr)) == pstSTRNCPY) {
	  if (ensemblearg[1] && !ensemblearg[0]) {
	    USR_FATAL_CONT(T_STMT(expr), "Assignment of array to scalar");
	  }
	}

	return;
      }
      



      if (T_OPLS(expr) &&
	  T_IDENT(T_OPLS(expr)) &&
	  T_TYPE(T_IDENT(T_OPLS(expr))) &&
	  D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr))))) {
	formals = T_PARAMLS(D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))));
      }

      while (actuals && formals) {
	int reset_flood_check=0;
	formal = G_IDENT(formals);
	/* 
	 *  Make sure actual parameter (which may be an expression)
	 *  type checks correctly
	 *
	 *  Global variable functionParam is used by the the typechecking
	 *  system to disable certain checks when checking the parameters
	 *  of a function.  One example is the typechecking of indexed arrays
	 *  being passed into a function (there doesn't need to be an array
	 *  ref as the parent of the array, so we turn this check off).
	 *  oldFP is used to remember the prior value such that when we
	 *  know when we are done typechecking the first enclosing function.
	 */

	if ((S_SUBCLASS(formal) == SC_INOUT || S_SUBCLASS(formal) == SC_OUT) &&
	    !(T_TYPEINFO_REG(actuals) ||
	      expr_is_free(actuals))) {
	  scalar_out = 1;
	  if (free_control_flow || shard_rank) {
	    USR_FATAL_CONT(T_STMT(expr),
			   "Cannot pass scalar by reference in shattered control flow");
	  }
	  if (in_mscan) {
	    USR_FATAL_CONT(T_STMT(expr),
			   "Cannot pass scalar by reference in interleave");
	  }
	}
	if ((S_STYPE(formal) & SC_FREE) &&
	    !expr_is_free(actuals)) {
	  USR_FATAL_CONT(T_STMT(expr), 
			 "Unfree actual passed to free formal parameter");
	}
        if (!(S_STYPE(formal) & SC_FREE) &&
            expr_is_free(actuals)) {
          unc_promoted = 1;
          if (T_TYPE(T_IDENT(T_OPLS(expr))) &&
              D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))) &&
              T_PARALLEL(D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))))) {
            USR_FATAL_CONT(T_STMT(expr),
                           "Free expression passed to function that cannot take it");
          }
        }
        if (!(S_STYPE(formal) & SC_FREE) &&
            (S_SUBCLASS(formal) == SC_INOUT || S_SUBCLASS(formal) == SC_OUT) &&
            !expr_is_free(actuals)) {
          unc_not_promoted = 1;
        }

	oldFP = functionParam;  
	functionParam = 1;
	
	if (D_IS_ENSEMBLE(S_DTYPE(formal))) {
	  region_t tmpscope={NULL,NULL,MASK_NONE,NULL};
	  char paramstr[]="parameter xxx";

	  functionParam = 2;
	  tmpscope.region = D_ENS_REG(S_DTYPE(formal));

	  RMSPushScope(&tmpscope);
	  sprintf(paramstr,"Parameter %d",paramnum);
	  if (T_TYPEINFO_REG(actuals)) {
	    CheckFloodability(actuals,paramstr,
			      S_SUBCLASS(formal) == SC_INOUT || S_SUBCLASS(formal) == SC_OUT,
			      T_LINENO(T_STMT(actuals)));
	  }
	  RMSPopScope(&tmpscope);
	  did_flood_check=1;
	  reset_flood_check=1;
	}

	typecheck_expr(actuals);

	if (reset_flood_check) {
	  did_flood_check = 0;
	  reset_flood_check = 0;
	}

	functionParam = oldFP;

	/*
	 *  Make sure each of the parameters are of compatible types
	 *  Treat like an assignment, where the actual parameter is the
	 *  RHS of the assignment and the formal parameter is the LHS
	 *  After compatibility check, make sure ranks are compatible
	 */
	if ( T_IDENT(actuals) ) {
	  isliteral = ( S_CLASS(T_IDENT(actuals)) == S_LITERAL );
        } else {
	  isliteral = 0; /* assume not a literal */
	}

        /* strict type checking if the formal parameter is an array or
	   ensemble */
        if (formal && 
	    S_DTYPE(formal) && 
	    (S_SUBCLASS(formal) == SC_INOUT ||
	     S_SUBCLASS(formal) == SC_OUT || 
	     D_IS_ENSEMBLE(S_DTYPE(formal)) ||
	     D_CLASS(S_DTYPE(formal)) == DT_ARRAY)) {
	  STRICT = TRUE;
        } else {
	  STRICT = FALSE;
        }

        if (!typecheck_compat(S_DTYPE(formal), 
			      (T_TYPE(actuals) == SIZE) ? pdtINT : T_TYPEINFO(actuals), isliteral, STRICT, 0)) {
	  if ((strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_DESTROY") != 0) &&
	      (strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_PRESERVE") != 0)) {
	    USR_FATAL_CONT(T_STMT(expr), "Formal/actual parameter type mismatch "
			   "(procedure '%s', arg %d).",
			   S_IDENT(S_PARENT(formal)), paramnum);
	  }
	  else if (T_NEXT(actuals)) { /* only typecheck this once, on first parameter, ugh */
	    typecheck_preserve_destroy(expr);
	  }
	}

	if (D_IS_ENSEMBLE(S_DTYPE(formal))) {
	  if (!typecheck_rank(T_TYPEINFO_REG(actuals),D_ENS_REG(S_DTYPE(formal)))) {
	    USR_FATAL_CONT(T_STMT(expr),"Formal/actual parameter rank mismatch "
			   "(procedure '%s', arg %d).",S_IDENT(S_PARENT(formal)),
			   paramnum);
	  }
        }

        /*
         *  Check if formal == ensemble and actual == Indexi
         *  That formal rank great enough to hold Indexi.  
	 *  (Check that formal rank >=  i)
	 *  Only perform this check if formal parameter is an ensemble 
	 *  parameter.  (promotion occurs when a scalar parameter)
	 */
        if ( S_DTYPE(formal) != NULL && D_IS_ENSEMBLE(S_DTYPE(formal)) && 
	    !typecheck_index_rank(D_ENS_REG(S_DTYPE(formal)), T_IDENT(actuals)) ) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Rank of formal parameter not large "
			 "enough to hold Indexi");
        }
	
	/*
	 *  Catch if arbitrary expressions are being passed by var
	 *  If formal is a var param and
	 *  actual is not a variable, bidot, or array ref, or the
	 *  the actual is a constant then output an error
	 */
	if (S_SUBCLASS(formal) == SC_INOUT || S_SUBCLASS(formal) == SC_OUT) {
	  if (T_TYPE(actuals) != CONSTANT && 
	      T_TYPE(actuals) != VARIABLE && 
	      T_TYPE(actuals) != BIDOT && 
	      T_TYPE(actuals) != ARRAY_REF) {
	    if (T_TYPEINFO(actuals) != pdtSTRING) {
	      USR_FATAL_CONT(T_STMT(expr),
			     "Cannot pass expression to '%s' by var (arg %d)",
			     S_IDENT(T_IDENT(T_OPLS(expr))), paramnum);
	    }
	  }
	  if (expr_const(actuals) && datatype_scalar(T_TYPEINFO(actuals))) {
	    USR_FATAL_CONT(T_STMT(expr),
			   "Cannot pass constant to '%s' by var (arg %d)",
			   S_IDENT(T_IDENT(T_OPLS(expr))), paramnum);
	  }
	  if (datatype_find_ensemble(S_DTYPE(formal))) {
	    if ((T_TYPEINFO_REG(actuals)==NULL) && 
		       !datatype_find_ensemble(T_TYPEINFO(actuals))) {
	      USR_FATAL_CONT(T_STMT(expr),
			     "Cannot pass promoted scalar by var to '%s' "
			     "(arg %d).",
			     S_IDENT(T_IDENT(T_OPLS(expr))), paramnum);
	    }
	  }
	}

	if (D_CLASS(T_TYPE(formal)) != DT_ENSEMBLE) {  /* formal is scalar */
	  if (T_TYPEINFO_REG(actuals) || 
	      expr_is_indexi(actuals)) { /* actual is ensemble */
	    par_promoted = 1;
	    if (!T_TYPEINFO_REG(actuals) || 
		shard_rank != D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(actuals)))) { 
	      /* not shard w/ conforming rank */
	      if (T_OPLS(expr) &&                /* is function parallel? */
		  T_IDENT(T_OPLS(expr)) &&
		  T_TYPE(T_IDENT(T_OPLS(expr))) &&
		  D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))) &&
		  T_PARALLEL(D_FUN_BODY(T_TYPE(T_IDENT(T_OPLS(expr)))))) {
		USR_FATAL_CONT(T_STMT(expr),
			       "Parallel function %s may not be promoted "
			       "(arg %d)",
			       S_IDENT(T_IDENT(T_OPLS(expr))),paramnum);
	      }
	    } 
	  }
	}

	do {
	  actuals = T_NEXT(actuals);
	} while (actuals != NULL && T_TYPE(actuals) == NULLEXPR); /* skip past null expressions */

	formals = G_NEXT(formals);
        paramnum++;
      }

      if (unc_promoted && unc_not_promoted) {
        USR_FATAL_CONT(T_STMT(expr),
                       "Function cannot be partially made free");
      }

      if (par_promoted && scalar_out) {
	USR_FATAL_CONT(T_STMT(expr),
		       "Scalar cannot be passed by reference to promoted function");
      }
      
      if (actuals || formals) {
	USR_FATAL_CONT(T_STMT(expr), "Wrong number of arguments in function");
      }
	
      break;
    }

    /* binary operators */
  case BIOP_GETS: 

  case BIASSIGNMENT: 
    if (expr_is_free(T_NEXT(T_OPLS(expr))) &&
	!(expr_is_free(T_OPLS(expr)) ||
          expr_parallel(T_OPLS(expr)) ||
          (expr_find_at(T_OPLS(expr)) != NULL &&
           T_SUBTYPE(expr_find_at(T_OPLS(expr))) == AT_RANDACC))) {
      USR_FATAL_CONT(T_STMT(expr), "Assignment of free to unfree");
    }

    if (shard_rank || free_control_flow || in_mscan) {
      if (T_OPLS(expr) && !T_TYPEINFO_REG(T_OPLS(expr)) && !expr_is_free(T_OPLS(expr))) {
	if (!(T_IDENT(T_OPLS(expr)) &&
	      strncmp("_fntemp", S_IDENT(T_IDENT(T_OPLS(expr))), 7) == 0)) {

	  if (in_mscan) {
	    USR_FATAL_CONT(T_STMT(expr),
			   "scalar assignment in interleave");
	  }
	  else {
	    USR_FATAL_CONT(T_STMT(expr),
			   "scalar assignment in shattered control flow");
	  }
	}
      }
    }
    on_lhs = 1;
    typecheck_expr(T_OPLS(expr));
    on_lhs = 0;
    typecheck_expr(T_NEXT(T_OPLS(expr)));

    /*
     *  Make sure types are compatible for assignment
     */
    isliteral = 0;
    if ( T_IDENT(T_NEXT(T_OPLS(expr))) != NULL ) {
      isliteral = (S_CLASS(T_IDENT(T_NEXT(T_OPLS(expr)))) == S_LITERAL);
    }
    if ( !typecheck_compat(T_TYPEINFO(T_OPLS(expr)),
			   T_TYPEINFO(T_NEXT(T_OPLS(expr))),isliteral,
			   strict_check,1)) {
      USR_FATAL_CONT(T_STMT(expr), "Assignment of incompatible types");
    }

    /*
     *  If the lhs and rhs are arrays, make sure that they conform
     *  correctly with respect to indexing into the array
     *    - empty [] on lhs must match empty [] on rhs and both
     *      arrays must index the same range for that dimension
     *    - nonempty [] on lhs must match nonempty [] on rhs
     */

    find_lhs_arrayref = T_OPLS(expr);
    while ( T_TYPE(find_lhs_arrayref) != VARIABLE ) {
      if ( T_TYPE(find_lhs_arrayref) == ARRAY_REF ) {
	break;
      }
      
      find_lhs_arrayref = T_OPLS(find_lhs_arrayref);
      if ( find_lhs_arrayref == NULL ) {
	break;
      }
    }

    if ( find_lhs_arrayref != NULL &&
	T_TYPE(find_lhs_arrayref) == ARRAY_REF ) {
      /* global variable used to pass lhs array ref
	 into checking routine */
      LHS_ArrayRef = find_lhs_arrayref;
      Seen_Array_Refs = glist_create( LHS_ArrayRef, GLIST_NODE_SIZE );
      
      traverse_exprls( T_NEXT(T_OPLS(expr)),
		      0,
		      typecheck_array_bounds );
      
      /* not necessary, but let's be safe */
      LHS_ArrayRef = NULL;
      glist_destroy( Seen_Array_Refs, GLIST_NODE_SIZE );
    }
      
    /*
     *  Make sure that promotion occurs correctly
     *  (that we are not trying to assign an ensemble into a scalar)
     *
     *  typecheck_assign() is called twice.  In the first case the typeinfo
     *  is examined find out if the RHS is an ensemble.  If the typeinfo
     *  does not yeild this information then the ensemble information is
     *  checked.  This is done since function expressions store their 
     *  ensemble type information in the typeinfo while other expressions
     *  (such as variables) do not.  Variables store their ensemble type 
     *  information in the ensemble_var field of the expression.
     */
    if ( !typecheck_assign( T_TYPEINFO_REG(T_OPLS(expr)),NULL) ||
         !typecheck_assign( T_TYPEINFO_REG(T_OPLS(expr)), 
	   T_TYPEINFO_REG(T_NEXT(T_OPLS(expr))) ) ) {
      int indexvar = 0;
      int randaccarr = 0;

      indexvar = expr_is_indexi(T_OPLS(expr));
      randaccarr = (expr_find_at(T_OPLS(expr)) != NULL) && (T_SUBTYPE(expr_find_at(T_OPLS(expr))) == AT_RANDACC);

      if (!indexvar && !randaccarr && !expr_is_free(T_OPLS(expr))) {
	  USR_FATAL_CONT(T_STMT(expr), "Assignment of array to scalar");
      }
    }
    
    /*
     *  Make sure the LHS is not a constant
     *    (make sure has a symtab entry first)
     *  Would be nice to have this in typecheck_assign, but we need 
     *  access to the symbol table entry to get the subtype information
     *  and typecheck_assign is set up to be passed a datatype_struct
     */
    {
      symboltable_t* varpst = expr_find_root_pst(T_OPLS(expr));
      if ( varpst != NULL ) {
	switch( S_CLASS(varpst) ) {
	case S_PARAMETER:
	  if (S_IS_CONSTANT(varpst)) {
	    USR_FATAL_CONT(T_STMT(expr), "Parameter %s may not be assigned; "
			   "Use in, out, or inout", S_IDENT(varpst));
	  }
	  break;
	case S_COMPONENT:
	case S_VARIABLE:
	  if (!S_IS_CONSTANT(varpst)) {
	    break;
	  }
	case S_TYPE:
	case S_LITERAL:
	case S_SUBPROGRAM:
	case S_UNKNOWN:
	case S_ENUMTAG:
	case S_STRUCTURE:
	case S_VARIANT:
	case S_ENUMELEMENT:
	default:
	  USR_FATAL_CONT(T_STMT(expr), "Assignment into a constant");
	  break;
	}
      }
    }
    
    /*
     *  Check ranks of LHS and RHS, should be the same
     *  (or one or both of them are generic ensembles)
     *
     *  For reason for calling twice, see explaination for calling
     *  typecheck_assign() twice in it's comment above.
     */
    if (T_TYPEINFO_REG(T_NEXT(T_OPLS(expr))) != NULL &&
	!typecheck_rank(T_TYPEINFO_REG(T_OPLS(expr)), 
			T_TYPEINFO_REG(T_NEXT(T_OPLS(expr))))) {
      USR_FATAL_CONT(T_STMT(expr),
		     "Arrays of different rank not assignable");
    }

    /*
     *  Check if LHS == ensemble and RHS == Indexi, that LHS rank 
     *  great enough to hold Indexi.  (Check that LHS rank >=  i)
     */
    if ( !typecheck_index_rank( T_TYPEINFO_REG(T_OPLS(expr)), 
	    T_IDENT(T_NEXT(T_OPLS(expr))) ) ) {
      USR_FATAL_CONT(T_STMT(expr),
		     "Rank of LHS variable not large enough for Indexi");
    }

    /* check that the floodability of the ensemble and its region
       scope match */

    if (T_TYPEINFO_REG(T_OPLS(expr))) {
      CheckFloodability(T_OPLS(expr),"L-value",1,T_LINENO(T_STMT(expr)));
    }

    break;
    
  case BILOG_AND:
  case BILOG_OR:
    
  case BIPLUS:
  case BIMINUS:
  case BITIMES:
  case BIDIVIDE:
  case BIMOD:

  case BIEXP:
    
  case BIGREAT_THAN:
  case BILESS_THAN:
  case BIG_THAN_EQ:
  case BIL_THAN_EQ:
  case BIEQUAL:
  case BINOT_EQUAL:
    
    typecheck_expr(T_OPLS(expr));
    typecheck_expr(T_NEXT(T_OPLS(expr)));

    /*
     * Make sure we are operating over correct types */
    switch ( T_TYPE(expr) ) {
    case BIMOD:
      if (!(_T_IS_INTEGER(T_TYPEINFO(T_OPLS(expr))) && 
	    _T_IS_INTEGER(T_TYPEINFO(T_NEXT(T_OPLS(expr)))))) {
	USR_FATAL_CONT(T_STMT(expr), "%% operator requires integer types");
      }
      break;
      
    case BILOG_AND:
    case BILOG_OR:
      if ( strict_check ) {
	if ( S_CLASS(T_TYPEINFO(expr)) != DT_BOOLEAN ) {
	  USR_FATAL_CONT(T_STMT(expr),
			 "Applying boolean operation on non-boolean value(s)");
	}
	break;
      }
      
    case BIPLUS:
    case BIMINUS:
    case BITIMES:
    case BIDIVIDE:
    
    case BIEXP:

    case BIGREAT_THAN:
    case BILESS_THAN:
    case BIG_THAN_EQ:
    case BIL_THAN_EQ:
    
      /*
       *  Check that the types of the LHS and RHS are basic types
       */
      if ( !typecheck_basic( T_TYPEINFO(T_OPLS(expr)) ) &&
	  !typecheck_basic( T_TYPEINFO(T_NEXT(T_OPLS(expr))) ) ) {
	USR_FATAL_CONT(T_STMT(expr), "Operation not on valid types");
      }
      break;

    case BIEQUAL:
    case BINOT_EQUAL:
      /*
       *  Special Case for equal since can operate on booleans and
       *     basic types
       *
       *  Check that the types of the LHS and RHS are basic types
       *  or boolean
       */
      if ( !typecheck_basic( T_TYPEINFO(T_OPLS(expr)) ) &&
	  !typecheck_basic( T_TYPEINFO(T_NEXT(T_OPLS(expr))) ) ) {
	if ( S_CLASS(T_TYPEINFO(expr)) != DT_BOOLEAN ) {
	  USR_FATAL_CONT(T_STMT(expr), "Operation not on valid types");
	}
      }
      break;

    default:
      break;
    }

    /*
     *  Make sure types are compatible for operation
     */
    isliteral = 0;
    if ( T_IDENT(T_NEXT(T_OPLS(expr))) != NULL ) {
      isliteral = (S_CLASS(T_IDENT(T_NEXT(T_OPLS(expr)))) == S_LITERAL);
    }
    if (!typecheck_compat(T_TYPEINFO(T_OPLS(expr)),
			  T_TYPEINFO(T_NEXT(T_OPLS(expr))),isliteral,
			  strict_check,1)) {
      USR_FATAL_CONT(T_STMT(expr), "Operation on incompatible types");
    }

    /*
     *  Make sure that the operation is operating over same 
     *  rank ensembles (or an ensemble and a promoted scalar)
     */
    if (!typecheck_rank( T_TYPEINFO_REG(T_OPLS(expr)), 
			 T_TYPEINFO_REG(T_NEXT(T_OPLS(expr))) ) ) {
      USR_FATAL_CONT(T_STMT(expr), "Operation on arrays of different rank");
    }

    /*
     *  Check if LHS == ensemble and RHS == Indexi or
     *        if LHS == Indexi and RHS == ensemble, that 
     *  rank of ensemble great enough to hold Indexi
     */
    if ( (T_TYPEINFO_REG(T_OPLS(expr)) != NULL && 
	  !typecheck_index_rank(T_TYPEINFO_REG(T_OPLS(expr)), 
	                        T_IDENT(T_NEXT(T_OPLS(expr))))) ||

	 (T_TYPEINFO_REG(T_NEXT(T_OPLS(expr))) &&
          !typecheck_index_rank(T_TYPEINFO_REG(T_NEXT(T_OPLS(expr))), 
	                        T_IDENT(T_OPLS(expr)))) ) {
      USR_FATAL_CONT(T_STMT(expr), "Rank of array not large enough for Indexi");
    }

    break;
    
  case BIDOT:
    {
      datatype_t *dt;
      int reset_flood_check=0;

      if (T_TYPEINFO_REG(expr) && !on_lhs && !did_flood_check) {
	CheckFloodability(expr,"RHS-expr",0,T_LINENO(T_STMT(expr)));
	did_flood_check=1;
	reset_flood_check=1;
      }

      typecheck_expr(T_OPLS(expr));

       if (reset_flood_check) {
	did_flood_check=0;
	reset_flood_check=0;
      }

      INT_COND_FATAL((T_TYPEINFO(T_OPLS(expr))!=NULL), T_STMT(expr),
		     "null typeinfo ptr");
      dt = T_TYPEINFO(T_OPLS(expr));
      
      /* skip down past nested arrays -- req. for nloops will be inserted */
      while (D_CLASS(dt) == DT_ARRAY)
	dt = D_ARR_TYPE(dt);
      
      /* this is also done in parser */
      if (D_CLASS(dt) != DT_STRUCTURE && !datatype_complex(dt)) {
	USR_FATAL_CONT(T_STMT(expr), ". used with nonstructure");
      }

      break;
    }
  default:
    break;
  }
}


static void typecheck_exprls(expr_t *exprls) {
  while (exprls != NULL) {
    typecheck_expr(exprls);
    exprls = T_NEXT(exprls);
  }
}


/* FN: null_array_refs - return true if expr contains a null array ref
 * echris - 4-3-97
 */

static int null_array_refs(expr_t *expr) {
  if (expr == NULL) {
    return (FALSE);
  }

  if (null_array_refs(T_NEXT(expr))) 
    return (TRUE);

  if (T_TYPE(expr) == ARRAY_REF) {
    if (T_NEXT(T_OPLS(expr)) == NULL) {  /* null array ref? */
      return (TRUE);
    }
  } else {
    return (null_array_refs(T_OPLS(expr)));
  }
  return (FALSE);  /* BLC -- added 6/12/97 to avoid compiler warning */
}


static void typecheck_loop(loop_t *loop, statement_t *stmt) {
  int my_shard = 0;		/* set if this stmt starts a shard */
  int rank;

  if (loop == NULL) {
    INT_FATAL(stmt, "Null loop in typecheck_loop()");
    return;
  }

  rank = stmt_is_shard(stmt);

  switch (T_TYPE(loop)) {
  case L_DO:
    if (!((T_TYPE(T_IVAR(loop))==BIAT && 
	   T_TYPE(T_OPLS(T_IVAR(loop)))==VARIABLE)
	  || T_TYPE(T_IVAR(loop)) == VARIABLE)) {
      USR_FATAL_CONT(stmt,"for loop with non-variable iterator\n");
    }
    /* test for shard */
    if (in_mscan) {
      if (!T_TYPEINFO_REG(T_IVAR(loop)) && !expr_is_free(T_IVAR(loop))) {
	USR_FATAL_CONT(stmt,"Scalar iterator used in for loop in interleave");
      }
    }
    if (rank) {
      if (!T_TYPEINFO_REG(T_IVAR(loop)) && !expr_is_free(T_IVAR(loop))) {
	USR_FATAL_CONT(stmt,"Scalar iterator used for shattered for loop");
      }
      if (shard_rank || free_control_flow) {		/* in shard already? */
	if (shard_rank && shard_rank != rank) {
	  USR_FATAL_CONT(stmt,
			 "Nested shattered control flow (of different rank) in loop statement");
	}
      } else {			/* not in shard already */
	shard_rank = rank;	/* setup global flag */
	my_shard = 1;
      }
    } else if (expr_is_free(T_IVAR(loop)) ||
	       expr_is_free(T_START(loop)) ||
	       expr_is_free(T_STOP(loop)) ||
	       expr_is_free(T_STEP(loop))) {
      free_control_flow = 1;
      my_shard = 1;
    }
    else {
      if ((shard_rank || free_control_flow) && !expr_is_free(T_IVAR(loop))) {
	USR_FATAL_CONT(stmt,"Scalar iterator used within shattered control flow");
      }
    }

    /* if shard, test comformability of start, stop, step */

    if (my_shard) {		/* check start */
      rank = expr_rank(T_IVAR(loop));
      if (rank != 0 && shard_rank && shard_rank != rank) {
	USR_FATAL_CONT(stmt,"Rank does not conform for shattered control flow in for loop (iteration variable");
      }

      rank = expr_rank(T_START(loop));
      if (rank != 0 && shard_rank && shard_rank != rank) {
	USR_FATAL_CONT(stmt,
		       "Rank does not conform for shattered control flow in for loop (start)");
      }

      rank = expr_rank(T_STOP(loop));
      if (rank != 0 && shard_rank && shard_rank != rank) {
	USR_FATAL_CONT(stmt,
		       "Rank does not conform for shattered control flow in for loop (stop)");
      }

      rank = expr_rank(T_STEP(loop));
      if (rank != 0 && shard_rank && shard_rank != rank) {
	USR_FATAL_CONT(stmt,
		       "Rank does not conform for shattered control flow in for loop (step)");
      }
    }

    if (null_array_refs(T_START(loop))) {
      USR_FATAL_CONT(stmt, "Empty array reference in loop expression (start)");
    }

    if (null_array_refs(T_STOP(loop))) {
      USR_FATAL_CONT(stmt, "Empty array reference in loop expression (stop)");
    }

    if (null_array_refs(T_STEP(loop))) {
      USR_FATAL_CONT(stmt, "Empty array reference in loop expression (step)");
    }



    typecheck_expr(T_IVAR(loop));
    typecheck_expr(T_START(loop));
    typecheck_expr(T_STOP(loop));
    typecheck_expr(T_STEP(loop));
    
    break;
  case L_WHILE_DO:
  case L_REPEAT_UNTIL:

    if ( strict_check ) {
      if ( S_CLASS(T_TYPEINFO(T_LOOPCOND(loop))) != DT_BOOLEAN ) {
	USR_FATAL_CONT(stmt,
		       "Loop condition does not evaluate to boolean type");
      }
    } else if ( !typecheck_basic(T_TYPEINFO(T_LOOPCOND(loop))) ||
	       S_CLASS(T_TYPEINFO(T_LOOPCOND(loop))) == DT_VOID ) {
      USR_FATAL_CONT(stmt,
		     "Loop condition does not evaluate to a boolean or "
		     "numeric type");
    }

    if (null_array_refs(T_LOOPCOND(loop))) {
      USR_FATAL_CONT(stmt,
		     "Empty array reference in loop conditional");
    }

					 
    typecheck_expr(T_LOOPCOND(loop));

    /* test for shard */
    if (rank) {
      if (shard_rank) {		/* in shard already */
	if (shard_rank && shard_rank != rank) {
	  USR_FATAL_CONT(stmt,
			 "Nested shattered control flow (of different rank) in while loop");
	}
      } else {
	shard_rank = rank;
	my_shard = 1;
      }
    }
    else if (expr_is_free(T_LOOPCOND(loop))) {
      free_control_flow = 1;
      my_shard = 1;
    }

    break;
  default:
    INT_FATAL(stmt, "Bad type (%d) in typecheck_loop", T_TYPE(loop));
  }
  typecheck_stmtls(T_BODY(loop));

  if (my_shard) {
    shard_rank = 0;
    free_control_flow = 0;
  }
}

/*
 *  typecheck_array_limit
 *    Check upper or lower element of a array dimension expression
 *    to make sure valid
 */
static void typecheck_array_limit(expr_t *limit_expr,char *array_name, int first ) {
  if ( array_name == NULL ) {
    array_name = "unknown";
  }

  /*  
   *  Traverse through expression until get to an atom
   */
  if ( T_OPLS(limit_expr) != NULL ) {
    typecheck_array_limit( T_OPLS(limit_expr), array_name, first );
  }
  
  /*
   *  Check that a constant atom
   */
  if ( T_IDENT(limit_expr) != NULL ) {
    switch( S_CLASS(T_IDENT(limit_expr)) ) {
    case S_LITERAL:
    case S_VARIABLE:
    case S_PARAMETER:
      break;
    case S_SUBPROGRAM:
    case S_COMPONENT:
    case S_TYPE:
    case S_UNKNOWN:
    case S_ENUMTAG:
    case S_STRUCTURE:
    case S_VARIANT:
    case S_ENUMELEMENT:
    default:
      if (!first) {
	USR_FATAL_CONT(T_STMT(limit_expr),
		       "Use of non-constant '%s' in definition for array '%s'.", 
		       S_IDENT(T_IDENT(limit_expr)), array_name );
      }
      break;
    }
    
    /*
     *  If one of the valid symbols, then make sure the symbol is of
     *  a valid data type
     */
    switch( S_CLASS(T_IDENT(limit_expr)) ) {
    case S_VARIABLE:
      if (!(S_IS_CONSTANT(T_IDENT(limit_expr)))) {
	break;
      }
    case S_LITERAL:
      if ( S_DTYPE(T_IDENT(limit_expr)) ) {
	switch( D_CLASS(S_DTYPE(T_IDENT(limit_expr))) ) {
	case DT_INTEGER:
	case DT_CHAR:
	case DT_ENUM:
	case DT_SHORT:
	case DT_LONG:
	case DT_UNSIGNED_INT:
	case DT_UNSIGNED_SHORT:
	case DT_UNSIGNED_LONG:
	case DT_UNSIGNED_BYTE:
	case DT_SIGNED_BYTE:
	  break;
	  
	default:
	  USR_FATAL_CONT(T_STMT(limit_expr),
			 "Invalid type for '%s' in definition of array '%s'.", 
			 S_IDENT(T_IDENT(limit_expr)), array_name );
	  break;
	}
      }
      break;
      
    default:
      break;
      
    }
  }
  
  
  /*
   *  If there is another expression (this is a complex expression)
   *  then analyze the next expression
   *  Except for parameters to procedure calls.
   */
  if ( T_NEXT(limit_expr) != NULL &&
      (T_IDENT(limit_expr) != NULL &&
       S_CLASS(T_IDENT(limit_expr)) != S_SUBPROGRAM) ) {
    typecheck_array_limit( T_NEXT(limit_expr), array_name, first );
  }
  
   return;
}


/*
 *  typecheck_array_dim
 *    Check the dimensions of a array definition
 */
static void typecheck_array_dim(symboltable_t *stp ) {
  datatype_t *type;
  dimension_t *dim_list, *list_p;
  expr_t *up_var, *low_var;
  
  type = S_DTYPE( stp );
  dim_list = D_ARR_DIM( type );

  /*
   *  Step through each dimension definition of the array
   */
  for ( list_p = dim_list; list_p != NULL; list_p = DIM_NEXT(list_p) ) {
    up_var = DIM_HI( list_p );
    low_var = DIM_LO( list_p );
    
    typecheck_array_limit( up_var, S_IDENT(stp), list_p == dim_list );
    typecheck_array_limit( low_var, S_IDENT(stp), list_p == dim_list );
  }
}




static void typecheck_if(if_t *ifstmt, statement_t *stmt) {
  int my_shard = 0;		/* set if this stmt start a shard */
  int rank;

  if (ifstmt == NULL) {
    INT_FATAL(stmt, "Null ifstmt in typecheck_if()");
    return;
  }

  if (null_array_refs(T_IFCOND(ifstmt))) {
    USR_FATAL_CONT(stmt, "Empty array reference in if conditional");
  }

  rank = stmt_is_shard(stmt);

  /* test for shard */
  if (rank) {
    if (shard_rank) {		/* in shard already */
      if (shard_rank && shard_rank != rank) {
	USR_FATAL_CONT(stmt, "Nested shattered control flow (of different rank) in if statement");
      }
    } else {			/* not already in shard */
      shard_rank = rank;
      my_shard = 1;
    }
  }
  else if (expr_is_free(T_IFCOND(ifstmt))) {
    free_control_flow = 1;
    my_shard = 1;
  }

  if ( strict_check ) {
     if ( S_CLASS(T_TYPEINFO(T_IFCOND(ifstmt))) != DT_BOOLEAN ) {
       USR_FATAL_CONT(stmt, "If condition does not evaluate to boolean type");
     }
  } else if ( !typecheck_basic(T_TYPEINFO(T_IFCOND(ifstmt))) ||
	     S_CLASS(T_TYPEINFO(T_IFCOND(ifstmt))) == DT_VOID ) {
    USR_FATAL_CONT(stmt, "If condition does not evaluate to a boolean or "
		   "numeric type");
  }
  
					 
  typecheck_expr(T_IFCOND(ifstmt));
  
  typecheck_stmtls(T_THEN(ifstmt));
  typecheck_stmtls(T_ELSE(ifstmt));

  if (my_shard) {
    shard_rank = 0;
    free_control_flow = 0;
  }
}


static void typecheck_declaration(symboltable_t* pst) {
  datatype_t* pdt;

  if ( S_CLASS(pst) == S_VARIABLE && D_CLASS(S_DTYPE(pst)) == DT_ARRAY ) {
    typecheck_array_dim( pst );
  }
  pdt = datatype_find_region(S_DTYPE(pst));
  if (pdt) {
    if (S_IS_CONSTANT(pst) && D_REG_DIST(pdt) && !(S_IS_CONSTANT(D_REG_DIST(pdt)))) {
      if (!S_ANON(pst)) {
	USR_FATAL_CONTX(S_LINENO(pst), S_FILENAME(pst), "Constant region %s cannot be declared over variable distribution %s",
			S_IDENT(pst), S_IDENT(D_REG_DIST(pdt)));
      }
    }
    if (S_VAR_INIT(pst) && IN_VAL(S_VAR_INIT(pst)) &&
	T_TYPE(IN_VAL(S_VAR_INIT(pst))) != SBE &&
	T_TYPE(IN_VAL(S_VAR_INIT(pst))) != BIPREP &&
	T_TYPE(IN_VAL(S_VAR_INIT(pst))) != BIWITH &&
	!datatype_find_region(S_DTYPE(expr_find_root_pst(IN_VAL(S_VAR_INIT(pst)))))) {
      USR_FATAL_CONTX(S_LINENO(pst), S_FILENAME(pst), "Region type expected");
    }
  }
  pdt = datatype_find_distribution(S_DTYPE(pst));
  if (pdt) {
    if (S_IS_CONSTANT(pst) && D_DIST_GRID(pdt) && !(S_IS_CONSTANT(D_DIST_GRID(pdt)))) {
      USR_FATAL_CONTX(S_LINENO(pst), S_FILENAME(pst), "Constant distribution %s cannot be declared over variable grid %s",
		      S_IDENT(pst), S_IDENT(D_DIST_GRID(pdt)));
    }
  }
}


static void typecheck_stmt(statement_t *stmt) {
  expr_t* expr;

  if (stmt == NULL) {
    return;
  }

  if  (T_IS_SET1(stmt)) {
    return;
  }

  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_END:
  case S_HALT:
    {
      statement_t *stmtls;
      expr_t *expr;
      
      stmtls = T_HALT(stmt);
      while (stmtls != NULL) {
	expr = IO_EXPR1(T_IO(stmtls));
	if (expr) {
	  if (T_TYPEINFO_REG(expr) && !shard_rank && !free_control_flow) {
	    USR_FATAL_CONT(stmt, "no parallel expressions allowed within halt");
	  }
	}
	stmtls = T_NEXT(stmtls);
      }
    }
    break;
  case S_CONTINUE:
    break;
  case S_COMM:
    break;
  case S_EXIT:
    if (!stmt_in_loop(stmt)) {
      USR_FATAL_CONT(stmt, "'exit' not within loop construct");
    }
    break;
  case S_IO:			/* brad -- traverse I/O statements */
    expr = IO_EXPR1(T_IO(stmt));

    typecheck_expr(expr);
    if (expr && T_TYPEINFO(expr) && 
	D_CLASS(T_TYPEINFO(expr)) == DT_SUBPROGRAM) {
      USR_FATAL_CONT(stmt, "Cannot read or write procedures");
    }
    if (IO_FILE(T_IO(stmt)))
      typecheck_expr(IO_FILE(T_IO(stmt)));
    if (shard_rank || free_control_flow) {
      USR_FATAL_CONT(stmt, "IO in shattered control flow");
    }
    if (in_mscan) {
      USR_FATAL_CONT(stmt, "IO in interleave");
    }
    break;
  case S_WRAP:
  case S_REFLECT:	
    typecheck_expr(T_OPLS(T_WRAP(stmt)));

    if (shard_rank || free_control_flow || in_mscan) {
      if (T_TYPE(stmt) != S_WRAP || (T_WRAP_SEND(T_WRAP(stmt)))) {
	if (in_mscan) {
	USR_FATAL_CONT(stmt,
		       "Wrap/reflect used in interleave");
	}
	else {
	  USR_FATAL_CONT(stmt,
			 "Wrap/reflect used in shattered control flow");
	}
      }
    }

    if (!expr_is_lvalue(T_OPLS(T_WRAP(stmt)))) {
      USR_FATAL_CONT(stmt,
		     "Wrap/reflect on expression without l-value");
    }

    /* Check to see if arg is ensemble */
    if (!T_TYPEINFO_REG(T_OPLS(T_WRAP(stmt)))) {
      USR_FATAL_CONT(stmt, "Wrap/reflect on non-array");
    }

    break;
  case S_MLOOP:
    typecheck_stmtls(T_BODY(T_MLOOP(stmt)));
    break;
  case S_NLOOP:
    typecheck_stmtls(T_NLOOP_BODY(T_NLOOP(stmt)));
    break;
  case S_REGION_SCOPE:
    {
      
      expr_t * maskexpr;

      maskexpr = T_MASK_EXPR(T_REGION(stmt));

      if (T_SUBTYPE(T_REGION_SYM(T_REGION(stmt))) == DYNAMIC_REGION) {
	typecheck_region_dim(T_REGION_SYM(T_REGION(stmt)),1,stmt);
      }
    
      RMSPushScope(T_REGION(stmt));
      if (maskexpr) {
	CheckFloodability(maskexpr,"Mask",0,T_LINENO(stmt));
      }
      typecheck_stmtls(T_BODY(T_REGION(stmt)));
      RMSPopScope(T_REGION(stmt));

      if ( shard_rank /* || free_control_flow (allow free regions) */ ) {
	USR_FATAL_CONT(stmt,
		       "Cannot specify a new region scope within shattered control flow");
      }

      if ( in_mscan) {
	USR_FATAL_CONT(stmt,
		       "Cannot specify a new region scope within interleave");
      }
    
      /* echris todo: do the below properly */
      /* this code is not general, must search through compound stmts */
      /* We should not do this check in this way; we should use marios' */
      /* interproc region analysis */
      if (T_REGION(stmt) && (T_BODY(T_REGION(stmt)))) {
	if (((T_TYPE(T_BODY(T_REGION(stmt))) == S_WRAP) ||
	     (T_TYPE(T_BODY(T_REGION(stmt))) == S_REFLECT)) &&
	    (!T_REGION_SYM(T_REGION(stmt)) ||
	     !expr_is_ofreg(T_REGION_SYM(T_REGION(stmt))))) {
	  USR_FATAL_CONT(stmt, "Wrap/reflect used with non-of region");
	}
      }
      
      if (maskexpr) {
	symboltable_t *maskvar;

	typecheck_expr(maskexpr);
	
	if (expr_is_lvalue(maskexpr)) {
	  
	  maskvar = expr_find_root_pst(maskexpr);
	  
	  /* done in parser */
	  if (T_REGION_SYM(T_REGION(stmt)) &&
	      (D_CLASS(T_TYPEINFO(T_REGION_SYM(T_REGION((stmt))))) != DT_REGION)) {
	    USR_FATAL_CONT(stmt, "Mask applied to nonregion");
	  }
	  
	  if ((!maskvar) || !T_TYPEINFO_REG(maskexpr)) {
	    USR_FATAL_CONT(stmt, "Mask used with non-array");
	  } else {
	    if (T_TYPEINFO(maskexpr) != pdtSIGNED_BYTE &&
		T_TYPEINFO(maskexpr) != pdtUNSIGNED_BYTE &&
		T_TYPEINFO(maskexpr) != pdtBOOLEAN) {
	      USR_FATAL_CONT(stmt, "Mask expression must be of type boolean, "
			     "sbyte, or ubyte");
	    }
	  }
	} else {
	  USR_FATAL_CONT(stmt, "Mask expression must be an l-value");
	}
      }
    }
    break;

  case S_MSCAN:
    if (in_mscan) {
      USR_FATAL_CONT(stmt, "Cannot nest scan statements");
    }
    in_mscan++;
    typecheck_stmtls(T_CMPD_STLS(stmt));
    in_mscan--;
    break;
  case S_COMPOUND:
    {
      symboltable_t* stp;
      for (stp = T_CMPD_DECL(stmt); stp != NULL; stp = S_SIBLING(stp)) {
	typecheck_declaration(T_CMPD_DECL(stmt));
      }
    }
    typecheck_stmtls(T_CMPD_STLS(stmt));
    break;
  case S_EXPR:
    typecheck_expr(T_EXPR(stmt));
    break;
  case S_IF:
    typecheck_if(T_IF(stmt), stmt);
    break;
  case S_LOOP:
    typecheck_loop(T_LOOP(stmt), stmt);
    break;
  case S_RETURN:
    {
      expr_t *retexpr;
      datatype_t *retdt;
      expr_t *reg;

      if (shard_rank || free_control_flow) {
	USR_FATAL_CONT(stmt, "Return used within shattered control flow");
      }
      if (in_mscan) {
	USR_FATAL_CONT(stmt, "Return used within interleave");
      }

      retexpr = T_RETURN(stmt);
      typecheck_expr(retexpr);

      if (T_RETURN(stmt) != NULL) {
	retdt = S_FUN_TYPE(T_FCN(T_PARFCN(stmt)));
	reg = T_TYPEINFO_REG(retexpr);
	if (reg != NULL && T_IDENT(reg) != pst_qreg[0]) {
	  if (S_CLASS(retdt) == DT_ENSEMBLE) {
	    if (D_ENS_NUM(retdt) != D_REG_NUM(T_TYPEINFO(reg))) {
	      USR_FATAL_CONT(stmt,
			     "Return expression has different rank (%d) than "
			     "return type (%d)",
			     D_REG_NUM(T_TYPEINFO(reg)), D_ENS_NUM(retdt));
	    }
	  } else if (datatype_find_ensemble(retdt) == NULL) {
	    USR_FATAL_CONT(stmt,"Returning parallel expression for non-parallel"
			   " return type");
	  }
	}
	if (expr_is_free(retexpr) &&
	    !(S_STYPE(T_FCN(T_PARFCN(stmt))) & SC_FREE)) {
	  USR_FATAL_CONT(stmt,
			 "Can only return free expressions in "
			 "free procedures");
	}
	if (!typecheck_compat(retdt,T_TYPEINFO(retexpr),0,0,0)) {
	  USR_FATAL_CONT(stmt, "Return expression doesn't match return type");
	}
      }
    }
    break;
  default:
    INT_FATAL(stmt,"Bad statement type (%d) in typecheck_stmt()",T_TYPE(stmt));
  }
}


static void typecheck_stmtls(statement_t *stmtls) {
  while (stmtls != NULL) {
    typecheck_stmt(stmtls);
    stmtls = T_NEXT(stmtls);
  }
}


static void get_typecheck_fcn(function_t *fcn) {
  symboltable_t *stp;

  for ( stp = T_DECL(fcn); stp != NULL; stp = S_SIBLING(stp) ) {
    typecheck_declaration(stp);
  }
  typecheck_stmtls(T_STLS(fcn));
}


static void typecheck(module_t *module) {
  function_t	*ftp;
  symboltable_t *stp;
  
  DB0(20,"typecheck()\n");

  if ( !typecheck_code )
      return;
      
  /*
   *  Make sure the Region declarations are good
   *  (only use literals, constants, and config vars of non-fractional type
   */

  for ( stp = T_DECL(module); stp != NULL; stp = S_SIBLING(stp) ) {
    typecheck_declaration(stp);
  }

  stp = T_DECL(D_FUN_BODY(T_TYPE(pstMAIN)));
  while (stp != NULL) {
    if (S_FG_PARAM(stp)) {
      USR_FATAL_CONTX(S_LINENO(pstMAIN), S_FILENAME(pstMAIN),
		      "entry procedure (%s) cannot take parameters",
		      S_IDENT(pstMAIN));
      break;
    }
    stp = S_SIBLING(stp);
  }


  for (; module != NULL; module = T_NEXT(module)) {
    ftp = T_FCNLS(module);
    if (ftp == NULL) {
      INT_FATAL(NULL, "No functions in module in typecheck()");
    }
    while (ftp != NULL) {
      get_typecheck_fcn(ftp);
      ftp = T_NEXT(ftp);
    }
  }
}


int call_typecheck(module_t *module, char *s) {
  RMSInit();
  FATALCONTINIT();
  typecheck(module);
  FATALCONTDONE();
  RMSFinalize();
  return 0;
}


static void typecheck_region(expr_t* expr) {
  if (T_TYPE(expr) == SBE) {
    dimension_t* dimlist = T_SBE(expr);

    while (dimlist != NULL) {
      if (DIM_TYPE(dimlist) == DIM_FLAT || DIM_TYPE(dimlist) == DIM_RANGE) {
	if (D_CLASS(typeinfo_expr(DIM_LO(dimlist))) != DT_INTEGER) {
	  USR_FATAL_CONT(T_STMT(expr), "Region bounds must be integers");
	}
      }
      if (DIM_TYPE(dimlist) == DIM_RANGE) {
	if (D_CLASS(typeinfo_expr(DIM_HI(dimlist))) != DT_INTEGER) {
	  USR_FATAL_CONT(T_STMT(expr), "Region bounds must be integers");
	}
      }
      dimlist = DIM_NEXT(dimlist);
    }
  }
  else if (D_CLASS(T_TYPEINFO(expr)) != DT_REGION) {
    USR_FATAL_CONT(T_STMT(expr), "Region type expected");
  }
}


static void typecheck_preserve_destroy(expr_t* expr) {
  int preserve = strcmp(S_IDENT(T_IDENT(T_OPLS(expr))), "_PRESERVE") == 0;
  if ((D_CLASS(S_DTYPE(T_IDENT(T_NEXT(T_OPLS(expr))))) != DT_REGION) &&
      (D_CLASS(S_DTYPE(T_IDENT(T_NEXT(T_OPLS(expr))))) != DT_DIRECTION) &&
      (D_CLASS(S_DTYPE(T_IDENT(T_NEXT(T_OPLS(expr))))) != DT_DISTRIBUTION) &&
      (D_CLASS(S_DTYPE(T_IDENT(T_NEXT(T_OPLS(expr))))) != DT_GRID)) {
    if (preserve) {
      USR_FATAL_CONT(T_STMT(expr), "Illegal <#= statement, bad LHS");
    } else {
      USR_FATAL_CONT(T_STMT(expr), "Illegal <== statement, bad LHS");
    }
  }
  if (S_IS_CONSTANT(T_IDENT(T_NEXT(T_OPLS(expr))))) {
    if (preserve) {
      USR_FATAL_CONT(T_STMT(expr), "Constant used in <#= statement");
    } else {
      USR_FATAL_CONT(T_STMT(expr), "Constant used in <== statement");
    }
  }
  if (D_CLASS(S_DTYPE(T_IDENT(T_NEXT(T_OPLS(expr))))) == DT_REGION) {
    typecheck_region(T_NEXT(T_NEXT(T_OPLS(expr))));
  }
}
