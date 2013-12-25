/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/*
 * KNOWN DEFICIENCY for strided arrays, the scalarization will only
 * fire on some of the tablet, the first column, first+k column,
 * first+2k, ... .  This is easy to fix by modifying short_subrec and
 * add_shorts to deal with a specific starting column 1..k.
 *
 */

/*
 * FILE: stencil.c - stencil codes optimization
 * DATE: 14 March 2000
 * CREATOR: deitz
 *
 * SUMMARY: see quals paper.
 */

/*
 * use call_stencil to run the stencil optimization; constrainmloops,
 * depend_array, depgraph and typeinfo are necessary precursors and
 * must be up-to-date.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/mloop.h"
#include "../include/stmtutil.h"
#include "../include/symmac.h"
#include "../include/traverse.h"
#include "../include/treemac.h"
#include "../include/typeinfo.h"
#include "../include/buildsym.h"
#include "../include/buildstmt.h"
#include "../include/Repgen.h"
#include "../include/rmstack.h"
#include "../include/stencil.h"
#include "../include/ntablet.h"
#include "../include/nudge.h"
#include "../include/zutil.h"
#include "../include/main.h"

int call_stencil (module_t*, char*);

#define STENCIL_FUSE TRUE   /* optimize between statements in same mloop */
#define STENCIL_VERT TRUE   /* find subtabs with equal weights in cols   */
#define STENCIL_HORI TRUE   /* find subtabs with equal weights in rows   */
#define STENCIL_SEPA TRUE   /* find subtabs that are separable           */
#define STENCIL_REINS TRUE  /* reinsert subtabs into tab to find more    */
#define STENCIL_FACTOR TRUE /* factor the resultant stencil expression   */
#define STENCIL_RECOG TRUE  /* manipulate stmt to make it recognizable   */
#define STENCIL_SHORT TRUE  /* find short subtabs with height one         */

/*
 * neighborhood tablet weight function for tall rectangles
 */
static expr_t* weight_tall(expr_t* expr)
{
  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to weight_tall");
  }

  if (T_PARENT(expr) == NULL) {
    INT_FATAL(NULL, "parentless expr passed to weight_tall");
  }

  if (T_IS_ASSIGNOP(T_TYPE(T_PARENT(expr)))) {
    return new_const_int(1);
  }
  else if (T_TYPE(T_PARENT(expr)) == BITIMES) {
    return expr_sibling(expr);
  }
  else {
    return weight_tall(T_PARENT(expr));
  }
}

/*
 * neighborhood tablet weight function for short rectangles
 */
static expr_t* weight_short(expr_t* expr)
{
  if (expr == NULL) {
    INT_FATAL(NULL, "null expr passed to weight_short");
  }
  return NULL;
}

/*
 * neighborhood tablet valid function for tall rectangles
 */
static int valid_tall (expr_t* expr)
{
  static int numtimes;
  
  switch (T_TYPE (expr)) {
  case BIAT:
  case BIDOT:
  case VARIABLE:
  case ARRAY_REF:
    numtimes = 0;
	if (mloopassigned_expr(expr)) {
	  return 0;
	}
    return valid_tall (T_PARENT (expr));
  case BIASSIGNMENT:
  case BIOP_GETS:
    return (T_SUBTYPE(expr) == PLUS);
  case BIPLUS:
    return valid_tall (T_PARENT (expr));
  case BITIMES:
    if ((++numtimes > 1) ||
	(mloopinvariant_expr(T_OPLS(expr)) != T_MLOOP_RANK(T_MLOOP(T_IN_MLOOP(T_STMT(expr)))) &&
	 mloopinvariant_expr(T_NEXT(T_OPLS(expr))) != T_MLOOP_RANK(T_MLOOP(T_IN_MLOOP(T_STMT(expr)))))) {
      return 0;
    }
    else return valid_tall (T_PARENT (expr));
  default:
    return 0;
  }
}

/*
 * neighborhood tablet valid function for short rectangles
 */
static int valid_short (expr_t* expr) {
  return !mloopassigned_expr(expr);
}

/*
 * returns one if the weights in a, b, c, d are such that a/c = b/d
 * and all are ints null expr is taken as one, assumed none of the
 * weights are zero
 */
static int wratio (expr_t* a, expr_t* b, expr_t* c, expr_t* d)
{
  long aval, bval, cval, dval;

  if (a == NULL) {
    aval = 1;
  }
  else if (expr_computable_const (a) && D_CLASS(T_TYPEINFO(a)) == DT_INTEGER) {
    aval = expr_intval (a);
  }
  else {
    return 0;
  }
  if (b == NULL) {
    bval = 1;
  }
  else if (expr_computable_const (b) && D_CLASS(T_TYPEINFO(b)) == DT_INTEGER) {
    bval = expr_intval (b);
  }
  else {
    return 0;
  }
  if (c == NULL) {
    cval = 1;
  }
  else if (expr_computable_const (c) && D_CLASS(T_TYPEINFO(c)) == DT_INTEGER) {
    cval = expr_intval (c);
  }
  else {
    return 0;
  }
  if (d == NULL) {
    dval = 1;
  }
  else if (expr_computable_const (d) && D_CLASS(T_TYPEINFO(d)) == DT_INTEGER) {
    dval = expr_intval (d);
  }
  else {
    return 0;
  }
  if ((double)aval / (double)cval == (double)bval / (double)dval) {
    return 1;
  }
  else {
    return 0;
  }
}

/*
 * takes a subtab of height at most 2 and makes it as
 * wide as it can be.  the elements at (r1,c1) and (r2,c2) must be in
 * the same column of the subtab (c1 not necessarily equal to c2) and
 * on different rows.
 */
