/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/*
 * FILE: ntablet.c - mloop specific neighborhood tablet
 * DATE: 10 MAY 2000
 * CREATOR: deitz
 *
 * SUMMARY: The neighborhood tablet is an abstraction designed for use
 * by both the stencil optimization and the nudge optimization.
 * Previously, the neighborhood tablet was referred to as the stencil
 * rectangle.  The key difference is that the mloop statement needs
 * not be a stencil statement.  The weights are generalized and set to
 * null by the functions in this routine.  The stencil optimization
 * uses these as weights, the nudge optimization ignores them.  The
 * aid is set to the source array.  A sub-tablet, previously a
 * sub-rectangle and in the context of the stencil optimization: a
 * redundancy group, is the size of the neighborhood tablet, but
 * contains only one or zero for each element in this tablet.  The
 * sub-tablet-list is a linked list of sub-tablets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/datatype.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/runtime.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/scale.h"
#include "../include/ntablet.h"
#include "../include/zutil.h"

/****************************************************************** TABLET ***/

tabnode* get_tabnode(tab* t, int r, int c)
{
  if (t == NULL) {
    INT_FATAL(NULL, "get_tabnode: tab arg is null");
  }
  if (r < 0 || r >= TAB_ROWS(t)) {
    INT_FATAL(NULL, "get_tabnode: row arg is out of bounds");
  }
  if (c <0 || c >= TAB_COLS(t)) {
    INT_FATAL(NULL, "get_tabnode: column arg is out of bounds");
  }
  return &(TAB_ELTS(t)[r*t->cols+c]);
}

static void copy_tabnode(tab* nt, int nr, int nc, tab* t, int r, int c)
{
  int i;

  if (nt == NULL) {
    INT_FATAL(NULL, "copy_tabnode: 'to' tab arg is null");
  }
  if (nr < 0 || nr >= TAB_ROWS(nt)) {
    INT_FATAL(NULL, "copy_tabnode: 'to' row arg is out of bounds");
  }
  if (nc < 0 || nc >= TAB_COLS(nt)) {
    INT_FATAL(NULL, "copy_tabnode: 'to' column arg is out of bounds");
  }
  if (t == NULL) {
    INT_FATAL(NULL, "copy_tabnode: 'from' tab arg is null");
  }
  if (r < 0 || r >= TAB_ROWS(t)) {
    INT_FATAL(NULL, "copy_tabnode: 'from' row arg is out of bounds");
  }
  if (c < 0 || c >= TAB_COLS(t)) {
    INT_FATAL(NULL, "copy_tabnode: 'from' column arg is out of bounds");
  }
  for (i = 0; i < MAXRANK; i++) {
    TAB_DIR(nt, nr, nc)[i] = TAB_DIR(t, r, c)[i];
  }
  TAB_WID(nt, nr, nc) = TAB_WID(t, r, c);
  TAB_AID(nt, nr, nc) = TAB_AID(t, r, c);
  TAB_SID(nt, nr, nc) = TAB_SID(t, r, c);
  TAB_WEXPR(nt, nr, nc) = TAB_WEXPR(t, r, c);
  TAB_AEXPR(nt, nr, nc) = TAB_AEXPR(t, r, c);
  TAB_SEXPR(nt, nr, nc) = TAB_SEXPR(t, r, c);
  TAB_NXT(nt, nr, nc) = TAB_NXT(t, r, c);
}

static void clear_tabnode(tab* t, int r, int c)
{
  int i;

  if (t == NULL) {
    INT_FATAL(NULL, "clear_tabnode: tab arg is null");
  }
  if (r < 0 || r >= TAB_ROWS(t)) {
    INT_FATAL(NULL, "clear_tabnode: row arg is out of bounds");
  }
  if (c < 0 || c >= TAB_COLS(t)) {
    INT_FATAL(NULL, "clear_tabnode: column arg is out of bounds");
  }
  for (i = 0; i < MAXRANK; i++) {
    TAB_DIR(t, r, c)[i] = 0;
  }
  TAB_WID(t, r, c) = 0;
  TAB_AID(t, r, c) = 0;
  TAB_SID(t, r, c) = 0;
  TAB_WEXPR(t, r, c) = NULL;
  TAB_AEXPR(t, r, c) = NULL;
  TAB_SEXPR(t, r, c) = NULL;
  TAB_NXT(t, r, c) = 0;
}

void print_tab(tab* t)
{
  int r, c;

  if (t == NULL) {
    INT_FATAL(NULL, "print_tab: tab arg is null");
  }
  printf("TAB: (%d rows, %d cols) ", TAB_ROWS(t), TAB_COLS(t));
  printf("w/ stride = %d & nzero = %d\n", TAB_STRIDE(t), TAB_NZERO(t));
  printf("   MINDIR: ");
  for (c = 0; c < t->rank; c++) {
    printf("%4d ", t->mindir[c]);
  }
  printf("\n   MAXDIR: ");
  for (c = 0; c < t->rank; c++) {
    printf("%4d ", t->maxdir[c]);
  }
  printf("\n   POSDIR: ");
  for (c = 0; c < t->rank; c++) {
    printf("%4d ", t->posdir[c]);
  }
  printf("\n");
  for (r = 0; r < TAB_ROWS(t); r++) {
    for (c = 0; c < TAB_COLS(t); c++) {
      printf("%3d ", TAB_WID(t, r, c));
      printf("(A%2d, ", TAB_AID(t, r, c));
      printf("S%2d) %2d   ", TAB_SID(t, r, c), TAB_NXT(t, r, c));
    }
    printf("\n");
  }
}

