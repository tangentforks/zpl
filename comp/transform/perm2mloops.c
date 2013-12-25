/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "../include/alias.h"
#include "../include/buildstmt.h"
#include "../include/buildsymutil.h"
#include "../include/buildzplstmt.h"
#include "../include/createvar.h"
#include "../include/datatype.h"
#include "../include/dbg_code_gen.h"
#include "../include/dimension.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/inout.h"
#include "../include/live.h"
#include "../include/parsetree.h"
#include "../include/passes.h"
#include "../include/red2mloops.h"
#include "../include/rmstack.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/stmtutil.h"
#include "../include/statement.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"
#include "../include/buildsym.h"
#include "../include/main.h"
#include "../include/zutil.h"

#define PRPLEVELS new_const_int(permrleprocmap == -1 ? dims : permrleprocmap)
#define PRILEVELS new_const_int(permrleindices == -1 ? dims : permrleindices)

static statement_t* ps;       /* permute statement         */
static int lineno;            /* line number in file       */
static char* filename;        /* name of file              */
static int isscatter;         /* is the permute a scatter? */
static expr_t* src;           /* source expression         */
static expr_t* srctypecast;   /* source type cast          */
static expr_t* srctypecastnb; /* source type not base cast */
static expr_t* srctypesize;   /* source type size          */
static expr_t* dst;           /* destination expression    */
static expr_t* map;           /* maps for permute (# dims) */
static int dims;              /* rank of permute           */
static expr_t* rank;          /* rank of permute as expr_t */
static expr_t* npuid;         /* numperms as expr_t        */
static expr_t* region;        /* region of permute         */
int simple_index;  /** a remap with all maps using IndexIs only **/
int simple_maps[MAXRANK];
int complex_index;  /** a remap with all maps using IndexIs or scalars (not only) **/
int rgridmap = 0;

typedef struct pm_struct {
  int id;
  expr_t* permmap;
  expr_t* maplist;
  expr_t* region;
  int isscatter;
  set_t* destroyset; /* if these expressions change, destroy the map */
} pminfo;

static pminfo* permmaps = NULL;

static expr_t* create_permute_var(char* name, datatype_t* pdt) {
  symboltable_t* pst;
  expr_t* expr;

  pst = LU_INS(name);
  S_CLASS(pst) = S_VARIABLE;
  S_STD_CONTEXT(pst) = TRUE;
  S_DTYPE(pst) = pdt;
  expr = build_typed_0ary_op(VARIABLE, pst);
  return expr;
}

/*
 *  converts all parallel arrays to random access parallel arrays
 */
static void randacc_expr(expr_t* expr, expr_t* maplist) {
  /* BLC: This starts to get bogus: we used to attach T_RANDACC
     information to BIATs when they signified the root of an
     ensemble expression.  Now that they don't, we just attach
     it onto the current node; however, if the maplist is NULL,
     we attach the sentinel 0x1 (ugh!) so that we know that
     there's a NULL maplist.

     Better might be to insert a RANDACC node into the AST, but
     this scares me too much at this point and I need to get
     this checked in.  (in fact, such a node could be an explicit
     sentinel for roots of ensemble subexpressions, as BIATs and
     Null-BIATs used to be.  If the maplist was null, we'd just
     generate a normal walker/access macro.  */
  if (expr != NULL) {
    if (expr_at_ensemble_root(expr)) {
      if (T_RANDACC(expr) != NULL) {
	INT_FATAL(T_STMT(expr), "Trying to double up randacc list");
      }
      if (maplist != NULL) {
	T_RANDACC(expr) = copy_exprls(maplist, NULL);
      } else {
	T_RANDACC(expr) = (expr_t*)0x1;
      }
      T_MAPSIZE(expr) = dims;
    } else {
      randacc_expr(T_OPLS(expr), maplist);
      if (T_OPLS(expr)) {
	randacc_expr(T_NEXT(T_OPLS(expr)), maplist);
      }
    }
  }
}


/*
 *  read permute expression, validate it, and set the global variables
 */
