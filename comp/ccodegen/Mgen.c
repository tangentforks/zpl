/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/Agen.h"
#include "../include/Bgen.h"
#include "../include/Cgen.h"
#define MGEN_DECL
#include "../include/Mgen.h"
#include "../include/Privgen.h"
#include "../include/Repgen.h"
#include "../include/Spsgen.h"
#include "../include/Wgen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/contraction.h"
#include "../include/coverage.h"
#include "../include/dimension.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/stmtutil.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"

#define TILE_NONE  0
#define TILE_OUTER 1
#define TILE_INNER 2
#define TILE_FIXUP 3

/* globals for this file */

/* MAXRANK dependent */
static mloop_t _rmomloop = {0,NULL,0,NULL,NULL,{0,1,2,3,4,5},{0,0,0,0,0,0},
			    {1,1,1,1,1,1},{0,1,2,3,4,5},{1,1,1,1,1,1},
			    FALSE,NULL,{NULL,NULL,NULL,NULL,NULL,NULL,NULL},
			    {NULL,NULL,NULL,NULL,NULL,NULL},
			    {NULL,NULL,NULL,NULL,NULL,NULL},
			    {NULL,NULL,NULL,NULL,NULL,NULL},
			    {NULL,NULL,NULL,NULL,NULL,NULL},
			    NULL,{1,1,1,1,1,1},{0,0,0,0,0,0}};
static mloop_t* rmomloop = &(_rmomloop);
static int rmoorder[MAXRANK] = {0,1,2,3,4,5};
static int rmoup[MAXRANK] = {1,1,1,1,1,1};

/* are we currently unrolling the ith loop, and if so, what number body are
   we on and is the direction up? */
static int unrolling[MAXRANK] = {0,0,0,0,0,0}; 
static int unroll_num[MAXRANK] = {0,0,0,0,0,0};
static int unroll_up[MAXRANK] = {0,0,0,0,0,0};

/* what type of next tile bumper should we generate */
static int outermosttiled = -1;
static int nexttiletype = BUMP_TILE;

static char* fluffymloop = NULL;

const int mloop_dim[MAXRANK] = {MLOOP_DIM1,MLOOP_DIM2,MLOOP_DIM3,
				MLOOP_DIM4,MLOOP_DIM5,MLOOP_DIM6};
static const int start_mloop[MAXRANK+1] = {0,START_1D_MLOOP,START_2D_MLOOP,
					   START_3D_MLOOP,START_4D_MLOOP,
					   START_5D_MLOOP,START_6D_MLOOP};

static int current_loop_type;

void InitMgen(void) {
  current_mloop_type = MLOOP_NONE;
  current_mloop = NULL;
  in_mloop = 0;
  current_dimnum = 0;
}


static void gen_reg(FILE *outfile,expr_t *reg) {
  gen_name(outfile, reg);
}


static void gen_mloop_vars(FILE* outfile,genlist_t* list) {
  symboltable_t* pst;

  while (list != NULL) {
    pst = G_IDENT(list);
    if ((S_PROHIBITIONS(pst) & current_loop_type) == 0) {
      gen_pst(outfile,G_IDENT(list));
    }
    list = G_NEXT(list);
  }
}


static void gen_mask_test_start(FILE *outfile,expr_t *mask,int maskbit,
				int numdims) {
  if (mask != NULL) {
    fprintf(outfile,"if (");
    if (expr_is_qmask(mask) || 
	(T_TYPE(mask)==BIAT && expr_is_qmask(T_OPLS(mask))) ||
	maskbit == -1) {
      fprintf(outfile,"_TEST_GENERIC_MASK(%d)",numdims);
    } else {
      if (!maskbit) {
	fprintf(outfile,"!");
      }
      gen_expr(outfile, mask);
    }
    fprintf(outfile,") {\n");
  }
}


static void gen_mask_test_stop(FILE *outfile,expr_t *mask,int maskbit) {
  if (mask != NULL) {
    fprintf(outfile,"}\n");
  }
}


static void gen_mloop_scope(FILE *outfile,expr_t* reg,int begin,
			    int linenum) {
  if (begin) {
    if (reg) {
      fprintf(outfile,"if (_REG_I_OWN(");
      gen_reg(outfile,reg);
      fprintf(outfile,")) ");
    }
    fprintf(outfile,"{");
  } else {
    fprintf(outfile,"}");
  }
  fprintf(outfile,"  /* ");
  if (begin) {
    fprintf(outfile,"begin");
  } else {
    fprintf(outfile,"end");
  }
  fprintf(outfile," MLOOP");
  if (linenum) {
    fprintf(outfile," for line %d",linenum);
  }
  fprintf(outfile," */\n");
}