static tab* alloc_tab(int rows, int cols)
{
  tab* t;

  t = (tab*)malloc(sizeof(tab));
  if (t == NULL) {
    INT_FATAL(NULL, "alloc_tab: tab not allocated");
  }
  TAB_ELTS(t) = (tabnode *)malloc(rows*cols*sizeof(tabnode));
  if (TAB_ELTS(t) == NULL && rows*cols*sizeof(tabnode) != 0) {
    INT_FATAL(NULL, "alloc_tab: tab elts not allocated");
  }
  TAB_COLS(t) = cols;
  TAB_ROWS(t) = rows;
  return t;
}

void free_tab(tab* t)
{
  if (t == NULL) {
    INT_FATAL(NULL, "free_tab: tab arg is null");
  }
  free(TAB_ELTS(t));
  free(t);
}

int has_tab(tab* t, int r)
{
  if (t == NULL) {
    INT_FATAL(NULL, "has_tab: tab arg is null");
  }
  if (r < 0 || r >= TAB_ROWS(t)) {
    INT_FATAL(NULL, "has_tab: row arg is out of bounds");
  }
  return TAB_WID(t, r, 0) || TAB_NXT(t, r, 0);
}

void fix_tab(tab* t) {
  int r, c;
  int nxtrow, nxtcol;

  if (t == NULL) {
    INT_FATAL(NULL, "fix_tab: tab arg is null");
  }
  nxtrow = 0;
  for (r = TAB_ROWS(t)-1; r >= 0; r--) {
    nxtcol = 0;
    for (c = TAB_COLS(t)-1; c >= 0; c--) {
      if (TAB_WID(t, r, c)) {
	TAB_NXT(t, r, c) = (c == TAB_COLS(t)-1) ? nxtrow : nxtcol;
	nxtrow = r;
	nxtcol = c;
      }
      else {
	TAB_NXT(t, r, c) = (c == TAB_COLS(t)-1) ? nxtrow : nxtcol;
      }
    }
  }
}

tab* resize_tab(tab* t, int rows, int cols)
{
  tab* nt;
  int r, c;
  int nxtrow, nxtcol;

  if (t == NULL) {
    INT_FATAL(NULL, "resize_tab: tab arg is null");
  }
  nt = alloc_tab(rows, cols);
  TAB_RANK(nt) = TAB_RANK(t);
  TAB_STRIDE(nt) = TAB_STRIDE(t);
  TAB_INNERD(nt) = TAB_INNERD(t);
  TAB_DIRUP(nt) = TAB_DIRUP(t);
  TAB_NZERO(nt) = 0;
  TAB_NSRC(nt) = TAB_NSRC(t);
  TAB_NDST(nt) = TAB_NDST(t);
  nxtrow = 0;
  for (r = TAB_ROWS(nt)-1; r >= 0; r--) {
    nxtcol = 0;
    for (c = TAB_COLS(nt)-1; c >= 0; c--) {
      if (r < TAB_ROWS(t) && c < TAB_COLS(t) && TAB_WID(t, r, c)) {
	TAB_NZERO(nt)++;
	copy_tabnode(nt, r, c, t, r, c);
	TAB_NXT(nt, r, c) = (c == TAB_COLS(nt)-1) ? nxtrow : nxtcol;
	nxtrow = r;
	nxtcol = c;
      }
      else {
	clear_tabnode(nt, r, c);
	TAB_NXT(nt, r, c) = (c == TAB_COLS(nt)-1) ? nxtrow : nxtcol;
      }
    }
  }
  free_tab(t);
  return nt;
}

tab* copy_tab(tab* t)
{
  tab *nt;
  int r, c;

  if (t == NULL) {
    INT_FATAL(NULL, "copy_tab: tab arg is null");
  }
  nt = alloc_tab(TAB_ROWS(t), TAB_COLS(t));
  for (r = 0; r < TAB_ROWS(t); r++) {
    for (c = 0; c < TAB_COLS(t); c++) {
      copy_tabnode(nt, r, c, t, r, c);
    }
  }
  TAB_RANK(nt) = TAB_RANK(t);
  TAB_STRIDE(nt) = TAB_STRIDE(t);
  TAB_INNERD(nt) = TAB_INNERD(t);
  TAB_DIRUP(nt) = TAB_DIRUP(t);
  TAB_NZERO(nt) = TAB_NZERO(t);
  TAB_NSRC(nt) = TAB_NSRC(t);
  TAB_NDST(nt) = TAB_NDST(t);
  return nt;
}

void clear_tab(tab* t)
{
  int r, c;

  if (t == NULL) {
    INT_FATAL(NULL, "copy_tab: tab arg is null");
  }
  for (r = 0; r < TAB_ROWS(t); r++) {
    for (c = 0; c < TAB_COLS(t); c++) {
      clear_tabnode(t, r, c);
    }
  }
  TAB_NSRC(t) = 0;
  TAB_NDST(t) = 0;
  TAB_NZERO(t) = 0;
}

