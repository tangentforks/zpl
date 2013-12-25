/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* FILE: Wgen.c - walker code gen
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include "../include/Agen.h"
#include "../include/Bgen.h"
#include "../include/Cgen.h"
#include "../include/Mgen.h"
#include "../include/Privgen.h"
#include "../include/Spsgen.h"
#include "../include/Wgen.h"
#include "../include/alias.h"
#include "../include/datatype.h"
#include "../include/dimension.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/runtime.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/nudge.h"

static elist gelist=NULL;
int use_charstar_pointers=1;
int use_additive_offsets=0;

static int in_old_mloop = 0;

void WgenSetInOldMloop() {
  in_old_mloop++;
}


void WgenSetOutOldMloop() {
  in_old_mloop--;
}


static void gen_walker_name_help(FILE *outfile,expr_t *expr,int uid) {
  switch(T_TYPE(expr)) {
  case VARIABLE:
    if (outfile)
      fgenf(outfile, "%_", S_IDENT(T_IDENT(expr)));
    else
      return;
    break;
  case BIDOT:
    gen_walker_name_help(outfile,T_OPLS(expr),uid);
    if (outfile)
      fgenf(outfile, "%_", S_IDENT(T_IDENT(expr)));
    else
      return;
    break;
  case ARRAY_REF:
    gen_walker_name_help(outfile,T_OPLS(expr),uid);
    /* note that this use of a "unique id" as the indexing expression is
       completely arbitrary.  We could convert the actual indices into
       an identifier-friendly string, but this was just simpler to do and
       got rid of questions of names that look different but alias -BLC */
    fprintf(outfile,"Z%dZ",uid);
    break;
  default:
    INT_FATAL(NULL,"Problem in gen_walker_name_help()");
    break;
  }
}


void gen_walker_name(FILE* outfile,elist node) {
  gen_walker_name_help(outfile,ELIST_DATA(node),ELIST_INDEX(node));
}


void gen_walker_name_from_expr(FILE* outfile, expr_t* expr) {
  elist node;

  node = (elist)glist_find((glist)gelist,expr);
  if (node != NULL) {
    gen_walker_name(outfile, node);
  } else {
    if (T_TYPE(expr) != VARIABLE && T_TYPE(expr) != CONSTANT) {
      INT_FATAL(T_STMT(expr), 
		"expression not found in gen_walker_name_from_expr()");
    } else {
      brad_no_access++;
      gen_expr(outfile, expr);
      brad_no_access--;
    }
  }
}


static void gen_bumper(FILE *outfile,elist node,int dim) {
  fprintf(outfile,"_Bumper");
  fprintf(outfile,"_");
  gen_walker_name(outfile,node);
  fprintf(outfile,"_%d",dim);
}


static void gen_tile_bumper(FILE* outfile,elist node,int dim,int bumptype,
			    int nopeel) {
  if (nopeel) fprintf(outfile,"_G");
  gen_bumper(outfile,node,dim);
  switch (bumptype) {
  case BUMP_NEXT:
    break;
  case BUMP_NEXT_IN_THIN:
    fprintf(outfile,"_Thin");
    break;
  case BUMP_TILE:
    fprintf(outfile,"_Tile");
    break;
  case BUMP_THIN_TILE:
    if (!nopeel) fprintf(outfile,"_STil");
    else fprintf(outfile,"_Tile");
    break;
  default:
    INT_FATAL(NULL,"Unknown bumper type in gen_tile_bumper");
  }
}


static void gen_walker(FILE *outfile,elist node, expr_t* dir) {
  fprintf(outfile,"_Walker_");
  gen_walker_name(outfile,node);
  if (dir != NULL) {
    fprintf(outfile, "_");
    gen_dir_name_as_ident(outfile, dir);
  }
}


static void gen_walker_type(FILE *outfile,elist node,int type) {
  gen_pdt(outfile,T_TYPEINFO(ELIST_DATA(node)),type);
}


static void gen_walker_decl_type(FILE* outfile, elist node) {
  if (use_charstar_pointers) {
    fprintf(outfile, "char");
  } else {
    gen_walker_type(outfile, node, PDT_LOCL);
  }
  fprintf(outfile, "*");
}


static void gen_walker_cast(FILE* outfile, elist node, int init) {
  /* for char* walkers, cast needed for use;
     otherwise, for init */
  if (use_charstar_pointers  != init) {
    fprintf(outfile, "(");
    gen_walker_type(outfile,node,PDT_PCST);
    fprintf(outfile," *)");
  }
}


static void gen_bumper_scale(FILE* outfile, elist node) {
  if (!use_charstar_pointers) {
    fprintf(outfile, ")/((int)sizeof(");
    gen_walker_type(outfile, node, PDT_SIZE);
    fprintf(outfile,")");
  }
}


static void gen_offset(FILE *outfile,elist node,expr_t *dir) {
  fprintf(outfile,"_Offset_");
  gen_walker_name(outfile,node);
  fprintf(outfile,"_");
  gen_expr(outfile, dir);
}