static void gen_mloop_decl_inits(FILE *outfile,expr_t *reg,int numdims,
				 int order[],int up[],elist wblist,
				 rlist *reglist,int unroll[],
				 int tiled[], expr_t *tiled_expr[],int flat[]) {
  int i;
  int dim;

  if (current_mloop_type & MLOOP_STRIDED_BIT) {
    fprintf(outfile,"_STR");
  }
  fprintf(outfile,"_DECL_INIT_%dD(",numdims);
  gen_reg(outfile,reg);
  fprintf(outfile,");\n");
  for (i=0;i<numdims;i++) {
    dim = order[i];
    if (tiled && (tiled[dim] > 1)) {
      fprintf(outfile,"_DECL_INIT_XD_TIL");
      if (current_mloop_type & MLOOP_STRIDED_BIT) {
	fprintf(outfile,"_STR");
      }
      fprintf(outfile,"_");
      gen_updn(outfile,up[dim]);
      fprintf(outfile,"(");
      gen_reg(outfile,reg);
      fprintf(outfile,",%d,",dim);
      if (tiled_expr && tiled_expr[dim]) gen_expr(outfile, tiled_expr[dim]);
      else fprintf(outfile,"%d",tiled[dim]);
      fprintf(outfile,");\n");
    } else if (unroll && unroll[dim] > 1) {
      fprintf(outfile,"_DECL_INIT_XD_UNR");
      if (current_mloop_type & MLOOP_STRIDED_BIT) {
	fprintf(outfile,"_STR");
      }
      fprintf(outfile,"_");
      gen_updn(outfile,up[dim]);
      fprintf(outfile,"(");
      gen_reg(outfile,reg);
      fprintf(outfile,",%d,%d);\n",dim,unroll[dim]);
    }
  }
  gen_walker_decl_inits(outfile,reg,numdims,order,up,tiled,wblist,reglist,flat);
  fprintf(outfile,"\n");
}


static void gen_mloop_for_open(FILE *outfile,expr_t *reg,int dim,int up,
			       int other_up,int inner_loop,int unroll,
			       int tiled, expr_t *tiled_expr, int tiletype, 
			       int nopeel,int numdims,int flat) {
  if (unroll > 1) {
    fprintf(outfile,"_UNROLLED");
  } else if (unroll < -1) {
    fprintf(outfile,"_CONT");
  }
  if (tiled < -1 || tiled > 1) {
    if (nopeel) fprintf(outfile, "_G"); /* when !peeling, use generic loops */
    fprintf(outfile,"_TILED");
    if (tiletype == TILE_OUTER) {
      fprintf(outfile,"_OUTER");
    } else if (tiletype == TILE_INNER) {
      fprintf(outfile,"_INNER");
    } else {
      fprintf(outfile,"_CONT");
    }
  }
  if (current_mloop_type & MLOOP_STRIDED_BIT) {
    fprintf(outfile,"_STR");
  }
  if (current_mloop_type & MLOOP_SPARSE_BIT) {
    gen_sps_mloop_prefix(outfile,reg,inner_loop,dim,up);
  }
  fprintf(outfile,"_MLOOP_");
  gen_updn(outfile,up);
  if (current_mloop_type & MLOOP_SPARSE_BIT) {
    gen_sps_mloop_postfix(outfile,inner_loop,numdims,dim);
  }

  /* tiled loops often need to indicate the direction of the other loop */
  if (tiled < -1 || tiled > 1) {
    if (tiletype == TILE_OUTER) {
      fprintf(outfile,"_FOR_");
      gen_updn(outfile,other_up);
    } else if (tiletype == TILE_INNER) {
    } else {
      fprintf(outfile,"_IN_");
      gen_updn(outfile,other_up);
    }
  }

  if (flat) {
    fprintf(outfile,"_FLAT");
  }

  fprintf(outfile,"(");
  gen_reg(outfile,reg);
  fprintf(outfile,",%d",dim);
  if (tiled < -1 || tiled > 1) {
    fprintf(outfile,",");
    if (tiled_expr) gen_expr(outfile, tiled_expr);
    else fprintf(outfile,"%d",abs(tiled));
  }
  fprintf(outfile,") {\n");
}


static void gen_mloop_loop_nest(FILE *outfile,expr_t *reg,int numdims,
				int order[],int up[],genlist_t* varls[],
				statement_t* prepre[],statement_t* pre[],
				rlist reglist,int unroll[],int flat[]) {
  int i;
  int dim;
  int inner_loop;
  int inner_dim;
  int loop_up;

  for (i=0;i<numdims;i++) {
    dim = order[i];
    inner_loop = (i == numdims-1);
    inner_dim = order[numdims-1];
    loop_up = up[dim];

    if (prepre != NULL) {
      gen_stmtls(outfile, prepre[i+1]);
    }

    gen_sps_prepre_stmt(outfile,reg,reglist,up,order,flat,i,numdims);

    gen_mloop_for_open(outfile,reg,dim,loop_up,1,inner_loop,unroll[i],0,NULL,
		       0,FALSE,numdims,0);

    in_mloop |= mloop_dim[dim];

    if (varls != NULL) {
      gen_mloop_vars(outfile,varls[i+2]);
    }

    gen_sps_pre_stmt(outfile,reg,reglist,loop_up,order,i,numdims);
    if (pre != NULL) {
      gen_stmtls(outfile, pre[i+1]);
    }
  }
}


static void gen_mloop_for_close(FILE *outfile,int dim,elist wblist,int numdims,
				int oldstyle, int *order, int flat) {
  if (oldstyle && !flat) {
    bump_walker(outfile,wblist,dim,numdims,order);
  }
  fprintf(outfile,"}\n");
}


static mloop_type categorize_mloop(expr_t *reg) {
  int strided=0;
  int sparse;
  mloop_type retval;

  retval = 0;

  sparse = !expr_is_dense_reg(reg);
  if (sparse && expr_is_qreg(reg)) {
    if (NoSparseRegCover(codegen_fn)) {
      sparse = 0;
    }
  }
  if (!sparse) {
    strided = expr_is_strided_reg(reg,1);
    if (strided && expr_is_qreg(reg)) {
      if (NoStridedRegCover(codegen_fn)) {
	strided = 0;
      }
    }
  }

  retval |= MLOOP_ON_BIT;
  if (sparse) {
    retval |= MLOOP_SPARSE_BIT;
  }
  if (strided) {
    retval |= MLOOP_STRIDED_BIT;
  }

  return retval;
}