static void widen2_subtab (tab* r, subtab* s, int r1, int c1, int r2, int c2,
			   int eqc, int eqr)
{
  int i, j;

  INT_COND_FATAL (r != NULL, NULL, "attempt to widen subtab in null tab");
  INT_COND_FATAL (s != NULL, NULL, "attempt to widen null subtab");
  INT_COND_FATAL (s->rows == r->rows, NULL, "subtab.rows != tab.rows");
  INT_COND_FATAL (s->cols == r->cols, NULL, "subtab.cols != tab.cols");
  INT_COND_FATAL (r1 >= 0 && r2 >= 0, NULL, "negative row in widen");
  INT_COND_FATAL (r1 < r->rows && r2 < r->rows, NULL, "row too high in widen");
  INT_COND_FATAL (c1 >= 0 && c2 >= 0, NULL, "negative col in widen");
  INT_COND_FATAL (c1 < r->cols && c2 < r->cols, NULL, "col too high in widen");
  s->width = 0;
  for (i = c1 - min (c1, c2), j = c2 - min (c1, c2);
       i < r->cols && j < r->cols; i++, j++) {
    if ((!eqc || TAB_WID (r, r1, i) == TAB_WID (r, r2, j)) &&
	(!eqr || (TAB_WID (r, r1, i) == TAB_WID (r, r1, c1) &&
		  TAB_WID (r, r2, j) == TAB_WID (r, r2, c2))) &&
	(eqc || eqr || wratio (TAB_WEXPR (r, r1, i), TAB_WEXPR (r, r1, c1),
			       TAB_WEXPR (r, r2, j), TAB_WEXPR (r, r2, c2))) &&
	TAB_SID (r, r1, i) == TAB_SID (r, r2, j) &&
	i % TAB_STRIDE (r) == c1 % TAB_STRIDE (r) &&
	j % TAB_STRIDE (r) == c2 % TAB_STRIDE (r) &&
	(STENCIL_FUSE ||
	 (TAB_SID (r, r1, c1) == TAB_SID (r, r1, i) &&
	  TAB_SID (r, r2, c2) == TAB_SID (r, r2, j))) &&
	TAB_WID (r, r1, i) > 0) {
      set_subtab (s, r1, i);
      set_subtab (s, r2, j);
      s->width++;
    }
  }
}

/*
 * takes a subtab of height at most 2 and makes it as
 * high as it can be.  the rows of r1 and r2 form the tab of height 2.
 */
static void heighten2_subtab (tab* r, subtab* s, int r1, int r2, int eqc, 
			      int eqr)
{
  int i, j, k;
  int cl, cr; /* left and right most elements in row r1 */
  int add; /* boolean for whether the row can be added */
  int hwgt; /* horizontal weight -- leftmost weight */

  INT_COND_FATAL (r != NULL, NULL, "attempt to heighten subtab in null tab");
  INT_COND_FATAL (s != NULL, NULL, "attempt to heighten null subtab");
  INT_COND_FATAL (s->rows == r->rows, NULL, "subtab.rows != tab.rows");
  INT_COND_FATAL (s->cols == r->cols, NULL, "subtab.cols != tab.cols");
  INT_COND_FATAL (r1 >= 0 && r2 >= 0, NULL, "negative row in highen");
  INT_COND_FATAL (r1 < s->rows && r2 < s->rows, NULL, "row too big in highen");

  cl = (get_subtab(s, r1, 0)) ? 0 : nxt_subtab(s, r1, 0);
  k = cl;
  while (nxt_subtab(s, r1, k) != 0 && k != SUBTAB_COLS(s)-1) {
    k = nxt_subtab(s, r1, k);
  }
  cr = k;

  k = (has_tab(r, 0)) ? 0 : TAB_NXT(r, 0, TAB_COLS(r)-1);
  while (has_tab(r, k)) {
    if (k != r1 && k != r2) {
      add = 0;
      i = (TAB_WID(r, k, 0)) ? 0 : TAB_NXT(r, k, 0);
      while (TAB_WID(r, k, i) && i < r->cols - (cr - cl) && !add) {

	hwgt = 0;
	add = 1;
	j = cl;
	do {
	  if (hwgt == 0) {
	    hwgt = TAB_WID(r, k, j);
	  }
	  if (TAB_WID(r, k, i + j - cl) == 0 ||
	      (eqc && TAB_WID (r, r1, j) != TAB_WID (r, k, i + j - cl)) ||
	      (eqr && TAB_WID (r, k, i + j - cl) != hwgt) ||
	      (!eqc && !eqr &&
	       !wratio (TAB_WEXPR(r, r1, cl), TAB_WEXPR(r, r1, j),
			TAB_WEXPR(r, k, i), TAB_WEXPR(r, k, i+j-cl))) ||
	      TAB_SID (r, r1, j) != TAB_SID (r, k, i+j-cl)) {
	    add = 0;
	  }
	  if (j == SUBTAB_COLS(s)-1) {
	    break;
	  }
	  j = nxt_subtab(s, r1, j);
	} while (add && j != 0);

	if (!add) {
	  if (i == TAB_COLS(r)-1) {
	    break;
	  }

	  i = TAB_NXT(r, k, i);

	  if (i == 0) {
	    break;
	  }
	}
      }
      if (add) {
	s->height++;
        for (j = i; j <= i + cr - cl; j++) {
	  if (get_subtab (s, r1, cl + j - i)) {
	    set_subtab (s, k, j);
	  }
        }
      }
    }
    k = TAB_NXT(r, k, TAB_COLS(r)-1);
    if (k == 0) {
      break;
    }
  }
}

/*
 * returns a candidate list of maximal subtabs, appending it to sl, of
 * tab t where the columns in the subtab have equal weights if eqc is
 * TRUE and the rows in the subtab have equal weights if eqr is TRUE
 * and where the redundancy conditions are satisfied.
 */
static subtablist* get_maximals (subtablist* sl, tab* t, int eqc, int eqr)
{
  int i, j, k, m, n, p;
  subtab* s;

  TAB_TRAVERSE_BEGIN(t, i, k) {
    if (TAB_NXT(t, i, TAB_COLS(t)-1)) {
      TAB_TRAVERSE_BEGIN_AT(t, j, m, i + 1) {
	if (TAB_SID (t, i, k) == TAB_SID (t, j, m)) {
	  n = k;
	  p = m;
	  while ((TAB_NXT(t, i, n) && n != TAB_COLS(t)-1) ||
		 (TAB_NXT(t, j, p) && p != TAB_COLS(t)-1)) {
	    if (n-k<p-m && TAB_NXT(t, i, n) && n != TAB_COLS(t)-1) {
	      n = TAB_NXT(t, i, n);
	    }
	    else if (TAB_NXT(t, j, p) && p != TAB_COLS(t)-1) {
	      p = TAB_NXT(t, j, p);
	    }
	    else {
	      break;
	    }
	    if (n - k == p - m) {
	      if ((TAB_SID (t, i, n) == TAB_SID (t, j, p)) &&
		  (STENCIL_FUSE ||
		   (TAB_SID (t, i, k) == TAB_SID (t, i, n) &&
		    TAB_SID (t, j, m) == TAB_SID (t, j, p))) &&
		  (!eqc || (TAB_WID (t, i, k) == TAB_WID (t, j, m) &&
			    TAB_WID (t, i, n) == TAB_WID (t, j, p))) &&
		  (!eqr || (TAB_WID (t, i, k) == TAB_WID (t, i, n) &&
			    TAB_WID (t, j, m) == TAB_WID (t, j, p))) &&
		  (k % TAB_STRIDE (t) == n % TAB_STRIDE (t)) &&
		  (eqc || eqr || wratio (TAB_WEXPR (t, i, k), TAB_WEXPR (t, i, n),
					 TAB_WEXPR (t, j, m), TAB_WEXPR (t, j, p)))) {
		if (!quadin_subtablist (sl, i, k, j, m, n - k)) {
		  s = quad_subtab (t, i, k, j, m, n-k);
		  widen2_subtab (t, s, i, k, j, m, eqc, eqr);
		  heighten2_subtab (t, s, i, j, eqc, eqr);
		  sl = push_subtablist (sl, s);
		}
	      }
	    }
	  }
	}
	TAB_TRAVERSE_TAIL(t, j, m);
      }
    }
    TAB_TRAVERSE_TAIL(t, i, k);
  }

  return sl;
}