int gen_walker_with_offset(FILE *outfile,expr_t *expr) {
  elist node;
  expr_t *dir;
  int density;
  int i;
  expr_t* atexpr;
  expr_t* nudgexpr;
  
  node = (elist)glist_find((glist)gelist,expr);
  if (node == NULL) {
    INT_FATAL(NULL,"BIAT expression not found in list");
  }
  density = ELIST_DENSITY(node);
  if ((current_mloop_type & MLOOP_SPARSE_BIT) && (density == EXPR_DENSE)) {
    return 0;
  }

  dir = currentDirection;
  if (density == EXPR_DENSE) {
    atexpr = ELIST_ATNODE(node);
    if (expr_nudged(atexpr)) {
      nudgexpr = atexpr;
    } else if (expr_nudged(T_PARENT(expr))) {
      nudgexpr = T_PARENT(expr);
    } else {
      nudgexpr = NULL;
    }
    fprintf(outfile,"(");
    fprintf(outfile,"*");
    gen_walker_cast(outfile,node,FALSE);
    if (dir != NULL || nudgexpr != NULL) {
      fprintf(outfile,"(");
    }
    if (use_additive_offsets || in_old_mloop) {
      gen_walker(outfile, node, NULL);
      if (dir != NULL) {
	fprintf(outfile," + ");
	gen_offset(outfile,node,dir);
      }
    } else {
      gen_walker(outfile, node, dir);
    }
    if (nudgexpr != NULL) {
      for (i = 0; i < expr_rank (nudgexpr); i++) {
	if (T_GET_NUDGE (nudgexpr, i)) {
	  fprintf (outfile, " + ");
	  if (T_GET_NUDGE (nudgexpr, i) != 1) {
	    fprintf (outfile, "%d * ", T_GET_NUDGE (nudgexpr, i));
	  }
	  gen_bumper (outfile, node, i);
	}
      }
    }
    if (dir != NULL || nudgexpr != NULL) {
      fprintf (outfile, ")");
    }
    fprintf(outfile,")");
  } else {
    if (ELIST_WRITTEN(node)) {
      fprintf(outfile,"*");
    }
    gen_sps_arr_access(outfile,node,dir);
  }

  return 1;
}


/* FUNCTION: search_downfor_ensemble_expr
 * Search for BIAT expr nodes starting at expr.
 * RETURN: a list of all the BIAT nodes found
 * echris  7-1-94
 */
static glist search_downfor_ensemble_exprs_help(expr_t *expr, 
						expr_t* at_save_in) {
  int i;
  auto expr_t *expr_save;
  glist list1, list2, list3;
  elist newnode;
  expr_t* tmp;
  expr_t* at_save_out = at_save_in;

  if (expr && T_DUMMY(expr) != 1) {
    expr_save = NULL;


    /* SPARSE CHANGE #2: to make sparse compute work and other things
       fail, switch the comment on the following two lines: */
 /* if (expr_at_ensemble_root(expr)) { */
    if (expr_at_ensemble_root(expr) && 
	D_CLASS(T_TYPEINFO(expr)) != DT_REGION) {
      expr_save = expr;
    }

    if (T_TYPE(expr) == BIAT) {
      at_save_out = expr;
    }
    list1 = search_downfor_ensemble_exprs_help(T_OPLS(expr), at_save_out);
    list2 = search_downfor_ensemble_exprs_help(T_NEXT(expr), at_save_in);

    list3 = glist_append_list(list1, list2);

    if (T_TYPE(expr) == BIAT && T_SUBTYPE(expr) == AT_RANDACC) {
      tmp = T_RANDACC(expr);
      if (tmp != NULL) {
	for (i = 0; i < T_MAPSIZE(expr); i++) {
	  list1 = search_downfor_ensemble_exprs_help(tmp, at_save_out);
	  list3 = glist_append_list(list1, list3);
	  tmp = T_NEXT(tmp);
	}
      }
    }
    /*** above, is that still used? ***/
    if (T_MAP(expr) && T_MAP(expr) != (expr_t*)0x1) {
      tmp = T_MAP(expr);
      for (i = 0; i < T_MAPSIZE(expr); i++) {
	list1 = search_downfor_ensemble_exprs_help(tmp, at_save_out);
	list3 = glist_append_list(list1, list3);
	tmp = T_NEXT(tmp);
      }
    }

    if (list3) {
      if (expr_save) {
	newnode = (elist)glist_append(list3, expr_save, ELIST_NODE_SIZE);
	ELIST_ATNODE(newnode) = at_save_out;
	return (list3);
      } else {
	return (list3);
      }
    } else
      if (expr_save) {
	newnode = (elist)glist_create(expr_save, ELIST_NODE_SIZE);
	ELIST_ATNODE(newnode) = at_save_out;
	return (glist)newnode;
      } else {
	return (NULL);
      }
  }
  return (NULL);
}

glist search_downfor_ensemble_exprs(expr_t *expr) {
  return search_downfor_ensemble_exprs_help(expr, NULL);
}


static expr_t *find_ens_expression(expr_t *expr) {
  expr_t *retval;

  if (T_TYPEINFO_REG(expr) == NULL) {
    INT_FATAL(NULL,
	      "Assumption error 1 in find_ens_expression() (Wgen.c: brad)");
  }
  
  switch (T_TYPE(expr)) {
  case VARIABLE:
    if (D_CLASS(S_DTYPE(T_IDENT(expr))) != DT_ENSEMBLE &&
	D_CLASS(S_DTYPE(T_IDENT(expr))) != DT_REGION) {
      INT_FATAL(T_STMT(expr),
		"Assumption error 2 in find_ens_expression() (Wgen.c: brad)");
    }
    retval = expr;
    break;

  case BIAT:
    retval=find_ens_expression(T_OPLS(expr));
    break;

  case ARRAY_REF:
  case BIDOT:
    if (T_TYPEINFO_REG(T_OPLS(expr)) == NULL) {
      retval = expr;
    } else {
      retval=find_ens_expression(T_OPLS(expr));
    }
    break;

  default:
    INT_FATAL(NULL, "Unexpected type in find_ens_expression: %d "
	      "(Wgen.c - brad)", T_TYPE(expr));
    break;
  }

  return retval;
}


