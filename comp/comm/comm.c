/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/bb_traverse.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/dbg_code_gen.h"
#include "../include/comm.h"
#include "../include/contraction.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/glist.h"
#include "../include/inout.h"
#include "../include/macros.h"
#include "../include/main.h"
#include "../include/passes.h"
#include "../include/rmstack.h"
#include "../include/runtime.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/stmtutil.h"
#include "../include/struct.h"
#include "../include/symboltable.h"
#include "../include/symtab.h"
#include "../include/traverse.h"
#include "../include/treemac.h"

static int remove_redundant;  /*** remove redundant communication ***/
static int combine;           /*** combine comm for different arrays ***/
static int pipeline;          /*** pipeline communication ***/
                              /*** NOTE: probably doesn't work for LHS @'s ***/

static int max_latency=FALSE; /*** combine to maximize latency hiding ***/
static int lhscomm=0;         /*** do LHS @ comm insertion ***/

static int cid = 1;           /*** unique comm id ***/

typedef enum {
  BEFORE=0,
  AFTER,
  XXX
} placement_t;

#define FORWARD 0
#define REVERSE 1

#define COMPARE_DEPEND 0
#define COMPARE_EQUAL 1


/*** internal stuff ***/
typedef struct use_info_struct use_info_t;
typedef struct use_info_struct {
  symboltable_t *var;
  attype_t attype;
  expr_t *expr;
  expr_t *region;
  expr_t *mask;
  genlist_t* directions;
  genlist_t* dirtype;

  int cid;
  statement_t *recv;
  statement_t *postsend;

  statement_t *inserted;

  use_info_t *next;
} use_info_struct;

#define UI_VAR(u) (u)->var
#define UI_ATTYPE(u) (u)->attype
#define UI_EXPR(u) (u)->expr
#define UI_REGION(u) (u)->region
#define UI_MASK(u) (u)->mask
#define UI_DIRS(u) (u)->directions
#define UI_DIRTYPES(u) (u)->dirtype
#define UI_CID(u) (u)->cid
#define UI_RECV(u) (u)->recv
#define UI_POSTSEND(u) (u)->postsend
#define UI_INSERTED(u) (u)->inserted
#define UI_NEXT(u) (u)->next

/*** for arguments to traversal routines ***/
typedef enum {
  FIRST_STMT=0,
  LAST_STMT,
  DOING_PHASE1,
  AWAITING_PHASE2,
  AWAITING_PHASE3,
  ALREADY_COMBINED,
  NUM_TR_ARGS
} tr_args;

/*** table stuff ***/
typedef enum {
  CID_LESS=0,
  CID_EQUAL,
  CID_GREATER,
  CID_LAST
} cid_relation_t;

typedef enum {
  AT_SAME=0,
  AT_S1_PRIME,
  AT_S2_PRIME,
  AT_REL_LAST
} at_relation_t;