static void gen_mloop_start(FILE *outfile,mloop_t* mloop,elist wblist) {
  rlist reglist=NULL;
  expr_t* reg = T_MLOOP_REG(mloop);
  int numdims = T_MLOOP_RANK(mloop);
  statement_t* body = T_MLOOP_BODY(mloop);
  int linenum = 0;
  genlist_t** varls = T_MLOOP_VARS_V(mloop);
  int* order = T_MLOOP_ORDER_V(mloop);
  int* up = T_MLOOP_DIRECTION_V(mloop);
  statement_t** prepre = T_MLOOP_PREPRE_V(mloop);
  statement_t** pre = T_MLOOP_PRE_V(mloop);
  expr_t* mask = T_MLOOP_MASK(mloop);
  int maskbit = T_MLOOP_WITH(mloop);
  int* unroll = T_MLOOP_UNROLL_V(mloop);
  int* tiled = T_MLOOP_TILE_V(mloop);
  expr_t** tiled_expr = T_MLOOP_TILE_EXPR_V(mloop);
  int* flat = T_MLOOP_FLAT_V(mloop);

  if (body) {
    linenum = T_LINENO(body);
  }
  if (current_mloop_type != MLOOP_NONE || current_mloop != NULL) {
    INT_FATAL(NULL,"nested MLOOP detected");
  }
  current_mloop_type = categorize_mloop(reg);
  current_mloop = mloop;
  gen_mloop_scope(outfile,reg,1,linenum);
  if (varls != NULL) {
    gen_mloop_vars(outfile,varls[0]);
  }
  gen_mloop_decl_inits(outfile,reg,numdims,order,up,wblist,&reglist,unroll,
		       tiled,tiled_expr,flat);

  /* Turn on replacements after walkers/bumpers set up (?) */
  in_mloop = start_mloop[numdims];

  if (varls != NULL) {
    gen_mloop_vars(outfile,varls[1]);
  }
  disable_new_access = 0;
  gen_mloop_loop_nest(outfile,reg,numdims,order,up,varls,prepre,pre,reglist,
		      unroll,flat);
  fprintf(outfile,"\n");
  gen_mask_test_start(outfile,mask,maskbit,numdims);
}


static void gen_mloop_stop(FILE *outfile,mloop_t* mloop,elist wblist) {
  int i;
  expr_t* mask = T_MLOOP_MASK(mloop);
  int maskbit = T_MLOOP_WITH(mloop);
  int numdims = T_MLOOP_RANK(mloop);
  statement_t** post = T_MLOOP_POST_V(mloop);
  statement_t** postpost = T_MLOOP_POSTPOST_V(mloop);
  int* order = T_MLOOP_ORDER_V(mloop);
  statement_t* body = T_MLOOP_BODY(mloop);
  int linenum = 0;
  int dim;
  int* flat = T_MLOOP_FLAT_V(mloop);

  if (body) {
    linenum = T_LINENO(body);
  }
  if (current_mloop_type == MLOOP_NONE || current_mloop == NULL) {
    INT_FATAL(NULL,"Closing an MLOOP that hasn't been opened");
  }
  fprintf(outfile,"\n");
  gen_mask_test_stop(outfile,mask,maskbit);

  for(i=numdims-1;i>=0;i--) {
    dim = order[i];
    if (post) {
      gen_stmtls(outfile, post[dim]);
    }
    gen_mloop_for_close(outfile,order[i],wblist,numdims,1,order,flat[dim]);
    if (postpost) {
      gen_stmtls(outfile, postpost[dim]);
    }
  }

  gen_mloop_scope(outfile,NULL,0,linenum);
  disable_new_access = 1;
  current_mloop_type = MLOOP_NONE;
  current_mloop = NULL;
  in_mloop = 0;
}


void gen_mloop_rmo_start(FILE *outfile,expr_t *reg,expr_t *mask,
			 int maskbit,int numdims,elist wblist,int linenum) {
  WgenSetInOldMloop();
  T_MLOOP_RANK(rmomloop) = numdims;
  T_MLOOP_REG(rmomloop) = reg;
  T_MLOOP_WITH(rmomloop) = maskbit;
  T_MLOOP_MASK(rmomloop) = mask;

  if (cmo_alloc) {
    int i;

    for (i=1;i<=numdims;i++) {
      T_MLOOP_ORDER(rmomloop,i) = numdims-i;
    }
  }

  gen_mloop_start(outfile,rmomloop,wblist);
}


void gen_mloop_rmo_stop(FILE *outfile,expr_t *reg,expr_t *mask,
			int maskbit,int numdims,elist wblist,int linenum) {
  WgenSetOutOldMloop();
  T_MLOOP_RANK(rmomloop) = numdims;
  T_MLOOP_REG(rmomloop) = reg;
  T_MLOOP_WITH(rmomloop) = maskbit;
  T_MLOOP_MASK(rmomloop) = mask;

  if (cmo_alloc) {
    int i;

    for (i=1;i<=numdims;i++) {
      T_MLOOP_ORDER(rmomloop,i) = numdims-i;
    }
  }

  gen_mloop_stop(outfile,rmomloop,wblist);
}