static int same_arrayref(expr_t *expr1,expr_t *expr2) {
  int retval;

  while (expr1 != NULL && expr2 != NULL) {
    if (T_TYPE(expr1) != T_TYPE(expr2)) {
      retval = 0;
    } else {
      switch(T_TYPE(expr1)) {
      case VARIABLE:
      case CONSTANT:
	retval = (T_IDENT(expr1) == T_IDENT(expr2));
	break;
      default:
	retval = 0;
	break;
      }
      expr1 = T_NEXT(expr1);
      expr2 = T_NEXT(expr2);
    }
    if (retval == 0) {
      break;
    }
  }

  return retval;
}


static int check_same_var(expr_t *expr1,expr_t *expr2) {
  int retval;

  if (T_TYPE(expr1) != T_TYPE(expr2)) {
    retval = 0;
  } else {
    switch (T_TYPE(expr1)) {
    case VARIABLE:
      if (T_IDENT(expr1) == NULL) {
	INT_FATAL(NULL, 
		  "Assumption error 1 in check_same_var() (Wgen.c: brad)");
      }
      retval = (T_IDENT(expr1) == T_IDENT(expr2));
      break;

    case BIAT:
      retval = check_same_var(T_OPLS(expr1),T_OPLS(expr2));
      break;

    case ARRAY_REF:
      retval = check_same_var(T_OPLS(expr1),T_OPLS(expr2));
      if (retval) {
	retval = same_arrayref(nthoperand(expr1,2),nthoperand(expr2,2));
      }
      break;

    case BIDOT:
      if (S_IDENT(T_IDENT(expr1)) != S_IDENT(T_IDENT(expr2))) {
	retval = 0;
      } else {
	retval = check_same_var(T_OPLS(expr1),T_OPLS(expr2));
      }
      break;

    default:
      INT_FATAL(NULL, "Unexpected type in check_same_var: %d (Wgen.c - brad)",
		 T_TYPE(expr1));
      break;
    }
  }

  return retval;
}


static int equivalent_ensembles(expr_t *expr1,expr_t *expr2) {
  expr_t *sexpr1;
  expr_t *sexpr2;
  int retval;

  sexpr1 = find_ens_expression(expr1);  /* strip both expressions to */
  sexpr2 = find_ens_expression(expr2);  /* first ens subexpression */

  if (T_TYPEINFO_REG(sexpr1) != T_TYPEINFO_REG(sexpr2) ||
      T_TYPEINFO(sexpr1) != T_TYPEINFO(sexpr2)) {  /* obvious mismatch */
    retval = 0;
  } else {  /* in this case, we must see if the two are the same var */
    retval = check_same_var(sexpr1,sexpr2);
  }    

  return retval;
}


/* FUNCTION: search_for_same_ensemble
 * Search through list for ensemble "congruent" to that in expr.  Only
 * search first length elements.  If expr is not of type BIAT, search list
 * for same expression.
 * RETURN: the counter of the 1st node that matches
 *         -1 otherwise
 * echris  7-5-94
 */
static elist find_same_ensemble(elist list, expr_t *expr, int length) {
  elist node;
  expr_t *lexpr;
  
  node = list;
  while (length--) {
    if (!node) {
      break;
    }
    lexpr = ELIST_DATA(node);
    if (equivalent_ensembles(expr,lexpr)) {
      return node;
    }
    node = ELIST_NEXT(node);
  }
  return NULL;
}


static int find_same_dir(glist list,expr_t *dir) {
  glist node;
  
  node = list;
  while (node != NULL) {
    if (expr_equal(GLIST_DATA(node),dir)) {
      return 1;
    }
    node = GLIST_NEXT(node);
  }
  return 0;
}


/* FUNCTION: search_for_ensembles_in_stmt_ls
 * Search for BIAT expr nodes in statement list stmt_ls
 * RETURN: a list of all the BIAT nodes found
 * NOTE: numbers each node (i.e. fills in index field)
 * echris  7-1-94
 */