void subout_tab(tab* t, subtab* s)
{
  int r, c;

  if (t == NULL) {
    INT_FATAL(NULL, "subout_tab: tab arg is null");
  }
  if (s == NULL) {
    INT_FATAL(NULL, "subout_tab: subtab arg is null");
  }
  if (SUBTAB_ROWS(s) > t->rows || SUBTAB_COLS(s) > t->cols) {
    INT_FATAL(NULL, "subout_tab: subtab is larger than tab");
  }
  for (r = 0; r < SUBTAB_ROWS(s); r++) {
    for (c = 0; c < SUBTAB_COLS(s); c++) {
      if (get_subtab(s, r, c)) {
	clear_tabnode(t, r, c);
	TAB_NZERO(t)--;
      }
    }
  }
  fix_tab(t);
  if (TAB_NZERO(t) < 0) {
    INT_FATAL(NULL, "subout_tab: tab less than empty");
  }
}

static void add_tabnode(tab* t, expr_t* aexpr, expr_t* sexpr, 
			 int sid, expr_t* (*wfunc)(expr_t*))
{
  int i;
  static int aid;
  static int wid;
  int drel[MAXRANK]; /* relative ditabtion */
  int srel[MAXRANK]; /* relative step      */
  int success = 1;
  int nullat;
  expr_t* arrexpr;     /* the A in A@east; could store this instead */
  expr_t* tabarrexpr;  /* same, but for tab expr */

  if (T_TYPE(aexpr) != BIAT) {
    arrexpr = aexpr;
  } else {
    arrexpr = expr_find_ensemble_root(aexpr);
  }
  if (t->rows-1 == 0) { /* set aid to zero for first */
    aid = 0;
    wid = 0;
  }
  TAB_WID(t, t->rows-1, 0) = ++wid;
  TAB_AID(t, t->rows-1, 0) = ++aid;
  TAB_SID(t, t->rows-1, 0) = sid;
  TAB_WEXPR(t, t->rows-1, 0) = wfunc(aexpr);
  TAB_AEXPR(t, t->rows-1, 0) = aexpr;
  TAB_SEXPR(t, t->rows-1, 0) = sexpr;
  for (i = 0; i < t->rows-1; i++) {
    if (expr_equal(wfunc(aexpr), TAB_WEXPR(t, i, 0))) {
      TAB_WID(t, t->rows-1, 0) = TAB_WID(t, i, 0);
    }
    if (T_TYPE(TAB_AEXPR(t,i,0)) != BIAT) {
      tabarrexpr = TAB_AEXPR(t,i,0);
    } else {
      tabarrexpr = expr_find_ensemble_root(TAB_AEXPR(t,i,0));
    }
    if (expr_equal(arrexpr, tabarrexpr)) {
      TAB_AID(t, t->rows-1, 0) = TAB_AID(t, i, 0);
    }
  }
  if (TAB_AID(t, t->rows-1, 0) != aid) {
    aid--;
  }
  if (TAB_WID(t, t->rows-1, 0) != wid) {
    wid--;
  }
  if (TAB_NSRC(t) < TAB_AID(t, t->rows-1, 0)) {
    TAB_NSRC(t) = TAB_AID(t, t->rows-1, 0);
  }
  if (TAB_NDST(t) < TAB_SID(t, t->rows-1, 0)) {
    TAB_NDST(t) = TAB_SID(t, t->rows-1, 0);
  }
  nullat = (T_TYPE(aexpr) != BIAT);
  for (i = 0; i < t->rank; i++) {
    /* BC+SD: V_DIR_VECT below only works for literal directions */
    TAB_DIR(t, t->rows-1, 0)[i] = 
      nullat ? 0
      : dir_comp_to_int(T_NEXT(T_OPLS(aexpr)), i, &success);
    if (!success) {
      INT_FATAL(NULL, "add_tabnode failed due to dir_comp_to_int");
    }
  }
  if (!nullat) {
    i = reg_dir_info(aexpr, srel, drel);
    INT_COND_FATAL(i != -1, NULL, "reg_dir_info used to know, now not");
    if (srel[t->innerd] != t->stride) {
      INT_COND_FATAL(srel[t->innerd] > t->stride, NULL, "invalid tab step");
      TAB_DIR(t, t->rows-1, 0)[t->innerd] *= srel[t->innerd] / t->stride;
    }
  }
}

static tab* fill_tabnodes(tab* tn, expr_t* aexpr, expr_t* sexpr, 
			   int sid, expr_t* (*wfunc)(expr_t*), 
			   int (*vfunc)(expr_t*))
{
  if (T_IS_BINARY(T_TYPE(aexpr)) && T_TYPE(aexpr) != BIAT) {
    tn = fill_tabnodes(tn, T_OPLS(aexpr), sexpr, sid, wfunc, vfunc);
    tn = fill_tabnodes(tn, T_NEXT(T_OPLS(aexpr)), sexpr, sid, wfunc, vfunc);
  }
  else if ((T_TYPE(aexpr) == BIAT && T_SUBTYPE(aexpr) != AT_RANDACC) ||
	   expr_at_ensemble_root(aexpr)) {
    if (vfunc(aexpr) && 
	datatype_is_dense_reg(T_TYPEINFO(T_TYPEINFO_REG(aexpr)))) {
      tn = resize_tab(tn, TAB_ROWS(tn)+1, TAB_COLS(tn)); 
      add_tabnode(tn, aexpr, sexpr, sid, wfunc);
    }
  }
  return tn;
}