int gen_sub_mloop_start(FILE *outfile,expr_t *reg,int current_dim,
			int *scandims) {
  int numdims;
  int i;
  int j;
  int scanned;
  int numopened=0;
  rlist reglist;

  numdims = D_REG_NUM(T_TYPEINFO(reg));

  /*
  if (current_mloop_type != MLOOP_NONE || current_mloop != NULL) {
    INT_FATAL(NULL,"nested MLOOP detected");
  }
  current_mloop_type = categorize_mloop(reg,numdims);
  current_mloop = mloop;
  */
  gen_mloop_scope(outfile,NULL,1,0);
  gen_mloop_decl_inits(outfile,reg,numdims,rmoorder,rmoup,NULL,&reglist,
		       NULL,NULL,NULL,NULL);
  for (i=0;i<numdims;i++) {
    scanned=0;
    for (j=0;j<current_dim;j++) {
      if (i==scandims[j]) {
	scanned=1;
	fprintf(outfile,"_i%d=_REG_MYHI(",i);
	gen_reg(outfile, reg);
	fprintf(outfile,",%d);\n",i);
      }
    }
    if (!scanned) {
      gen_mloop_for_open(outfile,reg,i,1,1,0,1,0,NULL,0,FALSE,numdims,0);
      numopened++;
    }
  }
  return numopened;
}


void gen_sub_mloop_stop(FILE *outfile,int numopened) {
  int i;

  /*
  if (current_mloop_type == MLOOP_NONE || current_mloop == NULL) {
    INT_FATAL(NULL,"Closing an MLOOP that hasn't been opened");
    }*/
  for (i=0;i<numopened;i++) {
    fprintf(outfile,"}\n");
  }
  gen_mloop_scope(outfile,NULL,0,0);
  /*
  current_mloop_type = MLOOP_NONE;
  current_mloop = NULL;
  in_mloop = 0;
  */
}


static void gen_special_mloop_stmt(FILE* outfile, statement_t* stmt,
				   int numdims) {
  switch (T_SUBTYPE(stmt)) {
  case N_SPS_INIT_COUNT_PRE:
  case N_SPS_INIT_COUNT:
  case N_SPS_INIT_COUNT_POST:
  case N_SPS_INIT_LINK_PRE:
  case N_SPS_INIT_LINK:
  case N_SPS_INIT_LINK_POST:
    gen_sps_special_mloop_stmt(outfile, stmt, numdims);
  }
}


/* This is used to generate pre/post statements */
static void gen_mloop_stmtls(FILE* outfile,statement_t* stmtls, int numdims) {
  while (stmtls != NULL) {
    /* generate statement if it isn't prohibited for this type of loop */
    if ((T_PROHIBITIONS(stmtls) & current_loop_type) == 0) {
      if (T_TYPE(stmtls) == S_NULL && T_SUBTYPE(stmtls) != N_NORMAL) {
	gen_special_mloop_stmt(outfile,stmtls, numdims);
      } else {
	gen_stmt(outfile, stmtls);
      }
    }

    stmtls = T_NEXT(stmtls);
  }
}


static void gen_mloop_body(FILE* outfile,mloop_t* mloop) {
  int numdims = T_MLOOP_RANK(mloop);
  statement_t* body = T_MLOOP_BODY(mloop);
  expr_t* mask = T_MLOOP_MASK(mloop);
  int maskbit = T_MLOOP_WITH(mloop);

  gen_mask_test_start(outfile,mask,maskbit,numdims);
  current_loop_type = 0;
  gen_mloop_stmtls(outfile,body, numdims);
  fprintf(outfile,"\n");
  gen_mask_test_stop(outfile,mask,maskbit);
}


static void gen_index_bump(FILE* outfile,int dim,int up) {
  if (current_mloop_type & MLOOP_STRIDED_BIT) {
    fprintf(outfile,"_STR");
  }
  fprintf(outfile,"_BUMP_I_");
  gen_updn(outfile,up);
  fprintf(outfile,"(%d);\n",dim);
}


int mgen_unroll_number(int dim) {
  if (dim >= 0 && dim < MAXRANK && unrolling[dim]) {
    return unroll_up[dim] * unroll_num[dim];
  } else {
    return 0;
  }
}


static void set_outermost_tiled_dim(mloop_t* mloop) {
  int numdims = T_MLOOP_RANK(mloop);
  int* order = T_MLOOP_ORDER_V(mloop);
  int* tile = T_MLOOP_TILE_V(mloop);
  int i;

  outermosttiled = -1;
  nexttiletype = BUMP_TILE;
  for (i=0; i<numdims; i++) {
    if (tile[order[i]] != 1) {
      outermosttiled = order[i];
      break;
    }
  }
}





/* This procedure generate's an MLOOP's inner loops.  For untiled
   MLOOPs, inner loops are normal, full loops.  For tiled loops, inner
   loops are either the inner part of a 2-loop nesting (if the tiling
   factor is greater than 1) or a continuation loop (if the tiling
   factor is less than -1 -- set by the outer-loop generation
   procedure).  This procedure also unrolls loop iterations if
   specified. */