elist search_for_ensembles_in_stmt_ls(statement_t *stmt_ls) {
  /* remember to free expr list someplace */
  expr_t *expr;
  glist list, work_list;
  statement_t *stmt_ls2;

  work_list = NULL;
  list = NULL;
  while (stmt_ls) {
    switch(T_TYPE(stmt_ls)) {
    case S_EXPR: 
      expr = T_EXPR(stmt_ls);
      list = search_downfor_ensemble_exprs(expr);
      work_list = glist_append_list(work_list, list);
      break;
    case S_NLOOP:
      stmt_ls2 = T_NLOOP_BODY(T_NLOOP(stmt_ls));
      list = (glist) search_for_ensembles_in_stmt_ls(stmt_ls2);
      work_list = glist_append_list(work_list, list);
      break;
    case S_IF:
      stmt_ls2 = T_THEN(T_IF(stmt_ls));
      list = (glist) search_for_ensembles_in_stmt_ls(stmt_ls2);
      work_list = glist_append_list(work_list, list); 
      stmt_ls2 = T_ELSE(T_IF(stmt_ls));
      list = (glist) search_for_ensembles_in_stmt_ls(stmt_ls2);
      work_list = glist_append_list(work_list, list);

      expr = T_IFCOND(T_IF(stmt_ls));
      list = search_downfor_ensemble_exprs(expr);
      work_list = glist_append_list(work_list, list);

      break;
    case S_LOOP:
      {
	stmt_ls2 = T_BODY(T_LOOP(stmt_ls));
	list = (glist) search_for_ensembles_in_stmt_ls(stmt_ls2);
	work_list = glist_append_list(work_list, list);

	switch (T_TYPE(T_LOOP(stmt_ls))) {
	case L_DO:
	  expr = T_IVAR(T_LOOP(stmt_ls));
	  list = search_downfor_ensemble_exprs(expr);
	  work_list = glist_append_list(work_list, list);

	  expr = T_START(T_LOOP(stmt_ls));
	  list = search_downfor_ensemble_exprs(expr);
	  work_list = glist_append_list(work_list, list);

	  expr = T_STOP(T_LOOP(stmt_ls));
	  list = search_downfor_ensemble_exprs(expr);
	  work_list = glist_append_list(work_list, list);

	  expr = T_STEP(T_LOOP(stmt_ls));
	  list = search_downfor_ensemble_exprs(expr);
	  work_list = glist_append_list(work_list, list);

	  break;

	case L_WHILE_DO:
	case L_REPEAT_UNTIL:
	  expr = T_LOOPCOND(T_LOOP(stmt_ls));
	  list = search_downfor_ensemble_exprs(expr);
	  work_list = glist_append_list(work_list, list);

	  break;
	default:
	  INT_FATAL(stmt_ls, "bad loop type (%d) in "
		    "search_for_ensembles_in_stmt_ls",T_TYPE(T_LOOP(stmt_ls)));
	}
      }
      break;

    case S_COMPOUND:
      stmt_ls2 = T_CMPD_STLS(stmt_ls);
      list = (glist) search_for_ensembles_in_stmt_ls(stmt_ls2);
      work_list = glist_append_list(work_list, list);
      break;

    default:
      break;
    }
    stmt_ls = T_NEXT(stmt_ls);
  }

  return ((elist)work_list);
}


void number_elist(elist list) {
  elist node;
  expr_t *expr;
  expr_t* atexpr;
  expr_t *dir;
  int done_so_far = 0;
  elist oldnode;
  glist dirlist;

  node = list;
  while (node) {
    expr = ELIST_DATA(node);
    atexpr = ELIST_ATNODE(node);
    if (atexpr) {
      dir = T_NEXT(T_OPLS(atexpr));
    } else {
      dir = NULL;
    }
    ELIST_INDEX(node) = done_so_far;
    ELIST_ARR_COUNTS(node) = 0;
    ELIST_DIRLIST(node) = NULL;
    oldnode = find_same_ensemble(list, expr, done_so_far);
    if (oldnode) {
      ELIST_INDEX(node) = ELIST_INDEX(oldnode);
      dirlist = ELIST_DIRLIST(oldnode);
      if (dirlist == NULL) {
	ELIST_DIRLIST(oldnode) = glist_create(dir,GLIST_NODE_SIZE);
      } else {
	if (!find_same_dir(dirlist,dir)) {
	  glist_append(dirlist,dir,GLIST_NODE_SIZE);
	}
      }
    } else {
      ELIST_DIRLIST(node) = glist_create(dir,GLIST_NODE_SIZE);
      ELIST_ARR_COUNTS(node) = 1;
    }

    node = ELIST_NEXT(node);
    done_so_far++;
  }
}


static int expr_written(expr_t* expr,statement_t* body) {
  statement_t* stmt;
  set_t* outset;
  genlist_t* outexprlist;

  if (body == NULL) { /* if we're not sure of the body, we must assume it is */
    return 1;
  }
  stmt = body;
  while (stmt != NULL) {
    outset = T_OUT(stmt);
    while (outset) {
      outexprlist = SET_EXPRS(outset);
      while (outexprlist) {
	if (exprs_alias(expr,G_EXP(outexprlist))) {
	  return 1;
	}

	outexprlist = G_NEXT(outexprlist);
      }

      outset = SET_NEXT(outset);
    }

    stmt = T_NEXT(stmt);
  }

  return 0;
}


static rlist categorize_elist(elist list,expr_t *reg,statement_t* body) {
  elist node;
  expr_t *expr;
  int density;
  symboltable_t *arrpst;
  expr_t *arrreg;
  rlist reglist=NULL;
  dlist dirlist;

  /* if the region controlling the MLOOP is sparse, add it to the reglist 
   with a single NULL direction */
  if (!expr_is_dense_reg(reg)) {
    reglist = rlist_create(reg);
    dirlist = dlist_create(NULL);
    RLIST_DIRLIST(reglist) = dirlist;
    DIRLIST_EXPRLS(dirlist) = NULL;
  }

  node = list;
  while (node) {
    expr = ELIST_DATA(node);
    arrpst = expr_find_ens_pst(expr);
    arrreg = T_TYPEINFO_REG(expr);
    if (expr_is_dense_reg(arrreg)) {
      density = EXPR_DENSE;
    } else {
      density = EXPR_SPARSE;
    }
    if (density == EXPR_SPARSE) {
      reglist = add_sps_expr_to_reglist(reglist,node,arrreg,reg,&density);
    }
    ELIST_DENSITY(node) = density;
    ELIST_WRITTEN(node) = expr_written(expr,body);

    node = ELIST_NEXT(node);
  }
  return reglist;
}


