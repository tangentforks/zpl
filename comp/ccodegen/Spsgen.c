/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Mgen.h"
#include "../include/Privgen.h"
#include "../include/Spsgen.h"
#include "../include/Wgen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/glistmac.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/symboltable.h"
#include "../include/treemac.h"


static int guard_sparsity=1;
int sparsity_type = _DIR_SPS;


static int node_represents_region_computation(elist node) {
  return D_CLASS(T_TYPEINFO(ELIST_DATA(node))) == DT_REGION;
}


static void gen_sps_walker_type(FILE *outfile,elist node,int type) {
  if (node_represents_region_computation(node)) {
    gen_pdt(outfile,pdtINT,type);
  } else {
    gen_pdt(outfile,T_TYPEINFO(ELIST_DATA(node)),type);
  }
}


static void gen_sps_walker_name(FILE* outfile,elist node,expr_t* dir) {
  gen_walker_name(outfile,node);
  if (dir != NULL) {
    fprintf(outfile,"_");
    gen_expr(outfile,dir);
  }
}


void gen_sps_arr_access(FILE* outfile,elist node,expr_t* dir) {
  if (ELIST_WRITTEN(node)) {
    fprintf(outfile,"_SPS_PTR(");
  } else {
    fprintf(outfile,"_SPS_VAL(");
  }
  gen_sps_walker_name(outfile,node,dir);
  fprintf(outfile,")");
}


static void gen_sps_regident(FILE *outfile,rlist node,dlist dirnode) {
  expr_t *dir;

  if (dirnode == NULL) {
    dir = NULL;
  } else {
    dir = DIRLIST_DIR(dirnode);
  }
  fprintf(outfile,"%s",S_IDENT(T_IDENT(RLIST_DATA(node))));
  if (dir != NULL) {
    fprintf(outfile,"_");
    gen_expr(outfile,dir);
  }
}


static int reg_is_mloop_reg(expr_t* mloop_reg,rlist rnode,dlist dnode) {
  expr_t* reg;
  expr_t* dir;

  reg = RLIST_DATA(rnode);
  dir = DIRLIST_DIR(dnode);
  
  return (expr_equal(reg, mloop_reg) && (dir == NULL));
}


static rlist search_list_for_reg(rlist list,expr_t *reg) {
  rlist tmp;
  expr_t *old_reg;
  
  tmp = list;
  while (tmp) {
    old_reg = RLIST_DATA(tmp);
    /* BLC -- if a sparse array's region isn't known, will need to get it
       from the descriptor rather than referring to it by name.  For now
       just trying to get something working. */
    if (expr_equal(old_reg, reg)) {
      return tmp;
    }
    tmp = RLIST_NEXT(tmp);
  }
  return NULL;
}


static dlist search_list_for_dir(dlist list,expr_t *dir) {
  dlist tmp;
  expr_t *old_dir;
  
  tmp = list;
  while (tmp) {
    old_dir = DIRLIST_DIR(tmp);
    if (expr_equal(old_dir,dir)) {
      return tmp;
    }
    tmp = DIRLIST_NEXT(tmp);
  }
  return NULL;
}


rlist add_sps_expr_to_reglist(rlist reglist,elist node,expr_t* arrreg,
			      expr_t* mloop_reg,int* density) {
  expr_t *dir;
  rlist regnode;
  glist dirnode1;
  int regmismatch;
  dlist dirnode2;
  dlist dirlist;

  if (ELIST_ARR_COUNTS(node)) {
    dirnode1 = ELIST_DIRLIST(node);
    while (dirnode1) {
      dir = GLIST_DATA(dirnode1);
      regmismatch = !expr_regs_share_sparsity(arrreg,mloop_reg);
      if (regmismatch) {
	*density = EXPR_SPARSE_DIFF;
      }
      
      /* add the array's region to the reglist if it isn't already */
      if (reglist == NULL) {
	reglist = rlist_create(arrreg);
	RLIST_DIRLIST(reglist) = NULL;
      }
      regnode = search_list_for_reg(reglist,arrreg);
      if (regnode == NULL) {
	regnode = rlist_append(reglist,arrreg);
	RLIST_DIRLIST(regnode) = NULL;
      }
      
      /* add the array's direction to the region node's dirlist if it
	 isn't already */
      dirlist = RLIST_DIRLIST(regnode);
      if (dirlist == NULL) {
	dirlist = dlist_create(dir);
	RLIST_DIRLIST(regnode) = dirlist;
	DIRLIST_EXPRLS(dirlist) = NULL;
      }
      dirnode2 = search_list_for_dir(dirlist,dir);
      if (dirnode2 == NULL) {
	dirnode2 = dlist_append(dirlist,dir);
	DIRLIST_EXPRLS(dirnode2) = NULL;
      }
      
      /* add the expression itself to the direction node in the dirlist */
      if (DIRLIST_EXPRLS(dirnode2)) {
	glist_append(DIRLIST_EXPRLS(dirnode2),node,GLIST_NODE_SIZE);
      } else {
	DIRLIST_EXPRLS(dirnode2) = glist_create(node,GLIST_NODE_SIZE);
      }
      
      dirnode1 = GLIST_NEXT(dirnode1);
    }
  }

  return reglist;
}