/*** truth table to aid in placement of communication statements ***/
static placement_t placement_table[AT_REL_LAST][CID_LAST][C_IM_LAST][C_IM_LAST] = 
{
  /*** for COMM that is induced by the SAME @ type ***/
  {
    /*** CID_LESS ***/
    {
      /*** C_NEW ***/ 
      { AFTER,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_DR ***/ 
      { AFTER, AFTER,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SR ***/ 
      { AFTER, AFTER, AFTER,BEFORE,BEFORE,BEFORE},
      /*** C_DN ***/ 
      {BEFORE,BEFORE, AFTER, AFTER, AFTER, AFTER},
      /*** C_SV ***/ 
      { AFTER, AFTER, AFTER,BEFORE, AFTER,BEFORE},
      /*** C_OLD ***/ 
      { AFTER, AFTER, AFTER,BEFORE, AFTER, AFTER},
    },

    /*** CID_EQUAL ***/
    {
      /*** C_NEW ***/ 
      {   XXX,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_DR ***/ 
      { AFTER,   XXX,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SR ***/ 
      { AFTER, AFTER,   XXX,BEFORE,BEFORE,BEFORE},
      /*** C_DN ***/ 
      { AFTER, AFTER, AFTER,   XXX,BEFORE,BEFORE},
      /*** C_SV ***/ 
      { AFTER, AFTER, AFTER, AFTER,   XXX,BEFORE},
      /*** C_OLD ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER,   XXX},
    },

    /*** CID_GREATER ***/
    {
      /*** C_NEW ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_DR ***/ 
      { AFTER,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SR ***/ 
      { AFTER, AFTER,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_DN ***/ 
      { AFTER, AFTER, AFTER,BEFORE, AFTER, AFTER},
      /*** C_SV ***/ 
      { AFTER, AFTER, AFTER,BEFORE,BEFORE,BEFORE},
      /*** C_OLD ***/ 
      { AFTER, AFTER, AFTER,BEFORE, AFTER,BEFORE},
    }
  },

  /*** for COMM that is induced by the DIFFERENT @ types (s1 is AT_PRIME) ***/
  {
    /*** CID_LESS ***/
    {
      /*** C_NEW ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_DR ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SR ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
      /*** C_DN ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SV ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
      /*** C_OLD ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
    },

    /*** CID_EQUAL ***/
    {
      /*** C_NEW ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_DR ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SR ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
      /*** C_DN ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SV ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
      /*** C_OLD ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
    },

    /*** CID_GREATER ***/
    {
      /*** C_NEW ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_DR ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SR ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
      /*** C_DN ***/ 
      {BEFORE,BEFORE,BEFORE,BEFORE,BEFORE,BEFORE},
      /*** C_SV ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
      /*** C_OLD ***/ 
      { AFTER, AFTER, AFTER, AFTER, AFTER, AFTER},
    }
  },

  /*** for COMM that is induced by the DIFFERENT @ types (s2 is AT_PRIME) ***/
  {
    /*** CID_LESS ***/
    {
      /*** C_NEW ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_DR ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_SR ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_DN ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_SV ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_OLD ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
    },

    /*** CID_EQUAL ***/
    {
      /*** C_NEW ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_DR ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_SR ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_DN ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_SV ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_OLD ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
    },

    /*** CID_GREATER ***/
    {
      /*** C_NEW ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_DR ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_SR ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_DN ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_SV ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
      /*** C_OLD ***/ 
      {BEFORE,BEFORE, AFTER,BEFORE, AFTER, AFTER},
    },
  }

};

#if (!RELEASE_VER)
/*** for statistics gathering ***/
static int table_stats[AT_REL_LAST][CID_LAST][C_IM_LAST][C_IM_LAST] = 
{
  {
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    },
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    },
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    }
  },
  {
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    },
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    },
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    }
  },
  {
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    },
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    },
    {
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
    }
  },
};
#endif

/*** some memory managment ***/
static glist UIstack = NULL;




int get_cid(void) {
  return(cid++);
}



/*************************/
/*** support functions ***/
/*************************/

static use_info_t *new_use_info(symboltable_t *var, expr_t *expr) {
  use_info_t *new;

  new = (use_info_t *) PMALLOC(sizeof(use_info_t));

  UI_VAR(new) = var;
  if (T_TYPE(expr) != BIAT) {
    UI_ATTYPE(new) = AT_NORMAL;
  }
  else {
    UI_ATTYPE(new) = (attype_t) T_SUBTYPE(expr);
  }
  UI_EXPR(new) = expr;
  UI_REGION(new) = NULL;
  UI_MASK(new) = NULL;
  UI_DIRS(new) = NULL;
  UI_DIRTYPES(new) = NULL;
  UI_CID(new) = -1;
  UI_RECV(new) = NULL;
  UI_POSTSEND(new) = NULL;
  UI_INSERTED(new) = NULL;
  UI_NEXT(new) = NULL;

  return new;
}


/* BLC -- currently unused 
static void free_use_info(use_info_t *use) {
  use_info_t *u, *tail;

  for (tail = use, u = UI_NEXT(tail); u != NULL; tail = u, u = UI_NEXT(u)) {
    PFREE(tail, sizeof(use_info_t));
  }

  PFREE(tail, sizeof(use_info_t));
}
*/


static use_info_t * copy_use_info(use_info_t *use) {
  use_info_t *new;

  new = (use_info_t *) PMALLOC(sizeof(use_info_t));

  /*** for a feeble attempt at memory management ***/
  if (UIstack == NULL) {
    UIstack = glist_create(new, GLIST_NODE_SIZE);
  }
  else {
    UIstack = glist_prepend(UIstack, new, GLIST_NODE_SIZE);
  }

  UI_VAR(new) = UI_VAR(use);
  UI_EXPR(new) = UI_EXPR(use);
  UI_REGION(new) = UI_REGION(use);
  UI_MASK(new) = UI_MASK(use);
  UI_DIRS(new) = copy_genlist(UI_DIRS(use));
  UI_DIRTYPES(new) = copy_genlist(UI_DIRTYPES(use));
  UI_CID(new) = -1;

  UI_RECV(new) = UI_RECV(use);
  UI_POSTSEND(new) = UI_POSTSEND(use);
  UI_INSERTED(new) = UI_INSERTED(use);
  UI_NEXT(new) = NULL;

  return new;
}


/******************************************************/
/*** add info about comm needed (should only be     ***/
/***   called first time the need for comm (i.e. @) ***/
/***   is detected                                  ***/
/******************************************************/

static void add_use_info(use_info_t *use, expr_t *expr,
			 expr_t *region, expr_t *mask) {
  if (UI_CID(use) != -1) {
    INT_FATAL(NULL, "This use is already being used!");
  }

  UI_EXPR(use) = expr;
  UI_REGION(use) = region;
  UI_MASK(use) = mask;
  UI_DIRS(use) = NULL;
  UI_DIRTYPES(use) = NULL;
  UI_CID(use) = get_cid();
}


static int genlist_symtab_member(genlist_t *list, genlist_t* typelist,
				 expr_t *element, attype_t type) {
  genlist_t *l;
  genlist_t* l2;
  int found;

  l = list;
  l2 = typelist;
  found = FALSE;

  while (!found && (l != NULL)) {
    if (expr_equal(G_EXPR(l),element) && (G_ATTYPE(l2) == type)) {
      found = TRUE;
    } else {
      l = G_NEXT(l);
      l2 = G_NEXT(l2);
    }
  }

  return(found);
}


/*********************************************/
/*** add another direction and update recv ***/
/*********************************************/

static void add_direction(genlist_t** list, genlist_t** typelist, 
			  expr_t *direction, attype_t attype) {
  genlist_t *new;

  if (direction == NULL) {
    return;
  } else if (genlist_symtab_member(*list, *typelist, direction, attype)) {
    return;
  } else {
    new = alloc_gen();
    G_EXPR(new) = direction;
    G_NEXT(new) = *list;
    *list = new;

    new = alloc_gen();
    G_ATTYPE_DEF(new) = (int)attype;
    G_NEXT(new) = *typelist;
    *typelist = new;
  }
}


static void add_dir_info(use_info_t *use,expr_t *direction,
			 statement_t *recv, attype_t attype) {
  add_direction(&(UI_DIRS(use)), &(UI_DIRTYPES(use)), direction, attype);
  UI_RECV(use) = recv;
}



/***
 *** at_expr_equal(expr_t *, expr_t *)
 ***
 *** return 1 if the expression to the "left" of the @s are the same
 ***  (i.e., expression MINUS the direction is the same)
 ***/
static int
at_expr_equal(expr_t *e1, expr_t *e2)
{
  expr_t *ate1, *ate2;

  if ((T_TYPE(e1) == BIAT)) {
    /*** if this is an AT_PRIME, e2 must also be an AT_PRIME ***/
    if ((T_SUBTYPE(e1) == AT_PRIME) &&
	((T_TYPE(e2) != BIAT) ||
	 ((T_TYPE(e2) == BIAT) && (T_SUBTYPE(e2) != AT_PRIME)))) {
      return 0;
    }
    ate1 = expr_find_ensemble_root(e1);
  } else {
    ate1 = e1;
  }

  if ((T_TYPE(e2) == BIAT)) {
    /*** if this is an AT_PRIME, e1 must also be an AT_PRIME ***/
    if ((T_SUBTYPE(e2) == AT_PRIME) &&
	((T_TYPE(e1) != BIAT) ||
	 ((T_TYPE(e1) == BIAT) && (T_SUBTYPE(e1) != AT_PRIME)))) {
      return 0;
    }
    ate2 = expr_find_ensemble_root(e2);
  } else {
    ate2 = e2;
  }

  return (expr_equal(ate1, ate2));

}


/********************************************/
/*** find the list item that involves var ***/
/********************************************/

static use_info_t *find_use_info(glist list, symboltable_t *var, expr_t *expr, 
				 int how) {
  glist l;
  use_info_t *use;

  if ((how != COMPARE_DEPEND) && (how != COMPARE_EQUAL)) {
    INT_FATAL(NULL, "Invalid comparison type");
  }

  use = NULL;
  if (list != NULL) {
    l = list;
    use = (use_info_t *) glist_top(l);
    while (l != NULL) {
      if ((var == UI_VAR(use)) && ((how==COMPARE_DEPEND)?
				   dependent_exprs(expr, UI_EXPR(use), MAY):
				   at_expr_equal(expr, UI_EXPR(use)))) {
	l = NULL;
      }
      else {
	l = GLIST_NEXT(l);
	if (l != NULL) {
	  use = (use_info_t *) glist_top(l);
	} else {
	  use = NULL;
	}
      }
    }
  }

  return use;
}


/*** does s1 dominate s2 (s1 and s2 are assumed to be in the same bb ***/
int bb_dominates(statement_t *s1, statement_t *s2) {
  statement_t *s;

  s = s1;
  while (s != NULL && s != s2) {
    s = T_NEXT(s);
  }

  if (s != NULL) {
    return TRUE;
  }
  else {
    return FALSE;
  }

}


/****************************************************************/
/***                                                          ***/
/*** use this to find a pending comm that is dependent on the ***/
/***  expression.                                             ***/
/***                                                          ***/
/*** return the modified list and use_info_t                  ***/
/***                                                          ***/
/****************************************************************/

static glist remove_dependent(glist list, expr_t *expr, use_info_t **use) {
  glist l, ret_list;

  ret_list = list;
  if (list == NULL) {
    *use = NULL;
  }
  else {
    l = list;
    *use = (use_info_t *) glist_top(l);
    while (l != NULL) {
      if (dependent_exprs(expr, UI_EXPR(*use), MAY)) {
	ret_list = glist_remove(list, l);
	l = NULL;
      }
      else {
	l = GLIST_NEXT(l);
	if (l != NULL) {
	  *use = (use_info_t *) glist_top(l);
	}
	else {
	  *use = NULL;
	}
      }
    }
  }

  return ret_list;
}


/*************************************************************/
/*** remove list item with same recv, region and direction ***/
/*************************************************************/

static glist remove_similar_use_info(glist list, use_info_t *similar,
				     use_info_t ** use) {
  expr_t *region;
  attype_t attype;
  glist l, ret_list;

  region = UI_REGION(similar);
  attype = UI_ATTYPE(similar);

  ret_list = list;
  if (list == NULL) {
    *use = NULL;
  } else {
    l = list;
    *use = (use_info_t *) glist_top(l);
    while (l != NULL) {
      if (((attype == (UI_ATTYPE(*use))) && /*** at type and ***/
	   (region == UI_REGION(*use))) &&   /*** region and ***/
	  ((!max_latency) ||                /*** !max_latency OR ***/
	   (max_latency &&                  /*** max_latency AND ***/
	    bb_dominates(UI_RECV(similar),UI_RECV(*use)))))/*** the receive dominates ***/ {
	ret_list = glist_remove(list, l);
	l = NULL;
      } else {
	l = GLIST_NEXT(l);
	if (l != NULL) {
	  *use = (use_info_t *) glist_top(l);
	} else {
	  *use = NULL;
	}
      }
    }
  }

  return ret_list;
}


/***************************************************/
/*** combine lists with the region and direction ***/
/***************************************************/

static glist combine_similar(glist list, use_info_t *use,
			     glist *already_combined) {
  glist ret_list;
  use_info_t *other, *u2;

  u2 = use;
  for (ret_list = remove_similar_use_info(list, use, &other);
       other != NULL;
       ret_list = remove_similar_use_info(ret_list, use, &other)) {
    if (UI_ATTYPE(use) == AT_NORMAL) {
      if (T_SUBTYPE(UI_EXPR(use)) == AT_PRIME) {
	INT_FATAL(T_STMT(UI_EXPR(use)), "'@ being combined as @");
      }
      /*** save pointer to combined uses (BUT NOT FOR AT_PRIME) ***/
      if (*already_combined == NULL) {
	*already_combined = glist_create(other, GLIST_NODE_SIZE);
      }
      else {
	*already_combined = glist_prepend(*already_combined, other,
					  GLIST_NODE_SIZE);
      }
    }

    /*** update recvs ***/
    if (bb_dominates(UI_RECV(other), UI_RECV(use))) {
      UI_RECV(use) = UI_RECV(other);
      UI_CID(use) = UI_CID(other);
    }
    /*** must pick the earliest post send ***/
    if (UI_POSTSEND(other) != NULL) {
      if (UI_POSTSEND(use) != NULL) {
	if (bb_dominates(UI_POSTSEND(other), UI_POSTSEND(use))) {
	  UI_POSTSEND(use) = UI_POSTSEND(other);
	}
      }
      else {
	UI_POSTSEND(use) = UI_POSTSEND(other);
      }
    }

    UI_NEXT(u2) = other;
    u2 = other;
    INT_COND_FATAL((UI_NEXT(u2) == NULL), NULL, "Next USE should be NULL.");
  }

  return ret_list;
}


/*** get the constraint of s1 on s2 ***/
/***   return BEFORE:     s1 should be placed BEFORE s2 ***/
/***          AFTER:      s1 should be placed AFTER s2 ***/
/***          DONT_CARE:  s1 can be placed either BEFORE or AFTER s2 ***/
static placement_t get_placement_constraint(statement_t *s1, statement_t *s2) {
  commtype_t c1, c2;
  at_relation_t at_rel;
  cid_relation_t cid_rel;

  if ((T_TYPE(s1) != S_COMM) || (T_TYPE(s2) != S_COMM)) {
    INT_FATAL(NULL, "Non-comm statement");
  }

  c1 = T_COMM_TYPE(s1);
  c2 = T_COMM_TYPE(s2);
  if (T_COMM_ID(s1) < T_COMM_ID(s2)) {
    cid_rel = CID_LESS;
  }
  else if (T_COMM_ID(s1) > T_COMM_ID(s2)) {
    cid_rel = CID_GREATER;
  }
  else {
    cid_rel = CID_EQUAL;
  }

  if ((c1 == c2) && (cid_rel == CID_EQUAL)) {
    INT_FATAL(NULL, "Comms should not be the same");
  }

  switch (T_SUBTYPE(s1)) {
  case AT_NORMAL:
  case AT_WRAP:
    switch (T_SUBTYPE(s2)) {
    case AT_NORMAL:
    case AT_WRAP:
      at_rel = AT_SAME;
      break;
    case AT_PRIME:
      at_rel = AT_S2_PRIME;
      break;
    default:
      INT_FATAL(NULL, "Bad attype.");
      break;
    }
    break;
  case AT_PRIME:
    switch (T_SUBTYPE(s2)) {
    case AT_NORMAL:
    case AT_WRAP:
      at_rel = AT_S1_PRIME;
      break;
    case AT_PRIME:
      at_rel = AT_SAME;
      break;
    default:
      INT_FATAL(NULL, "Bad attype.");
      break;
    }
    break;
  default:
    INT_FATAL(NULL, "Bad attype.");
    break;
  }

#if (!RELEASE_VER)
  /*** take some stats ***/
  table_stats[at_rel][cid_rel][c1][c2] += 1;
#endif
  return placement_table[at_rel][cid_rel][c1][c2];

}


/*** ensure the "best" ordering of continuous comm statements ***/
/*** - keep relative order of comm ***/
/*** NOTES: Since we traverse backwards, a bigger cid mean the comm     ***/
/***        that the comm is "received" later in the code.  The setcid  ***/
/***        pass changes this property when assigning static comm ids   ***/
/***        because it goes forward.                                    ***/

static statement_t *find_best_insertion_pt(statement_t *new,statement_t *s,
					   placement_t placement) {
  statement_t *insertion_pt, *tmp;
  int done = FALSE;
  placement_t rel_place;

  if (T_TYPE(s) == S_COMM) {
    INT_FATAL(s, "Comm nodes cannot be used to insert comm!");
  }

  insertion_pt = s;

  if ((pipeline == FALSE) && (T_SUBTYPE(new) != AT_PRIME)) {
    return insertion_pt;
  }

  tmp = insertion_pt;
  while (!done) {
    if (placement == BEFORE) {
      tmp = T_PREV(tmp);
      while (tmp && T_TYPE(tmp) == S_NULL) {
	tmp = T_PREV(tmp);
      }
    }
    else {
      tmp = T_NEXT(tmp);
      while (tmp && T_TYPE(tmp) == S_NULL) {
	tmp = T_NEXT(tmp);
      }
    }

    if ((tmp != NULL) && (T_TYPE(tmp) == S_COMM)) {
      rel_place = get_placement_constraint(new, tmp);
      if (placement == BEFORE) {
	if (rel_place == AFTER) {
	  done = TRUE;
	}
      }
      else {
	if (rel_place == BEFORE) {
	  done = TRUE;
	}
      }
    }
    else {
      done = TRUE;
    }

    if (!done) {
      insertion_pt = tmp;
    }
  }

  return insertion_pt;
}


static statement_t *insert_comm(commtype_t type,
				statement_t *s,
				statement_t *copy,
				placement_t placement,
				expr_t *expression,
				int id,
				expr_t *region,
				genlist_t *directions,
				genlist_t* dirtypes) {
  statement_t *comm;			/* new communication node */
  statement_t *insertion_pt;
  statement_t dummy;

  if (copy == NULL) {
    if (directions == NULL) {
      INT_FATAL(NULL, "Direction is NULL.");
    }

    switch (type) {
    case C_NEW:
    case C_DR:
    case C_DN:
      comm = build_comm_statement(type, lhscomm, id, region,
				  expr_find_at(expression),
				  copy_genlist(directions), 
				  copy_genlist(dirtypes),
				  T_LINENO(s), T_FILENAME(s));
      break;
    case C_SR:
    case C_SV:
    case C_OLD:
      comm = build_comm_statement(type, lhscomm, id, region,
				  expr_find_at(expression),
				  copy_genlist(directions), 
				  copy_genlist(dirtypes),
				  T_LINENO(s), T_FILENAME(s));
      break;
    default:
      INT_FATAL(NULL, "Invalid comm type!");
    }
    T_SUBTYPE(comm) = T_SUBTYPE(expr_find_at(expression));
  }
  else {
    comm = copy;
  }

  T_NEXT(comm) = T_PREV(comm) = NULL;
  insertion_pt = find_best_insertion_pt(comm, s, placement);

  switch (placement) {
  case BEFORE:
    if (T_PREV(insertion_pt) == NULL) {
      T_PREV(insertion_pt) = &dummy;
    }
    insertbefore_stmt(comm, insertion_pt);	/*** inserted BEFORE this stmt ***/
    if (T_PREV(comm) == &dummy) {
      T_PREV(comm) = NULL;
    }
    break;
  case AFTER:
    insertafter_stmt(comm, insertion_pt);	/*** inserted AFTER this stmt ***/
    break;
  default:
    INT_FATAL(NULL, "No such placement of comm statement");
  }

  return comm;

}


/* echris: note that sbef and saft are only different for mscan comm
 */

static statement_t *insert_from_info(use_info_t *use, commtype_t type,
				     statement_t *sbef, statement_t *saft,
				     placement_t placement) {
  use_info_t *other;
  expr_t *at_expr;
  commtype_t ct;
  statement_t *comm[C_IM_LAST];
  statement_t *ret_stmt;

  if ((((pipeline == FALSE) && (type != C_NEW)) ||
       ((T_SUBTYPE(sbef) != MLOOP_MSCAN) && (T_TYPE(sbef) != S_NULL) &&
	(placement == XXX))) &&
      (((pipeline == FALSE) && (type != C_NEW)) ||
       ((T_SUBTYPE(saft) != MLOOP_MSCAN) && (T_TYPE(saft) != S_NULL) &&
	(placement == XXX)))) {
    /*** not pushing comm apart ***/
    INT_FATAL(NULL, "Bad call to insert_from_info().");
  }

  if (UI_EXPR(use) == NULL) {
    INT_FATAL(NULL, "Use expression NULL");
  }

  IFDB(15) {
    DBS1(15, "\t*** Inserting%s", (lhscomm ? " LHS " : " "));
    switch(type) {
    case C_NEW:
      if (placement == XXX) {
	DBS0(15, "MSCAN ");
      }
      else if (pipeline == FALSE) {
	DBS0(15, "four part ");
      }
      DBS0(15, "comm");
      break;
    case C_SR:
      DBS0(15, "send");
      break;
    case C_DN:
      DBS0(15, "recv");
      break;
    default:
      INT_FATAL(NULL, "Incorrect commtype_t");
    }
    DBS6(15, " (%d) for [%s (%s)] %s@%s at line %d\n",
	 UI_CID(use), (UI_REGION(use) ? S_IDENT(T_IDENT(UI_REGION(use))) : "NULL"),
	 (UI_MASK(use) ? "mask" : "no mask"),
	 S_IDENT(UI_VAR(use)),
	 S_IDENT(expr_find_root_pst(G_EXPR(UI_DIRS(use)))),
	 T_LINENO(sbef));
  }

  if (placement == XXX) {
    /*** insert AT_PRIME comm ***/
    /*** sungeun *** don't know if this will work for LHS @'s ***/
    comm[C_NEW] = insert_comm(C_NEW, sbef, NULL, BEFORE, UI_EXPR(use),
			      UI_CID(use), build_0ary_op(CONSTANT,pst_MSCANREG),
			      copy_genlist(UI_DIRS(use)),
			      copy_genlist(UI_DIRTYPES(use)));
    comm[C_DR] = insert_comm(C_DR, sbef, NULL, BEFORE, UI_EXPR(use),
			     UI_CID(use), build_0ary_op(CONSTANT, pst_MSCANREG),
			     copy_genlist(UI_DIRS(use)),
			      copy_genlist(UI_DIRTYPES(use)));
    comm[C_SR] = insert_comm(C_SR, saft, NULL, AFTER, UI_EXPR(use),
			     UI_CID(use), build_0ary_op(CONSTANT, pst_MSCANREG),
			     copy_genlist(UI_DIRS(use)),
			      copy_genlist(UI_DIRTYPES(use)));
    comm[C_DN] = insert_comm(C_DN, sbef, NULL, BEFORE, UI_EXPR(use),
			     UI_CID(use), build_0ary_op(CONSTANT, pst_MSCANREG),
			     copy_genlist(UI_DIRS(use)),
			      copy_genlist(UI_DIRTYPES(use)));
    comm[C_SV] = insert_comm(C_SV, saft, NULL, AFTER, UI_EXPR(use),
			     UI_CID(use), build_0ary_op(CONSTANT, pst_MSCANREG),
			     copy_genlist(UI_DIRS(use)),
			     copy_genlist(UI_DIRTYPES(use)));
    comm[C_OLD] = insert_comm(C_OLD, saft, NULL, AFTER, UI_EXPR(use),
			      UI_CID(use), build_0ary_op(CONSTANT, pst_MSCANREG),
			      copy_genlist(UI_DIRS(use)),
			      copy_genlist(UI_DIRTYPES(use)));
    for (ct = C_NEW; ct <= C_OLD; ct++) {
      /*** set prev and next pointers ***/
      if (ct != C_NEW) {
	T_COMM_PREV(comm[ct]) = comm[ct-1];
	T_COMM_NEXT(T_COMM_PREV(comm[ct])) = comm[ct];
      }
      /*** set PROHIBITIONS fields ***/
      T_PROHIBITIONS(comm[ct]) |= TILED_OUTER_MLOOP_FLAG;
    }
    ret_stmt = comm[C_NEW];
  } /* if (placement == XXX) i.e., AT_PRIME comm */

  else if (pipeline == FALSE) {
    /*** insert all four parts of normal comm eagerly ***/
    /*** perhaps this should be lazily ***/
    if (lhscomm == 0) {
      if (placement != BEFORE) {
	INT_FATAL(NULL, "Placement must be BEFORE when not pipelining RHS comm!");
      }
      for (ct = C_NEW; ct <= C_OLD; ct++) {
	/*** insert the other parts, too ***/
	comm[ct] = insert_comm(ct, sbef, NULL, BEFORE, UI_EXPR(use),
			       UI_CID(use), UI_REGION(use),
			       copy_genlist(UI_DIRS(use)),
			       copy_genlist(UI_DIRTYPES(use)));
      }
    } else {
      /*** LHS @ ***/
      if (placement != AFTER) {
	INT_FATAL(NULL, "Placement must be AFTER when not pipelining LHS comm!");
      }

      /*** for LHS @'s under a region with a mask applied ***/
      /***  the current solution is to make sure that all "fluff" ***/
      /***  is up to date by doing a RHS comm before the statement ***/
      /***  (LHS @) which is then followed by the LHS comm ***/
      if (UI_MASK(use)) {
	/*** this is gross, but what can you do? ***/
	lhscomm = 0;
	for (ct = C_NEW; ct <= C_OLD; ct++) {
	  /*** insert the other parts, too ***/
	  comm[ct] = insert_comm(ct, sbef, NULL, BEFORE, UI_EXPR(use),
				 UI_CID(use), UI_REGION(use),
				 copy_genlist(UI_DIRS(use)),
				 copy_genlist(UI_DIRTYPES(use)));
	  if (ct > C_NEW) {
	    T_COMM_PREV(comm[ct]) = comm[ct-1];
	    T_COMM_NEXT(T_COMM_PREV(comm[ct])) = comm[ct];
	  }
	}
	T_COMM_PREV(comm[C_NEW]) = NULL;
	T_COMM_NEXT(comm[C_OLD]) = NULL;
	lhscomm = 1;
      }

      for (ct = C_OLD; (int) ct >= 0; ct--) {
	/*** insert the other parts, too ***/
	comm[ct] = insert_comm(ct, sbef, NULL, AFTER, UI_EXPR(use),
			       UI_CID(use), UI_REGION(use),
			       copy_genlist(UI_DIRS(use)),
			       copy_genlist(UI_DIRTYPES(use)));
      }
    }

    for (ct = C_NEW+1; ct <= C_OLD; ct++) {
      T_COMM_PREV(comm[ct]) = comm[ct-1];
      T_COMM_NEXT(T_COMM_PREV(comm[ct])) = comm[ct];
    }
    T_COMM_PREV(comm[C_NEW]) = NULL;
    T_COMM_NEXT(comm[C_OLD]) = NULL;

    ret_stmt = comm[C_DN];
  }
  else {
    if (sbef != saft) {
      INT_FATAL(NULL, "Bad arguments to insert_from_info()");
    }
    ret_stmt = insert_comm(type, sbef, NULL, placement, UI_EXPR(use), 
			   UI_CID(use), UI_REGION(use), 
			   copy_genlist(UI_DIRS(use)),
			   copy_genlist(UI_DIRTYPES(use)));
  }

  for (other = UI_NEXT(use); other != NULL; other = UI_NEXT(other)) {
    if (UI_EXPR(other) == NULL) {
      INT_FATAL(NULL, "Use expression NULL");
    }

    at_expr = expr_find_at(UI_EXPR(other));
    if ((pipeline == FALSE) || (placement == XXX)) {
      /*** add comm info for the other parts, too ***/
      for (ct = C_NEW; ct <= C_OLD; ct++) {
	add_comm_info(comm[ct], at_expr, copy_genlist(UI_DIRS(other)),
		      copy_genlist(UI_DIRTYPES(other)));
      }
    }
    else {
      add_comm_info(ret_stmt, at_expr, copy_genlist(UI_DIRS(other)),
		    copy_genlist(UI_DIRTYPES(other)));
    }
  }

  return ret_stmt;
}


static statement_t *insert_from_inserted(statement_t *inserted,commtype_t type,
					 statement_t *s,placement_t placement) {
  statement_t *comm;

  if (pipeline == FALSE) {     /*** not pushing comm apart ***/
    INT_FATAL(NULL, "Bad call to insert_from_inserted() -- not pipelining.");
  }

  IFDB(15) {
    switch(type) {
    case C_DR:
      DBS2(15, "\t*** Inserting prerecv (%d) at line %d\n",
	   T_COMM_ID(inserted), T_LINENO(s));
      break;
    case C_SV:
      DBS2(15, "\t*** Inserting postsend (%d) at line %d\n",
	   T_COMM_ID(inserted), T_LINENO(s));
      break;
    default:
      INT_FATAL(NULL, "Incorrect commtype_t");
    }
  }

  comm = copy_comm_stmt(inserted);
  T_COMM_TYPE(comm) = type;
  T_LINENO(comm) = T_LINENO(s);

  insert_comm(type, s, comm, placement, NULL, -1, NULL, NULL, NULL);

  return comm;
}


static void insertDR(statement_t *inserted,statement_t *s,
		     placement_t placement) {
  statement_t *cnew, *cdr;

  cnew = insert_from_inserted(inserted, C_NEW, s, placement);
  cdr = insert_from_inserted(inserted, C_DR, s, placement);

  T_COMM_NEXT(cnew) = cdr;
  T_COMM_PREV(cdr) = cnew;

  INT_COND_FATAL((T_COMM_TYPE(inserted) == C_SR), NULL, "Wrong inserted type.");

  T_COMM_PREV(inserted) = cdr;
  T_COMM_NEXT(cdr) = inserted;
}


static statement_t *insertSR(use_info_t *use,statement_t *s,
			     placement_t placement) {
  statement_t *ret_stmt;

  ret_stmt = insert_from_info(use, C_SR, s, s, placement);

  return ret_stmt;
}


static statement_t *insertDN(void *stuff,statement_t *s,placement_t placement) {
  statement_t *ret_stmt;

  if ((lhscomm == 0) && (placement == AFTER)) {
    INT_FATAL(NULL, "Placement should never be AFTER for DN.");
  } else if ((lhscomm == 1) && (placement == BEFORE)) {
    INT_FATAL(NULL, "Placement should never be BEFORE for (LHS) DN.");
  }

  if (pipeline == TRUE) {
    statement_t *inserted = (statement_t *) stuff;

    if (lhscomm == 1) {
      INT_FATAL(NULL, "Pipelining not supported for LHS @'s\n");
    }

    ret_stmt = insert_from_inserted(inserted, C_DN, s, placement);

    INT_COND_FATAL((T_COMM_TYPE(inserted) == C_SR), NULL, "Wrong inserted type.");

    T_COMM_PREV(ret_stmt) = inserted;
    T_COMM_NEXT(inserted) = ret_stmt;
  }
  else {
    ret_stmt = insert_from_info((use_info_t *) stuff, C_NEW, s, s, placement);
  }

  return ret_stmt;
}


static void insertSV(statement_t *inserted, statement_t *s,
		     placement_t placement) {
  statement_t *csv, *cold;

  csv = insert_from_inserted(inserted, C_SV, s, placement);
  cold = insert_from_inserted(inserted, C_OLD, s, placement);

  T_COMM_NEXT(csv) = cold;
  T_COMM_PREV(cold) = csv;

  INT_COND_FATAL((T_COMM_TYPE(inserted) == C_DN), NULL, "Wrong inserted type.");

  T_COMM_PREV(csv) = inserted;
  T_COMM_NEXT(inserted) = csv;
}


/****************************************************************/
/***                                                          ***/
/*** use this to remove redundant communication of a variable ***/
/***  by specifying var, expr, region and direction.          ***/
/***                                                          ***/
/*** return the modified list and use_info_t                  ***/
/***                                                          ***/
/****************************************************************/

static glist remove_redundant_info(glist list, symboltable_t *var, expr_t *expr,
				   expr_t *region,use_info_t **use) {
  glist l, ret_list;

  ret_list = list;
  if (list == NULL) {
    *use = NULL;
  }
  else {
    l = list;
    *use = (use_info_t *) glist_top(l);
    while (l != NULL) {
      if ((var == UI_VAR(*use)) &&
	  (at_expr_equal(expr, UI_EXPR(*use))) &&
	  (expr_equal(region, UI_REGION(*use)))) {
	ret_list = glist_remove(list, l);
	l = NULL;
      }
      else {
	l = GLIST_NEXT(l);
	if (l != NULL) {
	  *use = (use_info_t *) glist_top(l);
	}
	else {
	  *use = NULL;
	}
      }
    }
  }

  if (*use != NULL) {
    /*** found redundant stuff, update CID ***/
    UI_CID(*use) = get_cid();
  }

  return ret_list;
}


#ifdef _WHAT_THE_HECK_IS_THIS_
/*** find the region covering a statement locally ***/
/*** return NULL if none found ***/
/*** this will change when marios scheme is fully integrated ***/

static symboltable_t *find_region(expr_t *expr) {
  region_t *scope;
  symboltable_t *reg;

  if (expr == NULL) {
    return RMSCurrentRegion();
  }

  switch(T_TYPE(expr)) {
  case REDUCE:
  case FLOOD:
    scope = T_REGMASK(expr);
    if (scope) {
      reg = T_REGION_SYM(scope);
      if (!symtab_is_qreg(reg)) {
	return reg;
      }
    }
  default:
    return find_region(T_PARENT(expr));
  }
}
#endif

/*************************************/
/*** remove list item for AT_PRIME ***/
/*************************************/

static glist remove_at_prime(glist list, use_info_t ** use) {
  glist l, ret_list;

  ret_list = list;
  if (list == NULL) {
    *use = NULL;
  } else {
    l = list;
    *use = (use_info_t *) glist_top(l);
    while (l != NULL) {
      if (UI_ATTYPE(*use) == AT_PRIME) {
	ret_list = glist_remove(list, l);
	l = NULL;
      } else {
	l = GLIST_NEXT(l);
	if (l != NULL) {
	  *use = (use_info_t *) glist_top(l);
	} else {
	  *use = NULL;
	}
      }
    }
  }

  return ret_list;
}


static void uncombine_comm(use_info_t *use) {
  statement_t *s, *inserted;
  comm_info_t *ci, *old_ci;
  int i, pos, found;

  /*** get the first COMM statement ***/
  inserted = UI_INSERTED(use);
  s = inserted;
  while ((s != NULL) && T_COMM_TYPE(s) != C_NEW) {
    s = T_COMM_PREV(s);
  }
  INT_COND_FATAL((s != NULL), NULL, "C_NEW not found.");

  found = FALSE;
  pos = 1;
  ci = T_COMM_INFO(s);
  while (!found && ci != NULL) {
    /*** don't worry about subtype since AT_PRIME shouldn't get us here ***/
    if (expr_equal(T_COMM_INFO_ENS(ci), expr_find_at(UI_EXPR(use)))) {
      found = TRUE;
    }
    else {
      ci = T_COMM_INFO_NEXT(ci);
      pos++;   /*** increment the position in the list ***/
    }
  }
  INT_COND_FATAL((ci != NULL), NULL, "Comm info not found.");

  /*** All statements pertaining to a particular comm should have ***/
  /***  the same structure, so removing the pos-th comm_info      ***/
  /***  from each statement will uncombine the use.  Notice that  ***/
  /***  for now, only the use associated with the C_NEW is used   ***/
  /***  in code generation, but we will maintain the information  ***/
  /***  for all the statements just in case.                      ***/
  for (s = inserted; s != NULL; s = T_COMM_NEXT(s)) {
    ci = T_COMM_INFO(s);
    for (i = 0; i < pos-1; i++) {
      /*** find the pointer to the "previous" use ***/
      ci = T_COMM_INFO_NEXT(ci);
      INT_COND_FATAL((T_COMM_INFO_NEXT(ci) != NULL), NULL, "Comm info NULL.");
    }
    old_ci = T_COMM_INFO_NEXT(ci);
    T_COMM_INFO_NEXT(ci) = T_COMM_INFO_NEXT(old_ci);

    /*** probably enough, though the dirs may be left ***/
    free_expr(T_COMM_INFO_ENS(old_ci));
    PFREE(old_ci, sizeof(comm_info_t));
  }

  /*** reset cid ***/
  UI_CID(use) = get_cid();

}


/***********************************************************************/
/*** return true if comm stmt involves any of the vars in the IN set ***/
/***********************************************************************/

static int is_inserted(set_t *inset, statement_t *s) {
  set_t *set;
  comm_info_t *comm_info;
  symboltable_t *var;

  if (T_TYPE(s) != S_COMM) {
    INT_FATAL(s, "Statement is not of type S_COMM");
  }

  for (comm_info = T_COMM_INFO(s);
       comm_info != NULL;
       comm_info = T_COMM_INFO_NEXT(comm_info)) {

    var = expr_find_root_pst(T_COMM_INFO_ENS(comm_info));
    if (var == NULL) {
      INT_FATAL(s, "No variable expression!");
    }

    for (set = inset; set != NULL; set = SET_NEXT(set)) {
      if (var == SET_VAR(set)) {
	return TRUE;
      }
    }
  }

  return FALSE;
}



/***
 *** Four Phased Communication Insertion Algorithm
 ***
 *** Phase 1 (DR - Destination Ready)
 ***         Ready to receive data in place (last use before @ use)
 ***
 *** Phase 2 (SR - Source Ready)
 ***         Begin sending data in place (last def before @ use)
 ***
 *** Phase 3 (DN - Destination Needed)
 ***         Receive data for @ use (blocking)
 ***
 *** Phase 4 (SV - Source Volatile)
 ***         Complete send before this point (first def after @ use)
 ***
 ***/

static statement_t *per_statement(statement_t *s, int **argv, int argc) {
  glist awaiting_phase1;
  glist doing_phase1;
  glist awaiting_phase2;
  glist finish_phase2 = NULL;
  glist awaiting_phase3;
  glist already_combined;
  use_info_t *use, *tmpU;
  statement_t *inserted;
  statement_t *last;
  set_t *inset;
  set_t *outset;
  genlist_t *glp;
  expr_t *direction;
  expr_t *at_expr;
  expr_t *at_expr2;
  attype_t attype;

  if ((s == NULL) || (T_TYPE(s) == S_NLOOP)) {
    return s;
  }

  if (argc != NUM_TR_ARGS) {
    INT_FATAL(s, "Don't know what arguments are being passed!");
  }

  DBS1(15, "Traversing statement %d...\n", T_LINENO(s));
  IFDB(25) {
    dbg_gen_stmt(stdout, s);
  }

  last = (statement_t *) argv[LAST_STMT];
  doing_phase1 = (glist) argv[DOING_PHASE1];
  awaiting_phase2 = (glist) argv[AWAITING_PHASE2];
  awaiting_phase3 = (glist) argv[AWAITING_PHASE3];
  already_combined = (glist) argv[ALREADY_COMBINED];

  DBS0(25, "*************** PHASE 1 ***************\n");
  awaiting_phase1 = NULL;
  for (doing_phase1 = glist_pop_head(doing_phase1, (void **) &inserted);
       inserted != NULL;
       doing_phase1 = glist_pop_head(doing_phase1, (void **) &inserted)) {
    if (is_inserted((lhscomm ? T_OUT(s) : T_IN(s)), inserted)) {
      insertDR(inserted, s, AFTER);
    } else {
      if (awaiting_phase1 == NULL) {
	awaiting_phase1 = glist_create(inserted, GLIST_NODE_SIZE);
      } else {
	glist_append(awaiting_phase1, inserted, GLIST_NODE_SIZE);
      }
    }
  }

  DBS0(25, "*************** PHASE 2 ***************\n");
  for (outset = (lhscomm ? T_IN(s) : T_OUT(s));
       outset != NULL; outset = SET_NEXT(outset)) {
    for (glp = SET_EXPRS(outset); glp != NULL; glp = G_NEXT(glp)) {
      /*** if (T_TYPEINFO_REG(G_EXP(glp))) ***/ {
	at_expr = G_EXP(glp);

	if (find_use_info(awaiting_phase2, SET_VAR(outset),
			  at_expr, COMPARE_DEPEND) != NULL) {
	  DBS1(20, "*** Need communication for %s...\n",
	       S_IDENT(SET_VAR(outset)));

	  /*** remove all pending comm for this variable ***/
	  for (awaiting_phase2 = remove_dependent(awaiting_phase2,
						  at_expr, &use);
	       use != NULL;
	       awaiting_phase2 = remove_dependent(awaiting_phase2,
						  at_expr, &use)) {
	    if (combine == TRUE) {
	      awaiting_phase2 = combine_similar(awaiting_phase2, use,
						&already_combined);
	    }

	    /*** save the USE to insert recv ***/
	    if (finish_phase2 == NULL) {
	      finish_phase2 = glist_create(use, GLIST_NODE_SIZE);
	    }
	    else {
	      finish_phase2 = glist_prepend(finish_phase2, use,
					    GLIST_NODE_SIZE);
	    }

	    if (pipeline == TRUE) {
	      if (lhscomm == 1) {
		INT_FATAL(NULL, "Pipelining not supported for LHS @'s\n");
	      }
	      /*** insert this send ***/
	      inserted = insertSR(use, s, AFTER);
	      for (tmpU = use; tmpU != NULL; tmpU = UI_NEXT(tmpU)) {
		UI_INSERTED(tmpU) = inserted;
	      }

	      if (is_inserted(T_IN(s), inserted)) {
		/*** insert this pre-recv, now ***/
		insertDR(inserted, s, AFTER);
	      }
	      else if (awaiting_phase1 == NULL) {
		/*** insert this pre-recv, later ***/
		awaiting_phase1 = glist_create(inserted, GLIST_NODE_SIZE);
	      }
	      else {
		glist_append(awaiting_phase1, inserted, GLIST_NODE_SIZE);
	      }
	    }
	  }
	}
	else if (find_use_info(already_combined, SET_VAR(outset),
			       at_expr, COMPARE_DEPEND) != NULL) {
	  /*** leave combined ***/
	  already_combined = remove_dependent(already_combined, at_expr, &use);
	}
      }
    }
  }

  for (finish_phase2 = glist_pop_head(finish_phase2, (void **) &use);
       use != NULL;
       finish_phase2 = glist_pop_head(finish_phase2, (void **) &use)) {
    /*** insert this recv ***/

    if (pipeline == TRUE) {
      if (lhscomm == 1) {
	INT_FATAL(NULL, "Pipelining not supported for LHS @'s\n");
      }
      inserted = insertDN((void *) UI_INSERTED(use), UI_RECV(use), BEFORE);

      /*** insert this post-send ***/
      if (UI_POSTSEND(use) == NULL) {
	insertSV(inserted, last, AFTER);
      }
      else {
	insertSV(inserted, UI_POSTSEND(use), BEFORE);
      }
    } else {
      insertDN((void *) use, UI_RECV(use), (lhscomm ? AFTER : BEFORE));
    }
  }

  DBS0(25, "*************** PHASE 3 ***************\n");
  for (inset = (lhscomm ? T_OUT(s) : T_IN(s));
       inset != NULL; inset = SET_NEXT(inset)) {
    /*** @ applied to some expression ***/
    for (glp = SET_EXPRS(inset); glp != NULL; glp = G_NEXT(glp)) {
      /*** check expressions ***/
      at_expr = G_EXP(glp);
      at_expr2 = expr_find_at(at_expr);
      if (at_expr2 != NULL) {
	if (T_SUBTYPE(at_expr2) == AT_WRAP) {
	  /*	  if (D_REG_NUM(S_DTYPE(T_TYPEINFO_REG(at_expr2))) > 2) {
	    INT_FATAL(s,"can't handle ^@ for dims greater than 2 yet");
	    }*/
	}
	direction = T_NEXT(T_OPLS(at_expr2));
	    
	attype = (attype_t)T_SUBTYPE(at_expr2);
	    
	use = NULL;
	if (combine == TRUE) {
	  already_combined = remove_dependent(already_combined,at_expr, &use);
	  if (use != NULL) {
	    /*** must un-combine ***/
	    uncombine_comm(use);
	  }
	}
	
	if (remove_redundant == TRUE) {
	  /*** combine with pending communication ***/
	  if (use == NULL) {
	    awaiting_phase2 =
	      remove_redundant_info(awaiting_phase2, SET_VAR(inset),
				    at_expr, RMSCurrentRegion(), &use);
	  }
	}
	
	if (use == NULL) {
	  int blah;
	  /*** new/separate communication ***/
	  use = find_use_info(awaiting_phase3, SET_VAR(inset),
			      at_expr, COMPARE_EQUAL);
	  if (use == NULL) {
	    use = new_use_info(SET_VAR(inset), at_expr);
	  } else {
	    use = copy_use_info(use);
	  }
	  
	  add_use_info(use, at_expr, RMSCurrentRegion(),
		       RMSCurrentMask(&blah));
	}
	
	/*** add this direction to the use ***/
	add_dir_info(use, direction, s, attype);
	
	if (awaiting_phase2 == NULL) {
	  awaiting_phase2 = glist_create(use, GLIST_NODE_SIZE);
	} else {
	  glist_append(awaiting_phase2, use, GLIST_NODE_SIZE);
	}
      }
    }
  }
  /*** DO COMM FOR MSCAN (AT_PRIME) HERE AND NOW!                   ***/
  /*** Insert COMM as follows:                                      ***/
  /***        C_NEW                                                 ***/
  /***        C_DR                                                  ***/
  /***        C_DN                                                  ***/
  /***        S_MLOOP (MLOOP_MSCAN)                                 ***/
  /***        C_SR                                                  ***/
  /***        C_SN                                                  ***/
  /***        C_OLD                                                 ***/
  /*** NOTE: By inserting the MSCAN comm here, avoid keeping around ***/
  /***       state for later statements.  It's not perfect -- for   ***/
  /***       example, I could make special cases for AT_PRIME in    ***/
  /***       phase 3, but this way works and is just fine for the   ***/
  /***       common case (AT_NORMAL).                               ***/
  if ((T_TYPE(s) == S_MLOOP) && MLOOP_IS_MSCAN(s)) {
    for (awaiting_phase2 = remove_at_prime(awaiting_phase2, &use);
	 use != NULL;
	 awaiting_phase2 = remove_at_prime(awaiting_phase2, &use)) {
      if (combine == TRUE) {
	awaiting_phase2 = combine_similar(awaiting_phase2, use, NULL);
      }
      insert_from_info(use, C_NEW, 
		       (T_MLOOP_PREPRE(T_MLOOP(s),0)?
			T_MLOOP_PREPRE(T_MLOOP(s),0):
			(T_MLOOP_PREPRE(T_MLOOP(s),0) =
			 alloc_statement(S_NULL,T_LINENO(s),T_FILENAME(s)))),
		       (T_MLOOP_POSTPOST(T_MLOOP(s),0)?
			T_MLOOP_POSTPOST(T_MLOOP(s),0):
			(T_MLOOP_POSTPOST(T_MLOOP(s),0) =
			 alloc_statement(S_NULL,T_LINENO(s),T_FILENAME(s)))),
		       XXX);
      T_MLOOP_PREPRE(T_MLOOP(s),0) = 
	first_stmt_in_stmtls(T_MLOOP_PREPRE(T_MLOOP(s),0));
    }
  }

  DBS0(25, "*************** PHASE 4 ***************\n");
  for (outset = (lhscomm ? T_IN(s) : T_OUT(s));
       outset != NULL; outset = SET_NEXT(outset)) {
    for (glp = SET_EXPRS(outset); glp != NULL; glp = G_NEXT(glp)) {
      if (T_TYPEINFO_REG(G_EXP(glp))) {
	at_expr = G_EXP(glp);
	use = find_use_info(awaiting_phase3, SET_VAR(outset),
			    at_expr, COMPARE_EQUAL);

	if (use == NULL) {
	  use = new_use_info(SET_VAR(outset), at_expr);
	  if (awaiting_phase3 == NULL) {
	    awaiting_phase3 = glist_create(use, GLIST_NODE_SIZE);
	  } else {
	    awaiting_phase3 = glist_prepend(awaiting_phase3, use, GLIST_NODE_SIZE);
	  }
	}
	UI_POSTSEND(use) = s;
      }
    }
  }

  argv[DOING_PHASE1] = (int *) awaiting_phase1;
  argv[AWAITING_PHASE2] = (int *) awaiting_phase2;
  argv[AWAITING_PHASE3] = (int *) awaiting_phase3;
  argv[ALREADY_COMBINED] = (int *) already_combined;

  return s;
}


static int RegionIsDynamic(expr_t *reg) {
  switch (T_SUBTYPE(reg)) {
  case DYNAMIC_REGION:
  case INTERNAL_REGION:
    return 1;
  case SIMPLE_REGION:
    return 0;
  case AT_REGION:
  case IN_REGION:
  case OF_REGION:
  case BY_REGION:
    return RegionIsDynamic(T_OPLS(reg));
  default:
    INT_FATAL(NULL, "Unexpected region type in RegionIsDynamic()");
    break;
  }
  return 0;
}


/**************************************************************************/
/*** resolve pending comm at the top of a basic block or dynamic region ***/
/**************************************************************************/

static statement_t *resolve_pending(statement_t *s, int **argv, int argc) {
  glist preprocess_phase1;
  glist awaiting_phase1 = NULL;
  glist awaiting_phase2 = NULL;
  glist finish_phase2 = NULL;
  glist do_not_do_phase2 = NULL;
  glist already_combined = NULL;
  use_info_t *use, *tmpU;
  statement_t *inserted;
  statement_t *insertion_pt;
  int is_dyn_reg;            /*** DYNAMIC REGIONS STUFF IS NOT UP TO DATE ***/
  int is_ext_dep; /*** sungeun *** 08/07/01 *** see below ***/
  statement_t *first, *real_first, *last;

  if (s == NULL) {
    return s;
  }

  if (argc != NUM_TR_ARGS) {
    INT_FATAL(s, "Don't know what arguments are being passed!");
  }

  is_dyn_reg = FALSE;
#ifdef BLAH
  /*** leave this here in case region scopes DO NOT break bb boundarys ***/
  if (T_TYPE(s) == S_REGION_SCOPE) {
    if ((T_REGION_SYM(T_REGION(s))) &&
	(RegionIsDynamic(T_REGION_SYM(T_REGION(s))))) {
      is_dyn_reg = TRUE;
    }
  }
#endif

  /*** sungeun *** 08/07/01 ***
   *** comm calls cannot span "fixed position" function calls
   ***  these include standard context, external functions, I/O
   ***  statements halt, 
   ***
   *** the outset of a statement containing such calls
   ***  should have been set to the external universal set,
   ***  i.e., SET_VAR(T_OUT(s)) == ExternalDep.
   ***
   *** note that T_PREV(s) is the statement in question
   ***
   *** sungeun *** 07/07/03
   *** not sure if this works in the case of LHS @'s, but
   ***  for now, they are not being pipelined, so it's correctness
   ***  is not being tested.
   ***
   ***/
  is_ext_dep = FALSE; /* SJD 10/16/03: Otherwise uninitialized
			 Commented out else initialization */
  if (((lhscomm == 0) && (T_PREV(s) != NULL)) ||
      ((lhscomm == 1) && (T_NEXT(s) != NULL))) {
    set_t *out;
    for (out = (lhscomm ? T_OUT(T_NEXT(s)) : T_OUT(T_PREV(s)));
	 out != NULL; out = SET_NEXT(out)) {
      if (SET_VAR(out) == ExternalDep) {
	/*** all communication must be resolved here and now ***/
	is_ext_dep = TRUE;
	break;
      }
    }
  }
/*   else { */
/*     *** we're at the top of the bb anyway, so who cares *** */
/*     is_ext_dep = FALSE; */
/*   } */

  first = (statement_t *) argv[FIRST_STMT];
  last = (statement_t *) argv[LAST_STMT];

  if ((!is_dyn_reg) && (!is_ext_dep) && (s != (lhscomm ? last : first))) {
    /*** not dynamic region scope and
	 no external dependence and
	 not at top of bb ***/
    return first;
  }

  if (is_dyn_reg) {
    insertion_pt = T_BODY(T_REGION(s));
  } else if (s == (lhscomm ? last : first)) {
    insertion_pt = s;
  } else if (is_ext_dep) {
    insertion_pt = s;
  } else {
    INT_FATAL(NULL, "How the heck did we get here?!?!");
  }

  DBS0(15, "*** Resolving remaining uses...\n");

  preprocess_phase1 = (glist) argv[DOING_PHASE1];
  awaiting_phase2 = (glist) argv[AWAITING_PHASE2];
  /*** don't really care about this, but we need it as a dummy ***/
  already_combined = (glist) argv[ALREADY_COMBINED];

  /*** fix up phase 1 ***/
  for (preprocess_phase1 = glist_pop_head(preprocess_phase1,
					  (void **) &inserted);
       inserted != NULL;
       preprocess_phase1 = glist_pop_head(preprocess_phase1,
					  (void **) &inserted)) {
    if (awaiting_phase1 == NULL) {
      awaiting_phase1 = glist_create(inserted, GLIST_NODE_SIZE);
    } else {
      awaiting_phase1 = glist_prepend(awaiting_phase1,
				      inserted, GLIST_NODE_SIZE);
    }
  }

  /*** insert pre_recvs ***/
  for (awaiting_phase1 = glist_pop_head(awaiting_phase1, (void **) &inserted);
       inserted != NULL;
       awaiting_phase1 = glist_pop_head(awaiting_phase1, (void **) &inserted)) {

    insertDR(inserted, insertion_pt, (lhscomm ? AFTER : BEFORE));

  }

  /*** insert sends ***/
  for (awaiting_phase2 = glist_pop_head(awaiting_phase2, (void **) &use);
       use != NULL;
       awaiting_phase2 = glist_pop_head(awaiting_phase2, (void **) &use)) {

    /*********************************************/
    /*** if resolving comm at the top of a bb: ***/
    /***        resolve everything             ***/
    /***              OR                       ***/
    /*** if exiting a dyn reg scope:           ***/
    /***   resolve comm with dynamic region    ***/
    /*********************************************/
    if (!is_dyn_reg || (is_dyn_reg && RegionIsDynamic(UI_REGION(use)))) {

      if (combine == TRUE) {
	awaiting_phase2 = combine_similar(awaiting_phase2, use,
					  &already_combined);
      }

      /*** save the USE to insert recv ***/
      if (finish_phase2 == NULL) {
	finish_phase2 = glist_create(use, GLIST_NODE_SIZE);
      }
      else {
	finish_phase2 = glist_prepend(finish_phase2, use, GLIST_NODE_SIZE);
      }

      if (pipeline == TRUE) {
	if (lhscomm == 1) {
	  INT_FATAL(NULL, "Pipelining not supported for LHS @'s\n");
	}
	/*** insert this send ***/
	inserted = insertSR(use, insertion_pt, BEFORE);
	for (tmpU = use; tmpU != NULL; tmpU = UI_NEXT(tmpU)) {
	  UI_INSERTED(tmpU) = inserted;
	}

	/*** insert this pre-recv, later ***/
	if (awaiting_phase1 == NULL) {
	  awaiting_phase1 = glist_create(inserted, GLIST_NODE_SIZE);
	}
	else {
	  awaiting_phase1 = glist_prepend(awaiting_phase1,
					  inserted, GLIST_NODE_SIZE);
	}
      }
    }
    else {
      /*** this statement is a dynamic region, but the comm ***/
      /***  does not use this dynamic region ***/
      if (do_not_do_phase2 == NULL) {
	do_not_do_phase2 = glist_create(use, GLIST_NODE_SIZE);
      } else {
	do_not_do_phase2 = glist_prepend(do_not_do_phase2,
					use, GLIST_NODE_SIZE);
      }
    }
  }

  for (finish_phase2 = glist_pop_head(finish_phase2, (void **) &use);
       use != NULL;
       finish_phase2 = glist_pop_head(finish_phase2, (void **) &use)) {
    /*** insert this recv ***/

    if (pipeline == TRUE) {
      if (lhscomm == 1) {
	INT_FATAL(NULL, "Pipelining not supported for LHS @'s\n");
      }

      inserted = insertDN((void *) UI_INSERTED(use), UI_RECV(use), BEFORE);

      /*** insert this post-send ***/
      if (UI_POSTSEND(use) == NULL) {
	insertSV(inserted, last, AFTER);
      }
      else {
	insertSV(inserted, UI_POSTSEND(use), BEFORE);
      }
    }
    else {
      inserted = insertDN((void *) use, UI_RECV(use),
			  (lhscomm ? AFTER : BEFORE));
    }
  }

  /*** insert pre_recvs ***/
  for (awaiting_phase1 = glist_pop_head(awaiting_phase1, (void **) &inserted);
       inserted != NULL;
       awaiting_phase1 = glist_pop_head(awaiting_phase1, (void **) &inserted)) {

    insertDR(inserted, insertion_pt, (lhscomm ? AFTER : BEFORE));

  }

  argv[DOING_PHASE1] = NULL;
  argv[AWAITING_PHASE2] = (int *) do_not_do_phase2;

  /*** why do we care about this? ***/
  real_first = first;
  while (T_PREV(real_first) != NULL) {
    real_first = T_PREV(real_first);
  }

  return real_first;
}


static statement_t *
traverse_bb_stmtls(statement_t *stmtls, int direction,
		   statement_t *(*pre)(statement_t *, int **, int),
		   statement_t *(*post)(statement_t *, int **, int),
		   int **argv, int argc)
{
  statement_t *s, *first, *next, *r_stmtls;

  for (s = stmtls; s != NULL; s = next) {
    if (T_TYPE(s) == S_NLOOP) {
      /*** find the "first" statement of the NLOOP ***/
      first = T_NLOOP_BODY(T_NLOOP(s));
      if (direction == REVERSE) {
	next = T_NEXT(first);
	while (next != NULL) {
	  first = next;
	  next = T_NEXT(first);
	}
      }
      traverse_bb_stmtls(first, direction, pre, post, argv, argc);
    }

    if (direction == FORWARD) {
      next = T_NEXT(s);            /*** move forward ***/
    }
    else {
      next = T_PREV(s);            /*** move backward ***/
    }

    if (pre != NULL) {             /*** apply function before traversal ***/
      r_stmtls = (*pre)(s, argv, argc);
    }

    if (post != NULL) {            /*** apply function after traversal ***/
      r_stmtls = (*post)(s, argv, argc);
    }

  }

  return r_stmtls;
}


static statement_t *traverse_bb(statement_t *stmtls, int direction,
				statement_t *(*pre)(statement_t *,int **,int),
				statement_t *(*post)(statement_t *,int **,int),
				int **argv, int argc) {
  statement_t* next;
  statement_t *first, *last;

  if (stmtls == NULL) {
    return stmtls;
  }

  first = stmtls;
  last = stmtls;
  next = T_NEXT(last);
  while (next != NULL) {
    last = next;
    next = T_NEXT(last);
  }

  if (T_PREV(first) != NULL || T_NEXT(last) != NULL) {
    INT_FATAL(NULL, "T_PREV(first) or T_NEXT(last) is not NULL!");
  }

  argv[FIRST_STMT] = (int *) first;
  argv[LAST_STMT] = (int *) last;

  if (direction == FORWARD) {
    /*** last = T_NEXT_LEX(last); ***/
  }
  else if (direction == REVERSE) {
    /* swap first and last */
    first = (statement_t *) argv[LAST_STMT];
    /*** last = T_PREV_LEX((statement_t *) argv[FIRST_STMT]); ***/
  }
  else {
    INT_FATAL(NULL, "Incorrect traversal direction!");
  }

  return traverse_bb_stmtls(first, direction, pre, post, argv, argc);
}


static statement_t *comm_per_bb(statement_t *stmtls) {
  int **argv;
  int argc;
  int i, direction;

  if (stmtls == NULL) {
    return stmtls;
  }

  DBS2(15, "\nIn Basic Block at line %d in function %s\n",
       T_LINENO(stmtls), S_IDENT(T_FCN(T_PARFCN(stmtls))));

  argc = NUM_TR_ARGS;
  argv = (int **) PMALLOC(argc*sizeof(int *));

  for (i = 0; i < NUM_TR_ARGS; i++) {
    argv[i] = NULL;
  }

  if (lhscomm == 0) {
    direction = REVERSE;
  } else {
    direction = FORWARD;
  }

  return traverse_bb(stmtls,direction,per_statement,resolve_pending,argv,argc);
}


static void comm_per_module(module_t *mod) {
  /* set T_IN_MLOOP() pointer */
  set_mloop(mod);
  RMSInit();
  bb_traversal(mod, comm_per_bb, BBTR_NONE);
  RMSFinalize();
}


/*** free up memory used by work data structures ***/
static void freeUIstack(void) {
}


int call_insert_comm(module_t *mod, char *options) {
  max_latency = FALSE;
  switch (comm_insert) {
  case NAIVE_COMM:
    remove_redundant = FALSE;
    combine = FALSE;
    pipeline = FALSE;
    break;
  case OPTIMIZED_COMM:
    remove_redundant = TRUE;
    combine = TRUE;
    pipeline = TRUE;
    break;
  default:
    INT_FATAL(NULL, "Incorrect communication insertion type (%d)", 
	      comm_insert);
    break;
  }

  /*** NOTICE ***/
  /*** flags in the pass file override the command line option -c0 ***/
  if (get_flag_arg (options, 'r') == 0) {
    /*** remove redundant communication ***/
    remove_redundant = TRUE;
  }

  if (get_flag_arg (options, 'c') == 0) {
    /*** combine message of different ensemble ***/
    combine = TRUE;
  }

  if (get_flag_arg (options, 'p') == 0) {
    /*** pipeline communication***/
    pipeline = TRUE;
  }

  if (get_flag_arg (options, 'l') == 0) {
    /*** combine message to maximize latency hiding potential ***/
    max_latency = TRUE;
  }

  if (get_flag_arg (options, 'L') == 0) {
    /*** do LHS @ comm insertion ***/
    lhscomm = TRUE;

    /*** turn off pipelining, since it may interfere with RHS comm ***/
    pipeline = FALSE;

    DBS0(1, "Inserting LHS comm\n");
  } else {
    DBS0(1, "Inserting RHS comm\n");
  }

#if (!RELEASE_VER)
  IFDB(2) {
    if (remove_redundant == TRUE) {
      DBS0(2, "\tremoving redundant communication (default)\n");
    }
    if (combine == TRUE) {
      DBS0(2, "\tcombining communication (default)\n");
    }
    if (pipeline == TRUE) {
      DBS0(2, "\tpipelining communication (default)\n");
    }
    if (max_latency == FALSE) {
      DBS0(2, "\tmaximizing combining (default)\n");
    }
    else {
      DBS0(2, "\tcombining to maximize latency hiding\n");
    }
  }
#endif

  /***********************************/
  /*** INSERT COMM *** GO! GO! GO! ***/
  /***********************************/
  traverse_modules(mod, TRUE, comm_per_module, NULL);


#if (!RELEASE_VER)
  IFDB(20) {
    /*** spit out statistics about table usage ***/
    int at, i, j, k;
    int total = 0;

    for (at = 0; at < AT_REL_LAST; at++) {
      for (i = 0; i < CID_LAST; i++) {
	printf("{\n");
	for (j = 0; j < C_WRAP_PRERECV; j++) {
	  printf("  { ");
	  for (k = 0; k < C_WRAP_PRERECV; k++) {
	    printf("%4d ", table_stats[at][i][j][k]);
	    if (table_stats[at][i][j][k] > 0) {
	      total++;
	    }
	  }
	  printf("}\n");
	}
	printf("}\n");
      }
      printf("{\n");
    }
    printf("\n*** %.2f%% table coverage ***\n",
	   100.0*((double) total)/
	   (AT_REL_LAST*CID_LAST*C_WRAP_PRERECV*C_WRAP_PRERECV));
  }
#endif

  freeUIstack();

  return 0;
}