static void gen_bumper_decl_inits(FILE* outfile,int numdims,int order[],
				  int up[],int tiledloop,int tiledorder[],
				  int tiledup[],int tile[],expr_t *tile_expr[],
				  expr_t* expr,elist node,int nopeel,
				  int flat[]) {
  int i;
  int dim;

  if (!tiledloop) {
    /* generate dense bumpers */
    for (i=numdims-1;i>=0;i--) {
      dim = order[i];

      if (!flat[dim]) {
	fprintf(outfile,"const int ");

	gen_bumper(outfile,node,dim);
	fprintf(outfile," = (");
	gen_access_distance(outfile,expr,numdims,i,order,up,flat);
	gen_bumper_scale(outfile, node);
	fprintf(outfile,");\n");
      }
    } 
 } else {
    /* generate dense bumpers for tiled loop */
    for (i=numdims-1;i>=0;i--) {
      dim = order[i];
      if (tile[dim] != 1) {
	fprintf(outfile,"const int ");
	gen_tile_bumper(outfile,node,dim,BUMP_NEXT,FALSE);
	fprintf(outfile,"      = (");
	gen_bump_distance(outfile,expr,numdims,i,order,up,tile,tile_expr,
			  FALSE,flat);
	gen_bumper_scale(outfile, node);
	fprintf(outfile,");\n");

	if (nopeel) {
	  fprintf(outfile,"int ");
	  gen_tile_bumper(outfile,node,dim,BUMP_NEXT,TRUE);
	  fprintf(outfile,";\n");
	}
      }
    }
    for (i=numdims-2;i>=0;i--) {
      dim = order[i];
      if (1) { /* BLC -- some nontrivial condition that should be here */
        fprintf(outfile,"const int ");
	gen_tile_bumper(outfile,node,dim,BUMP_NEXT_IN_THIN,FALSE);
	fprintf(outfile," = (");
	gen_bump_distance(outfile,expr,numdims,i,order,up,tile,tile_expr,TRUE,
			  flat);
	gen_bumper_scale(outfile, node);
	fprintf(outfile,");\n");
      }
    }
    for (i=numdims-1;i>=0;i--) {
      dim = tiledorder[i];
      if (tile[dim]) {
	fprintf(outfile,"const int ");
	gen_tile_bumper(outfile,node,dim,BUMP_TILE,FALSE);
	fprintf(outfile," = (");
	gen_bump_tile_distance(outfile,expr,numdims,i,order,up,tile,tile_expr,
			       tiledorder,tiledup,FALSE,flat);
	gen_bumper_scale(outfile, node);
	fprintf(outfile,");\n");

      }
    }
    if (nopeel) {
      fprintf(outfile,"int ");
      gen_tile_bumper(outfile,node,tiledorder[numdims-1],BUMP_TILE,TRUE);
      fprintf(outfile,";\n");
    }

    for (i=numdims-1;i>=0;i--) {
      dim = tiledorder[i];
      fprintf(outfile,"const int ");
      gen_tile_bumper(outfile,node,dim,BUMP_THIN_TILE,FALSE);
      fprintf(outfile," = (");
      gen_bump_tile_distance(outfile,expr,numdims,i,order,up,tile,tile_expr,
			     tiledorder,tiledup,TRUE,flat);
      gen_bumper_scale(outfile, node);
      fprintf(outfile,");\n");
    }
  }
}


int walker_in_use(expr_t* expr) {
  return (glist_find((glist)gelist,expr) != NULL);
}


int walker_sparse(expr_t* expr) {
  elist node = (elist)glist_find((glist)gelist, expr);

  if (node != NULL) {
    return (ELIST_DENSITY(node) == EXPR_SPARSE);
  } else {
    return 0;
  }
}



/* FUNCTION: gen_walker_decl
 * echris  7-27-94
 * hacked on a lot by brad since then
 */