void gen_sps_reg_walkers(FILE* outfile,rlist reglist,expr_t* mloop_reg,
			 int numdims,int order[]) {
  rlist rnode;
  dlist dnode;
  expr_t* reg;
  int mainreg;

  /* generate sparse region walkers */
  rnode = reglist;
  while (rnode) {
    reg = RLIST_DATA(rnode);
    dnode = RLIST_DIRLIST(rnode);
    while (dnode) {
      mainreg = reg_is_mloop_reg(mloop_reg,rnode,dnode);
      fprintf(outfile,"_SPS_DECL_INIT_REG");
      if (mainreg) {
	fprintf(outfile,"_MAIN");
      }
      if (sparsity_type == _DIR_SPS) {
	fprintf(outfile,"_SPS");
      }
      if (order[numdims-1] == numdims-1) {
	fprintf(outfile,"_INNER");
      } 
      fprintf(outfile,"(");
      gen_sps_regident(outfile,rnode,dnode);
      if (sparsity_type == _DIR_DNS) {
	fprintf(outfile,",%d",order[numdims-1]);
      } else {
	fprintf(outfile,",%d",numdims);
      }
      fprintf(outfile,");\n");
      
      dnode = DIRLIST_NEXT(dnode);
    }

    rnode = RLIST_NEXT(rnode);
  }
}



static void gen_spsid_val(FILE* outfile,expr_t* mloop_reg,rlist rnode,
			  dlist dnode,int dim) {
  expr_t* dir;
  long nonzero_offset;
  int mainreg;

  dir = DIRLIST_DIR(dnode);
  if (!dir) {
    nonzero_offset = 0;
  } else {
    nonzero_offset = (dir_get_sign(dir, dim) != SIGN_ZERO);
  }
  mainreg = reg_is_mloop_reg(mloop_reg,rnode,dnode);

  fprintf(outfile,"_SPSID_VAL");
  if (nonzero_offset) {
    fprintf(outfile,"_OFF");
  } else if (mainreg) {
    fprintf(outfile,"_MAIN");
  }
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  if (!mainreg) {
    fprintf(outfile,",");
    gen_sps_regident(outfile,rnode,NULL);
  }
  fprintf(outfile,",%d",dim);
  if (nonzero_offset) {
    fprintf(outfile, ", ");
    dir_gen_comp(outfile, dir, dim);
  }
  fprintf(outfile,")");
}


static void gen_spsid_hit(FILE* outfile,expr_t* mloop_reg,rlist rnode,
			  dlist dnode,int dim) {
  fprintf(outfile,"_SPS_HIT");
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  fprintf(outfile,",%d",dim);
  fprintf(outfile,")");
}