static int perm_read(expr_t* permexpr) {
  if (permexpr == NULL || T_TYPE(permexpr) != PERMUTE) {
    return 0;
  }
  ps = T_STMT(permexpr);
  lineno = T_LINENO(ps);
  filename = T_FILENAME(ps);
  if (T_TYPE(ps) != S_EXPR && !T_IS_ASSIGNOP(T_TYPE(T_EXPR(ps)))) {
    INT_FATAL(ps, "perm_read: permute not in assignment statement");
  }
  isscatter = T_OPLS(T_EXPR(ps)) == permexpr;
  if (!((isscatter) ^ (T_NEXT(T_OPLS(T_EXPR(ps))) == permexpr))) {
    INT_FATAL(NULL, "perm_read: permute neither scatter nor gather");
  }
  src = (isscatter) ? T_NEXT(permexpr) : T_OPLS(permexpr);
  dst = (isscatter) ? T_OPLS(permexpr) : T_OPLS(T_PARENT(permexpr));
  map = T_MAP(permexpr);
  if (T_TYPEINFO_REG(isscatter ? dst : src) != NULL) {
    dims = T_MAPSIZE(permexpr);
    rank = new_const_int(dims);
  }
  else if (expr_is_free(isscatter ? dst : src)) {
    dims = 1;
    rank = new_const_int(dims);
  }
  rgridmap = 0;
  if (T_TYPE(map) == VARIABLE && T_IDENT(map) == lu_pst("_rgridremap")) {
    if (dims != 2) {
      USR_FATAL(NULL, "Remaps using :: maps are only supported for 1D remaps");
    }
    else {
      dims = 1;
      rank = new_const_int(dims);
      map = T_NEXT(map);
      rgridmap = 1;
    }
  }
  region = RMSCurrentRegion();
  if (T_IDENT(region) == pst_qreg[0]) { /* unresolved quote region */
    T_IDENT(region) = pst_qreg[dims];
  }
  npuid = new_const_int(numperms);

  { /** calculate simple_index -- an IndexI remap **/
    expr_t* tmp;
    int cnt;

    simple_index = 1;
    cnt = 0;
    for (tmp = map; tmp != NULL; cnt++, tmp = T_NEXT(tmp)) {
      int i;
      int found = 0;
      for (i = 0; i < MAXRANK; i++) {
	if (T_IDENT(tmp) == pstINDEX[i+1]) {
	  simple_maps[cnt] = i;
	  found = 1;
	}
      }
      if (!found) simple_index = 0;
    }
  }

  { /** calculate complex_index -- a foo(IndexI) remap **/
    expr_t* tmp;

    complex_index = 1;
    for (tmp = map; tmp != NULL; tmp = T_NEXT(tmp)) {
      if (!expr_contains_indexi(tmp) && T_TYPEINFO_REG(tmp)) {
	complex_index = 0;
      }
    }
  }

  return 1;
}

static expr_t* create_next_permmap_expr(void) {
  expr_t* expr;

  expr = create_permute_var(vstr("_pm[%d]", numperms-numshare), S_DTYPE(lu_pst("_permmap")));
  T_MAPSIZE(expr) = dims;
  return expr;
}

static void init_perm2mloops_expr(expr_t* permexpr) {
  pminfo* old;
  int i;
  int share;

  if (seqonly) {
    return;
  }

  if (!perm_read(permexpr)) {
    return;
  }
  if (!isscatter && T_TYPEINFO_REG(src) == NULL) {
    return;
  }
  old = permmaps;
  permmaps = (pminfo*)malloc((numperms+1)*sizeof(pminfo));
  for (i = 0; i < numperms; i++) {
    permmaps[i] = old[i];
  }
  free(old);
  share = 0;
  if (permmapshare && !expr_is_qreg(region)) {
    for (i = 0; i < numperms; i++) {
      expr_t* tmp1, * tmp2;
      if (isscatter != permmaps[i].isscatter) {
	continue;
      }
      if (region != permmaps[i].region) {
	continue;
      }
      for (tmp1 = permmaps[i].maplist, tmp2 = map;
	   tmp1 != NULL && tmp2 != NULL;
	   tmp1 = T_NEXT(tmp1), tmp2 = T_NEXT(tmp2)) {
	if (!expr_equal(tmp1, tmp2)) {
	  break;
	}
      }
      if (tmp1 || tmp2) {
	continue;
      }
      share = i+1;
      break;
    }
  }
  if (!share) {
    permmaps[numperms].id = numperms;
    permmaps[numperms].permmap = create_next_permmap_expr();
    permmaps[numperms].maplist = copy_exprls(map, NULL);
    permmaps[numperms].region = region;
    permmaps[numperms].isscatter = isscatter;
    permmaps[numperms].destroyset = getin(map);
  }
  else {
    permmaps[numperms] = permmaps[share-1];
    permmaps[numperms].destroyset = NULL;
    numshare++;
  }
  numperms++;
}