void gen_walker_decl_inits(FILE *outfile,expr_t *mloop_reg,int numdims,
			   int order[],int up[],int tile[],elist wblist,
			   rlist *reglist,int flat[]) {
  int i;
  elist node;
  glist dirnode;
  expr_t *dir;
  expr_t* expr;
  int first;

  gelist = wblist;
  number_elist(wblist);
  *reglist = categorize_elist(wblist,mloop_reg,NULL);

  node = wblist;
  while (node) {
    expr = ELIST_DATA(node);
    if (ELIST_ARR_COUNTS(node)) {

      if (ELIST_DENSITY(node) == EXPR_DENSE) {
	if ((current_mloop_type & MLOOP_SPARSE_BIT) == 0) {

	  /* generate a dense walker */
	  fprintf(outfile,"register ");
	  gen_walker_decl_type(outfile, node);
	  if (restrictkw) {
	    fprintf(outfile, " restrict ");
	  }
	  gen_walker(outfile, node, NULL);
	  fprintf(outfile," = ");
	  gen_walker_cast(outfile, node, TRUE);
	  gen_access_begin(outfile,expr,0);
	  gen_access_end(outfile,expr,NULL);
	  fprintf(outfile,";\n");

	  gen_bumper_decl_inits(outfile,numdims,order,up,0,NULL,NULL,NULL,
				NULL,expr,node,FALSE,flat);

	  /* generate dense offsets */
	  dirnode = ELIST_DIRLIST(node);
	  while (dirnode) {
	    dir = GLIST_DATA(dirnode);
	    if (dir) {
	      fprintf(outfile,"const int ");
	      gen_offset(outfile,node,dir);
	      fprintf(outfile, " = (");
	      first = 1;
	      for (i = 0; i < numdims; i++) {
		if (D_DIR_SIGN(T_TYPEINFO(dir),i) != SIGN_ZERO) {
		  if (!first) {
		    fprintf(outfile, "+");
		  }
		  first=0;
		  
		  if (T_TYPEINFO_REG(expr) == NULL) {
		    INT_FATAL(NULL,"BLC -- bad assumption in offset generation");
		  }
		  if (expr_is_ever_strided_ens(expr)) {
		    /*current_mloop_type == MLOOP_STRIDED) { 
		      ^^^ if loop is dense, array can't be strided 
		      Could potentially do interprocedural analysis to
		      tighten this up even further  */
		    fprintf(outfile,"_STR");
		  }
		  fprintf(outfile,"_OFFSET(");
		  brad_no_access++;
		  gen_expr(outfile, expr);
		  brad_no_access--;
		  fprintf(outfile, ",%d,",i);
		  dir_gen_comp(outfile,dir,i);
		  fprintf(outfile,")");
		}
	      }
	      if (first) {
		fprintf(outfile,"0");
	      }
	      gen_bumper_scale(outfile, node);
	      fprintf(outfile,");\n");
	    }
	    dirnode = GLIST_NEXT(dirnode);
	  }
	}
      } else {
	/* generate sparse base and walkers */
	gen_sps_base_n_walker_decl_inits(outfile,node);
      }
    }
    node = ELIST_NEXT(node);
  }

  gen_sps_reg_walkers(outfile,*reglist,mloop_reg,numdims,order);
}


static void gen_walker_init(FILE* outfile,mloop_t* mloop,expr_t* dir) {
  int numdims = T_MLOOP_RANK(mloop);
  int* up = T_MLOOP_DIRECTION_V(mloop);
  int* tile_up = T_MLOOP_TILE_DIRECTION_V(mloop);
  int* tile = T_MLOOP_TILE_V(mloop);
  expr_t** tile_expr = T_MLOOP_TILE_EXPR_V(mloop);
  int* flat = T_MLOOP_FLAT_V(mloop);
  int i;

  for (i=0; i<numdims; i++) {
    fprintf(outfile,",");
    fprintf(outfile,"_WALK_");
    if (tile[i] > 1) {
      fprintf(outfile,"TILE_");
      gen_updn(outfile,tile_up[i]);
      fprintf(outfile,"_");
    }
    gen_updn(outfile,up[i]);
    if (flat[i]) {
      fprintf(outfile,"_FLAT");
    }
    fprintf(outfile,"(%d",i);
    if (tile[i] > 1) {
      fprintf(outfile,",");
      if (tile_expr[i]) gen_expr(outfile, tile_expr[i]);
      else fprintf(outfile,"%d",tile[i]);
    }
    fprintf(outfile,")");
    if (dir != NULL) {
      fprintf(outfile,"+");
      dir_gen_comp(outfile, dir, i);
    }
  }
  fprintf(outfile,")))");
}


