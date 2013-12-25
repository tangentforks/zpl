/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <assert.h>
#include "../include/Cgen.h"
#include "../include/error.h"
#include "../include/symtab.h"
#include "../include/inout.h"
#include "../include/set.h"
#include "../include/setmac.h"
#include "../include/symmac.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"
#include "../include/global.h"
#include "../include/stmtutil.h"
#include "../include/macros.h"
#include "../include/const.h"
#include "../include/expr.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/genlist.h"
#include "../include/runtime.h"


set_t *alloc_set(settype type) {
  set_t *new = (set_t *)PMALLOC(sizeof(set_t));

  DB1(70,"alloc_set(%d)\n",(int)type);

  SET_TYPE(new) = type;

  SET_DIR_IDENT(new) = NULL;

  SET_VAR(new) = NULL;
  SET_NEXT(new) = NULL;
  SET_IMOD(new) = 0;
  SET_EXPRS(new) = NULL;

  return new;
}

void free_set(set_t *setnode) {
  /*** should free SET_DIR_IDENT (genlist_t *), too ***/
  /*** should free SET_EXPRS (genlist_t *), too ***/
  if (setnode != NULL) {
    /* only frees 1st node; what about the rest? */
    PFREE((char *) setnode, sizeof(set_t));
  }
}


static set_t *s_member(set_t *node, set_t *set) {
  DBS0(100, "s_member called\n");

  if (set == NULL) {
    DB0(100, "s_member exiting - false \n");
    return(FALSE);
  }
  
  if (SET_VAR(node) == SET_VAR(set)) {
    DB0(100, "s_member exiting - true\n");
    return(set);
  }
  DB0(100, "s_member recursive call\n");
  return(s_member(node, SET_NEXT(set)));
}


set_t *s_union(set_t *set1,set_t *set2) {
  int abort;
  set_t *temp, *temp2, *first1, *set3;
  genlist_t *g1, *g2, *last2;
  
  IFDB(100) {
    DBS0(100, "s_union called\n");
    DBS0(100, "Sets are \n");
    p_set(zstdout, set1);
    p_set(zstdout, set2);
  }

  if (set1 == NULL) {
    return(set2);
  }
  if (set2 == NULL) {
    return(set1);
  }
  if (SET_TYPE(set1) == ALL) {
    free_set(set2);
    return(set1);
  }
  if (SET_TYPE(set2) == ALL) {
    free_set(set1);
    return(set2);
  }
  set3 = set2;
  
  for (first1 = set1; first1 != NULL; ) {
    temp = SET_NEXT(first1);
    if (! (temp2 = s_member(first1, set3))) {
      SET_NEXT(first1) = set3;
      set3 = first1;
    } else {
      SET_IMOD(temp2) &= SET_IMOD(first1);
      /* combine SET_DIR_IDENT genlists */
      if (SET_DIR_IDENT(first1) != NULL) {
	if (SET_DIR_IDENT(temp2) != NULL) {
	  g1 = SET_DIR_IDENT(temp2);
	  g2 = G_NEXT(g1);
	  while (g2 != NULL) {
	    g1 = g2;
	    g2 = G_NEXT(g2);
	  }
	  G_NEXT(g1) = SET_DIR_IDENT(first1);
	}
	else {
	  SET_DIR_IDENT(temp2) = SET_DIR_IDENT(first1);
	}
	SET_DIR_IDENT(first1) = NULL;
      }
      /* combine SET_EXPRS genlists */
      if (SET_EXPRS(first1) != NULL) {
	if (SET_EXPRS(temp2) != NULL) {
	  /* get last node */
	  last2 = SET_EXPRS(temp2);
	  while (G_NEXT(last2)) last2 = G_NEXT(last2);

	  /* for each elem in SET_EXPRS(first1) add if unique */
	  g1 = SET_EXPRS(first1);
	  while(g1) {
	    abort = FALSE;
	    for (g2 = SET_EXPRS(temp2); g2; g2 = G_NEXT(g2)) {
	      if (expr_equal(G_EXP(g1), G_EXP(g2))) {
		abort = TRUE;
		break;
	      }
	    }
	    if (!abort) {             /* no duplicate, add g1 node */
	      G_NEXT(last2) = g1;
	      last2 = g1;
	      g1 = G_NEXT(g1);
	      G_NEXT(last2) = NULL;
	    } else {                  /* duplicate, ignore g1 node */
	      /* free node? */
	      g1 = G_NEXT(g1);
	    }
	  }
	} else {
	  SET_EXPRS(temp2) = SET_EXPRS(first1);
	}
      }

      free_set(first1);
    }
    first1 = temp;
  }
  IFDB(100) {
    DBS0(100, "set union is \n");
    p_set(zstdout, set3);
    DBS0(100, "s_union exiting\n");
  }
  return(set3);
}


void set_dir(expr_t *expr, dir_depend *dir_info) {
  genlist_t *new_g;

  if (T_TYPE(expr) != BIAT)
    return;

  /*** prepend the symtab entry of the direction to the list ***/
  /*** of directions, INCLUDING NULL directions ***/
  new_g = alloc_gen();
  G_EXPR(new_g) = T_NEXT(T_OPLS(expr));
  G_NEXT(new_g) = DEP_DIR_IDENT(*dir_info);
  DEP_DIR_IDENT(*dir_info) = new_g;
}