static void gen_reg_expr_access(FILE* outfile,int dim,
				expr_t* mloop_reg,rlist rnode,
				dlist dnode,glist node,int idprecomp,
				int add_offset) {
  expr_t* dir;
  elist enode;

  dir = DIRLIST_DIR(dnode);
  enode = GLIST_DATA(node);

  gen_sps_arr_access(outfile,enode,dir);
  fprintf(outfile," = ");
  if (node_represents_region_computation(enode)) {
    if (add_offset) {
      gen_spsid_hit(outfile,mloop_reg, rnode, dnode, dim);
    } else {
      fprintf(outfile, "0");
    }
  } else {
    if (!ELIST_WRITTEN(enode)) {
      fprintf(outfile,"*");
    }
    fprintf(outfile,"(_SPS_ORIG(");
    gen_walker_name(outfile,enode);
    fprintf(outfile,")");
    if (add_offset) {
      fprintf(outfile," + ");
      if (idprecomp) {
	fprintf(outfile,"_spsid");
      } else {
	gen_spsid_val(outfile,mloop_reg,rnode,dnode,dim);
      }
    }
    fprintf(outfile,")");
  }
  fprintf(outfile,";\n");
}


static void gen_reg_exprls_accesses(FILE *outfile,int dim,
				    expr_t* mloop_reg,rlist rnode,
				    dlist dnode,int add_offset) {
  glist node;

  node = DIRLIST_EXPRLS(dnode);
  while (node) {
    gen_reg_expr_access(outfile,dim,mloop_reg,rnode,dnode,node,1,add_offset);

    node = GLIST_NEXT(node);
  }
}


static void gen_sps_set_val(FILE* outfile,int dim,expr_t* mloop_reg,
			    rlist rnode,dlist dnode) {
  int numexprs;

  /* generate capture of value/pointer */
  numexprs = glist_length(DIRLIST_EXPRLS(dnode));
  if (numexprs == 1) {
    gen_reg_expr_access(outfile,dim,mloop_reg,rnode,dnode,DIRLIST_EXPRLS(dnode),
			0,1);
  } else if (numexprs > 1) {
    fprintf(outfile,"{\n");
    fprintf(outfile,"const int _spsid = ");
    
    gen_spsid_val(outfile,mloop_reg,rnode,dnode,dim);
    
    fprintf(outfile,";\n");
    
    gen_reg_exprls_accesses(outfile,dim,mloop_reg,rnode,dnode,1);
    fprintf(outfile,"}\n");
  }
}


static void gen_sps_set_orig(FILE* outfile,int dim,expr_t* mloop_reg,
			     rlist rnode,dlist dnode) {
  gen_reg_exprls_accesses(outfile,dim,mloop_reg,rnode,dnode,0);
}


static void gen_sps_prep_mloop(FILE* outfile,rlist rnode,dlist dnode,int dim,
			       int numdims,int up) {
  fprintf(outfile,"_PREP_SPS_MLOOP_");
  gen_updn(outfile,up);
  if (dim == numdims-1) {
    fprintf(outfile,"_INNER");
  }
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  fprintf(outfile,",%d,%d);\n",numdims,dim);
}


static void gen_sps_dense_reg_walker_init(FILE* outfile,rlist rnode,dlist dnode,
					  expr_t* dir,int dim,int numdims,
					  int up,int offset,int inner_dim) {
  int i;

  fprintf(outfile,"_SPS_START_");
  gen_updn(outfile,up);
  if (offset) {
    fprintf(outfile,"_OFF");
  }
  if (inner_dim) {
    fprintf(outfile,"_INNER");
  }
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  fprintf(outfile,",");
  gen_sps_regident(outfile,rnode,NULL);
  fprintf(outfile,",%d,%d",numdims,dim);
  if (offset) {
    for (i=0; i<numdims; i++) {
      if (i != dim) {
	fprintf(outfile, ", ");
	dir_gen_comp(outfile, dir, i);
      }
    }
  }
  fprintf(outfile,");\n");
}


static void gen_sps_sparse_reg_walker_init(FILE* outfile,rlist rnode,
					   dlist dnode,expr_t* dir,int dim,
					   int numdims,int up[],int offset,
					   int main,int flat) {
  int i;

  if (numdims < 1 || numdims >MAXRANK) {
    INT_FATAL(NULL, "illegal sparse region walker");
  }
  
  fprintf(outfile,"_SPS_WALK_SETUP");
  if (main) {
    fprintf(outfile,"_MAIN");
    if (flat) {
      fprintf(outfile,"_FLAT");
    }
  }
  if (numdims == 1) {
    fprintf(outfile,"_1D_");
    gen_updn(outfile,up[0]);
  }
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  fprintf(outfile,",");
  gen_sps_regident(outfile,rnode,NULL);
  fprintf(outfile,",%d,",dim);
  for (i=0; i<numdims; i++) {
    if (i) {
      fprintf(outfile,"|");
    }
    if (up[i] >= 1) {
      fprintf(outfile,"_UP");
    } else {
      fprintf(outfile,"_DN");
    }
    fprintf(outfile,"%d",i);
  }
  fprintf(outfile,");\n");

}