/*
 * given a tab of width 1, returns a tab using calculated col and row
 */
static tab* converttn_tab(tab* t)
{
  int i, j;
  int cols, rows;
  int tmp;

  if (t == NULL) {
    INT_FATAL(NULL, "null tab passed to converttn_tab");
  }

  if (TAB_COLS(t) != 1) {
    INT_FATAL(NULL, "tab of width != 1 passed to converttn_tab");
  }

  for (j = 0; j < t->rows; j++) {
    tmp = TAB_DIR(t, j, 0)[0];
    TAB_DIR(t, j, 0)[0] = TAB_DIR(t, j, 0)[t->innerd];
    TAB_DIR(t, j, 0)[t->innerd] = tmp;
  }

  for (i = 0; i < t->rank; i++) {
    TAB_MINDIR(t)[i] = TAB_DIR(t, 0, 0)[i];
    TAB_MAXDIR(t)[i] = TAB_DIR(t, 0, 0)[i];
    for (j = 1; j < t->rows; j++) {
      if (TAB_DIR(t, j, 0)[i] < TAB_MINDIR(t)[i]) {
	TAB_MINDIR(t)[i] = TAB_DIR(t, j, 0)[i];
      }
      if (TAB_DIR(t, j, 0)[i] > TAB_MAXDIR(t)[i]) {
	TAB_MAXDIR(t)[i] = TAB_DIR(t, j, 0)[i];
      }
    }
  }
  if ((TAB_MAXDIR(t)[0]-TAB_MINDIR(t)[0]+1) % t->stride != 0) {
    TAB_MAXDIR(t)[0] +=
      (t->stride-((TAB_MAXDIR(t)[0]-TAB_MINDIR(t)[0]+1) % t->stride));
  }
  for (i = 1; i < t->rank; i++) {
    TAB_POSDIR(t)[i] = TAB_MAXDIR(t)[i-1]-TAB_MINDIR(t)[i-1]+1;
    if (i > 1) {
      t->posdir[i] = t->posdir[i]*t->posdir[i-1];
    }
    else {
      t->posdir[i] = 1;
    }
  }
  cols = t->maxdir[0]-t->mindir[0]+1;
  rows = 1;
  for (i = 1; i < t->rank; i++) {
    rows *= t->maxdir[i]-t->mindir[i]+1;
  }
  cols *= TAB_NDST(t);
  rows *= TAB_NSRC(t);
  return alloc_tab(rows, cols);
}

static int pos_tab(tab* t, int r, int c, int qcol)
{
  int col, row;
  int i;
  int cols, rows;

  INT_COND_FATAL(t != NULL, NULL, "null tab in pos tab");
  col = TAB_DIR(t, r, c)[0]-TAB_MINDIR(t)[0];
  for (row = 0, i = 1; i < t->rank; i++) {
    row +=(TAB_DIR(t, r, c)[i]-
	    TAB_MINDIR(t)[i]) * TAB_POSDIR(t)[i];
  }

  cols = t->maxdir[0]-t->mindir[0]+1;
  rows = 1;
  for (i = 1; i < t->rank; i++) {
    rows *= t->maxdir[i]-t->mindir[i]+1;
  }

  col += cols * (TAB_SID(t, r, c)-1);
  row += rows * (TAB_AID(t, r, c)-1);
  return qcol ? col : row;
}

/*
 * returns the stride of an expression in dimension d
 */
static int getstride_expr(expr_t* expr, int d)
{
  static int drel[MAXRANK]; /* relative ditabtion */
  static int srel[MAXRANK]; /* relative step      */

  INT_COND_FATAL(expr != NULL, NULL, "attempt to find step of null expr");
  if (T_IS_BINARY(T_TYPE(expr)) && T_TYPE(expr) != BIAT) {
    return lcm(getstride_expr(T_OPLS(expr), d), 
		getstride_expr(T_NEXT(T_OPLS(expr)), d));
  }
  else if (T_TYPE(expr) == BIAT) {
    if (reg_dir_info(expr, srel, drel) == -1) {
      return 0;
    }
    else {
      return srel[d];
    }
  }
  else return 1;
}

/*
 * returns the stride of the mloop in dimension d
 */
static int getstride_tab(mloop_t* mloop, int d)
{
  statement_t* stmt;
  int step;

  step = 1;
  for (stmt = T_MLOOP_BODY(mloop); stmt != NULL; stmt = T_NEXT(stmt)) {
    if (T_TYPE(stmt) == S_EXPR && T_IS_ASSIGNOP(T_TYPE(T_EXPR(stmt)))) {
      step = lcm(step, getstride_expr(T_NEXT(T_OPLS(T_EXPR(stmt))), d));
    }
  }
  return step;
}

