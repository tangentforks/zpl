/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __NTABLET_H_
#define __NTABLET_H_

typedef struct tabnode_struct {
  int dirvec[MAXRANK]; /* direction of node */
  expr_t* wexpr;       /* weight of node    */
  expr_t* aexpr;       /* array of node     */
  expr_t* sexpr;       /* statement of node */
  int wid;             /* weight tag        */
  int aid;             /* array tag         */
  int sid;             /* statement tag     */
  int nxt;             /* the next column in this row or zero if there are none
			  also, for the last column (at cols not width), it is
			  the next row
		       */
} tabnode;

typedef struct tab_struct {
  tabnode* elts;       /* elements of tablet                      */
  int rank;            /* rank of tablet (mloop's rank + 1)       */
  int stride;          /* stride of tablet, region loop step      */
  int innerd;          /* inner dimension of mloop traversal      */
  int dirup;           /* is the innerd traversed upward?         */
  int cols, rows;      /* number of columns and rows              */
  int nzero;           /* number of nonzero elements              */
  int nsrc;            /* number of source arrays                 */
  int ndst;            /* number of destination arrays/statements */
  int mindir[MAXRANK]; /* minimum in dirvec                       */
  int maxdir[MAXRANK]; /* maximum in dirvec                       */
  int posdir[MAXRANK]; /* position multiplier in dirvec           */
} tab;

typedef struct subtabnode_struct {
  int elt; /* 1 if there is an element at this position */
  int nxt; /* the next column in this row or zero if there are none
	      also, for the last column (at cols not width), it is
	      the next row
	   */
} subtabnode;

typedef struct subtab_struct {
  subtabnode* elts; /* elements of subtablet            */
  int cols, rows;   /* number of columns and rows       */
  int width;        /* width of subtab                  */
  int height;       /* height of subtab                 */
} subtab;

typedef struct subtablist_struct {
  subtab* s;                     /* subtab          */
  struct subtablist_struct* nxt; /* pointer to next */
} subtablist;

#define TAB_ELTS(t) ((t)->elts)
#define TAB_NODE(t,r,c) (get_tabnode((t),(r),(c)))
#define TAB_RANK(t) ((t)->rank)
#define TAB_STRIDE(t) ((t)->stride)
#define TAB_INNERD(t) ((t)->innerd)
#define TAB_DIRUP(t) ((t)->dirup)
#define TAB_COLS(t) ((t)->cols)
#define TAB_ROWS(t) ((t)->rows)
#define TAB_NZERO(t) ((t)->nzero)
#define TAB_NSRC(t) ((t)->nsrc)
#define TAB_NDST(t) ((t)->ndst)
#define TAB_DIR(t,r,c) (TAB_NODE((t),(r),(c))->dirvec)
#define TAB_WID(t,r,c) (TAB_NODE((t),(r),(c))->wid)
#define TAB_AID(t,r,c) (TAB_NODE((t),(r),(c))->aid)
#define TAB_SID(t,r,c) (TAB_NODE((t),(r),(c))->sid)
#define TAB_WEXPR(t,r,c) (TAB_NODE((t),(r),(c))->wexpr)
#define TAB_AEXPR(t,r,c) (TAB_NODE((t),(r),(c))->aexpr)
#define TAB_SEXPR(t,r,c) (TAB_NODE((t),(r),(c))->sexpr)
#define TAB_NXT(t,r,c) (TAB_NODE((t),(r),(c))->nxt)
#define TAB_MINDIR(t) ((t)->mindir)
#define TAB_MAXDIR(t) ((t)->maxdir)
#define TAB_POSDIR(t) ((t)->posdir)

tabnode* get_tabnode (tab*, int, int);
void print_tab (tab*);
void free_tab (tab*);
int has_tab(tab*, int);
tab* copy_tab (tab*);
void clear_tab (tab*);
void subout_tab (tab*, subtab*);
tab* fill_tab (mloop_t*, expr_t* (*)(expr_t*), int (*)(expr_t*));
void fix_tab(tab*); /* resets the nxt element to correct values */
tab* resize_tab (tab*, int, int);

#define SUBTAB_ELTS(s) ((s)->elts)
#define SUBTAB_ROWF(s) ((s)->rowf)
#define SUBTAB_COLS(s) ((s)->cols)
#define SUBTAB_ROWS(s) ((s)->rows)
#define SUBTAB_WIDE(s) ((s)->width)
#define SUBTAB_HIGH(s) ((s)->height)

void print_subtab(tab*, subtab*);
subtab* alloc_subtab(tab*);
void free_subtab(subtab*);
int has_subtab(subtab*, int);
int get_subtab(subtab*, int, int);
int nxt_subtab(subtab*, int, int);
void set_subtab(subtab*, int, int);
void clear_subtab(subtab*);
int quadin_subtab(subtab*, int, int, int, int, int);
subtab* quad_subtab(tab*, int, int, int, int, int);
int bound_subtab(tab*, subtab*);
int lo_subtab(tab*, subtab*, int);
int tabrow_subtab(tab*, subtab*, int);
int tabcol_subtab(tab*, subtab*, int, int);
int tabcolinv_subtab(tab*, subtab*, int, int);
int coltype_subtab(tab*, subtab*);
int rowtype_subtab(tab*, subtab*);

#define SUBTABLIST_THS(sl) ((sl)->s)
#define SUBTABLIST_NXT(sl) ((sl)->nxt)

void print_subtablist (tab*, subtablist*);
subtablist* push_subtablist (subtablist*, subtab*);
void free_subtablist (subtablist*);
int quadin_subtablist (subtablist*, int, int, int, int, int);

/***
 *** for traversing a tablet
 ***/

#define TAB_TRAVERSE_BEGIN(t, r, c)                                     \
          (r) = (has_tab((t), 0)) ? 0 : TAB_NXT((t), 0, TAB_COLS(t)-1); \
          (c) = (TAB_WID((t), (r), 0)) ? 0 : TAB_NXT((t), (r), 0);      \
          while (TAB_WID((t), (r), (c)) != 0)

/** start traversal at or after row rst **/
#define TAB_TRAVERSE_BEGIN_AT(t, r, c, rst)                                         \
          (r) = (has_tab((t), (rst))) ? (rst) : TAB_NXT((t), (rst), TAB_COLS(t)-1); \
          (c) = (TAB_WID((t), (r), 0)) ? 0 : TAB_NXT((t), (r), 0);                  \
          while (TAB_WID((t), (r), (c)) != 0)

#define TAB_TRAVERSE_TAIL(t, r, c)                             \
  if (TAB_NXT((t), (r), (c)) == 0 || (c) == TAB_COLS(t)-1) {   \
    (r) = TAB_NXT((t), (r), TAB_COLS(t)-1);                    \
    if ((r) == 0) {                                            \
      break;                                                   \
    }                                                          \
    else {                                                     \
      (c) = (TAB_WID((t), (r), 0)) ? 0 : TAB_NXT((t), (r), 0); \
    }                                                          \
  }                                                            \
  else (c) = TAB_NXT((t), (r), (c))                            \

#endif