static expr_t* outer_wgt (tab* t, subtab* s, int col)
{
  int i, r, c, w;

  if (rowtype_subtab(t, s)) {
    w = 1;
  }
  else {


    for (w = 0, i = 0; i < SUBTAB_HIGH(s); i++) {

      r = tabrow_subtab(t, s, i);
      c = tabcol_subtab(t, s, i, col);


      if (get_subtab(s, r, c)) {
	if (expr_computable_const(TAB_WEXPR(t, r, c)) &&
	    D_CLASS(T_TYPEINFO(TAB_WEXPR(t, r, c))) == DT_INTEGER) {
	  if (w == 0) {
	    w = expr_intval(TAB_WEXPR(t, r, c));
	  }
	  else {
	    w = gcd(abs(w), abs(expr_intval(TAB_WEXPR(t, r, c))));
	  }
	  w = expr_intval(TAB_WEXPR(t, r, c)) < 0 ? -w : w;
	}
	else {
	  return copy_expr(TAB_WEXPR(t, r, c));
	}
      }
    }
  }
  sprintf(buffer, "%d", w);
  return build_typed_0ary_op(CONSTANT, build_int(w));
}

static expr_t* inner_wgt (tab* t, subtab* s, int row)
{
  int r, c, w;
  expr_t* outer;

  if (coltype_subtab(t, s)) {
    sprintf(buffer, "%d", 1);
    return build_0ary_op(CONSTANT, build_int(1));
  }
  r = tabrow_subtab(t, s, row);
  outer = outer_wgt(t, s, 0);
  c = tabcol_subtab(t, s, row, 0);
  if (TAB_WEXPR(t, r, c) == NULL) {
    INT_FATAL(NULL, "DOH");
  }

  if (get_subtab(s, r, c)) {
    if (expr_computable_const(outer) &&
	D_CLASS(T_TYPEINFO(outer)) == DT_INTEGER &&
	expr_computable_const(TAB_WEXPR(t, r, c)) &&
	D_CLASS(T_TYPEINFO(TAB_WEXPR(t, r, c))) == DT_INTEGER) {
      w = expr_intval(TAB_WEXPR(t, r, c)) / expr_intval(outer);
      sprintf(buffer, "%d", w);
      return build_typed_0ary_op(CONSTANT, build_int(w));
    }
    else {
      return copy_expr(TAB_WEXPR(t, r, c));
    }
  }
  INT_FATAL(NULL, "row not in subtab in inner_wgt\n");
  return NULL;
}

/**
 ** returns the number of distinct expressions in
 ** an expression list using expr_equal as a guide
 ** ignores expressions equal to one or its negative
 **/
static int distinctexpr_exprls(expr_t* expr)
{
  expr_t* tmp;
  static expr_t* one = NULL;
  static expr_t* none = NULL;
  int val;

  if (one == NULL || none == NULL) {
    one = new_const_int(1);
    none = new_const_int(-1);
  }
  if (expr_equal(expr, one) || expr_equal(expr, none)) {
    if (T_NEXT(expr) == NULL) {
      return 0;
    }
    else {
      return distinctexpr_exprls(T_NEXT(expr));
    }
  }
  if (T_NEXT(expr) == NULL) {
    return 1;
  }
  for (tmp = T_NEXT(expr); tmp != NULL; tmp = T_NEXT(tmp)) {
    if (expr_equal(expr, tmp)) {
      return distinctexpr_exprls(T_NEXT(expr));
    }
  }
  val = 1 + distinctexpr_exprls(T_NEXT(expr));

  return val;

}

static int nummults_subtab(subtab* s, tab* t, int nmtab)
{
  int r, c;
  int cnt;
  expr_t* els;
  expr_t* tmp;

  els = inner_wgt(t, s, 0);
  tmp = els;
  for (r = 1; r < SUBTAB_HIGH(s); r++) {
    T_NEXT(tmp) = inner_wgt(t, s, r);
    tmp = T_NEXT(tmp);
  }
  cnt = distinctexpr_exprls(els);
  els = outer_wgt(t, s, 0);
  tmp = els;
  for (c = 1; c < SUBTAB_WIDE(s); c++) {
    T_NEXT(tmp) = outer_wgt(t, s, c);
    tmp = T_NEXT(tmp);
  }

  TAB_TRAVERSE_BEGIN(t, r, c) {
    if (!get_subtab(s, r, c)) {
      T_NEXT(tmp) = copy_expr(TAB_WEXPR(t, r, c));
      tmp = T_NEXT(tmp);
    }
    TAB_TRAVERSE_TAIL(t, r, c);
  }

  cnt += distinctexpr_exprls(els);

  return nmtab - cnt;
}

static int nummults_tab(tab* t) {
  int r, c;
  expr_t* els;
  expr_t* tmp;

  els = NULL;
  TAB_TRAVERSE_BEGIN(t, r, c) {
    if (els == NULL) {
      els = copy_expr(TAB_WEXPR(t, r, c));
      tmp = els;
    }
    else {
      T_NEXT(tmp) = copy_expr(TAB_WEXPR(t, r, c));
      tmp = T_NEXT(tmp);
    }
    TAB_TRAVERSE_TAIL(t, r, c);
  }

  return distinctexpr_exprls(els);
}