static tab* prune_tab(tab* t) {
  int* krow; /* flag to keep rows of the tablet */
  int* kcol; /* flag to keep cols of the tablet */
  int r, c, k, i;
  int nr, nc;
  int rows, cols;
  tab* nt;
  int gcol;
  int keep;
  int doto;

  if (t == NULL) {
    INT_FATAL(NULL, "attempt to prune null tab");
  }

  krow = (int*)malloc(sizeof(int)*t->rows);
  kcol = (int*)malloc(sizeof(int)*t->cols);

  rows = 0;
  for (r = 0; r < TAB_ROWS(t); r++) {
    krow[r] = 0;
    for (c = 0; c < TAB_COLS(t); c++) {
      if (TAB_AID(t, r, c)) {
	krow[r]++;
	if (krow[r] > 1) {
	  rows++;
	  c = TAB_COLS(t);
	}
      }
    }
    if (krow[r] <= 1) {
      krow[r] = 0;
    }
  }	

  if (TAB_COLS(t) % TAB_NDST(t) != 0) {
    INT_FATAL(NULL, "cols % ndst != 0 !!!");
  }

  gcol = TAB_COLS(t) / TAB_NDST(t);

  cols = TAB_COLS(t);
  for (c = 0; c < TAB_COLS(t); c++) {
    kcol[c] = 1;
  }

  doto = TAB_NDST(t);
  for (k = 0; k < doto; k++) {
    keep = 0;
    for (i = 0; i < gcol; i++) {
      c = k * gcol+i;
      for (r = 0; r < TAB_ROWS(t); r++) {
	if (krow[r] && TAB_WID(t, r, c)) {
	  keep = 1;
	  r = TAB_ROWS(t);
	  i = gcol;
	}
      }
    }
    if (!keep) {
      for (i = 0; i < gcol; i++) {
	c = k * gcol+i;
	kcol[c] = 0;
	cols--;
      }
      TAB_NDST(t)--;
    }
  }

  if (rows == 0 || cols == 0) { /* pruned to oblivion */
    return NULL;
  }

  nt = alloc_tab(rows, cols);
  TAB_NZERO(nt) = 0;
  for (nr = 0, r = 0; r < TAB_ROWS(t); r++) {
    if (krow[r]) {
      for (nc = 0, c = 0; c < TAB_COLS(t); c++) {
	if (kcol[c]) {
	  copy_tabnode(nt, nr, nc, t, r, c);
	  if (TAB_WID(nt, nr, nc)) {
	    TAB_NZERO(nt)++;
	  }
	  nc++;
	}	  
      }
      nr++;
    }
  }
  if (nr != rows || nc != cols) {
    INT_FATAL(NULL, "major error in prune_tab");
  }

  TAB_RANK(nt) = TAB_RANK(t);
  TAB_STRIDE(nt) = TAB_STRIDE(t);
  TAB_INNERD(nt) = TAB_INNERD(t);
  TAB_DIRUP(nt) = TAB_DIRUP(t);
  TAB_NSRC(nt) = TAB_NSRC(t);
  TAB_NDST(nt) = TAB_NDST(t);

  free(krow);
  free(kcol);
  fix_tab(nt);
  return nt;
}

tab* fill_tab(mloop_t* mloop, expr_t* (*wfunc)(expr_t*), 
	      int (*vfunc)(expr_t*))
{
  tab* t;
  tab* tn;
  int i;
  statement_t* stmt;
  expr_t* sexpr;
  int sid;

  tn = alloc_tab(0, 1);
  tn->rank = T_MLOOP_RANK(mloop);
  tn->innerd = T_MLOOP_ORDER(mloop, tn->rank);
  tn->dirup = T_MLOOP_DIRECTION(mloop, tn->innerd) > 0;
  tn->stride = getstride_tab(mloop, tn->innerd);
  tn->nsrc = 0;
  tn->ndst = 0;
  if (tn->stride > 0) {
    tn->rows = 0;
    sid = 1;
    for (stmt = T_MLOOP_BODY(mloop); stmt != NULL; stmt = T_NEXT(stmt)) {
      if (T_TYPE(stmt) == S_EXPR && T_IS_ASSIGNOP(T_TYPE(T_EXPR(stmt)))) {
	sexpr = T_EXPR(stmt);
	tn =
	  fill_tabnodes(tn, T_NEXT(T_OPLS(sexpr)), sexpr, sid, wfunc, vfunc);
	sid++;
      }
    }
    if (tn->rows > 0) {
      t = converttn_tab(tn);
      clear_tab(t);
      t->rank = tn->rank;
      t->dirup = tn->dirup;
      t->innerd = tn->innerd;
      t->stride = tn->stride;
      TAB_NDST(t) = tn->ndst;
      TAB_NSRC(t) = tn->nsrc;
      for (t->nzero = 0, i = 0; i < tn->rows; i++ ) {
	copy_tabnode(t, pos_tab(tn, i, 0, 0), 
		      pos_tab(tn, i, 0, 1), tn, i, 0);
	TAB_NZERO(t)++;
      }
      return prune_tab(t);
    }
    else return NULL;
  }
  else return NULL;
}