static void gen_mloop_rec_inner(FILE* outfile,mloop_t* mloop,int dimnum) {
  expr_t* reg = T_MLOOP_REG(mloop);
  int numdims = T_MLOOP_RANK(mloop);
  genlist_t** varls = T_MLOOP_VARS_V(mloop);
  int* order = T_MLOOP_ORDER_V(mloop);
  int* up = T_MLOOP_DIRECTION_V(mloop);
  int* tiledup = T_MLOOP_TILE_DIRECTION_V(mloop);
  int* tiledorder = T_MLOOP_TILE_ORDER_V(mloop);
  statement_t** prepre = T_MLOOP_PREPRE_V(mloop);
  statement_t** prestmts = T_MLOOP_PRE_V(mloop);
  statement_t** poststmts = T_MLOOP_POST_V(mloop);
  statement_t** postpost = T_MLOOP_POSTPOST_V(mloop);
  int* unroll = T_MLOOP_UNROLL_V(mloop);
  int* tile   = T_MLOOP_TILE_V(mloop);
  expr_t** tile_expr   = T_MLOOP_TILE_EXPR_V(mloop);
  elist wblist = T_MLOOP_WBLIST(mloop);
  rlist reglist = T_MLOOP_REGLIST(mloop);
  int nopeel = T_MLOOP_TILE_NOPEEL(mloop);
  int tcase[MAXRANK];
  int dim;
  int inner_loop;
  int loop_up;
  int unroll_factor;
  int tile_factor;
  expr_t *tile_factor_expr;
  int num_bodies;
  int local_loop_type;
  int tiletype=TILE_NONE;
  int bumptype;
  int prevnum;
  int prevdim;
  int tile_up;
  int save_current_dimnum = current_dimnum;
  int* flatv = T_MLOOP_FLAT_V(mloop);
  int flat;

  current_dimnum = dimnum;
  if (dimnum == numdims) { /* base case */
    gen_mloop_body(outfile,mloop);
  } else {         /* recursive step */
    dim = order[dimnum];
    inner_loop = (dimnum == numdims-1);
    loop_up = up[dim];
    tile_up = tiledup[dim];
    unroll_factor = unroll[dim];
    tile_factor = tile[dim];
    tile_factor_expr = tile_expr[dim];
    flat = flatv[dim];

    if (tile_factor == 1) {
      /* short-circuit recursive call to avoid unrolling mess below */
      gen_mloop_rec_inner(outfile,mloop,dimnum+1);
      current_dimnum = save_current_dimnum;
      return;
    } else if (tile_factor > 0) {
      tiletype = TILE_INNER;
    } else {
      tiletype = TILE_FIXUP;
    }

    if (tile_factor) {
      unrolling[dim] = 0;
      num_bodies = 1;
      if (tile_factor > 0) {
	local_loop_type = TILED_INNER_MLOOP_FLAG;
	if (unroll_factor > 1) {
	  if (unroll_factor == tile_factor) {
	    unrolling[dim] = 1;
	    num_bodies = unroll_factor;
	    local_loop_type = UNRLD_TILED_INNER_MLOOP_FLAG;
	  } else {
	    INT_FATAL(NULL,"Unrolling factor (%d) doesn't match tiling factor "
		      "(%d) for dimension %d",unroll_factor,tile_factor,dim);
	  }
	}
      } else {
	local_loop_type = TILED_FIXUP_MLOOP_FLAG;
	unroll_factor = 0;  /* no unrolling on fixup loop for now */
      }
    } else {
      if (unroll_factor > 1) {
	unrolling[dim] = 1;
	num_bodies = unroll_factor;
	local_loop_type = UNROLLED_MLOOP_FLAG;
      } else {
	unrolling[dim] = 0;
	num_bodies = 1;
	if (unroll_factor < -1) {
	  local_loop_type = UNRLD_FIXUP_MLOOP_FLAG;
	} else {
	  local_loop_type = NORMAL_MLOOP_FLAG;
	}
      }
    }
    current_loop_type = local_loop_type;

    if (prepre != NULL) {
      gen_mloop_stmtls(outfile,prepre[dim+1], numdims);
    }

    /* insert generic loop init around outermost inner loop when !peeling */
    if (nopeel && (dimnum == 0)) {
      gen_tile_g_init(outfile, numdims, 0, order, tiledorder, tiledup, tcase,
		      wblist);
    }

    gen_sps_prepre_stmt(outfile,reg,reglist,up,order,flatv,dimnum,numdims);

    gen_mloop_for_open(outfile,reg,dim,loop_up,tile_up,inner_loop,
		       unroll_factor, tile_factor,tile_factor_expr,
		       tiletype, nopeel,numdims,flat);
    in_mloop |= mloop_dim[dim];


    if (varls != NULL) {
      gen_mloop_vars(outfile,varls[dim+2]);
    }

    unroll_up[dim] = T_MLOOP_DIRECTION(mloop,dim);
    for (unroll_num[dim]=0; unroll_num[dim]< num_bodies; 
	 unroll_num[dim]++) {
      if (num_bodies > 1) {
	if (unroll_num[dim]) {
	  gen_index_bump(outfile,dim,loop_up);
	  fprintf(outfile,"\n");
	}
	fprintf(outfile,"/* ------- UNROLLED (dim %d copy #%d) ------- */\n",
		dim,unroll_num[dim]+1);
      }

      gen_sps_pre_stmt(outfile,reg,reglist,loop_up,order,dimnum,numdims);
      if (prestmts != NULL) {
	gen_mloop_stmtls(outfile,prestmts[dim+1], numdims);
      }

      /* recursive call */
      gen_mloop_rec_inner(outfile,mloop,dimnum+1);

      /* restore value of this variable, which may have changed on 
	 inner loops */
      current_loop_type = local_loop_type;

      if (poststmts) {
	gen_mloop_stmtls(outfile,poststmts[dim+1], numdims);
      }

      if (!flat) { /* don't bump for flat dimensions */
	/* determine bump type */
	bumptype = BUMP_NEXT;
	prevnum = dimnum+1;
	while (prevnum < numdims) {
	  prevdim = order[prevnum];
	  if (tile[prevdim] == 1) {
	    prevnum++;
	  } else {
	    if (tile[prevdim] < 0) {
	      bumptype = BUMP_NEXT_IN_THIN;
	    }
	    break;
	  }
	}
	bump_tiled_walker(outfile,wblist,dim,numdims,bumptype,nopeel,order);
      }
  }

    unrolling[dim] = 0;

    in_mloop ^= mloop_dim[dim];
    gen_mloop_for_close(outfile,order[dimnum],wblist,numdims,0,order,flat);
    
    if (postpost) {
      gen_mloop_stmtls(outfile,postpost[dim+1], numdims);
    }

    /* generate unrolling fixup loop */
    if (unroll_factor > 1) {
      if (tile_factor == 0 || tile_factor == 1) { /* only if not tiled */
	unroll[dim] = -(unroll_factor);
	/* another recursive call if we're unrolling */
	gen_mloop_rec_inner(outfile,mloop,dimnum);
	unroll[dim] = unroll_factor;
      }
    }
  }
  current_dimnum = save_current_dimnum;
}