static double benefit (subtab* s, tab* t, int nmtab)
{
  return
    (((double)((s->width - 1) * (s->height - 1))) * stencil_adds) +
    (((double)((s->width - 1) * s->height)) * stencil_mems) +
    ((nmtab != -1 ? ((double)(nummults_subtab(s, t, nmtab))) : 0.0) * stencil_muls) +
    (((double)(bound_subtab(t, s))) * stencil_regs);
}

/*
 * returns the maximum maximal subtab in r.
 */
static subtab* get_maximum (tab* r)
{
  subtablist* mals; /* maximals subtablist */
  subtablist* trav; /* temp subtablist     */
  subtab* max;      /* maximum subtab      */
  subtab* tmp;      /* temp subtab         */
  double bmax, bcan; /* max benefit and candidate benefit */
  int nmtab;

  max = NULL;
  mals = NULL;
  if (STENCIL_VERT) mals = get_maximals (mals, r, TRUE, FALSE);
  if (STENCIL_HORI) mals = get_maximals (mals, r, FALSE, TRUE);
  if (STENCIL_SEPA) mals = get_maximals (mals, r, FALSE, FALSE);

  if (mals) {
    nmtab = nummults_tab(r);
    max = mals->s;
    bmax = benefit(max, r, nmtab);
    mals->s = NULL;
    trav = mals->nxt;
    while (trav) {
      bcan = benefit(trav->s, r, nmtab);
      if (bcan > bmax) {
        tmp = max;
        max = trav->s;
        trav->s = tmp;
	bmax = bcan;
      }
      trav = trav->nxt;
    }
    free_subtablist (mals);
    if (bmax <= stencil_bbar) {
      free_subtab (max);
      max = NULL;
    }
  }
  return max;
}

static tab* subin_tab (tab* t, subtab* s);

/*
 * returns a possibly 'suboptimal' list of subtabs of r.  the greedy
 * algorithm includes the maximum maximal subtab and then taburses on
 * the rest of the tab.
 * NOTE - derivation of word like taburses
 *           it comes from recurses where rec is replaced with tab!
 */
static subtablist* get_greedy (tab** r, tab* t)
{
  subtab* max;
  subtablist* soln;
  int rgs;
  double ben;
  int nmtab;

  max = get_maximum (t);
  if (max) {
    nmtab = nummults_tab(t);
    ben = benefit(max, t, nmtab);
    rgs = bound_subtab(t, max);
    t = copy_tab (t);
    if (STENCIL_REINS) {
      *r = subin_tab (*r, max);
      t = subin_tab (t, max);
    }
    subout_tab (t, max);

    if (REPORT (REPORT_STENCIL)) {
      printf("SUBTAB (width = %d, height = %d, adds = %d, muls = %d, "
             "mems = %d, regs = %d, benefit = %6.3f)\n", max->width,
             max->height, (max->width-1)*(max->height-1),
             (max->height == 1) ? 0 : nmtab, (max->width-1)*max->height,
	     rgs, ben);
    }


    soln = get_greedy(r, t);
    soln = push_subtablist (soln, max);

    free_tab (t);
    return soln;
  }
  return NULL;
}

/*
 * returns a subtab  of width at least two from a given row of r.
 */
static subtab * short_subtab  (tab * r, int row)
{
  int i;
  subtab * s;

  s = alloc_subtab  (r);
  clear_subtab  (s);
  s->width = 0;
  s->height = 0;
  for (i = 0; i < r->cols; i += r->stride) {
    if (TAB_WID(r, row, i)) {
      set_subtab  (s, row, i);
      s->width++;
      s->height = 1;
    }
  }
  if (s->width > 1) {
    return s;
  }
  else {
    free_subtab  (s);
    return NULL;
  }
}

/*
 * adds candidate subtab tangles of height one.
 */
static subtablist* add_shorts(tab * r, subtablist* mlist)
{
  int i;
  subtab * s;

  for (i = 0; i < r->rows; i++) {
    s = short_subtab  (r, i);
    if (s != NULL && benefit(s, r, -1) > stencil_bbar) {

      if (REPORT (REPORT_STENCIL)) {
	printf("SUBTAB (width = %d, height = %d, adds = %d, muls = %d, "
	       "mems = %d, regs = %d, benefit = %6.3f)\n", s->width,
	       s->height, (s->width-1)*(s->height-1),
	       0, (s->width-1)*s->height, bound_subtab(r, s),
	       benefit(s, r, -1));
      }


      mlist = push_subtablist (mlist, s);
    }
  }
  return mlist;
}

static subtablist* get_shorts(tab* t) {
  subtablist* soln;

  soln = NULL;
  soln = add_shorts(t, soln);
  return soln;
}

/*
 * increment the offset in dimension d by n for BIATs in expr using nudges
 */
static void inc_dirs_by (expr_t* expr, int d, int n)
{
  if (T_IS_BINARY (T_TYPE (expr)) && T_TYPE(expr) != BIAT) {
    inc_dirs_by (T_OPLS (expr), d, n);
    inc_dirs_by (T_NEXT (T_OPLS (expr)), d, n);
  }
  else if (T_TYPE (expr) == BIAT || T_TYPE (expr) == NUDGE) {
    T_SET_NUDGE (expr, d, abs(n));
  }
  else if (T_TYPE (expr) == VARIABLE && 
	   strncmp (S_IDENT(T_IDENT(expr)), "spv", 3) == 0) {
    T_INSTANCE_NUM(expr) += n;
  } else if (expr_at_ensemble_root(expr)) {
    T_SET_NUDGE(expr, d, abs(n));
  } else {
    /*
    INT_FATAL(NULL, "didn't expect this\n");
    */
  }
}

/*
 * removes all the zeroes inserted with zeroreplace_expr.
 */
static void out_zero (tab* t, statement_t* preinner, statement_t* preouter)
{
  int r, c;
  statement_t* stmt;

  INT_COND_FATAL (t != NULL, NULL, "attempt to out zero null tablet");
  if (t != NULL) {
    for (r = 0; r < TAB_ROWS (t); r++) {
      for (c = 0; c < TAB_COLS (t); c++) {
	if (TAB_SEXPR (t, r, c) != NULL) {
	  if (STENCIL_FACTOR) {
	    factor_expr (T_NEXT(T_OPLS(TAB_SEXPR(t, r, c))));
	  }
	  else {
	    T_NEXT(T_OPLS(TAB_SEXPR(t, r, c))) =
	      clean_expr (T_NEXT(T_OPLS(TAB_SEXPR(t, r, c))));
	  }
	}
      }
    }
  }

  for (stmt = preinner; stmt != NULL; stmt = T_NEXT(stmt)) {
    if (STENCIL_FACTOR) {
      factor_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
    }
    else {
      T_NEXT(T_OPLS(T_EXPR(stmt))) = clean_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
    }
  }

  for (stmt = preouter; stmt != NULL; stmt = T_NEXT(stmt)) {
    if (STENCIL_FACTOR) {
      factor_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
    }
    else {
      T_NEXT(T_OPLS(T_EXPR(stmt))) = clean_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
    }
  }

}