static void gen_sps_reg_walker_inits(FILE* outfile,expr_t* mloop_reg,
				     rlist reglist,int dim,int up[],
				     int dimnum,int inner_dim,int flat) {
  rlist rnode;
  expr_t *dir;
  int off_agnst;
  dlist dnode;
  int numdims;
  int i;
  int up_now = up[dimnum];

  numdims = D_REG_NUM(T_TYPEINFO(mloop_reg));

  rnode = reglist;
  while (rnode) {
    dnode = RLIST_DIRLIST(rnode);
    while (dnode) {
      dir = DIRLIST_DIR(dnode);
      off_agnst = 0;
      if (dir) {
	for (i=0; i<numdims; i++) {
	  if (i != dim) {
	    if (dir_get_sign(dir,i) != SIGN_ZERO) {
	      off_agnst = 1;
	      break;
	    }
	  }
	}
      } else {
	off_agnst = 0;
      }
      
      switch (sparsity_type) {
      case _DIR_DNS:
	if (reg_is_mloop_reg(mloop_reg,rnode,dnode)) {
	  gen_sps_prep_mloop(outfile,rnode,dnode,dim,numdims,up_now);
	} else {
	  gen_sps_dense_reg_walker_init(outfile,rnode,dnode,dir,dim,numdims,
					up_now,off_agnst,inner_dim);
	}
	break;
      case _DIR_SPS:
	gen_sps_sparse_reg_walker_init(outfile,rnode,dnode,dir,dim,numdims,up,
				       off_agnst,
				       reg_is_mloop_reg(mloop_reg,rnode,dnode),
				       flat);
	if (reg_is_mloop_reg(mloop_reg,rnode,dnode)) {
	  
	}
	break;
      }

      dnode = DIRLIST_NEXT(dnode);
    }

    rnode = RLIST_NEXT(rnode);
  }
}


static void gen_sps_catchup_cond(FILE* outfile,rlist rnode,dlist dnode,int dim,
				 int up,expr_t* offset) {
  /* generate conditional to guard against unnecessarily updating */
  fprintf(outfile,"if (_SPS_NEEDS_CATCHING_");
  gen_updn(outfile,up);
  if (offset) {
    fprintf(outfile,"_OFF");
  }
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  fprintf(outfile,",%d",dim);
  if (offset) {
    fprintf(outfile, ", ");
    dir_gen_comp(outfile, offset, dim);
  }
  fprintf(outfile,")) {\n");
}


static void gen_sps_catchup(FILE* outfile,rlist rnode,dlist dnode,int dim,
			    int up,expr_t* offset,int numdims,int inner_loop) {
  fprintf(outfile,"_SPS_CATCH_");
  gen_updn(outfile,up);
  if (offset) {
    fprintf(outfile,"_OFF");
  }
  if (inner_loop) {
    if (dim == numdims-1) { /* if this is inner in both senses */
      fprintf(outfile,"_INNER");
    }
    mark_region_needs(RLIST_DATA(rnode),dim,dim != numdims-1, up);
  }
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  fprintf(outfile,",");
  gen_sps_regident(outfile,rnode,NULL);
  fprintf(outfile,",%d",dim);
  if (offset) {
    fprintf(outfile, ", ");
    dir_gen_comp(outfile, offset, dim);
  }
  fprintf(outfile,");\n");
}