void gen_rec_walker_decl_inits(FILE *outfile,mloop_t* mloop) {
  expr_t* mloop_reg = T_MLOOP_REG(mloop);
  int numdims = T_MLOOP_RANK(mloop);
  int* tile = T_MLOOP_TILE_V(mloop);
  expr_t** tile_expr = T_MLOOP_TILE_EXPR_V(mloop);
  int* order = T_MLOOP_ORDER_V(mloop);
  int* up = T_MLOOP_DIRECTION_V(mloop);
  int* tileorder = T_MLOOP_TILE_ORDER_V(mloop);
  int* tileup = T_MLOOP_TILE_DIRECTION_V(mloop);
  int nopeel = T_MLOOP_TILE_NOPEEL(mloop);
  elist wblist = T_MLOOP_WBLIST(mloop);
  int* flat = T_MLOOP_FLAT_V(mloop);
  rlist reglist;
  int i;
  elist node;
  glist dirnode;
  expr_t *dir;
  expr_t* expr;
  int first;
  int tiledloop;

  gelist = wblist;

  number_elist(wblist);
  reglist = categorize_elist(wblist,mloop_reg,T_MLOOP_BODY(mloop));
  T_MLOOP_REGLIST(mloop) = reglist;

  tiledloop = 0;
  for (i=0;i<numdims;i++) {
    if (tile[i] != 0) {
      tiledloop = 1;
    }
  }

  node = wblist;
  while (node) {
    expr = ELIST_DATA(node);
    if (ELIST_ARR_COUNTS(node)) {

      if (ELIST_DENSITY(node) == EXPR_DENSE) {
	if (!new_access) {
	  /* print nothing */
	} else if ((current_mloop_type & MLOOP_SPARSE_BIT) == 0) {

	  /* generate a dense walker */
	  fprintf(outfile,"register ");
	  gen_walker_decl_type(outfile, node);
	  if (restrictkw) {
	    if (ELIST_DIRLIST(node) == NULL ||
		GLIST_DATA(ELIST_DIRLIST(node)) == NULL) {
	      /* if offset walkers are used, can't rely on restrict being
		 safe -- see test/apps/sweep.z as an example. */
	      fprintf(outfile, " restrict ");
	    }
	  }
	  gen_walker(outfile, node, NULL);
	  fprintf(outfile," = ");
	  gen_walker_cast(outfile, node, TRUE);
	  gen_access_begin(outfile,expr,0);
	  gen_walker_init(outfile,mloop,NULL);
	  fprintf(outfile,";\n");

	  gen_bumper_decl_inits(outfile,numdims,order,up,tiledloop,tileorder,
				tileup,tile,tile_expr,expr,node,
				nopeel,flat);

	  /* generate dense offsets */
	  dirnode = ELIST_DIRLIST(node);
	  while (dirnode) {
	    dir = GLIST_DATA(dirnode);
	    if (dir) {
	      if (use_additive_offsets) {
		fprintf(outfile,"const int ");
		gen_offset(outfile,node,dir);
		fprintf(outfile, " = (");
		first = 1;
		for (i = 0; i < numdims; i++) {
		  if (D_DIR_SIGN(T_TYPEINFO(dir),i) != SIGN_ZERO) {
		    if (!first) {
		      fprintf(outfile, "+");
		    }
		    first=0;
		    
		    if (T_TYPEINFO_REG(expr) == NULL) {
		      INT_FATAL(NULL,"bad assumption in offset generation");
		    }
		    if (expr_is_ever_strided_ens(expr)) {
		      /*current_mloop_type == MLOOP_STRIDED) { 
			^^^ if loop is dense, array can't be strided 
			    Could potentially do interprocedural analysis to 
                            tighten this up even further */
		      fprintf(outfile,"_STR");
		    }
		    fprintf(outfile,"_OFFSET(");
		    gen_expr(outfile, T_OPLS(expr));
		    fprintf(outfile, ",%d,",i);
		    dir_gen_comp(outfile,dir,i);
		    fprintf(outfile,")");
		  }
		}
		if (first) {
		  fprintf(outfile,"0");
		}
		gen_bumper_scale(outfile, node);
		fprintf(outfile,");\n");
	      } else {
		gen_walker_decl_type(outfile, node);
		if (0 && restrictkw) {
		  /* using restrict walkers to implement @-expressions
		     is not legal if the @ aliases the original walker
		     -- see test/apps/sweep.z, e.g. */
		  fprintf(outfile, " restrict ");
		}
		gen_walker(outfile, node, dir);
		fprintf(outfile," = ");
		gen_walker_cast(outfile, node, TRUE);
		gen_access_begin(outfile,expr,0);
		gen_walker_init(outfile,mloop,dir);
		fprintf(outfile,";\n");
	      }
	    }
	    dirnode = GLIST_NEXT(dirnode);
	  }
	}
      } else {
	/* generate sparse base and walkers */
	gen_sps_base_n_walker_decl_inits(outfile,node);
      }
    }
    node = ELIST_NEXT(node);
  }

  gen_sps_reg_walkers(outfile,reglist,mloop_reg,numdims,order);
}


/* when the innermost loop iterates over a floodable/rgrid dim, the
 * bumpers for that dim will we 0; this function returns true when this
 * is the case
 */

static int bumper_zero(elist node, int dim, int numdims, int *order,
		       FILE *outfile) {
  datatype_t* regdt;

  /* bail out when insufficient info is available */
  if (!ELIST_DATA(node)) {
    return 0;
  }
  regdt = T_TYPEINFO(T_TYPEINFO_REG(ELIST_DATA(node)));

  /* is this dim a flood or rgrid dim? */
  if ((dim == order[numdims-1]) &&
      (D_REG_DIM_TYPE(regdt, dim) == DIM_FLOOD ||
       D_REG_DIM_TYPE(regdt, dim) == DIM_GRID)) {
    return 1;		/* yes, bumping unnecessary */
  }

  /* otherwise, must bump */
  return 0;
}


/* FUNCTION: bump_walker
 * bumps walker in dimension dim
 * echris  7-1-94 */

void bump_walker(FILE *outfile,elist list,int dim,int numdims,int *order) {
  elist node;

  if (!new_access) {
    return;
  }

  node = list;
  if ((current_mloop_type & MLOOP_SPARSE_BIT) == 0) {
    while(node) {
      if (ELIST_ARR_COUNTS(node)) {
	if (ELIST_DENSITY(node) == EXPR_DENSE) {
	  if (!bumper_zero(node,dim,numdims,order,outfile)) {
	    gen_walker(outfile, node, NULL);
	    fprintf(outfile," += ");
	    gen_bumper(outfile,node,dim);
	    fprintf(outfile,";\n");
	  }
	}
      }
      node = ELIST_NEXT(node);    
    }
  }
}


void bump_tiled_walker(FILE *outfile,elist list,int dim,int numdims,
		       int bumpertype,int nopeel,int *order) {
  elist node;

  if (!new_access) {
    return;
  }
  
  node = list;
  if ((current_mloop_type & MLOOP_SPARSE_BIT) == 0) {
    while(node) {
      if (ELIST_ARR_COUNTS(node)) {
	if (ELIST_DENSITY(node) == EXPR_DENSE) {
	  if (!bumper_zero(node,dim,numdims,order,outfile)
	      || BUMP_NEXT!=bumpertype) {
	    gen_walker(outfile, node, NULL);
	    fprintf(outfile," += ");
	    gen_tile_bumper(outfile,node,dim,bumpertype,nopeel);
	    fprintf(outfile,";\n");
	    if (!use_additive_offsets) {
	      glist dirnode = ELIST_DIRLIST(node);
	      
	      while (dirnode) {
		expr_t* dir = GLIST_DATA(dirnode);

		if (dir != NULL) {
		  gen_walker(outfile, node, dir);
		  fprintf(outfile," += ");
		  gen_tile_bumper(outfile, node, dim, bumpertype, nopeel);
		  fprintf(outfile, ";\n");
		}

		dirnode = GLIST_NEXT(dirnode);
	      }
	    }
	  }
	}
      }
      node = ELIST_NEXT(node);    
    }
  }
}