/*
 * finds wexpr in expr or returns null
 */
static expr_t* findweight (expr_t* expr, expr_t* wexpr)
{
  expr_t* temp;

  if (expr == NULL) {
    INT_FATAL (NULL, "expr passed to findweight is NULL");
  }

  if (wexpr == NULL) {
    INT_FATAL (NULL, "wexpr passed to findweight is NULL");
  }

  if (T_TYPE(expr) == BIPLUS) {
    temp = findweight (T_OPLS (expr), wexpr);
    if (temp != NULL) {
      return temp;
    }
    return findweight (T_NEXT (T_OPLS (expr)), wexpr);
  }
  else if (T_TYPE(expr) == BITIMES) {
    if (expr_equal (T_OPLS(expr), wexpr)) {
      return T_OPLS(expr);
    }
    else {
      return NULL;
    }
  }
  else {
    return NULL;
  }
}

static expr_t* addcolelt (expr_t* expr, tab* t, subtab* s, int r, int c)
{
  expr_t* new;
  expr_t* wexpr;
  expr_t* aexpr;
  expr_t* temp;
  int tr, tc;

  tr = tabrow_subtab(t, s, r);
  tc = tabcol_subtab(t, s, r, c);

  aexpr = copy_expr (TAB_AEXPR (t, tr, tc));
  wexpr = inner_wgt(t, s, r);
  if (expr == NULL) {
    new = build_binary_op (BITIMES, aexpr, wexpr);
  }
  else {
    temp = findweight (expr, wexpr);
    if (temp != NULL) {
      if (T_OPLS (T_PARENT (temp)) == temp) {
        new = build_binary_op (BIPLUS, T_NEXT (temp), aexpr);
        T_NEXT (temp) = new;
        T_PREV (new) = temp;
        T_PARENT (new) = T_PARENT (temp);
      }
      else {
        INT_FATAL (NULL, "weight on bad side in addcolelt");
      }
      new = expr;
    }
    else {
      new = build_binary_op (BITIMES, aexpr, wexpr);
      new = build_binary_op (BIPLUS, new, expr);
    }
  }
  return new;
}

static int get_wid (tab* t, expr_t* wexpr)
{
  int c, r;
  int hiwid;

  hiwid = 0;
  for (r = 0; r < TAB_ROWS(t); r++) {
    for (c = 0; c < TAB_COLS(t); c++) {
      if (TAB_WID (t, r, c) != 0) {
	if (TAB_WEXPR (t, r, c) == NULL &&
	    expr_computable_const (wexpr) &&
	    expr_intval (wexpr) == 1) {
	  return TAB_WID (t, r, c);
	}
	else if (expr_equal (TAB_WEXPR (t, r, c), wexpr)) {
	  return TAB_WID (t, r, c);
	}
	else {
	  if (TAB_WID (t, r, c) >= hiwid) {
	    hiwid = TAB_WID (t, r, c) + 1;
	  }
	}
      }
    }
  }
  return hiwid;
}

static tab* subin_tab (tab* t, subtab* s)
{
  int c, r, tc, tr;
  int aid;
  expr_t* aexpr;
  int sid;
  expr_t* sexpr;
  int wid;
  expr_t* wexpr;

  if (t == NULL) {
    INT_FATAL (NULL, "null tab passed to subint_tab");
  }

  if (s == NULL) {
    INT_FATAL (NULL, "null subtab passed to subint_tab");
  }

  if (t->rows != s->rows) {
    INT_FATAL (NULL, "tab and subtab of different height in subint_tab");
  }

  if (t->cols != s->cols) {
    INT_FATAL (NULL, "tab and subtab of different width in subint_tab");
  }

  t = resize_tab (t, TAB_ROWS(t) + 1, TAB_COLS(t));
  for (c = 0; c < SUBTAB_WIDE(s); c++) {
    aexpr = sexpr = wexpr = NULL;
    aid = sid = wid = 0;
    for (r = 0; r < SUBTAB_HIGH(s); r++) {
      tr = tabrow_subtab(t, s, r);
      tc = tabcol_subtab(t, s, r, c);
      aexpr = addcolelt(aexpr, t, s, r, c);
      aid = -1;
      sexpr = TAB_SEXPR(t, tr, tc);
      sid = TAB_SID(t, tr, tc);
    }
    wexpr = outer_wgt(t, s, c);
    if (aexpr != NULL && T_TYPE (aexpr) == BITIMES) {
      aexpr = T_NEXT (T_OPLS (aexpr));
      T_PARENT (aexpr) = NULL;
      T_PREV (aexpr) = NULL;
    }
    typeinfo_expr (wexpr);
    wid = get_wid (t, wexpr);
    if (aid != 0) {
      if (tc >= TAB_COLS(t)) {
	INT_FATAL(NULL, "new column right of tablet in subin_tab");
      }
      TAB_AEXPR(t, TAB_ROWS(t) - 1, tc) = aexpr;
      TAB_AID(t, TAB_ROWS(t) - 1, tc) = aid;
      TAB_WEXPR(t, TAB_ROWS(t) - 1, tc) = wexpr;
      TAB_WID(t, TAB_ROWS(t) - 1, tc) = wid;
      TAB_SEXPR(t, TAB_ROWS(t) - 1, tc) = sexpr;
      TAB_SID(t, TAB_ROWS(t) - 1, tc) = sid;
    }
  }
  return t;
}

static void selremove_expr (expr_t* sexpr, expr_t* aexpr, expr_t* wexpr)
{
  expr_t* wgt;

  if (sexpr == NULL || aexpr == NULL) {
    INT_FATAL (NULL, "null stmt or array passed to selremove_expr");
  }

  if (expr_equal (sexpr, aexpr)) {
    wgt = weight_tall (sexpr);

    if (wgt == NULL) {
      if (wexpr == NULL || 
	  (expr_computable_const(wexpr) &&
	   D_CLASS(T_TYPEINFO(wexpr)) == DT_INTEGER &&
	   expr_intval(wexpr) == 1)) {
	zeroreplace_expr (sexpr);
      }
    }
    else {
      if (expr_equal (wgt, wexpr)) {
	zeroreplace_expr (sexpr);
      }
    }
  }
  else if (T_IS_BINARY(T_TYPE(sexpr)) && T_TYPE(sexpr) != BIAT) {
    selremove_expr(T_OPLS(sexpr), aexpr, wexpr);
    if (T_NEXT(T_OPLS(sexpr)) != NULL) {
      selremove_expr(T_NEXT(T_OPLS(sexpr)), aexpr, wexpr);
    }
  }
}