static void gen_sps_advance_dense(FILE* outfile,expr_t* mloop_reg,
				  rlist rnode,dlist dnode,int dim,int up,
				  expr_t* offset,int inner_loop) {
  int numdims = D_REG_NUM(T_TYPEINFO(mloop_reg));

  if (!reg_is_mloop_reg(mloop_reg,rnode,dnode)) {
    if (guard_sparsity) {
      gen_sps_catchup_cond(outfile,rnode,dnode,dim,up,offset);
    }
    
    /* generate stuff to catch this region's walker up */
    gen_sps_catchup(outfile,rnode,dnode,dim,up,offset,numdims,inner_loop);
  }
  
  gen_sps_set_val(outfile,dim,mloop_reg,rnode,dnode);
  
  if (!reg_is_mloop_reg(mloop_reg,rnode,dnode)) {
    if (guard_sparsity) {
      fprintf(outfile,"}\n");
    }
  }
}


static void gen_sps_copy_walker(FILE* outfile,rlist rnode,dlist dnode,int dim,
				int next_dim,int main,int numdims,int up) {
  fprintf(outfile,"_SPS_COPY_WALKER");
  if (main) {
    fprintf(outfile,"_MAIN");
  }
  if (next_dim == numdims-1) {
    fprintf(outfile,"_INNER_");
    gen_updn(outfile,up);
  }
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  if (!main) {
    fprintf(outfile,",");
    gen_sps_regident(outfile,rnode,NULL);
  }
  fprintf(outfile,",%d,%d);\n",dim,next_dim); 
}


static void gen_sps_check_hit(FILE* outfile,expr_t* mloop_reg,
			      rlist rnode,dlist dnode,int order[],int dimnum,
			      int numdims,expr_t* offset,int up) {
  int i;
  int dim = order[dimnum];
  int next = order[dimnum+1];

  fprintf(outfile,"if (_SPS_HIT");
  if (offset) {
    fprintf(outfile,"_OFF");
  }
  fprintf(outfile,"(");
  gen_sps_regident(outfile,rnode,dnode);
  fprintf(outfile,",%d",dim);
  if (offset) {
    fprintf(outfile,", ");
    dir_gen_comp(outfile, offset, dim);
  }
  fprintf(outfile,")) {\n");

  gen_sps_copy_walker(outfile,rnode,dnode,dim,next,0,numdims,up);

  fprintf(outfile,"} else {\n");
  fprintf(outfile,"_SPS_MAXIFY%d(",numdims-(dimnum+1));
  gen_sps_regident(outfile,rnode,dnode);
  for (i=dimnum+1; i<numdims; i++) {
    fprintf(outfile,",%d",order[i]);
  }
  fprintf(outfile,");\n");
  gen_sps_set_orig(outfile,dim,mloop_reg,rnode,dnode);
  fprintf(outfile,"}\n");
}



static void gen_sps_advance_sparse(FILE* outfile,expr_t* mloop_reg,
				   rlist rnode,dlist dnode,int order[],
				   int dimnum,int numdims,int up,
				   expr_t* offset) {
  int dim = order[dimnum];
  int next = order[dimnum+1];

  if (!reg_is_mloop_reg(mloop_reg,rnode,dnode)) {
    gen_sps_catchup_cond(outfile,rnode,dnode,dim,up,offset);
    gen_sps_catchup(outfile,rnode,dnode,dim,up,offset,numdims,
		    dimnum == numdims-1);
    if (dimnum == numdims-1) {
      gen_sps_set_val(outfile,dim,mloop_reg,rnode,dnode);
    } else {
      gen_sps_check_hit(outfile,mloop_reg,rnode,dnode,order,dimnum,numdims,
			offset,up);
    }
    fprintf(outfile,"}\n");
  } else {
    if (dimnum != numdims-1) {
      gen_sps_copy_walker(outfile,rnode,dnode,dim,next,1,numdims,up);
    } else {
      gen_sps_set_val(outfile,dim,mloop_reg,rnode,dnode);
    }
  }
}