/*************************************************************** SUBTABLET ***/

void print_subtab(tab* t, subtab* s)
{
  int r, c;

  INT_COND_FATAL(s != NULL, NULL, "attempt to print null subtab");
  printf("SUBTAB:(%d rows, %d cols) ", SUBTAB_ROWS(s), SUBTAB_COLS(s));
  printf("w/ w = %d & h = %d  ", SUBTAB_WIDE(s), SUBTAB_HIGH(s));
  printf("bnd = %d\n", bound_subtab(t, s));
  for (r = 0; r < SUBTAB_ROWS(s); r++) {
    /*
    if (has_subtab(s, r)) {
      printf("lo = %d  ", lo_subtab(s, t->stride, TAB_NDST(t), r));
    }
    else {
      printf("         ");
    }	
    */
    for (c = 0; c < SUBTAB_COLS(s); c++) {
      printf("%1d {%2d}  ", get_subtab(s, r, c), nxt_subtab(s, r, c));
    }
    printf("\n");
  }
}

subtab* alloc_subtab(tab* t)
{
  subtab* s;

  if (t == NULL) {
    INT_FATAL(NULL, "alloc_subtab: tab arg is null");
  }
  s = (subtab*)malloc(sizeof(subtab));
  if (s == NULL) {
    INT_FATAL(NULL, "alloc_subtab: subtab not allocated");
  }
  SUBTAB_ELTS(s) = 
    (subtabnode*)malloc(TAB_ROWS(t)*TAB_COLS(t)*sizeof(subtabnode));
  if (SUBTAB_ELTS(s) == NULL) {
    INT_FATAL(NULL, "alloc_subtab: subtab elts not allocated");
  }
  SUBTAB_COLS(s) = TAB_COLS(t);
  SUBTAB_ROWS(s) = TAB_ROWS(t);
  return s;
}

void free_subtab(subtab* s)
{
  if (s == NULL) {
    INT_FATAL(NULL, "free_subtab: subtab arg is null");
  }
  free(SUBTAB_ELTS(s));
  free(s);
}

int has_subtab(subtab* s, int r)
{
  if (s == NULL) {
    INT_FATAL(NULL, "has_subtab: subtab arg is null");
  }
  if (r < 0 || r >= SUBTAB_ROWS(s)) {
    INT_FATAL(NULL, "has_subtab: row arg is out of bounds");
  }
  return get_subtab(s, r, 0) || nxt_subtab(s, r, 0);
}

int get_subtab(subtab* s, int r, int c)
{
  if (s == NULL) {
    INT_FATAL(NULL, "get_subtab: subtab argument is null");
  }
  if (r < 0 || r >= SUBTAB_ROWS(s)) {
    INT_FATAL(NULL, "get_subtab: row arg is out of bounds");
  }
  if (c < 0 || c >= SUBTAB_COLS(s)) {
    INT_FATAL(NULL, "get_subtab: column arg is out of bounds");
  }
  return SUBTAB_ELTS(s)[r*SUBTAB_COLS(s)+c].elt;
}

int nxt_subtab(subtab* s, int r, int c)
{
  if (s == NULL) {
    INT_FATAL(NULL, "nxt_subtab: subtab argument is null");
  }
  if (r < 0 || r >= SUBTAB_ROWS(s)) {
    INT_FATAL(NULL, "nxt_subtab: row arg is out of bounds");
  }
  if (c < 0 || c >= SUBTAB_COLS(s)) {
    INT_FATAL(NULL, "nxt_subtab: column arg is out of bounds");
  }
  return SUBTAB_ELTS(s)[r * SUBTAB_COLS(s)+c].nxt;
}

void set_subtab(subtab* s, int r, int c)
{
  int i;

  if (s == NULL) {
    INT_FATAL(NULL, "set_subtab: subtab argument is null");
  }
  if (r < 0 || r >= SUBTAB_ROWS(s)) {
    INT_FATAL(NULL, "set_subtab: row arg is out of bounds");
  }
  if (c < 0 || c >= SUBTAB_COLS(s)) {
    INT_FATAL(NULL, "set_subtab: column arg is out of bounds");
  }
  if (r > 0) {
    for (i = r-1; i >= 0; i--) {
      SUBTAB_ELTS(s)[i*SUBTAB_COLS(s)+SUBTAB_COLS(s)-1].nxt = r;
      if (has_subtab(s, i)) {
	break;
      }
    }
  }
  if (c > 0) {
    for (i = c-1; i >= 0; i--) {
      SUBTAB_ELTS(s)[r * SUBTAB_COLS(s)+i].nxt = c;
      if (get_subtab(s, r, i)) {
	break;
      }
    }
  }
  SUBTAB_ELTS(s)[r * SUBTAB_COLS(s)+c].elt = 1;
}

void clear_subtab(subtab* s)
{
  int r, c;

  if (s == NULL) {
    INT_FATAL(NULL, "clear_subtab: subtab argument is null");
  }
  for (r = 0; r < SUBTAB_ROWS(s); r++) {
    for (c = 0; c < SUBTAB_COLS(s); c++) {
      SUBTAB_ELTS(s)[r * SUBTAB_COLS(s)+c].elt = 0;
      SUBTAB_ELTS(s)[r * SUBTAB_COLS(s)+c].nxt = 0;
    }
  }
}