static expr_t* astrepl_subtab (tab* t, subtab* s, int n)
{
  expr_t* te;
  expr_t* wexpr;
  expr_t* sexpr;
  expr_t* anycol; /* any of the possible colexprs (ndst of them) */
  int r, d, tr, tc;

  INT_COND_FATAL (t != NULL, NULL, "null tablet in expr_subtab call");
  INT_COND_FATAL (s != NULL, NULL, "attempt to get expr out of null subtab");
  anycol = NULL;
  for (d = 0; d < t->ndst; d++) {
    te = NULL;
    for (r = 0; r < SUBTAB_HIGH(s); r++) {
      tr = tabrow_subtab(t, s, r);
      tc = lo_subtab(t, s, tr) + n;
      if (tc < s->cols / t->ndst) {
	tc += d * (s->cols / t->ndst);
	if (get_subtab(s, tr, tc)) {
	  selremove_expr(TAB_SEXPR(t, tr, tc), TAB_AEXPR(t, tr, tc),
			 TAB_WEXPR(t, tr, tc));
	  te = addcolelt(te, t, s, r, tabcolinv_subtab(t, s, tr, tc));
	  sexpr = TAB_SEXPR(t, tr, tc);
	}
      }
    }
    if (te != NULL) {
      wexpr = outer_wgt(t, s, tabcolinv_subtab(t, s, tr, tc));
      if (T_TYPE (te) == BITIMES) {
	te = T_NEXT (T_OPLS (te));
	T_PARENT (te) = NULL;
	T_PREV (te) = NULL;
      }
      anycol = te;
      te = build_typed_binary_op (BITIMES, copy_expr (te), wexpr);
      T_NEXT(T_OPLS(sexpr)) =
	build_binary_op (BIPLUS, te, T_NEXT(T_OPLS(sexpr)));
      T_PARENT(T_NEXT(T_OPLS(sexpr))) = sexpr;
      T_PREV(T_NEXT(T_OPLS(sexpr))) = T_OPLS(sexpr);
    }
  }
  typeinfo_expr (anycol);
  return anycol;
}

static expr_t* scalarize (tab* t, subtab* s, int n)
{
  int d, tr, tc;
  expr_t* anycol;

  if (t == NULL) {
    INT_FATAL(NULL, "null tab in scalarize");
  }
  if (s == NULL) {
    INT_FATAL(NULL, "null subtab in scalarize");
  }
  anycol = NULL;
  for (d = 0; d < t->ndst; d++) {
    tr = tabrow_subtab(t, s, 0);
    tc = lo_subtab(t, s, tr) + n;
    if (tc >= s->cols / t->ndst) {
      INT_FATAL(NULL, "error of lo in scalarize");
    }
    tc += d * (s->cols / t->ndst);
    if (get_subtab(s, tr, tc)) {
      anycol = TAB_AEXPR (t, tr, tc);
      d = t->ndst;
    }
  }
  typeinfo_expr(anycol);
  return anycol;
}

static void make_prestmts (mloop_t* mloop,
			   statement_t* preinner,
			   statement_t* preouter)
{
  statement_t* start; /* first stmt in preinner or preouter  */
  statement_t* pre;   /* pre stmt to be added to T_MLOOP_PRE */
  statement_t* temp;  /* temporary stmt to store T_NEXT(pre) */
  int numdims;
  int dim;

  if (preinner != NULL) {
    preinner = reverse_stmtls (preinner);
  }

  if (preouter != NULL) {
    preouter = reverse_stmtls (preouter);
  }

  numdims = T_MLOOP_RANK(mloop);
  for (start = preinner;
       start != NULL;
       start = (start == preinner) ? preouter : NULL) {
    for (pre = start;
	 pre != NULL;
	 pre = temp) {
      temp = T_NEXT(pre);
      T_NEXT(pre) = NULL;

      /* determine appropriate dimension for inserting pre statement -BLC */
      if (start == preinner) {
	dim = T_MLOOP_ORDER(mloop, numdims);
      } else {
	if (numdims == 1) {
	  dim = -1;
	} else {
	  dim = T_MLOOP_ORDER(mloop, numdims - 1);
	}
      }

      T_ADD_MLOOP_PRE (mloop, pre, dim);
    }
  }
}

static void replaceexpr_mlooptab (tab* t, mloop_t* mloop,
				  expr_t* old, expr_t* new)
{
  statement_t* stmt;
  int r, c;

  T_MEANING (new) = copy_expr (old);
  for (stmt = T_MLOOP_BODY (mloop); stmt != NULL; stmt = T_NEXT (stmt)) {
    if (T_TYPE (stmt) == S_EXPR && T_IS_ASSIGNOP (T_TYPE (T_EXPR (stmt)))) {
      replaceall_atom_exprs (T_EXPR(stmt), old, new);
    }
  }
  for (r = 0; r < TAB_ROWS(t); r++) {
    for (c = 0; c < TAB_COLS(t); c++) {
      if (TAB_AEXPR(t, r, c) != NULL) {
	replaceall_atom_exprs (TAB_AEXPR(t, r, c), old, new);
	if (expr_equal (TAB_AEXPR(t, r, c), old)) {
	  TAB_AEXPR(t, r, c) = new;
	}
      }
    }
  }
}

static void undoreplaceall_expr (mloop_t* mloop, expr_t* super, expr_t* expr)
{
  if (super == NULL) {
    INT_FATAL (NULL, "null super passed to undoreplaceall_expr");
  }

  if (super == NULL) {
    INT_FATAL (NULL, "null super passed to undoreplaceall_expr");
  }

  if (T_MEANING (expr) != NULL) {
    add_replacement (mloop, T_MEANING(expr), copy_expr(expr));
    replaceall_expr (super, expr, T_MEANING(expr));
    undoreplaceall_expr (mloop, super, super);
  }
  else if (T_OPLS (expr) != NULL) {
    undoreplaceall_expr (mloop, super, T_OPLS(expr));
    if (T_NEXT(T_OPLS(expr)) != NULL) {
      undoreplaceall_expr (mloop, super, T_NEXT(T_OPLS(expr)));
    }
  }
}