static void gen_sps_reg_walker_bumps(FILE* outfile,expr_t* mloop_reg,
				     rlist reglist,int order[],int dimnum,
				     int numdims,int up) {
  rlist rnode;
  expr_t *dir;
  dlist dnode;
  int dim = order[dimnum];
  expr_t* offdir;

  rnode = reglist;
  if (rnode) {
    fprintf(outfile,"\n");
  }
  while (rnode) {
    fprintf(outfile,"/* Updating ");
    gen_sps_regident(outfile,rnode,NULL);
    if (dimnum == numdims-1) {
      fprintf(outfile," arrays */\n");
    } else {
      fprintf(outfile," walkers */\n");
    }

    dnode = RLIST_DIRLIST(rnode);
    while (dnode) {
      dir = DIRLIST_DIR(dnode);
      if (dir) {
	if (dir_get_sign(dir, dim) != SIGN_ZERO) {
	  offdir = dir;
	} else {
	  offdir = NULL;
	}
      } else {
	offdir = NULL;
      }

      switch (sparsity_type) {
      case _DIR_DNS:
	gen_sps_advance_dense(outfile,mloop_reg,rnode,dnode,dim,up,offdir,
			      (dimnum == numdims-1));
	break;
      case _DIR_SPS:
	gen_sps_advance_sparse(outfile,mloop_reg,rnode,dnode,order,dimnum,
			       numdims,up,offdir);
	break;
      }

      dnode = DIRLIST_NEXT(dnode);
    }

    fprintf(outfile,"\n");
    rnode = RLIST_NEXT(rnode);
  }
}


void gen_sps_base_n_walker_decl_inits(FILE* outfile,elist node) {
  glist dirnode;
  int someone_written=0;

  /* generate a pointer/value per array/direction combination */
  dirnode = ELIST_DIRLIST(node);
  while (dirnode) {
    fprintf(outfile,"_SPS_DECL_INIT_ARR_");
    if (ELIST_WRITTEN(node)) {
      fprintf(outfile,"W");
      someone_written=1;
    } else {
      fprintf(outfile,"R");
    }
    fprintf(outfile,"(");
    gen_sps_walker_type(outfile,node,PDT_LOCL);
    fprintf(outfile,",");
    gen_sps_walker_name(outfile,node,GLIST_DATA(dirnode));
    fprintf(outfile,");\n");
    
    dirnode = GLIST_NEXT(dirnode);
  }

  /* do not generate an origin for a region that we're computing over */
  if (!node_represents_region_computation(node)) {
    /* generate a single origin for all the above to reference */
    dirnode = ELIST_DIRLIST(node);
    if (dirnode) {
      fprintf(outfile,"_SPS_DECL_INIT_ARR_ORIG_");
      if (someone_written) {
	fprintf(outfile,"W");
      } else {
	fprintf(outfile,"R");
      }
      fprintf(outfile,"(");
      gen_sps_walker_type(outfile,node,PDT_LOCL);
      fprintf(outfile,",");
      gen_walker_name(outfile,node);
      fprintf(outfile,");\n");
    }
  }
}


static datatype_t* find_reg_type(expr_t* regExpr) {
  return T_TYPEINFO(regExpr);
}


static void set_need_ind(expr_t* reg,int dim) {
  if (reg != NULL) {
    
    D_REG_NEED_IND(find_reg_type(reg),dim) = 1;
    set_need_ind(T_OPLS(reg),dim);
  }
}


static void set_need_next(expr_t* reg,int dim) {
  if (reg != NULL) {
    D_REG_NEED_NEXT(find_reg_type(reg),dim) = 1;
    set_need_next(T_OPLS(reg),dim);
  }
}



static void set_need_prev(expr_t* reg,int dim) {
  if (reg != NULL) {
    D_REG_NEED_PREV(find_reg_type(reg),dim) = 1;
    set_need_prev(T_OPLS(reg),dim);
  }
}


static void set_need_densedir(expr_t* reg,int dim) {
  if (reg != NULL) {
    D_REG_NEED_DENSE_DIR(find_reg_type(reg),dim) = 1;
    set_need_densedir(T_OPLS(reg),dim);
  }
}


void mark_region_needs(expr_t* reg,int dim,int inner_loop,
		       int up) {
  if (reg != NULL) {
    if (!expr_is_sparse_reg(reg)) {
      return;
    }
    if (expr_is_qreg(reg)) {
      reg = RMSCoveringRegion();
    }

    set_need_ind(reg,dim);
    if (inner_loop) {
      if (up > 0) {
	set_need_next(reg,dim);
      } else {
	set_need_prev(reg,dim);
      }
    }
    /* mark base regions recursively */
    mark_region_needs(T_OPLS(reg), dim, inner_loop, up);
  }
}