set_t *s_copy(set_t *s) {
  set_t *new;
  
  if (s == (set_t *) NULL) {
    return ((set_t *) NULL);
  }
  new = alloc_set(SET_TYPE(s));
  SET_DIR_IDENT(new) = copy_genlist(SET_DIR_IDENT(s));

  switch (SET_TYPE(new)) {
  case VAR:
    SET_VAR(new) = SET_VAR(s);
    SET_IMOD(new) = SET_IMOD(s);
    break;
  case COMPL:
    SET_VAR(new) = SET_VAR(s);
    SET_IMOD(new) = SET_IMOD(s);
    break;
  case ALL:
    SET_VAR(new) = (symboltable_t *) NULL;
    SET_IMOD(new) = SET_IMOD(s);
    break;
  default:
    INT_FATAL(NULL, "s_copy: unknown type of s");
    break;
  }
  SET_EXPRS(new) = copy_genlist(SET_EXPRS(s));
  SET_NEXT(new) = s_copy(SET_NEXT(s));
  return(new);
}


void p_set(FILE *fp,set_t *set) {
  set_t	*first; 
  int	count;

  DB0(100,"p_set(,)\n");

  count = 0;
  for (first = set; first != NULL; first = SET_NEXT(first), count++) {
    p_setelement(fp, first);
    fprintf(fp, " ");
    if (count == 20) {
      fprintf(fp, "\n");
      count = 0;
    }
  }
  DB0(100,"p_set(,) exiting\n");
}


static void print_genlist(FILE *fp, genlist_t *list) {
  while (list) {
    gen_expr(fp, G_EXPR(list));
    list = G_NEXT(list);
  }
}

void p_setelement(FILE *fp,set_t *set) {
  symboltable_t *svar;
  genlist_t *node;

  DB0(100,"p_setelement(,)\n");

  if (SET_TYPE(set) == ALL) {
    fprintf(fp, "Everything");
  } else {
    svar = SET_VAR(set);
    fprintf(fp, "%s", S_IDENT(svar));
      fprintf(fp, "{");
    /* print genlist - interpret elements as exprs */
    for (node = SET_EXPRS(set); node; node = G_NEXT(node)) {
      dbg_gen_expr(fp, G_EXP(node));
      if (G_NEXT(node))
	fprintf(fp,",");
    }
    fprintf(fp, "}");
    fprintf(fp, "(");
    print_genlist(fp, SET_DIR_IDENT(set));
    fprintf(fp, ")");
  }
  DB0(100,"p_setelement(,) exiting\n");
}



static statement_t *found(stmtlist_t *list,statement_t *stmt) {
  stmtlist_t *temp;

  for (temp = list; temp != NULL; temp = LIST_NEXT(temp)) {
    if (LIST_STMT(temp) == stmt)
      return (LIST_STMT(temp));
  }
  return(NULL);
}


static void addinone(FILE *fp,set_t *set,statement_t *stmt) {
  symboltable_t	*var;
  stmtlist_t	*node;
  statement_t	*s;

  DBS0(100,"addinone(,,)\n");

  if (SET_TYPE(set) == ALL) {
    DBS0(100,"addinone(,,) exiting, type ALL\n");
    return;
  }
  var = SET_VAR(set);
  s = found(S_INLIST(var), stmt);
  if (s == NULL) {
    node = alloc_stmtlist(stmt, S_INLIST(var));
    S_INLIST(var) = node;
  }
  DBS0(100,"addinone(,,) exiting\n");
}


static void prinstmtlist(FILE *fp,symboltable_t *stp) {
  stmtlist_t	*temp;

  fprintf(fp, "prinstmtlist()\n");
  
  fprintf(fp, "Inlist for var %s\n", S_IDENT(stp));
  for (temp = S_INLIST(stp); temp != NULL; temp = LIST_NEXT(temp)) {
    dbg_gen_stmt(fp, LIST_STMT(temp));
  }
  
  fprintf(fp, "prinstmtlist() exiting\n");
}


void addinset(FILE *fp,set_t *set,statement_t *stmt) {
  set_t *temp;

  DBS0(100,"addinset(,,)\n");

  for (temp = set; temp != NULL; temp = SET_NEXT(temp)) {
    addinone(fp, temp, stmt);
    IFDB(100) {
      prinstmtlist(fp, SET_VAR(temp));
    }
  }
  DBS0(100,"addinset(,,) exiting\n");
}


static void addoutone(FILE *fp,set_t *set,statement_t *stmt) {
  symboltable_t	*var;
  stmtlist_t	*node;
  statement_t	*s;

  DBS0(100, "addoutone called\n");
  if (SET_TYPE(set) == ALL) {
    return;
  }
  var = SET_VAR(set);
  s = found(S_OUTLIST(var), stmt);
  if (s == NULL) {
    node = alloc_stmtlist(stmt, S_OUTLIST(var));
    S_OUTLIST(var) = node;
  }
  DBS0(100, "addoutone exiting\n");
}


static void proutstmtlist(FILE *fp,symboltable_t *stp) {
  stmtlist_t	*temp;

  fprintf(fp, "proutstmtlist called\n");
  fprintf(fp, "Outlist for var %s\n", S_IDENT(stp));
  for (temp = S_OUTLIST(stp); temp != NULL; temp = LIST_NEXT(temp)) {
    dbg_gen_stmt(fp, LIST_STMT(temp));
  }
  fprintf(fp, "proutstmtlist exiting\n");
}


void addoutset(FILE *fp,set_t *set,statement_t *stmt) {
  set_t *temp;

  DBS0(100, "addoutset called\n");
  for (temp = set; temp != NULL; temp = SET_NEXT(temp)) {
    addoutone(fp, temp, stmt);
    IFDB(100) {
      proutstmtlist(fp, SET_VAR(temp));
    }
  }
  DBS0(100, "addoutset exiting\n");
}