/* FN: get_index - return index of element w/ value v
 * 7-5-00 echris
 * If order is an mloop order array, get_index(d,v,order) returns
 * the nesting depth of the loop that iterates over dim v
 * return -1 when v is not found
 */

static int get_index(int dims, int v, int *order)
{
  int i;

  for (i=0; i<dims; i++) if (order[i] == v) return (i);

  return (-1);
}

/* FN: bump_tile_index - return index of last 1 entry in tcast array
 * 7-5-00 echris
 * This function tells you the loop nest depth of the deepest loop
 * that iterates over an interior tile
 * return -1 when a 1 entry is not found
 */

static int bump_tile_index(int dims, int *tcase)
{
  int i;

  for (i=dims-1; i>=0; i--) if (tcase[i] == 1) return (i);

  return (-1);
}

/* FN: gen_tcase - gen init code for particular tiled generic loop
 * 7-5-00 echris
 * torder[] defines tile location (1=interior, 0=boundary)
 * torder[0] is outer loop
 */

static void gen_tcase(FILE *outfile, int dims, int *order, int *torder, 
		      int *direction, int *tcase, elist wb)
{
  int i;
  int in, in2;

  if (!new_access) {
    return;
  }
  while (wb) { 	/* for each variable */
    if (ELIST_ARR_COUNTS(wb)) {		/* ignore duplicates */

      /* asign _G_Bumper_var_dim */
      for (i=0; i<dims; i++) {
	gen_tile_bumper(outfile, wb, i, BUMP_NEXT, TRUE);
	fprintf(outfile, " = ");
	in = get_index(dims, i, order);
	if ((in<dims-1) && 
	    ((in2 = get_index(dims, order[in+1], torder)) >= 0) &&
	    (tcase[in2] == 0)) {
	  gen_tile_bumper(outfile, wb, i, BUMP_NEXT_IN_THIN, FALSE);
	} else {
	  gen_tile_bumper(outfile, wb, i, BUMP_NEXT, FALSE);
	}
	fprintf(outfile, ";\n");
      }

      /* asign _G_Bumper_var_Tile */ 
      gen_tile_bumper(outfile, wb, torder[dims-1], BUMP_TILE, TRUE);
      fprintf(outfile, " = ");
      in = bump_tile_index(dims, tcase);
      in2 = torder[in];

      if (tcase[0] == 1) gen_tile_bumper(outfile, wb, in2, BUMP_TILE, FALSE);
      else gen_tile_bumper(outfile, wb, in2, BUMP_THIN_TILE, FALSE);

      fprintf(outfile, ";\n");
    }

    wb = ELIST_NEXT(wb);
  }
}

/* FN: gen_tile_g_init - gen init code for tiled generic loops
 * 7-5-00 echris
 * This function generates the init code that prepares vars for generic
 * tiles (i.e., no peeling).  Must appear outside outermost "inner" loop
 */

void gen_tile_g_init(FILE *outfile, int dims, int depth, int *order, 
		     int *torder, int *direction, int *tcase, elist wb)
{
  int i, dir;

  /* depth is the if-stmt depth (0 is outermost) */

  if (depth == dims) {	/* base case */

    gen_tcase(outfile, dims, order, torder, direction, tcase, wb);
    fprintf(outfile, "/* tcase: ");
    for (i=0; i<dims; i++) fprintf(outfile, "%d ", tcase[i]);
    fprintf(outfile, "*/\n");
    
  } else {		/* recursive case */

    /* dir is iteration direction of the depth deep loop nest */
    dir = direction[torder[depth]];
    if (dir > 0) fprintf(outfile, "if (_ITLO(%d)<=_ITSTOP(%d)) {\n", 
			 torder[depth], torder[depth]);
    else fprintf(outfile, "if (_ITHI(%d)>=_ITSTOP(%d)) {\n", 
		 torder[depth], torder[depth]);

    /* interior case */
    fprintf(outfile, "_ITGSTOP(%d) = ", torder[depth]);
    if (dir > 0) fprintf(outfile, "_ITHI(%d);\n", torder[depth]);
    else fprintf(outfile, "_ITLO(%d);\n", torder[depth]);

    tcase[depth] = 1;
    gen_tile_g_init(outfile, dims, depth+1, order, torder, direction, tcase,
		    wb);

    fprintf(outfile, "} else {\n");

    /* boundary case */
    fprintf(outfile, "_ITGSTOP(%d) = ", torder[depth]);
    if (dir > 0) fprintf(outfile, "_IHI(%d)+1;\n", torder[depth]);
    else fprintf(outfile, "_ILO(%d)-1;\n", torder[depth]);

    tcase[depth] = 0;
    gen_tile_g_init(outfile, dims, depth+1, order, torder, direction, tcase,
		    wb);

    fprintf(outfile, "}\n");
  }

}