/* This procedure generates an MLOOP's outer loops.  Outer loops don't
   exist in MLOOPs that are not tiled.  For tiled MLOOPs, outer loops
   can either be normal loops (if the tiling factor is equal to 1) or
   the outer loop of a 2-loop tiling (if the tiling factor is greater
   than 1) */

static void gen_mloop_rec_outer(FILE* outfile,mloop_t* mloop,int dimnum) {
  expr_t* reg = T_MLOOP_REG(mloop);
  int numdims = T_MLOOP_RANK(mloop);
  int* order = T_MLOOP_TILE_ORDER_V(mloop);
  int* up = T_MLOOP_TILE_DIRECTION_V(mloop);
  int* inner_up = T_MLOOP_DIRECTION_V(mloop);
  genlist_t** varls = T_MLOOP_VARS_V(mloop);
  statement_t** prepre = T_MLOOP_PREPRE_V(mloop);
  statement_t** prestmts = T_MLOOP_PRE_V(mloop);
  statement_t** poststmts = T_MLOOP_POST_V(mloop);
  statement_t** postpost = T_MLOOP_POSTPOST_V(mloop);
  int* tile   = T_MLOOP_TILE_V(mloop);
  expr_t** tile_expr   = T_MLOOP_TILE_EXPR_V(mloop);
  elist wblist = T_MLOOP_WBLIST(mloop);
  int nopeel = T_MLOOP_TILE_NOPEEL(mloop);
  int inner_loop;
  int loop_up;
  int dim;
  int tile_factor;
  expr_t *tile_factor_expr;
  int local_loop_type;
  int tiletype;
  int other_up;
  int save_current_dimnum = current_dimnum;
  int* flatv = T_MLOOP_FLAT_V(mloop);
  int flat;

  current_dimnum = dimnum;
  if (dimnum == numdims) { /* base case */
    gen_mloop_rec_inner(outfile,mloop,0);
  } else {         /* recursive step */
    dim = order[dimnum];
    inner_loop = (dimnum == numdims-1);
    loop_up = up[dim];
    other_up = inner_up[dim];
    tile_factor = tile[dim];
    tile_factor_expr = tile_expr[dim];
    flat = flatv[dim];

    if (tile_factor > 0) {
      if (tile_factor > 1) {
	tiletype = TILE_OUTER;
	local_loop_type = TILED_OUTER_MLOOP_FLAG;
      } else {
	tiletype = TILE_NONE;
	local_loop_type = NORMAL_MLOOP_FLAG;
      }
      current_loop_type = local_loop_type;
      
      if (prepre != NULL) {
	gen_mloop_stmtls(outfile,prepre[dim+1], numdims);
      }
      
      if (dim == outermosttiled) {
	nexttiletype = BUMP_TILE;
      }

      gen_mloop_for_open(outfile,reg,dim,loop_up,other_up,inner_loop,0,
			 tile_factor,tile_factor_expr,tiletype,nopeel,numdims,
			 flat);

      if (tile_factor == 1) {
	in_mloop |= mloop_dim[dim];
      }

      if (varls != NULL) {
	gen_mloop_vars(outfile,varls[dim+2]);
      }


      if (prestmts != NULL) {
	gen_mloop_stmtls(outfile,prestmts[dim+1], numdims);
      }
    }

    gen_mloop_rec_outer(outfile,mloop,dimnum+1);

    if (tile_factor > 0) {
      current_loop_type = local_loop_type;

      if (poststmts) {
	gen_mloop_stmtls(outfile,poststmts[dim+1], numdims);
      }

      /* when !peeling, only bump tiled walker for inner-most loop */
      if (!nopeel || (dimnum+1 == numdims)) {
	bump_tiled_walker(outfile,wblist,dim,numdims,nexttiletype,nopeel,
			  order);
      }

      if (tile_factor == 1) {
	in_mloop ^= mloop_dim[dim];
      }

      gen_mloop_for_close(outfile,order[dimnum],wblist,numdims,0,order,flat);


      if (dim == outermosttiled) {
	nexttiletype = BUMP_THIN_TILE;
      }
      
      if (postpost) {
	gen_mloop_stmtls(outfile,postpost[dim+1], numdims);
      }
    }

    /* when !peeling and !base case, recurse */
    if (!nopeel && (tile_factor > 1)) {
      tile[dim] = -(tile_factor);

      /* another recursive call if we're tiling */
      gen_mloop_rec_outer(outfile,mloop,dimnum+1);

      tile[dim] = tile_factor;
    }
  }
  current_dimnum = save_current_dimnum;
}