void gen_sps_mloop_prefix(FILE* outfile,expr_t* reg,int inner_loop,
			  int dim,int up) {
  switch (sparsity_type) {
  case _DIR_DNS:
    if (inner_loop) {
      fprintf(outfile,"_SPS");
    }
    set_need_densedir(reg,dim);
    break;
  case _DIR_SPS:
    fprintf(outfile,"_SPS_MAIN");
    if (expr_is_clean_sparse_reg(reg)) {
      fprintf(outfile,"_CLEAN");
    }
    break;
  }
  if (inner_loop) {
    mark_region_needs(reg,dim,dim != D_REG_NUM(T_TYPEINFO(reg))-1,up);
  }
}


void gen_sps_mloop_postfix(FILE* outfile,int inner_loop,int numdims,int dim) {
  if (dim == numdims-1) {
    switch (sparsity_type) {
    case _DIR_DNS:
      if (inner_loop) {
	fprintf(outfile,"_INNER");
      }
      break;
    case _DIR_SPS:
      if (inner_loop) {
	fprintf(outfile,"_INNER");
      }
      break;
    }
  }
}


void gen_sps_prepre_stmt(FILE* outfile,expr_t* reg, rlist reglist,
			 int up[],int order[],int flat[],int dimnum,
			 int numdims) {
  int dim = order[dimnum];

  switch (sparsity_type) {
  case _DIR_DNS:
    if (dimnum == numdims-1) {  /* inner loop only */
      gen_sps_reg_walker_inits(outfile,reg,reglist,dim,up,dimnum,
			       (order[dimnum] == numdims-1),flat[dim]);
    }
    break;
  case _DIR_SPS:
    if (dimnum == 0) {
      gen_sps_reg_walker_inits(outfile,reg,reglist,dim,up,dimnum,0,flat[dim]);
    }
    break;
  }
}


void gen_sps_pre_stmt(FILE* outfile,expr_t* reg, rlist reglist,int up,
		      int order[],int dimnum,int numdims) {
  switch (sparsity_type) {
  case _DIR_DNS:
    if (dimnum == numdims-1) {  /* inner loop only */
      gen_sps_reg_walker_bumps(outfile,reg,reglist,order,dimnum,numdims,up);
    }
    break;
  case _DIR_SPS:
    gen_sps_reg_walker_bumps(outfile,reg,reglist,order,dimnum,numdims,up);
    break;
  }
}


void gen_sps_structure_use(void) {
  int i;
  hashentry_t *phe;
  symboltable_t *regionpst;
  char spsregfilename[FILEPATHLEN];
  FILE* spsregfile;
  int j;
  int numdims;

  sprintf(spsregfilename,"%s%s_%s",DESTPATH,trunc_in_filename,REGSPSFILE);
  spsregfile = fopen(spsregfilename,"w");
  if (spsregfile == NULL) {
    USR_FATAL(NULL, "Cannot open file '%s'",spsregfilename);
  }

  for (i=0; i<MAXHASH; i++) {
    phe = getsymtab(i);
    while (phe != NULL) {
      regionpst = HE_SYM(phe);
      while (regionpst != NULL) {
	if (symtab_is_sparse_reg(regionpst)) {
	  numdims = D_REG_NUM(S_DTYPE(regionpst));
	  fprintf(spsregfile,"static _spsmemreqs _%s_spsmemreqs[%d] = {",
		  S_IDENT(regionpst),numdims);
	  for (j=0; j<numdims; j++) {
	    if (j) {
	      fprintf(spsregfile,",");
	    }
	    fprintf(spsregfile,"{%d,%d,%d,%d}",
		    D_REG_NEED_IND(S_DTYPE(regionpst),j),
		    D_REG_NEED_NEXT(S_DTYPE(regionpst),j),
		    D_REG_NEED_PREV(S_DTYPE(regionpst),j),
		    D_REG_NEED_DENSE_DIR(S_DTYPE(regionpst),j));
	  }
	  fprintf(spsregfile,"};\n");
	}
	regionpst = S_NEXT(regionpst);
      }
      phe = HE_NEXT(phe);
    }
  }

  fclose(spsregfile);
}