static void undoreplaceexpr_mloop (mloop_t* mloop)
{
  statement_t* stmt;

  for (stmt = T_MLOOP_BODY (mloop); stmt != NULL; stmt = T_NEXT (stmt)) {
    if (T_TYPE (stmt) == S_EXPR && T_IS_ASSIGNOP (T_TYPE (T_EXPR (stmt)))) {
      undoreplaceall_expr (mloop, T_EXPR(stmt), T_EXPR(stmt));
    }
  }
}

/*
 * scalarly-like sets up the mloop variables, prestatements and
 * poststatements for a column where the column expression is in col,
 * the stencil is s, the mloop is mloop, rmost is one iff this is
 * the rightmost (last) column, srnum is the subtablet number and
 * colnum is the column number. note that oldpre and oldpst are only
 * valid if !lmost.
 */
static void columnize_scalar (expr_t* col, int srnum, int colnum,
			      mloop_t* mloop, tab* t, int bnd, int maxbnd,
			      statement_t** preinner, statement_t** preouter)
{
  static statement_t* oldpre; /* prestatement of previous column    */
  static expr_t* oldpstexpr;  /* pstexpr of previous column         */
  static symboltable_t* pst;  /* pre-sum variable for columns       */
  expr_t* pstexpr;            /* pst as an expr_t instance colnum   */
  char* name;                 /* name of the pre-sum variable       */
  statement_t* pre;           /* prestatement for column            */
  statement_t* post;          /* poststatement for column, shifting */
  int line;                   /* line number where mloop occurs     */
  char* file;                 /* file in which mloop occurs         */
  expr_t* te;                 /* temporary expresion                */
  int lo, hi;

  lo = colnum == 0;
  hi = colnum == bnd - 1;

  if (col == NULL && colnum == 0) {
    INT_FATAL(NULL, "null leftmost column");
  }
  line = T_LINENO (T_MLOOP_BODY (mloop));
  file = T_FILENAME (T_MLOOP_BODY (mloop));
  if ((TAB_DIRUP(t) && lo) || (!TAB_DIRUP(t) && hi)) {
    name = malloc(128);
    sprintf (name, "spv_%d_", srnum);
    pst = alloc_st (S_VARIABLE, name);
    S_LEVEL (pst) = 1;
    S_DTYPE (pst) = T_TYPEINFO (col);
    S_STYPE (pst) = SC_REGISTER;
    S_NUM_INSTANCES (pst) = stencil_opt == STENCIL_UNROLL ? maxbnd : bnd;
    if (stencil_opt == STENCIL_UNROLL) {
      S_INSTANCE_LOOP (pst) = t->innerd;
    }
    T_ADD_MLOOP_VAR (mloop, pst, -1);
  }
  if (stencil_opt == STENCIL_UNROLL) {
    colnum += maxbnd - bnd;
    bnd = maxbnd;
  }
  pstexpr = build_typed_0ary_op (VARIABLE, pst);
  T_INSTANCE_NUM (pstexpr) = colnum;
  if (col != NULL) {
    replaceexpr_mlooptab (t, mloop, col, pstexpr);
    te = build_binary_op (BIASSIGNMENT, copy_expr (col), copy_expr (pstexpr));
    pre = build_expr_statement (te, line, file);
  }
  else {
    pre = copy_stmt (oldpre);
    T_IDENT (T_OPLS (T_EXPR (pre))) = pst;
    T_INSTANCE_NUM (T_OPLS (T_EXPR (pre))) = colnum;
    inc_dirs_by (T_NEXT (T_OPLS (T_EXPR (pre))), t->innerd,
		 TAB_DIRUP(t) ? 1 : -1);
  }
  T_PROHIBITIONS(pre) |= TILED_OUTER_MLOOP_FLAG;

  if ((TAB_DIRUP(t) && hi) || (!TAB_DIRUP(t) && lo)) {
    T_NEXT (pre) = *preinner;
    *preinner = pre;
  }
  else {
    T_NEXT (pre) = *preouter;
    *preouter = pre;
  }
  if (!((TAB_DIRUP(t) && lo) || (!TAB_DIRUP(t) && hi))) {
    te = build_binary_op (BIASSIGNMENT, copy_expr (pstexpr), oldpstexpr);
    post = build_expr_statement (te, line, file);
    if (stencil_opt == STENCIL_UNROLL) {
      T_PROHIBITIONS (post) |= UNROLLED_MLOOP_FLAG;
    }
    T_ADD_MLOOP_POST (mloop, post, t->innerd);
  }
  oldpre = pre;
  oldpstexpr = pstexpr;
}

/*
 * makes the statements in an mloop more convenient to stencilize
 */
static void stenplify_mloop (mloop_t* mloop) {
  statement_t* stmt;

  for (stmt = T_MLOOP_BODY(mloop); stmt != NULL; stmt = T_NEXT(stmt)) {
    if (T_TYPE(stmt) == S_EXPR && T_IS_ASSIGNOP(T_TYPE(T_EXPR(stmt)))) {
      plainify_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
      typeinfo_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
      compose_expr (T_NEXT(T_OPLS(T_EXPR(stmt)))); 
      typeinfo_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
      groupmult_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
      typeinfo_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
      constfold_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
      typeinfo_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
      factor_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
      typeinfo_expr (T_NEXT(T_OPLS(T_EXPR(stmt))));
    }
  }
}

/*
 * takes an mloop statement and optimizes it if possible.
 */