static void gen_rec_mloop_decl_inits(FILE* outfile,mloop_t* mloop) {
  expr_t* reg = T_MLOOP_REG(mloop);
  int numdims = T_MLOOP_RANK(mloop);
  int* order = T_MLOOP_ORDER_V(mloop);
  int* up = T_MLOOP_DIRECTION_V(mloop);
  int* unroll = T_MLOOP_UNROLL_V(mloop);
  int* tiled = T_MLOOP_TILE_V(mloop);
  expr_t** tiled_expr = T_MLOOP_TILE_EXPR_V(mloop);
  int* tiledup = T_MLOOP_TILE_DIRECTION_V(mloop);
  int nopeel = T_MLOOP_TILE_NOPEEL(mloop);
  int* flat = T_MLOOP_FLAT_V(mloop);
  int i;
  int dim;
  int somethingflat=0;

  if (current_mloop_type & MLOOP_STRIDED_BIT) {
    fprintf(outfile,"_STR");
  }
  if (fluffymloop != NULL) {
    fprintf(outfile,"_FLF");
  }
  fprintf(outfile,"_DECL_INIT_%dD",numdims);
  for (i=0; i<numdims; i++) {
    if (flat[i]) {
      somethingflat=1;
    }
  }
  if (somethingflat) {
    fprintf(outfile,"_OPT");
  }
  fprintf(outfile,"(");
  if (somethingflat) {
    for (i=0; i<numdims; i++) {
      if (flat[i]) {
	fprintf(outfile,"F,");
      } else {
	fprintf(outfile,"N,");
      }
    }
  }
  if (fluffymloop != NULL) {
    fprintf(outfile,"%s,", fluffymloop);
  }
  gen_reg(outfile,reg);
  fprintf(outfile,");\n");
  if (nopeel) fprintf(outfile,"_DECL_GTILE_STOP_%dD();\n", numdims);
  for (i=0;i<numdims;i++) {
    dim = order[i];
    if (tiled && tiled[dim] > 1) {
      fprintf(outfile,"_DECL_INIT_XD_TIL");
      if (current_mloop_type & MLOOP_STRIDED_BIT) {
	fprintf(outfile,"_STR");
      }
      fprintf(outfile,"_");
      gen_updn(outfile,tiledup[dim]);
      fprintf(outfile,"(");
      gen_reg(outfile,reg);
      fprintf(outfile,",%d,",dim);
      if (tiled_expr[dim]) gen_expr(outfile, tiled_expr[dim]);
      else fprintf(outfile,"%d",tiled[dim]);
      fprintf(outfile,");\n");
    } else if (unroll && unroll[dim] > 1) {
      fprintf(outfile,"_DECL_INIT_XD_UNR");
      if (current_mloop_type & MLOOP_STRIDED_BIT) {
	fprintf(outfile,"_STR");
      }
      fprintf(outfile,"_");
      gen_updn(outfile,up[dim]);
      fprintf(outfile,"(");
      gen_reg(outfile,reg);
      fprintf(outfile,",%d,%d);\n",dim,unroll[dim]);
    }
  }
  gen_rec_walker_decl_inits(outfile,mloop);
  gen_cached_access_stuff(outfile,mloop);

  fprintf(outfile,"\n");
}


static void gen_mloop_rec(FILE* outfile,mloop_t* mloop) {
  expr_t* reg = T_MLOOP_REG(mloop);
  int numdims = T_MLOOP_RANK(mloop);
  statement_t* body = T_MLOOP_BODY(mloop);
  int linenum = 0;
  genlist_t** varls = T_MLOOP_VARS_V(mloop);
  statement_t** prepre = T_MLOOP_PREPRE_V(mloop);
  statement_t** prestmts = T_MLOOP_PRE_V(mloop);
  statement_t** poststmts = T_MLOOP_POST_V(mloop);
  statement_t** postpost = T_MLOOP_POSTPOST_V(mloop);

  if (body) {
    linenum = T_LINENO(body);
  }
  if (current_mloop_type != MLOOP_NONE || current_mloop != NULL) {
    INT_FATAL(NULL,"nested MLOOP detected");
  }

  current_loop_type = 0;
  current_dimnum = 0;

  if (prepre != NULL) {
    gen_mloop_stmtls(outfile,prepre[0], numdims);
  }

  current_mloop_type = categorize_mloop(reg);
  current_mloop = mloop;
  gen_mloop_scope(outfile,reg,1,linenum);
  if (varls != NULL) {
    gen_mloop_vars(outfile,varls[0]);
  }
  gen_rec_mloop_decl_inits(outfile,mloop);

  /* Turn on replacements after walkers/bumpers set up (?) */
  in_mloop = start_mloop[numdims];

  if (varls != NULL) {
    gen_mloop_vars(outfile,varls[1]);
  }
  if (prestmts != NULL) {
    gen_mloop_stmtls(outfile,prestmts[0], numdims);
  }
  disable_new_access = 0;

  set_outermost_tiled_dim(mloop);

  /* generate the loops and body recursively */
  gen_mloop_rec_outer(outfile,mloop,0);

  /* here ends the recursive bit */

  if (poststmts != NULL) {
    gen_mloop_stmtls(outfile,poststmts[0], numdims);
  }

  gen_mloop_scope(outfile,reg,0,linenum);
  disable_new_access = 1;
  current_mloop_type = MLOOP_NONE;
  current_mloop = NULL;
  in_mloop = 0;

  if (postpost != NULL) {
    gen_mloop_stmtls(outfile,postpost[0], numdims);
  }

  current_loop_type = 0;
}



/*************************************************************
  Generates code for MLOOPS
  Ruth Anderson 4-26-93
    
  Assumptions: 1) mloop stride = 1
  2) ORDER and DIRECTION have been placed in the
  mloop struct, even when the region is unknown at compile time.
  TODO:
  3) We need to know what rank of region the mloop needs so 
  we know what stack to pull the pointer off of, set numdims.

  In the for loops created, loop variable i# refers to looping thru dimension #.
  ************************************************************/