int quadin_subtab(subtab* s, int r1, int c1, int r2, int c2, int d)
{
  INT_COND_FATAL(s != NULL, NULL, "attempt to access null subtab");
  INT_COND_FATAL(r1 >= 0 && r1 < SUBTAB_ROWS(s), NULL, "unbounded subtab r1");
  INT_COND_FATAL(r2 >= 0 && r2 < SUBTAB_ROWS(s), NULL, "unbounded subtab r2");
  INT_COND_FATAL(c1 >= 0 && c1 < SUBTAB_COLS(s), NULL, "unbounded subtab c1");
  INT_COND_FATAL(c2 >= 0 && c2 < SUBTAB_COLS(s), NULL, "unbounded subtab c2");
  INT_COND_FATAL(c1+d >= 0 && c1+d < SUBTAB_COLS(s), NULL, "unbounded subtab c1+d");
  INT_COND_FATAL(c2+d >= 0 && c2+d < SUBTAB_COLS(s), NULL, "unbounded subtab c2+d");
  return (get_subtab(s, r1, c1) && get_subtab(s, r1, c1+d) &&
	  get_subtab(s, r2, c2) && get_subtab(s, r2, c2+d));
}

subtab* quad_subtab(tab* t, int r1, int c1, int r2, int c2, int d)
{
  subtab *s;

  INT_COND_FATAL(t != NULL, NULL, "attempt to access null tab");
  INT_COND_FATAL(r1 >= 0 && r1 < t->rows, NULL, "unbounded subtab r1");
  INT_COND_FATAL(r2 >= 0 && r2 < t->rows, NULL, "unbounded subtab r2");
  INT_COND_FATAL(c1 >= 0 && c1 < t->cols, NULL, "unbounded subtab c1");
  INT_COND_FATAL(c2 >= 0 && c2 < t->cols, NULL, "unbounded subtab c2");
  INT_COND_FATAL(c1+d >= 0 && c1+d < t->cols, NULL, "unbounded subtab c1+d");
  INT_COND_FATAL(c2+d >= 0 && c2+d < t->cols, NULL, "unbounded subtab c2+d");
  s = alloc_subtab(t);
  clear_subtab(s);
  s->width = 2;
  s->height = 2;
  set_subtab(s, r1, c1);
  set_subtab(s, r1, c1+d);
  set_subtab(s, r2, c2);
  set_subtab(s, r2, c2+d);
  return s;
}

int bound_subtab(tab* t, subtab* s)
{
  int tc, tr;
  int lo, hi;

  if (t == NULL) {
    INT_FATAL(NULL, "bound_subtab: tab arg is null");
  }
  if (s == NULL) {
    INT_FATAL(NULL, "bound_subtab: subtab arg is null");
  }
  lo = SUBTAB_COLS(s);
  hi = 0;
  tr = (has_subtab(s, 0)) ? 0 : nxt_subtab(s, 0, SUBTAB_COLS(s)-1);
  tc = (get_subtab(s, tr, 0)) ? 0 : nxt_subtab(s, tr, 0);
  do {
    if (tc % (SUBTAB_COLS(s) / TAB_NDST(t)) < lo) {
      lo = tc % (SUBTAB_COLS(s) / TAB_NDST(t));
    }
    if (tc % (SUBTAB_COLS(s) / TAB_NDST(t)) > hi) {
      hi = tc % (SUBTAB_COLS(s) / TAB_NDST(t));
    }
    if (tc == SUBTAB_COLS(s)-1) {
      break;
    }
    tc = nxt_subtab(s, tr, tc);
  } while (tc != 0);
  if ((hi-lo) % t->stride != 0) {
    INT_FATAL(NULL, "bound_subtab: stride error");
  }
  return (hi-lo)/t->stride+1;
}

int lo_subtab(tab* t, subtab* s, int row)
{
  int c, tc, lo;

  if (t == NULL) {
    INT_FATAL(NULL, "bound_subtab: tab arg is null");
  }
  if (s == NULL) {
    INT_FATAL(NULL, "bound_subtab: subtab arg is null");
  }
  if (!has_subtab(s, row)) {
    INT_FATAL(NULL, "lo_subtab called with empty row");
  }
  lo = SUBTAB_COLS(s);
  tc = (get_subtab(s, row, 0)) ? 0 : nxt_subtab(s, row, 0);
  for (c = 0; c < SUBTAB_WIDE(s); c++) {
    if (tc % (SUBTAB_COLS(s) / t->ndst) < lo) {
      lo = tc % (SUBTAB_COLS(s) / t->ndst);
    }
    tc = nxt_subtab(s, row, tc);
  }
  if (lo == SUBTAB_COLS(s)) {
    INT_FATAL(NULL, "lo not found in lo_subtab");
  }
  return lo;
}