static void stencilize (statement_t* mstmt)
{
  mloop_t* mloop;  /* mloop to undergo stencilization operation          */
  tab* t;          /* stencil tablet                      */
  subtablist* sl;  /* stencil partition                   */
  subtablist* tmp; /* temporary subtablist pointer to traverse list      */
  int n;           /* column number                                      */
  int srnum;       /* subtab number                                      */
  int bnd;         /* number of replacement vars for a subtab            */
  int maxbnd;      /* maximum bound of any subtab                        */
  expr_t* col;     /* column expression                                  */
  int line;        /* zpl source line number of mloop                    */
  char* file;      /* zpl source file containing mloop                   */
  statement_t* preinner, *preinner2;
  statement_t* preouter, *preouter2;
  statement_t* mloop_orig_body;  /* the original MLOOP body */

  preinner = NULL;
  preouter = NULL;
  preinner2 = NULL;
  preouter2 = NULL;
  sl = NULL;
  if (mstmt != NULL && T_TYPE (mstmt) == S_MLOOP && T_MLOOP (mstmt) != NULL) {

    /* make a copy of the MLOOP body and plug it into the MLOOP for 
       stenplify_mloop to play with */
    mloop = T_MLOOP(mstmt);

    if (mloop_is_sparse(mloop)) {
      return;  /* there's not much we can hope to do in optimizing a sparse
		  MLOOP, since there isn't a pattern to the accesses */
    }

    mloop_orig_body = T_MLOOP_BODY(mloop);
    T_MLOOP_BODY(mloop) = copy_stmtls(mloop_orig_body);
    srnum = 0;

    if (STENCIL_RECOG) stenplify_mloop (mloop);
    t = fill_tab (mloop, weight_tall, valid_tall);



    if (t) {

      sl = get_greedy (&t, t);


      if (sl) {
	STENCIL_DEBUG (line = T_LINENO (T_MLOOP_BODY (mloop)));
	STENCIL_DEBUG (file = T_FILENAME (T_MLOOP_BODY (mloop)));
	STENCIL_DEBUG (printf ("\nSTENCIL MLOOP %s:%d\n", file, line));
	STENCIL_DEBUG (print_tab (t));
	STENCIL_DEBUG (print_subtablist (t, sl));
	maxbnd = 0;
	for (tmp = sl; tmp != NULL; tmp = tmp->nxt) {
	  bnd = bound_subtab (t, tmp->s);
	  maxbnd = bnd > maxbnd ? bnd : maxbnd;
	}
	if (stencil_opt == STENCIL_UNROLL) {
	  T_MLOOP_UNROLL (mloop, t->innerd) =
	    lcm (T_MLOOP_UNROLL (mloop, t->innerd), maxbnd);
	}
	for (tmp = sl; tmp != NULL; tmp = tmp->nxt, srnum++) {
	  bnd = bound_subtab (t, tmp->s);
	  for (n = TAB_DIRUP(t) ? 0 : bnd - 1;
	       TAB_DIRUP(t) ? n < bnd : n >= 0;
	       TAB_DIRUP(t) ? n++ : n--) {
	    col = astrepl_subtab (t, tmp->s, n * TAB_STRIDE (t));
	    switch (stencil_opt) {
	    case STENCIL_UNROLL:
	    case STENCIL_SCALAR:
	    case STENCIL_NONE: /* scalarize compiler flag only */
	      columnize_scalar (col, srnum, n, mloop, t, bnd, maxbnd,
				&preinner, &preouter);
	      break;
	    default:
	      INT_FATAL (NULL, "unknown stencil optimization type");
	      break;
	    }
	  }
	}
	out_zero (t, preinner, preouter);
	free_subtablist (sl);
	destroy_stmtls(mloop_orig_body);
      }
      free_tab (t);
    }
    if (sl == NULL) {
      /* get rid of Steve's copy of the mloop body and replace the original */
      destroy_stmtls(T_MLOOP_BODY(mloop));
      T_MLOOP_BODY(mloop) = mloop_orig_body;
    }

    /********************************/
    /* NOW WE SCALARIZE             */

    fix_stmt(mstmt);
    typeinfo_stmt(mstmt);
    if (STENCIL_SHORT) {



      t = fill_tab (mloop, weight_short, valid_short);


      if (t) {

	sl = get_shorts (t);


	if (sl) {
	  STENCIL_DEBUG (line = T_LINENO (T_MLOOP_BODY (mloop)));
	  STENCIL_DEBUG (file = T_FILENAME (T_MLOOP_BODY (mloop)));
	  STENCIL_DEBUG (printf ("\nSTENCIL MLOOP %s:%d\n", file, line));
	  STENCIL_DEBUG (print_tab (t));
	  STENCIL_DEBUG (print_subtablist (t, sl));
	  maxbnd = 0;
	  for (tmp = sl; tmp != NULL; tmp = tmp->nxt) {
	    bnd = bound_subtab (t, tmp->s);
	    maxbnd = bnd > maxbnd ? bnd : maxbnd;
	  }
	  if (stencil_opt == STENCIL_UNROLL) {
	    T_MLOOP_UNROLL (mloop, t->innerd) =
	      lcm (T_MLOOP_UNROLL (mloop, t->innerd), maxbnd);
	  }
	  for (tmp = sl; tmp != NULL; tmp = tmp->nxt, srnum++) {
	    bnd = bound_subtab (t, tmp->s);
	    for (n = TAB_DIRUP(t) ? 0 : bnd - 1;
		 TAB_DIRUP(t) ? n < bnd : n >= 0;
		 TAB_DIRUP(t) ? n++ : n--) {
	      if (0) col = astrepl_subtab (t, tmp->s, n * TAB_STRIDE (t));
	      if (1) col = scalarize(t, tmp->s, n*TAB_STRIDE(t));
	      if (n == 0 && col == NULL) {
		col = scalarize(t, tmp->s, 0);
		INT_FATAL(NULL, "doh");
	      }
	      switch (stencil_opt) {
	      case STENCIL_UNROLL:
	      case STENCIL_SCALAR:
	      case STENCIL_NONE: /* scalarize compiler flag only */
		columnize_scalar (col, srnum, n, mloop, t, bnd, maxbnd,
				  &preinner2, &preouter2);
		break;
	      default:
		INT_FATAL (NULL, "unknown stencil optimization type");
		break;
	      }
	    }
	  }
	  out_zero (t, preinner2, preouter2);
	  free_subtablist (sl);
	}
	free_tab (t);
      }
    }
    make_prestmts (mloop, preinner2, preouter2);
    make_prestmts (mloop, preinner, preouter);
    if (0) undoreplaceexpr_mloop (mloop);
  }

}

int call_stencil (module_t* modls, char* s)
{
  module_t* mod;
  symboltable_t* pst;
  statement_t* stmt;

  if (stencil_opt == STENCIL_VECTOR) {
    USR_FATAL (NULL, "Vector stencil optimization is unsupported.");
  }

  if (stencil_opt != STENCIL_NONE) {
    RMSInit();
    for (mod = modls; mod != NULL; mod = T_NEXT (mod)) {
      for (pst = T_DECL (mod); pst != NULL; pst = S_SIBLING (pst)) {
	if (S_CLASS (pst) == S_SUBPROGRAM) {
	  if ((stmt = T_STLS (S_FUN_BODY (pst))) != NULL) {
	    traverse_stmtls (T_STLS (T_CMPD (stmt)), 0, stencilize, 0, 0);
	  }
	}
      }
    }
    RMSFinalize();
  }

  return 0;
}