static void gen_core_mloop(FILE* outfile,mloop_t *mloop) {
  int numdims;
  int i;
  statement_t *body;
  elist wblist;

  numdims = T_MLOOP_RANK(mloop);
 
  if (T_MLOOP_REG(mloop) == NULL) {
    T_MLOOP_REG(mloop) = buildqregexpr(numdims);
  }

  body = T_MLOOP_BODY(mloop);

  wblist = search_for_ensembles_in_stmt_ls(body);
  for (i=-1; i<MAXRANK; i++) {
    wblist = (elist)
      glist_append_list((glist)wblist,
			(glist)search_for_ensembles_in_stmt_ls(T_MLOOP_PREPRE(mloop,i)));
    wblist = (elist)
      glist_append_list((glist)wblist,
			(glist)search_for_ensembles_in_stmt_ls(T_MLOOP_PRE(mloop,i)));
    wblist = (elist)
      glist_append_list((glist)wblist,
			(glist)search_for_ensembles_in_stmt_ls(T_MLOOP_POST(mloop,i)));
    wblist = (elist)
      glist_append_list((glist)wblist,
			(glist)search_for_ensembles_in_stmt_ls(T_MLOOP_POSTPOST(mloop,i)));
  }
  if (T_MLOOP_MASK(mloop)) {
    wblist = (elist)
      glist_append_list((glist)wblist,
			(glist)search_downfor_ensemble_exprs(T_MLOOP_MASK(mloop)));
  }

  T_MLOOP_WBLIST(mloop) = wblist;
  gen_mloop_rec(outfile,mloop);

  glist_destroy((glist)wblist, ELIST_NODE_SIZE);
  T_MLOOP_WBLIST(mloop) = NULL;
}


static void print_loop_order(int numdims,int order[],int torder[],int tile[]) {
  int currentdim;
  int indent;
  int i;
  int j;

  indent=0;
  for (i=0; i<numdims; i++) {
    currentdim = torder[i];
    if (tile[currentdim] > 0) {
      for (j=0; j<indent; j++) {
	printf("  ");
      }
      indent++;
      printf("%d",currentdim);
      if (tile[currentdim] > 1) {
	printf("O");
      }
      printf("\n");
    }
  }
  for (i=0; i<numdims; i++) {
    currentdim = order[i];
    if (tile[currentdim] != 1) {
      for (j=0; j<indent; j++) {
	printf("  ");
      }
      indent++;
      printf("%d",currentdim);
      if (tile[currentdim] > 1) {
	printf("I");
      }
      printf("\n");
    }
  }
}


/* this function takes any dimensions that were tiled after cmloops and
   moves them to the outside of the tiled MLOOP */

static void ensure_flat_dimensions_outermost(mloop_t* mloop) {
  int numdims = T_MLOOP_RANK(mloop);
  int i;
  int tiled;
  int numflatdims;
  int currentdim;
  int j;
  int* flat = T_MLOOP_FLAT_V(mloop);
  int* torder = T_MLOOP_TILE_ORDER_V(mloop);
  int* tile = T_MLOOP_TILE_V(mloop);

  tiled = 0;
  for (i=0; i<numdims; i++) {
    if (tile[i]) {
      tiled = 1;
    }
  }

  if (tiled) {
    numflatdims=0;
    for (i=0; i<numdims; i++) {
      currentdim = torder[i];
      if (flat[currentdim]) {
	for (j=i; j>numflatdims; j--) {
	  torder[j] = torder[j-1];
	}
	torder[numflatdims] = currentdim;
	tile[currentdim] = 1;
	
	numflatdims++;
      }
    }
  }
}


/* BLC: This is the main function for generating an MLOOP */

void gen_mloop(FILE* outfile,mloop_t *mloop) {
  int numdims;

  /* check for bad arguments */
  if (mloop == NULL) {
    INT_FATAL(NULL, "Null mloop in gen_mloop()");
    return;
  }

  numdims = T_MLOOP_RANK(mloop);
  if (numdims < 1) {
    INT_FATAL(NULL, "ensemble of rank (%d) less than 1 encountered",
              numdims);
  }
  if (numdims > MAXRANK) {
    INT_FATAL(NULL, "ensemble of rank (%d) greater than MAXRANK (%d) "
              "encountered", numdims, MAXRANK);
  }

  IFDB(99) {
    print_loop_order(numdims,T_MLOOP_ORDER_V(mloop),T_MLOOP_TILE_ORDER_V(mloop),
		     T_MLOOP_TILE_V(mloop));
  }

  ensure_flat_dimensions_outermost(mloop);

  IFDB(99) {
    print_loop_order(numdims,T_MLOOP_ORDER_V(mloop),T_MLOOP_TILE_ORDER_V(mloop),
		     T_MLOOP_TILE_V(mloop));
  }

  gen_core_mloop(outfile,mloop);
}

void gen_fluffy_mloop(FILE* outfile,mloop_t* mloop,char* regname) {
  int save_hoist_access_ptrs = hoist_access_ptrs;
  int save_hoist_access_mults = hoist_access_mults;

  /* turn off hoisting in the context of the sparse initializations;
     we don't do it well yet (as can be seen by turning this on and
     running a sparse MG code, for example... */
  hoist_access_ptrs = 0;
  hoist_access_mults = 0;

  fluffymloop = regname;

  gen_mloop(outfile,mloop);

  fluffymloop = NULL;

  hoist_access_ptrs = save_hoist_access_ptrs;
  hoist_access_mults = save_hoist_access_mults;
}