/** returns the tab row of row for subtab s **/
int tabrow_subtab(tab* t, subtab* s, int row)
{
  int k, r;

  k = 0;
  r = 0;
  if (has_subtab(s, r)) {
    if (k == row) {
      return r;
    }
    k++;
  }
  r = nxt_subtab(s, r, SUBTAB_COLS(s)-1);
  while (r != 0) {
    if (k++ == row) {
      return r;
    }
    r = nxt_subtab(s, r, SUBTAB_COLS(s)-1);
  }
  INT_FATAL(NULL, "row of subtab out of tab in tabrow_subtab");
  return 0;
}

/** returns the tab column of col, row for subtab s **/
int tabcol_subtab(tab* t, subtab* s, int row, int col)
{
  int k, r, c;

  r = tabrow_subtab(t, s, row);
  k = 0;
  c = 0;
  if (get_subtab(s, r, c)) {
    if (k == col) {
      return c;
    }
    k++;
  }
  c = nxt_subtab(s, r, c);
  while (c != 0) {
    if (k++ == col) {
      return c;
    }
    if (c == TAB_COLS(t)) {
      break;
    }
    c = nxt_subtab(s, r, c);
  }
  INT_FATAL(NULL, "col of subtab out of tab in tabcol_subtab");
  return 0;
}

/** inverse of tabcol_subtab **/
int tabcolinv_subtab(tab* t, subtab* s, int row, int col)
{
  int k, c;

  k = 0;
  c = 0;
  if (get_subtab(s, row, c)) {
    if (c == col) {
      return k;
    }
    k++;
  }
  c = nxt_subtab(s, row, c);
  while (c != 0) {
    if (c == col) {
      return k;
    }
    k++;
    if (c == TAB_COLS(t)) {
      break;
    }
    c = nxt_subtab(s, row, c);
  }
  INT_FATAL(NULL, "col of tab out of subtab in tabcol_subtabinv");
  return 0;
}

int coltype_subtab(tab *t, subtab* s)
{
  int r, c, tr, tc, tr2, tc2;

  tr = (has_subtab(s, 0)) ? 0 : nxt_subtab(s, 0, SUBTAB_COLS(s)-1);
  tr2 = nxt_subtab(s, tr, SUBTAB_COLS(s)-1);
  for (r = 0; r < SUBTAB_HIGH(s)-1; r++) {
    tc = (get_subtab(s, tr, 0)) ? 0 : nxt_subtab(s, tr, 0);
    tc2 = (get_subtab(s, tr2, 0)) ? 0 : nxt_subtab(s, tr2, 0);
    for (c = 0; c < SUBTAB_WIDE(s); c++) {
      if (TAB_WID(t, tr, tc) != TAB_WID(t, tr2, tc2)) {
	return 0;
      }
      tc = nxt_subtab(s, tr, tc);
      tc2 = nxt_subtab(s, tr2, tc2);
    }
    tr = tr2;
    tr2 = nxt_subtab(s, tr2, SUBTAB_COLS(s)-1);
  }
  return 1;
}

int rowtype_subtab(tab *t, subtab* s)
{
  int r, c, tr, tc, tc2;

  if (coltype_subtab(t, s)) {
    return 0;
  }
  tr = (has_subtab(s, 0)) ? 0 : nxt_subtab(s, 0, SUBTAB_COLS(s)-1);
  for (r = 0; r < SUBTAB_HIGH(s); r++) {
    tc = (get_subtab(s, tr, 0)) ? 0 : nxt_subtab(s, tr, 0);
    tc2 = nxt_subtab(s, tr, tc);
    for (c = 0; c < SUBTAB_WIDE(s)-1; c++) {
      if (TAB_WID(t, tr, tc) != TAB_WID(t, tr, tc2)) {
	return 0;
      }
      tc = tc2;
      tc2 = nxt_subtab(s, tr, tc2);
    }
    tr = nxt_subtab(s, tr, SUBTAB_COLS(s)-1);
  }
  return 1;
}

/*********************************************************** SUBTABLETLIST ***/

void print_subtablist(tab *t, subtablist* sl)
{
  int c = 0;

  while (sl != NULL) {
    if (sl->s) {
      printf("---------\n");
      print_subtab(t, sl->s);
      c++;
    }
    sl = sl->nxt;
  }
  printf("--------- # of subtabs in list = %d\n", c);
}

subtablist* push_subtablist(subtablist* sl, subtab* s)
{
  subtablist* as;

  INT_COND_FATAL(s != NULL, NULL, "attempt to push null subtab");
  as = (subtablist*)malloc(sizeof(subtablist));
  INT_COND_FATAL(as != NULL, NULL, "unable to alloc subtablist");
  as->s = s;
  as->nxt = sl;
  return as;
}

void free_subtablist(subtablist* sl)
{
  INT_COND_FATAL(sl != NULL, NULL, "attempt to free null subtablist");
  if (sl->s) {
    free_subtab(sl->s);
  }
  if (sl->nxt) {
    free_subtablist(sl->nxt);
  }
  free(sl);
}

int quadin_subtablist(subtablist* sl, int r1, int c1, int r2, int c2, int d)
{
  while (sl != NULL) {
    if (quadin_subtab(sl->s, r1, c1, r2, c2, d)) {
      return 1;
    }
    sl = SUBTABLIST_NXT(sl);
  }
  return 0;
}