static void perm2mloops_expr(expr_t* pe /* permute expression*/) {
  expr_t* pm; /* permute map */
  expr_t* pd; /* permute data */
  expr_t* expr;
  statement_t* stmt;
  expr_t* mapcondvar;
  symboltable_t* pst;
  expr_t* distuat;
  expr_t* te;
  int i;
  expr_t* srcbucketarray;
  expr_t* dstbucketarray;
  int dstdead, srcdead;
  int dstdirect, srcdirect;
  int dstsrcconflict;
  int freeRemap = 0;
  int procMap = 1;
  int squelchIndices = 0;

  if (!perm_read(pe)) {
    return;
  }

  /*
   *  easy convert if we don't actually need to gather
   */
  if (T_TYPEINFO_REG(src) == NULL && !isscatter) {
    USR_WARN(ps, "Gather from non-parallel entity ignored");
    expr = build_binary_op(BIASSIGNMENT, copy_expr(src), copy_expr(dst));
    stmt = build_expr_statement(expr, lineno, filename);
    /*    stmt = build_mloop_statement(region, stmt, D_REG_NUM(S_DTYPE(region)), NULL, 0, lineno, filename);*/
    insertbefore_stmt(stmt, ps);
    T_PRE(stmt) = T_PRE(ps);
    T_POST(stmt) = T_POST(ps);
    remove_stmt(ps);
    return;
  }

  /*
   *  optimization if we are only running on a single processor
   */
  if (seqonly) {
    datatype_t* pdt;
    expr_t* temp;
    genlist_t* newgls;
    int aliasflag = 0;

    if (exprs_alias(dst, src)) {
      aliasflag = 1;
      pdt = build_ensemble_type(T_TYPEINFO(src), region, 0, 1, lineno, filename);
      pst = create_named_local_var(pdt, T_PARFCN(ps), vstr("_permute_tmp%d", numperms));
      S_SETUP(pst) = 0;
      temp = build_typed_0ary_op(VARIABLE, pst);
      T_TYPEINFO_REG(temp) = region;
      if (isscatter) {
	expr = build_binary_op(BIASSIGNMENT, copy_expr(src), copy_expr(temp));
	stmt = build_expr_statement(expr, lineno, filename);
	/*	stmt = build_mloop_statement(region, stmt, D_REG_NUM(S_DTYPE(region)), NULL, 0, lineno, filename);*/
	insertbefore_stmt(stmt, ps);
	newgls = alloc_gen();
	G_IDENT(newgls) = T_IDENT(temp);
	T_PRE(stmt) = cat_genlist_ls(T_PRE(ps), newgls);
      }
      else {
	expr = build_binary_op(BIASSIGNMENT, copy_expr(temp), copy_expr(dst));
	stmt = build_expr_statement(expr, lineno, filename);
/* 	stmt = build_mloop_statement(region, stmt, D_REG_NUM(S_DTYPE(region)), NULL, 0, lineno, filename); */
	insertafter_stmt(stmt, ps);
	newgls = alloc_gen();
	G_IDENT(newgls) = T_IDENT(temp);
	T_POST(stmt) = cat_genlist_ls(T_POST(ps), newgls);
      }
      if (isscatter) {
	src = build_unary_op(BIAT, copy_expr(temp));
      }
      else {
	dst = build_unary_op(BIAT, copy_expr(temp));
      }
    }
    if (isscatter) {
      expr = copy_expr(dst);
      randacc_expr(expr, map);
      expr = build_binary_op(T_TYPE(T_PARENT(pe)), copy_expr(src), expr);
      T_SUBTYPE(expr) = T_SUBTYPE(T_PARENT(pe));
    }
    else {
      expr = copy_expr(src);
      randacc_expr(expr, map);
      expr = build_binary_op(T_TYPE(T_PARENT(pe)), expr, copy_expr(dst));
      T_SUBTYPE(expr) = T_SUBTYPE(T_PARENT(pe));
    }
    stmt = build_expr_statement(expr, lineno, filename);
/*     stmt = build_mloop_statement(region, stmt, D_REG_NUM(S_DTYPE(region)), NULL, 0, lineno, filename); */
    insertbefore_stmt(stmt, ps);
    if (aliasflag) {
      if (isscatter) { 
	newgls = alloc_gen();
	G_IDENT(newgls) = T_IDENT(temp);
	T_POST(stmt) = cat_genlist_ls(T_POST(ps), newgls);
      }
      else {
	newgls = alloc_gen();
	G_IDENT(newgls) = T_IDENT(temp);
	T_PRE(stmt) = cat_genlist_ls(T_PRE(ps), newgls);
      }
    }
    else {
      T_PRE(stmt) = T_PRE(ps);
      T_POST(stmt) = T_POST(ps);
    }
    remove_stmt(ps);
    numperms++;
    return;
  }

  if (permprocmap == 0) {
    procMap = 0;

    /* sometimes we need a procmap,
       for example, if the map changes when it is read
       We should check for this situation --SJD
    */

  } else if (permprocmap == 1) {
    if (simple_index) {
      USR_FATAL(NULL, "Implementation Error: Do not use -nopermprocmap=1");
    }
    procMap = 1;
  } else if (permprocmap == -1) {

    /*** see note above ***/
    /*** heuristic to use or not use procmap ***/
    /*** if maps are Indexi, use it; otherwise don't ***/

    procMap = 0; /** ignore heuristic until procmap is fast computed along with the indices **/
  }

  pm = permmaps[numperms].permmap;

  /* insert permdata */
  pst = create_named_local_var(S_DTYPE(lu_pst("_permdata")), T_PARFCN(ps), vstr("_pd%d", numperms));
  pd = build_typed_0ary_op(VARIABLE,pst);

  /* insert mapcondvar */
  pst = create_named_local_var(pdtINT, T_PARFCN(ps), vstr("_pmbuild%d", numperms));
  mapcondvar = build_typed_0ary_op(VARIABLE,pst);

  srctypesize = create_permute_var(vstr("_srctypesize%d", numperms), T_TYPEINFO(src));
  T_TYPE(srctypesize) = SIZE;
  T_SUBTYPE(srctypesize) = 0;

  srctypecast = create_permute_var(vstr("_srctypecast%d", numperms), datatype_base(T_TYPEINFO(src)));
  T_TYPE(srctypecast) = SIZE;
  T_SUBTYPE(srctypecast) = 1;

  srctypecastnb = create_permute_var(vstr("_srctypecastnb%d", numperms), T_TYPEINFO(src));
  T_TYPE(srctypecastnb) = SIZE;
  T_SUBTYPE(srctypecastnb) = 1;

  /* determine distribution of permute */
  distuat = isscatter ? dst : src;
  distuat = expr_find_an_ensemble_root(distuat);
  if (distuat == NULL) {
    distuat = isscatter ? dst : src;
    if (expr_is_free(distuat)) {
      freeRemap = 1;
      squelchIndices = 1;
    }
    else {
      USR_FATAL(ps, "Illegal remap");
    }
  }
  if (rgridmap) {
    squelchIndices = 1;
  }
  if (!freeRemap) {
    distuat = build_function_call(lu_pst("_ARR_DIST"), 1, copy_expr(distuat));
  }
  else {
    distuat = build_0ary_op(CONSTANT, lu_pst("_null_distribution"));
  }

  srcbucketarray = expr_find_ensemble_root(src);
  srcdead = 1;
  if (srcbucketarray == NULL) {
    srcdead = 0;
  }
  else if (T_NEXT(ps) && !is_live(T_NEXT(ps), expr_find_ens_pst(srcbucketarray))) {
    if (T_IDENT(srcbucketarray) == T_IDENT(dst)) {
      srcdead = 0;
    }
  }
  else {
    srcdead = 0;
  }
  dstbucketarray = dst;
  dstdead = 0;

  dstdirect = 1;
  srcdirect = srcbucketarray == src;

  dstsrcconflict = varequals_expr(src, expr_find_ens_pst(dst));
  if (dstsrcconflict) {
    dstdirect = 0;
    srcdirect = 0;
  }
  if (!permsavedataspace) {
    dstsrcconflict = 1; /* specially disables using active part of dst in gather case */
    dstdead = 0;
    srcdead = 0;
  }
  if (!permdirect || D_REG_NUM(T_TYPEINFO(region)) != dims) { /* disable for rank change!!! */
    dstdirect = 0;
    srcdirect = 0;
  }

  if (T_TYPE(dstbucketarray) == ARRAY_REF &&
      (expr_parallel(T_NEXT(T_OPLS(dstbucketarray))) ||
       expr_is_free(T_NEXT(T_OPLS(dstbucketarray))))) {
    dstbucketarray = NULL;
  }

  if (!strncmp(S_IDENT(T_FCN(T_PARFCN(ps))), "_z_arr_copy", 11)) {
    if (permsavedataspace) {
      dstdead = 1;
      srcdead = 1;
    }
    if (permdirect) {
      dstdirect = 1;
      srcdirect = 1;
    }
  }

  /* MS - Map Setup */
  expr = build_function_call(lu_pst("_CALL_PERM_MS"), 7, pm,
			     mapcondvar, copy_expr(region),
			     rank, new_const_int(permmaps[numperms].id),
			     new_const_string(filename), new_const_int(lineno));
  stmt = build_expr_statement(expr,lineno,filename);
  insertbefore_stmt(stmt, ps);

  /* GC - Get Counts */
  if (!squelchIndices) {

    if (0 && simple_index) { /*** UNIMPLEMENTED ***/
      expr_t* sm_expr = NULL;
      int i;

      for (i = 0; i < dims; i++) {
	sm_expr = cat_expr_ls(sm_expr, new_const_int(simple_maps[i]));
      }      
      expr = build_function_call(lu_pst("_PERM_FAST_GCI_COMP_%dD", dims), 6,
				 pm,
				 copy_expr(mapcondvar),
				 new_const_int(isscatter),
				 copy_expr(region),
				 copy_expr(distuat),
				 sm_expr);
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
    }

    if (procMap) {
      expr = build_function_call(lu_pst("_PERM_GCI_BEGIN_%dD", dims), 4, pm, 
				 copy_expr(mapcondvar), copy_expr(npuid), copy_expr(distuat));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
      
      stmt = NULL;
      for (i = 0, te = map; te != NULL; te = T_NEXT(te), i++) {
	expr = build_function_call(lu_pst("_PERM_GC_TOPROC"), 3, copy_expr(te), new_const_int(i), copy_expr(npuid));
	stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      }
      expr = build_function_call(lu_pst("_PERM_GC_%dD", dims), 3, pm, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      expr = build_function_call(lu_pst("_PERM_GI_%dD", dims), 5, new_const_int(dstdirect || srcdirect), pm, PRILEVELS, copy_expr(npuid), copy_exprls(map, NULL));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      stmt = build_mscan_statement(NULL, stmt, lineno, filename);
      insertbefore_stmt(stmt,ps);

      expr = build_function_call(lu_pst("_PERM_GCI_END"), 5, pm, PRPLEVELS, PRILEVELS, copy_expr(rank), copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
    }
    else {
      expr = build_function_call(lu_pst("_PERM_NP_GCI_BEGIN_%dD", dims), 4, pm, 
				 copy_expr(mapcondvar), copy_expr(npuid), copy_expr(distuat));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
      
      stmt = NULL;
      for (i = 0, te = map; te != NULL; te = T_NEXT(te), i++) {
	expr = build_function_call(lu_pst("_PERM_NP_GC_TOPROC"), 3, copy_expr(te), new_const_int(i), copy_expr(npuid));
	stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      }
      expr = build_function_call(lu_pst("_PERM_NP_GC_%dD", dims), 3, pm, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      expr = build_function_call(lu_pst("_PERM_NP_GI_%dD", dims), 5, new_const_int(dstdirect || srcdirect), pm, PRILEVELS, copy_expr(npuid), copy_exprls(map, NULL));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      stmt = build_mscan_statement(NULL, stmt, lineno, filename);
      insertbefore_stmt(stmt,ps);
    
      expr = build_function_call(lu_pst("_PERM_NP_GCI_END"), 5, pm, PRPLEVELS, PRILEVELS, copy_expr(rank), copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
    }
  }
  else { /** Squelch Indices **/
    if (procMap) {
      expr = build_function_call(lu_pst("_PERM_NI_GCI_BEGIN_%dD", dims), 4, pm, 
				 copy_expr(mapcondvar), copy_expr(npuid), copy_expr(distuat));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
      
      stmt = NULL;
      for (i = 0, te = map; te != NULL; te = T_NEXT(te), i++) {
	expr = build_function_call(lu_pst("_PERM_NI_GC_TOPROC"), 3, copy_expr(te), new_const_int(i), copy_expr(npuid));
	stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      }
      expr = build_function_call(lu_pst("_PERM_NI_GC_%dD", dims), 3, pm, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      expr = build_function_call(lu_pst("_PERM_NI_GI"), 2, pm, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      stmt = build_mscan_statement(NULL, stmt, lineno, filename);
      insertbefore_stmt(stmt,ps);

      expr = build_function_call(lu_pst("_PERM_NI_GCI_END"), 5, pm, PRPLEVELS, PRILEVELS, copy_expr(rank), copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
    }
    else {
      expr = build_function_call(lu_pst("_PERM_NP_NI_GCI_BEGIN_%dD", dims), 4, pm, 
				 copy_expr(mapcondvar), copy_expr(npuid), copy_expr(distuat));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
      
      stmt = NULL;
      for (i = 0, te = map; te != NULL; te = T_NEXT(te), i++) {
	expr = build_function_call(lu_pst("_PERM_NP_NI_GC_TOPROC"), 3, copy_expr(te), new_const_int(i), copy_expr(npuid));
	stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      }
      expr = build_function_call(lu_pst("_PERM_NP_NI_GC_%dD", dims), 3, pm, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      expr = build_function_call(lu_pst("_PERM_NP_NI_GI"), 2, pm, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      stmt = build_mscan_statement(NULL, stmt, lineno, filename);
      insertbefore_stmt(stmt,ps);
    
      expr = build_function_call(lu_pst("_PERM_NP_NI_GCI_END"), 5, pm, PRPLEVELS, PRILEVELS, copy_expr(rank), copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
    }
  }

  /* IR - Indices Ready */
  expr = build_function_call(lu_pst("_CALL_PERM_IR"), 2, pm, mapcondvar);
  stmt = build_expr_statement(expr,lineno,filename);
  insertbefore_stmt(stmt, ps);

  /* IN - Indices Needed */
  expr = build_function_call(lu_pst("_CALL_PERM_IN"), 13, pm, mapcondvar,
			     new_const_int(isscatter),
			     srctypesize,
			     (dstbucketarray) ? copy_expr(dstbucketarray) : build_typed_0ary_op(VARIABLE, lu_pst("_null_array")),
			     (srcbucketarray) ? copy_expr(srcbucketarray) : build_typed_0ary_op(VARIABLE, lu_pst("_null_array")),
			     copy_expr(region),
			     PRILEVELS,
			     new_const_int(dstsrcconflict),
			     new_const_int(dstdead),
			     new_const_int(srcdead),
			     new_const_int(dstdirect),
			     new_const_int(srcdirect));
  stmt = build_expr_statement(expr,lineno,filename);
  insertbefore_stmt(stmt, ps);

  /* DS - Data Setup */
  expr = build_function_call(lu_pst("_CALL_PERM_DS"), 14,
			     new_const_int(permmaps[numperms].id),
			     pm,
			     pd,
			     new_const_int(isscatter),
			     srctypesize,
			     (dstbucketarray) ? copy_expr(dstbucketarray) : build_typed_0ary_op(VARIABLE, lu_pst("_null_array")),
			     (srcbucketarray) ? copy_expr(srcbucketarray) : build_typed_0ary_op(VARIABLE, lu_pst("_null_array")),
			     copy_expr(region),
			     PRILEVELS,
			     new_const_int(dstsrcconflict),
			     new_const_int(dstdead),
			     new_const_int(srcdead),
			     new_const_int(dstdirect),
			     new_const_int(srcdirect));
  stmt = build_expr_statement(expr,lineno,filename);
  insertbefore_stmt(stmt, ps);
  T_PRE(stmt) = T_PRE(ps);

  /* GD - Get Data */
  if (isscatter) {
    if (procMap) {
      expr = build_function_call(lu_pst("_SCATTER_GD_PRE_MLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);

      expr = build_function_call(lu_pst("_SCATTER_GD_PRE_NLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      expr = build_function_call(lu_pst("_SCATTER_GD"), 5, src, pm, pd, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr,lineno,filename));
      expr = build_function_call(lu_pst("_SCATTER_GD_POST_NLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      stmt = build_mscan_statement(NULL, stmt, lineno, filename);
      insertbefore_stmt(stmt,ps);

      expr = build_function_call(lu_pst("_SCATTER_GD_POST_MLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
    }
    else {
      expr = build_function_call(lu_pst("_SCATTER_NP_GD_PRE_MLOOP"), 6, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid), copy_expr(distuat));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);

      stmt = NULL;
      if (!squelchIndices) {
	for (i = 0, te = map; te != NULL; te = T_NEXT(te), i++) {
	  expr = build_function_call(lu_pst("_PERM_NP_GC_TOPROC"), 3, copy_expr(te), new_const_int(i), copy_expr(npuid));
	  stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
	}
	expr = build_function_call(lu_pst("_PERM_NP_GC_%dD", dims), 3, pm, PRPLEVELS, copy_expr(npuid));
	stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      }
      else {
	for (i = 0, te = map; te != NULL; te = T_NEXT(te), i++) {
	  expr = build_function_call(lu_pst("_PERM_NP_NI_GC_TOPROC"), 3, copy_expr(te), new_const_int(i), copy_expr(npuid));
	  stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
	}
	expr = build_function_call(lu_pst("_PERM_NP_NI_GC_%dD", dims), 3, pm, PRPLEVELS, copy_expr(npuid));
	stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      }

      expr = build_function_call(lu_pst("_SCATTER_NP_GD_PRE_NLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      expr = build_function_call(lu_pst("_SCATTER_NP_GD"), 5, src, pm, pd, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr,lineno,filename));
      expr = build_function_call(lu_pst("_SCATTER_NP_GD_POST_NLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      stmt = build_mscan_statement(NULL, stmt, lineno, filename);
      insertbefore_stmt(stmt,ps);

      expr = build_function_call(lu_pst("_SCATTER_NP_GD_POST_MLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
    }
  } 
  else {
    expr_t* srcexpr;
    expr_t* randsrcexpr;

    srcexpr = copy_expr(src);
    randacc_expr(srcexpr, NULL);
    randsrcexpr = srcexpr;

    if (!squelchIndices) {
      if (permloopdecode) {
	expr = build_function_call(lu_pst("_GATHER_GD_LOOP_PRE_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_GATHER_GD_LOOP"), 7, srcexpr, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_GATHER_GD_LOOP_ELSE_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_GATHER_GD_LOOP_BAIL"), 7, randsrcexpr, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_GATHER_GD_LOOP_POST_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
      }
      else {
	expr = build_function_call(lu_pst("_GATHER_GD_PRE_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_GATHER_GD"), 7, srcexpr, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_GATHER_GD_POST_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
      }
    }
    else {
      expr = build_function_call(lu_pst("_GATHER_NI_GD_PRE_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
      stmt = build_expr_statement(expr,lineno,filename);
      insertbefore_stmt(stmt,ps);
      srcexpr = copy_expr(srcexpr);
      /*
      T_MAP(T_OPLS(srcexpr)) = NULL;
      for (i = 0; i < T_MAPSIZE(T_OPLS(srcexpr)); i++) {
	T_MAP(T_OPLS(srcexpr)) = cat_expr_ls(T_MAP(T_OPLS(srcexpr)), new_const_int(0));  /\*** must zero out map ***\/
      }
      */
      if (rgridmap) {
	  expr_t* map;

	  if (T_TYPE(srcexpr) != ARRAY_REF) {
	      USR_FATAL(ps, "Not indexing into :: remap");
	  }
	  map = T_NEXT(T_OPLS(srcexpr));
	  srcexpr = copy_expr(T_OPLS(srcexpr));
	  T_MAP(srcexpr) = map;
	  T_MAPSIZE(srcexpr) = 1;
      }
      expr = build_function_call(lu_pst("_GATHER_NI_GD"), 7, srcexpr, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
      stmt = build_expr_statement(expr,lineno,filename);
      insertbefore_stmt(stmt,ps);
      expr = build_function_call(lu_pst("_GATHER_NI_GD_POST_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
      stmt = build_expr_statement(expr,lineno,filename);
    }
    insertbefore_stmt(stmt,ps);
  }

  expr = build_binary_op(BIASSIGNMENT, new_const_int(0), mapcondvar);
  stmt = build_expr_statement(expr, lineno, filename);
  insertbefore_stmt(stmt, ps);

  /* DR - Data Ready */
  expr = build_function_call(lu_pst("_CALL_PERM_DR"), 5, pm, pd, new_const_int(isscatter),
			     (dstbucketarray) ? copy_expr(dstbucketarray) : build_typed_0ary_op(VARIABLE, lu_pst("_null_array")),
			     (srcbucketarray) ? copy_expr(srcbucketarray) : build_typed_0ary_op(VARIABLE, lu_pst("_null_array")));
  
  stmt = build_expr_statement(expr,lineno,filename);
  insertbefore_stmt(stmt, ps);

  /* DN - Data Needed */
  expr = build_function_call(lu_pst("_CALL_PERM_DN"), 3, pm, pd, new_const_int(isscatter));
  stmt = build_expr_statement(expr,lineno,filename);
  insertbefore_stmt(stmt, ps);

  /* PD - Put Data */
  if (!isscatter) {
    if (procMap) {
    expr = build_function_call(lu_pst("_GATHER_PD_PRE_MLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
    stmt = build_expr_statement(expr, lineno, filename);
    insertbefore_stmt(stmt, ps);

    expr = build_function_call(lu_pst("_GATHER_PD_PRE_NLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
    stmt = build_expr_statement(expr, lineno, filename);
    expr = build_function_call(lu_pst("_GATHER_PD"), 5, copy_expr(dst), copy_expr(pm), copy_expr(pd), PRPLEVELS, copy_expr(npuid));
    stmt = cat_stmt_ls(stmt, build_expr_statement(expr,lineno,filename));
    expr = build_function_call(lu_pst("_GATHER_PD_POST_NLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
    stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
    stmt = build_mscan_statement(NULL, stmt, lineno, filename);
    insertbefore_stmt(stmt,ps);

    expr = build_function_call(lu_pst("_GATHER_PD_POST_MLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
    stmt = build_expr_statement(expr, lineno, filename);
    insertbefore_stmt(stmt, ps);
    }
    else {
      expr = build_function_call(lu_pst("_GATHER_NP_PD_PRE_MLOOP"), 6, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid), copy_expr(distuat));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);

      stmt = NULL;
      if (!squelchIndices) {
	for (i = 0, te = map; te != NULL; te = T_NEXT(te), i++) {
	  expr = build_function_call(lu_pst("_PERM_NP_GC_TOPROC"), 3, copy_expr(te), new_const_int(i), copy_expr(npuid));
	  stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
	}
	expr = build_function_call(lu_pst("_PERM_NP_GC_%dD", dims), 3, pm, PRPLEVELS, copy_expr(npuid));
	stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      }
      else {
	for (i = 0, te = map; te != NULL; te = T_NEXT(te), i++) {
	  expr = build_function_call(lu_pst("_PERM_NP_NI_GC_TOPROC"), 3, copy_expr(te), new_const_int(i), copy_expr(npuid));
	  stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
	}
	expr = build_function_call(lu_pst("_PERM_NP_NI_GC_%dD", dims), 3, pm, PRPLEVELS, copy_expr(npuid));
	stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      }
      expr = build_function_call(lu_pst("_GATHER_NP_PD_PRE_NLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      expr = build_function_call(lu_pst("_GATHER_NP_PD"), 5, copy_expr(dst), copy_expr(pm), copy_expr(pd), PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr,lineno,filename));
      expr = build_function_call(lu_pst("_GATHER_NP_PD_POST_NLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = cat_stmt_ls(stmt, build_expr_statement(expr, lineno, filename));
      stmt = build_mscan_statement(NULL, stmt, lineno, filename);
      insertbefore_stmt(stmt,ps);

      expr = build_function_call(lu_pst("_GATHER_NP_PD_POST_MLOOP"), 5, pm, pd, srctypecast, PRPLEVELS, copy_expr(npuid));
      stmt = build_expr_statement(expr, lineno, filename);
      insertbefore_stmt(stmt, ps);
    }
  }
  else {
    expr_t* dstexpr;
    expr_t* randdstexpr;
    expr_t* assignop;

    dstexpr = copy_expr(dst);
    randacc_expr(dstexpr, NULL);
    randdstexpr = dstexpr;

    if (T_TYPE(T_PARENT(pe)) == BIOP_GETS) {
      if (T_SUBTYPE(T_PARENT(pe)) == PLUS) {
	assignop = build_typed_0ary_op(CONSTANT, lu_pst("_assignop_plus"));
      }
      else {
	USR_FATAL(ps, "Unimplemented scatter combining operator");
      }
    }
    else {
      assignop = build_typed_0ary_op(CONSTANT, lu_pst("_assignop_arbitrary"));
    }

    if (!squelchIndices) {
      if (permloopdecode) {
	expr = build_function_call(lu_pst("_SCATTER_PD_LOOP_PRE_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_SCATTER_PD_LOOP"), 8, dstexpr, copy_expr(assignop), copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_SCATTER_PD_LOOP_ELSE_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_SCATTER_PD_LOOP_BAIL"), 8, randdstexpr, copy_expr(assignop), copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_SCATTER_PD_LOOP_POST_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
      }
      else {
	expr = build_function_call(lu_pst("_SCATTER_PD_PRE_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_SCATTER_PD"), 8, dstexpr, copy_expr(assignop), copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
	expr = build_function_call(lu_pst("_SCATTER_PD_POST_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
	stmt = build_expr_statement(expr,lineno,filename);
	insertbefore_stmt(stmt,ps);
      }
    }
    else {
      expr = build_function_call(lu_pst("_SCATTER_NI_PD_PRE_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
      stmt = build_expr_statement(expr,lineno,filename);
      insertbefore_stmt(stmt,ps);
      /*
      dstexpr = copy_expr(dstexpr);
      T_MAP(T_OPLS(dstexpr)) = NULL;
      for (i = 0; i < T_MAPSIZE(T_OPLS(dstexpr)); i++) {
	T_MAP(T_OPLS(dstexpr)) = cat_expr_ls(T_MAP(T_OPLS(dstexpr)), new_const_int(0));  /\*** must zero out map ***\/
      }
      */
      if (rgridmap) {
	  expr_t* map;

	  if (T_TYPE(dstexpr) != ARRAY_REF) {
	      USR_FATAL(ps, "Not indexing into :: remap");
	  }
	  map = T_NEXT(T_OPLS(dstexpr));
	  dstexpr = copy_expr(T_OPLS(dstexpr));
	  T_MAP(dstexpr) = map;
	  T_MAPSIZE(dstexpr) = 1;
      }
      expr = build_function_call(lu_pst("_SCATTER_NI_PD"), 8, dstexpr, copy_expr(assignop), copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
      stmt = build_expr_statement(expr,lineno,filename);
      insertbefore_stmt(stmt,ps);
      expr = build_function_call(lu_pst("_SCATTER_NI_PD_POST_NLOOP"), 6, copy_expr(rank), PRILEVELS, srctypecast, pm, pd, copy_expr(npuid));
      stmt = build_expr_statement(expr,lineno,filename);
      insertbefore_stmt(stmt,ps);
    }
  }

  /* DD - Data Destroy */
  expr = build_function_call(lu_pst("_CALL_PERM_DD"), 4,
			     new_const_int(permmaps[numperms].id),
			     pm,
			     pd,
			     new_const_int(isscatter));
  stmt = build_expr_statement(expr,lineno,filename);
  insertbefore_stmt(stmt, ps);
  T_POST(stmt) = T_POST(ps);

  /* MD - Map Destroy */
  if (!complex_index || expr_dyn_reg(region) || !permmapsave || 
      (T_IDENT(region) != NULL && S_SETUP(T_IDENT(region)) == 0)) {
    expr = build_function_call(lu_pst("_CALL_PERM_MD"), 1, pm);
    stmt = build_expr_statement(expr,lineno,filename);
    insertbefore_stmt(stmt, ps);
  }
  remove_stmt(ps);
  numperms++;
}

static void init_perm2mloops_stmt(statement_t* stmt) {
  if (stmt != NULL) {
    if (T_TYPE(stmt) == S_EXPR) {
      traverse_exprls(T_EXPR(stmt), 0, init_perm2mloops_expr);
    }
  }
}

static void init_perm2mloops_mod(module_t* mod) {
  function_t* fn;

  fn = T_FCNLS(mod);
  while (fn != NULL) {
    traverse_stmtls(T_STLS(fn), 0, init_perm2mloops_stmt, NULL, NULL);
    fn = T_NEXT(fn);
  }
}

static void perm2mloops_stmt(statement_t* stmt) {
  if (stmt != NULL) {
    if (T_TYPE(stmt) == S_EXPR) {
      traverse_exprls(T_EXPR(stmt), 0, perm2mloops_expr);
    }
  }
}

static void perm2mloops_mod(module_t* mod) {
  function_t* fn;

  fn = T_FCNLS(mod);
  while (fn != NULL) {
    traverse_stmtls(T_STLS(fn), 0, perm2mloops_stmt, NULL, NULL);
    fn = T_NEXT(fn);
  }
}

static void mapdestroy_stmt(statement_t* stmt) {
  int i;
  set_t* destroyset;
  set_t* outset;
  expr_t* call_perm_md;

  if (!complex_index || seqonly || !permmapsave || !stmt) {
    return;
  }

  for (i = 0; i < numperms; i++) {
    for (destroyset = permmaps[i].destroyset; destroyset != NULL; destroyset = SET_NEXT(destroyset)) {
      for (outset = T_OUT(stmt); outset != NULL; outset = SET_NEXT(outset)) {
	if (SET_VAR(destroyset) == SET_VAR(outset)) {
	  call_perm_md = build_function_call(lu_pst("_CALL_PERM_MD"), 1, permmaps[i].permmap);
	  insertbefore_stmt(build_expr_statement(call_perm_md, lineno, filename), stmt);
	  return;
	}
      }
    }
  }
}

static void mapdestroy_mod(module_t* mod) {
  function_t* fn;

  fn = T_FCNLS(mod);
  while (fn != NULL) {
    traverse_stmtls(T_STLS(fn), 0, mapdestroy_stmt, NULL, NULL);
    fn = T_NEXT(fn);
  }
}

int call_perm2mloops(module_t* mod, char* s) {
  numperms = 0;
  numshare = 0;
  RMSInit();
  traverse_modules(mod, TRUE, init_perm2mloops_mod, NULL);
  RMSFinalize();
  numperms = 0;
  RMSInit();
  traverse_modules(mod, TRUE, perm2mloops_mod, NULL);
  RMSFinalize();
  RMSInit();
  traverse_modules(mod, TRUE, mapdestroy_mod, NULL);
  RMSFinalize();
  return 0;
}
