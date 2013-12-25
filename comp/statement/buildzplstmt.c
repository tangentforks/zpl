/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***************************************************************************
 * ZPL
 *
 * Additional parsing support functions for building the symbol table (ST)
 * & parse tree (PT).
 *
 * George Forman	4/17/93
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/datatype.h"
#include "../include/dbg_code_gen.h"
#include "../include/error.h"
#include "../include/global.h"	/* includes: const.h struct.h symtab.h */
#include "../include/cglobal.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/macros.h"
#include "../include/parsetree.h" /* includes: iostmt.h constant.h pfmask.h */
#include "../include/treemac.h"
#include "../include/symbol.h"
#include "../include/parser.h"
#include "../include/db.h"
#include "../include/statement.h"
#include "../include/stmtutil.h"/* import: cat_stmt_ls() */
#include "../include/comm.h"	/* import: get_cid() for unique comm IDs */
#include "../include/buildsym.h"
#include "../include/scan.h"
#include "../include/dimension.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/buildsymutil.h"
#include "../include/dtype.h"
#include "../include/expr.h"
#include "../include/init.h"
#include "../include/genlist.h"
#include "../include/symboltable.h"
#include "../include/runtime.h"
#include "../include/typeinfo.h"
#include "../include/value.h"

/* globals from global.h */

int maxdim = 0;


/* BLC -- new stuff to handle anonymous records */

static int anontypeID = 0;
static int dircounter=1;


/* 
 * The following must match the order of the enum type:  symbolclass 
 * in symtab.h 
 */
char    *symbolnames[] = {
	"variable",
	"constant",	
	"literal",	
	"type",		
	"subprogram",	
	"parameter",	
	"component",	
	"enum element",	
	"unknown",
	"distribution",
	"region",
	"enum tag",	
	"structure",	
	"variant"	
};


/****************************************************************
 * helper functions
 ****************************************************************/

/* return the rank of a dimlist */

static int count_rank(dimension_t *list) {
  int count = 0;
  for ( ; list!=NULL; list= DIM_NEXT(list)) /* doesn't modify the list */
    count++;

  return count;
}


/* Function: arrayisfile
 * Return: 1 if datatype is of type DT_FILE at array nesting level level
 *         0 otherwise
 * echris  7-20-94
 */

static int arrayisfile(datatype_t *datatype, int level) {
  if (level == 0) {
    if (D_CLASS(datatype) == DT_FILE)
      return 1;
    else 
      return 0;
  } else {
    if (D_CLASS(datatype) == DT_ARRAY) {
      return (arrayisfile(D_ARR_TYPE(datatype), level-1));
    } else 
      return 0;
  }
}


/* Function: isfile2
 * Return: 1 if expr eventually evaluates to DT_FILE 
 *         0 otherwise
 * Note: level is the array nesting level
 * echris  7-20-94
 */

static int isfile2(expr_t *expr, int level) {
  if (expr==NULL) {
    return 0;
  }

  switch(T_TYPE(expr)) {

  case BIDOT:
    if (T_IDENT(expr) &&
	T_TYPE(T_IDENT(expr)))
      return (arrayisfile(T_TYPE(T_IDENT(expr)), level));
    else
      return 0;
    
  case VARIABLE:
    if (T_IDENT(expr) &&
	T_TYPE(T_IDENT(expr)) &&
	(D_CLASS(T_TYPE(T_IDENT(expr))) == DT_FILE))
      return 1;
    else {
      if (level > 0) {
	return (arrayisfile(T_TYPE(T_IDENT(expr)), level));
      } else
	return 0;
    }

  case ARRAY_REF:
    return (isfile2(T_OPLS(expr), level+1));

  default:
    return 0;
  }    
}


/* Function: isfile
 * Return: 1 if expr eventually evaluates to DT_FILE 
 *         0 otherwise
 * Note: this function just calls isfile2() with level=0
 * echris  7-20-94
 */

static int isfile(expr_t *expr) {
  return (isfile2(expr, 0));
}


void check_name_conflict(char *name) {
  int prefixlen;

  prefixlen = strlen(ZPLprefix);
  if (name[0] == '_') {
    if (strncmp(name,ZPLprefix,prefixlen) == 0) {
      if (is_reserved_C(name+prefixlen)) {
	return;
      }
    }
    if (firstsemi != 1) {
      USR_WARNX(yylineno, in_file, "Names that begin with an underscore may "
		"conflict with\n\tcompiler-generated names - %s",name);
    }
  }
}


void check_name_list_conflict(symlist_t *namelist) {
  while (namelist != NULL) {
    check_name_conflict(SYM_IDENT(namelist));
    namelist = SYM_NEXT(namelist);
  }
}


/****************************************************************
 * misc build_ functions:
 ****************************************************************/

/*
 * Description of 'intlist' type:
 * int  intlist[MAXRANK+1];
 * intlist elements are stored in intlist[0..MAXRANK-1].
 * The dimension of this intlist is stored in intlist[MAXRANK].
 */




int *build_intlist(int *intlist,int i) {  /* append integer i to intlist */
  int j;

  DB1(50, "build_intlist - %d\n", i);

  if (intlist == NULL) {
    intlist = (int *)PMALLOC(sizeof(int)*(MAXRANK+1));
    for (j=0; j<=MAXRANK; j++)
      intlist[j] = 0;
  }

  if (intlist[MAXRANK] == MAXRANK) {
    USR_FATAL(NULL, "integer list exceeds the maximum rank allowed (%d)",
	      MAXRANK);
  }
  else {

    intlist[ intlist[MAXRANK] ] = i;
    intlist[MAXRANK]++;
  }

  return intlist;
}


/* append to dimension list */

dimension_t* build_dimension(dimtypeclass dimtype) {
  dimension_t* new;

  new = (dimension_t*)PMALLOC(sizeof(dimension_t));
  ST_TYPE(new) = DIMENSION_T;

  DIM_TYPE(new) = dimtype;
  DIM_LO(new) = NULL;
  DIM_HI(new) = NULL;
  DIM_NEXT(new) = NULL;

  return new;
}


dimension_t* copy_dimension(dimension_t* olddim) {
  dimension_t* newdim;

  newdim = build_dimension(DIM_TYPE(olddim));
  DIM_LO(newdim) = DIM_LO(olddim);
  DIM_HI(newdim) = DIM_HI(olddim);

  return newdim;
}


dimension_t *build_dim_list(dimension_t *list,expr_t *lo,expr_t *hi) {
  dimension_t *new;
  dimtypeclass dimtype;

  DB0(50, "build_dim_list\n");

  /* Make a new dimension_t record: */
  if (lo == hi) {
    if (lo != NULL) {
      dimtype = DIM_FLAT;
    } else {
      dimtype = DIM_INHERIT;
    }
  } else {
    dimtype = DIM_RANGE;
  }
  new = build_dimension(dimtype);
  DIM_LO(new) = lo;
  DIM_HI(new) = hi;

  /* Append to existing dim list, if any: */
  return append_dim_list(list,new);
}


dimension_t *build_rgrid_dim_list(dimension_t *list) {
  dimension_t* new;

  new = build_dimension(DIM_GRID);

  return append_dim_list(list, new);
}


dimension_t *append_dim_list(dimension_t *list1,dimension_t *list2) {
  dimension_t *p;

  if (list1 == NULL) {
    return list2;
  } else {
    for (p = list1; DIM_NEXT(p) != NULL; p = DIM_NEXT(p)) {
    }
    DIM_NEXT(p) = list2;
    return list1;
  }
}

expr_t* build_sbe_expr(dimension_t* list) {
  expr_t* new;

  if (DIM_NEXT(list) == NULL && DIM_TYPE(list) == DIM_FLAT) {
    if (T_TYPE(DIM_LO(list)) != SBE) {
      if (expr_find_root_pst(DIM_LO(list)) != NULL) {
	if (D_CLASS(S_DTYPE(expr_find_root_pst(DIM_LO(list)))) == DT_REGION ||
	    D_CLASS(S_DTYPE(expr_find_root_pst(DIM_LO(list)))) == DT_DISTRIBUTION ||
	    D_CLASS(S_DTYPE(expr_find_root_pst(DIM_LO(list)))) == DT_DIRECTION) {
	  return DIM_LO(list);
	}
      }
    }
  }
  new = alloc_expr(SBE);
  T_SBE(new) = list;

  return new;
}


/* append sym to symlist_t */

symlist_t *build_sym_list(symlist_t *list,symlist_t* new) {
  symlist_t *p;

  if (list == NULL) {
    return new;
  }
  else {
    for (p = list;  SYM_NEXT(p) != NULL;  p = SYM_NEXT(p))
      ;
    SYM_NEXT(p) = new;
    return list;
  }
}

/* return reduction binop PT node */

static expr_t* build_redflood_op(exprtype op, expr_t* sbe, expr_t* expr, int lineno, char* filename) {
  expr_t *new;
  statement_t* stmt;

  DB1(30, "build_redflood_op - %d\n", op);

  new = alloc_expr(op);
  if (sbe == NULL) {
    T_REGMASK(new) = NULL;
  } else {
    stmt = convert_sbe_to_region_spec(sbe);
    T_REGMASK(new) = T_REGION(stmt);
  }
  T_OPLS(new) = expr;
  T_PARENT(expr) = new;

  return new;
}

expr_t *build_usrreduction_op(char* id, expr_t* sbe, expr_t* expr) {
  symboltable_t* fn;
  expr_t* new;

  fn = lu(id);
  if (fn == NULL || S_CLASS(fn) != S_SUBPROGRAM) {
    YY_FATALX(yylineno, in_file, "user reductions must be specified with functions");
  }
  new = build_redflood_op(REDUCE,sbe,expr, yylineno, in_file);
  T_SUBTYPE(new) = USER;
  T_IDENT(new) = fn;
  return new;
}

expr_t *build_reduction_op(binop_t op, expr_t* sbe, expr_t *expr) {
  expr_t *new;

  DB1(30, "build_reduction_op - %d\n", op);

  new = build_redflood_op(REDUCE,sbe,expr, yylineno, in_file);
  T_SUBTYPE(new) = op;

  return new;
}


expr_t *build_flood_op(exprtype op, expr_t* sbe, expr_t *expr) {
  expr_t *new;

  DB1(30, "build_flood_op - %d\n", op);

  if (op != FLOOD) {
    INT_FATAL(NULL,"Something's weird in build_flood_op");
  }

  new = build_redflood_op(FLOOD,sbe,expr, yylineno, in_file);

  return new;
}


/* return reduction binop PT node */

expr_t* build_usrscan_op(char* id, int* intlist, expr_t* expr) {
  expr_t* new;
  symboltable_t* fn;

  USR_FATAL(NULL, "User defined scans are not yet supported.");

  fn = lu(id);
  if (fn == NULL || S_CLASS(fn) != S_SUBPROGRAM) {
    YY_FATALX(yylineno, in_file, "user reductions must be specified with functions");
  }

  new = alloc_expr(SCAN);
  T_SUBTYPE(new) = USER;
  T_DIMS(new) = intlist;
  T_OPLS(new) = expr;
  T_PARENT(expr) = new;
  T_IDENT(new) = fn;
  return new;
}

expr_t *build_scan_op(binop_t op,int *intlist,expr_t *expr) {
  expr_t *new;

  DB1(30, "build_reduction_op - %d\n", op);

  new = alloc_expr(SCAN);
  T_SUBTYPE(new) = op;
  T_DIMS(new) = intlist;
  T_OPLS(new) = expr;
  T_PARENT(expr) = new;
  return new;
}


expr_t* build_randacc_op(expr_t* expr, expr_t* map) {
    int dims = 0;
    expr_t* tmp;

    for (tmp = map; tmp != NULL; tmp = T_NEXT(tmp)) {
	dims++;
    }
    T_MAP(expr) = map;
    T_MAPSIZE(expr) = dims;
    return expr;
}

expr_t *build_permute_op(permutetype_t type,expr_t *map,expr_t *expr) {
  expr_t *new;
  int maplen;

  new = alloc_expr(PERMUTE);
  T_SUBTYPE(new) = type;
  T_MAP(new) = map;
  maplen=0;
  while (map != NULL) {
    maplen++;
    /* replace blank dimensions with Indexi */
    if (T_TYPE(map) == VARIABLE && T_IDENT(map) == NULL) {
      T_IDENT(map) = pstINDEX[maplen];
    }
    map = T_NEXT(map);
  }
  T_MAPSIZE(new) = maplen;
  T_OPLS(new)=expr;
  return new;
}


/****************************************************************
 * statement build_... functions:     returns  (statement_t *)
 ****************************************************************/

comm_info_t *add_comm_info(statement_t *s,expr_t *ensemble,genlist_t *dirs,
			   genlist_t* dirtypes) {
  comm_info_t *new;

  new = (comm_info_t *) PMALLOC(sizeof(comm_info_t));
  T_COMM_INFO_ENS(new) = copy_expr(ensemble);
  T_COMM_INFO_DIR(new) = dirs;
  T_COMM_INFO_DIRTYPES(new) = dirtypes;

  T_COMM_INFO_NEXT(new) = T_COMM_INFO(s);
  T_COMM_INFO(s) = new;

  return new;
}
	      

statement_t *
build_comm_statement(commtype_t type,		/*** type ***/
		     int lhscomm,               /*** lhs @ ? ***/
		     int id,			/*** unique id ***/
		     expr_t *region,	/*** region ***/
		     expr_t *ensemble,		/*** ensemble expression ***/
		     genlist_t *dirs,		/*** @ directions ***/
		     genlist_t* dirtypes,       /*** @ or @^ ? ***/
		     int lineno,		/*** line number ***/
		     char *filename)		/*** filename ***/
{
  statement_t	*new;

  DB2(30, "build_comm_statement - type=%d cid=%d\n", type, id);

  if (dirs == NULL) {
    INT_FATAL(NULL, "Direction is NULL.");
  }

  new = alloc_statement(S_COMM, lineno, filename);

  T_COMM(new) = (comm_t *) PMALLOC(sizeof(comm_t));
  T_COMM_TYPE(new) = type;
  T_COMM_ID(new) = id;
  T_COMM_REG(new) = region;

  T_COMM_INFO(new) = NULL;
  add_comm_info(new, ensemble, dirs, dirtypes);

  T_COMM_LHSCOMM(new) = lhscomm;
  T_COMM_PREV(new) = NULL;
  T_COMM_NEXT(new) = NULL;

  return new;
}

static statement_t *build_zprof_statement(zproftype type, int lineno, 
					  char *filename) {
  statement_t *new;

  switch (type) {
  case ZPROF_COMMSTAT:
    break;
  default:
    INT_FATAL(NULL, "Bad zproftype.");
  }

  new = alloc_statement(S_ZPROF, lineno, filename);
  T_ZPROF(new) = (zprof_t *) PMALLOC(sizeof(zprof_t));
  T_ZPROFTYPE(new) = type;

  return new;
}


statement_t *
build_commstat_statement(commstattype type, int lineno, char *filename)
{
  statement_t *new;

  if ((type < START_COMM_TIMING) || (type > PRINT_COMM_TIMING)) {
    INT_FATAL(NULL, "Bad commstattype.");
  }

  IFDB(30) {
    DB0(30, "build_commstat_statement(");
    switch (type) {
    case START_COMM_TIMING:
      DB0(30, "START_COMM_TIMING");
      break;
    case START_COMM:
      DB0(30, "START_COMM");
      break;
    case END_COMM:
      DB0(30, "END_COMM");
      break;
    case END_COMM_TIMING:
      DB0(30, "END_COMM_TIMING");
      break;
    case PRINT_COMM_TIMING:
      DB0(30, "PRINT_COMM_TIMING");
      break;
    }
    DB2(30, ", %d, %s)\n", lineno, filename);
  }

  new = build_zprof_statement(ZPROF_COMMSTAT, lineno, filename);
  T_COMMSTAT(new) = type;

  return new;
}


/* 
 * build_io()
 *   Build a list of IO statements.
 *   This function takes a file parameter and a list of expressions and
 *   recursively builds an IO statement for each expresssion.
 */

static statement_t *build_io(iotype type,expr_t *file,expr_t *exprlist,
			     int lineno, char *filename) {
  /* recursive! */

  DB3(30, "build_io - %d %d %s\n", type, lineno, filename);

  if (exprlist==NULL && 
      (type==IO_WRITELN || type == IO_HALTLN)) { /* base case for IO_WRITELN */
    return build_1_io_statement( type, file, NULL, NULL, NULL,
				 lineno, filename);
  }
  else if (exprlist==NULL) {	/* base case for IO_READ and IO_WRITE */
    return NULL;
  }
  else {			/* recursive case */
    if (T_FLAG1(exprlist) == TRUE)	/* check for a control string */
    {
	return build_1_io_statement( type, file, exprlist, T_NEXT(exprlist),
			             build_io (type, file, 
					       T_NEXT(T_NEXT(exprlist)),
					       lineno, filename),
				     lineno, filename);
    }
    return build_1_io_statement( type, file, NULL, exprlist,
				build_io ( type, file, T_NEXT(exprlist),
					   lineno, filename),
				 lineno, filename);
  }
}



/* 
 * build_io_statements()
 *   This function builds a list of IO statements.
 *   The real work is done by build_io().  This function simply extracts the
 *   file parameter from the expression list, if it exists, and passes it
 *   as a parameter to build_io().
 */

statement_t *build_io_statements(iotype type,expr_t *exprlist, 
				  int lineno, char *filename) {
  DB3(30, "build_io_statements - %d %d %s\n", type, lineno, filename);

  /* Note: exprlist may be null */

  if (isfile(exprlist))
  {
      return build_io(type, exprlist, T_NEXT(exprlist), lineno, filename);
  }
  else 
  {
      return build_io(type, NULL, exprlist, lineno, filename);
  }
}



/* 
 * build_1_io_statement()
 *   This function builds a single IO statement.
 */

statement_t *build_1_io_statement(iotype type,expr_t *file,expr_t *control,
				   expr_t *expr,statement_t *next, 
				   int lineno, char *filename) {
  statement_t *new;
  
  DB3(30,"build_1_io_statement - %d %d %s\n", type, lineno, filename);

  /* writeln( a,b,c); should produce one IO_WRITE for each expr, then a WRITELN:
   * IO_WRITE(a); IO_WRITE(b); IO_WRITE(c); IO_WRITELN(NULL);
   */
  if (expr != NULL && type == IO_WRITELN) {
    type = IO_WRITE;
  }
  if (expr != NULL && type == IO_HALTLN) {
    type = IO_HALT;
  }

  new         = alloc_statement(S_IO,lineno,filename);
  T_NEXT(new) = next;
  T_IO(new)   = alloc_io(type);
  IO_EXPR1(T_IO(new)) = expr;
  IO_FILE(T_IO(new))= file;
  IO_CONTROL(T_IO(new)) = control;

  /* BLC -- new 3/16/94: make expression point to statement */

  if (expr!=NULL) {
    T_STMT(expr)=new;
  }

  return new;
}


statement_t *build_halt_statement(expr_t *exprlist,int lineno,char *filename) {
  statement_t *newstmt;

  newstmt = alloc_statement(S_HALT,lineno,filename);
  if (exprlist) {
    T_HALT(newstmt) = build_io_statements(IO_HALTLN,exprlist,lineno,filename);
  } else {
    T_HALT(newstmt) = NULL;
  }

  return newstmt;
}

void test_mloop_vars_pre_post(mloop_t* mloop) {
  int i;
  int numdims;
  symboltable_t* newvar;
  int unroll_factor = 4;
  int tile_factor = 4;

  numdims = T_MLOOP_RANK(mloop);

  for (i = -1; i < numdims; i++) {
    char* name;
    static int number=0;
    
    name = malloc(64);
    sprintf(name,"bradvar%d_",number++);
    newvar = alloc_st(S_VARIABLE,name);

    S_DTYPE(newvar) = pdtDOUBLE;
    S_LEVEL(newvar) = 1;

    if (i >= 0) {
      S_NUM_INSTANCES(newvar) = unroll_factor;
      S_INSTANCE_LOOP(newvar) = i;
    }

    T_ADD_MLOOP_VAR(mloop,newvar,i);
    S_PROHIBITIONS(newvar) |= TILED_INNER_MLOOP_FLAG;
    S_PROHIBITIONS(newvar) |= UNRLD_TILED_INNER_MLOOP_FLAG;

    if (i >= 0) {
      T_MLOOP_UNROLL(mloop,i) = unroll_factor;
      T_MLOOP_TILE(mloop,i) = tile_factor;

    }
  }
}


/* MLOOP on 1 region {stmts} */

statement_t *build_mloop_statement(expr_t *reg,statement_t *stmts,
				    int rank,expr_t *mask,int with,
				    int lineno, char *filename) {
  int i;
  statement_t	*new;
  statement_t* tmp;

  DB1(30, "build_mloop_statement - %s\n", S_IDENT(T_IDENT(reg)));

  new = alloc_statement(S_MLOOP,lineno,filename);
  T_SUBTYPE(new) = MLOOP_NORMAL;
  T_MLOOP(new) = (mloop_t *)PMALLOC(sizeof(mloop_t));
  T_MLOOP_RANK(T_MLOOP(new)) = rank;
  T_MLOOP_REG(T_MLOOP(new)) = reg;
  T_MLOOP_WITH(T_MLOOP(new)) = with;
  T_MLOOP_MASK(T_MLOOP(new)) = mask;
  T_MLOOP_BODY(T_MLOOP(new)) = stmts;
  for (i = 0; i < MAXRANK; i++) { 
    T_MLOOP_ORDER(T_MLOOP(new),i+1) = default_mloop_order[i];
    T_MLOOP_FLAT(T_MLOOP(new),i+1) = 0;
    T_MLOOP_DIRECTION(T_MLOOP(new), i) = default_mloop_up[i];
    T_MLOOP_TILE_ORDER(T_MLOOP(new),i+1) = default_mloop_tile_order[i]; 
    T_MLOOP_TILE_DIRECTION(T_MLOOP(new), i) = default_mloop_tile_up[i];
    T_MLOOP_UNROLL(T_MLOOP(new),i) = default_mloop_unroll[i];
    T_MLOOP_TILE(T_MLOOP(new),i) = default_mloop_tile[i];
    T_MLOOP_TILE_EXPR(T_MLOOP(new),i) = NULL;
    T_MLOOP_PREPRE(T_MLOOP(new),i) = NULL;
    T_MLOOP_PRE(T_MLOOP(new),i) = NULL;
    T_MLOOP_POST(T_MLOOP(new),i) = NULL;
    T_MLOOP_POSTPOST(T_MLOOP(new),i) = NULL;
  }
  T_MLOOP_CONSTR(T_MLOOP(new)) = NULL;
  for (i = -1; i < MAXRANK; i++) { 
    T_MLOOP_VARS(T_MLOOP(new),i) = NULL;
  }
  T_MLOOP_TILE_NOPEEL(T_MLOOP(new)) = FALSE;
  T_MLOOP_REPLS(T_MLOOP(new)) = NULL;
  T_MLOOP_WBLIST(T_MLOOP(new)) = NULL;
  T_MLOOP_REGLIST(T_MLOOP(new)) = NULL;

  for (tmp = stmts; tmp != NULL; tmp = T_NEXT(tmp)) {
    T_IN_MLOOP(tmp) = new;
  }

  /*  test_mloop_vars_pre_post(T_MLOOP(new));*/

  return new;
}


statement_t *build_nloop_statement(int num_dims, /*** number of dimensions **/
			dimension_t *dim_list, /*** dimension list from LHS **/
			statement_t *stmts,  /*** body ***/
			int depth,  /*** depth ***/
			int proximity, /*** mloop prooximity ***/
		        int lineno, /*** line number ***/
		        char *filename) { /*** file name ***/
  int i;
  statement_t	*new;

  DB0(30, "build_nloop_statement\n");

  new = alloc_statement(S_NLOOP,lineno,filename);
  T_NLOOP(new) = (nloop_t *) PMALLOC(sizeof(nloop_t));

  T_NLOOP_BODY(T_NLOOP(new)) = stmts;

  T_NLOOP_DIMS(T_NLOOP(new)) = num_dims;
  T_NLOOP_DIMLIST(T_NLOOP(new)) = dim_list;
  T_NLOOP_DEPTH(T_NLOOP(new)) = depth;
  T_NLOOP_MLOOP_PROXIMITY(T_NLOOP(new)) = proximity;

  T_ORDER_P(T_NLOOP(new)) = (int *) PMALLOC(num_dims*sizeof(int));
  T_DIRECTION_P(T_NLOOP(new)) = (int *) PMALLOC(num_dims*sizeof(int));
  T_STRIDE_P(T_NLOOP(new)) = (int *) PMALLOC(num_dims*sizeof(int));

  for (i = 0; i < num_dims; i++) { 
    T_NLOOP_ORDER(T_NLOOP(new), i+1) = i;
    T_NLOOP_DIRECTION(T_NLOOP(new), i) = 1;
    T_NLOOP_STRIDE(T_NLOOP(new), i) = 1;
  }

  return new;

}


static void TagExpressionWithStatement(expr_t *expr,statement_t *stmt) {
  expr_t *exprptr;

  T_STMT(expr) = stmt;
  exprptr = T_OPLS(expr);
  while (exprptr != NULL) {
    TagExpressionWithStatement(exprptr,stmt);
    exprptr = T_NEXT(exprptr);
  }
}


static int GetMaskRank(expr_t *expr) {
  if (expr == NULL) {
    return 0;
  }

  typeinfo_expr(expr);
  if (T_TYPEINFO_REG(expr) == NULL) {
    return 0;
  } else {
    return D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(expr)));
  }
}


static void ResolveQuoteRegions(expr_t *reg, int rank) {
  if (T_IDENT(reg) == pst_qreg[0]) {
    if (rank == 0) {
      USR_FATALX(yylineno, in_file, "Unable to determine quote region's rank");
    } else {
      T_IDENT(reg) = pst_qreg[rank];
    }
  }
}


static void ResolveQuoteMasks(expr_t **mask_expr, int rank) {
  if (*mask_expr == gen_quotemask) {
    if (rank == 0) {
      USR_FATALX(yylineno, in_file, "Unable to determine quote mask's rank");
    } else {
      *mask_expr = pexpr_qmask[rank];
    }
  }
}


statement_t *build_reg_mask_scope(expr_t *reg,expr_t *mask_expr,
				  masktype with,statement_t *stmts,int lineno,
				  char *filename) {
  statement_t	*new;
  int rank;

  rank = GetMaskRank(mask_expr);
  if (!rank) {
    rank = stmtls_rank(stmts);
  }
  ResolveQuoteRegions(reg, rank);
  ResolveQuoteMasks(&mask_expr, D_REG_NUM(typeinfo_expr(reg)));

  DB3(30, "build_reg_mask_scope - %d %d %s\n", with, lineno, filename);

  new = alloc_statement(S_REGION_SCOPE, lineno, filename);
  T_REGION(new) = (region_t *)PMALLOC(sizeof(region_t));
  T_REGION_SYM(T_REGION(new)) = reg;
  T_MASK_EXPR(T_REGION(new)) = mask_expr;
  T_MASK_BIT(T_REGION(new)) = with;
  T_BODY(T_REGION(new)) = stmts;

  if (mask_expr != NULL) {
    TagExpressionWithStatement(mask_expr,new);
  }

  return new;
}


/* wrap or reflect A,B; */

statement_t *build_wrap_statement(stmnttype kind,expr_t *exprlist,
				   int lineno, char *filename) {
  statement_t	*new;
  expr_t        *temp_expr, *temp2_expr;
  int uniqID = get_cid();

  DB1(30, "build_wrap_statement - %d\n", kind);
  INT_COND_FATAL((kind==S_WRAP || kind==S_REFLECT), NULL,
		 "only for WRAP/REFLECTs");
  INT_COND_FATAL((exprlist != NULL), NULL,
		 "no arguments to this WRAP/REFLECT");

  new = alloc_statement(kind,lineno,filename);
  T_WRAP(new) = (wrap_t *)PMALLOC(sizeof(wrap_t));

  /*************************************************************
    Check to see if valid expressions are being passed to Wrap/Reflects.
    Only variables, record and array references are valid at this time.
    In the future we might want to be checking to be sure that these
    expressions refer to ensemble type variables.  The check done here is 
    actually not very thorough.  I could try to use parse_structure in
    r2mloops and traverse_exprls in xpand/traverse.c to figure out
    the dimension here. . .  .

    excellent comments, team!  - LS
    *************************************************************/
  temp_expr = exprlist;
  while (temp_expr != NULL) {
    temp2_expr = temp_expr;
    while (temp2_expr !=NULL) {
      T_STMT(temp2_expr) = new;
      temp2_expr = T_OPLS(temp2_expr);
    }
    temp_expr = T_NEXT(temp_expr); 
  }

  T_OPLS(T_WRAP(new)) = exprlist;

  if (kind == S_WRAP) {		/* Wrap yields 2 nodes: Send and Receive */
    T_WRAP_SEND(T_WRAP(new)) = TRUE;
    T_WRAP_CID(T_WRAP(new))  = uniqID;
  }

  return new;
}




/****************************************************************
 * build_...._type functions:     returns  (datatype_t *)
 ****************************************************************/

/* var k : array [1..N] of real */
datatype_t *build_array_type(dimension_t *dimlist,datatype_t *base) {
  datatype_t *new;

  DB0(30, "build_array_type\n");

  /* REMOVE OLD TEST
  if (!dimlist_const(dimlist) && count_rank(dimlist) > 1) {
    YY_FATAL_CONTX(yylineno, in_file, "Compiler currently only "
		   "supports multi-dimensional arrays of constant size.");
  }
  */

  new = alloc_dt(DT_ARRAY);
  D_ARR_TYPE(new)  = base;
  D_ARR_DIM(new)   = dimlist;
  D_ARR_NUM(new)   = count_rank( dimlist);
  D_ARR_ATYPE(new) = -1;      /* field not filled until first cast in Dgen.c */
  return new;
}


static void CreateAnonymousType(datatype_t *nameless) {
  symboltable_t *pst;
  char name[] = "_ANON_TYPE_xxx";
  int templevel;

  if (firstsemi == 1) {
    D_NAME(nameless) = NULL;
    return;
  }

  sprintf(name,"_ANON_TYPE_%d",anontypeID++);
  templevel = current_level;
  current_level = 0;
  pst = LU_INS(name);
  current_level = templevel;
  S_CLASS(pst) = S_TYPE;
  S_DTYPE(pst) = nameless;
  D_NAME(nameless) = pst;
}


void HandleAnonymousRecords(datatype_t *base) {
  if (D_NAME(base)==NULL) {
    if (D_CLASS(base)==DT_STRUCTURE ||
	D_CLASS(base)==DT_ARRAY ||
	D_CLASS(base)==DT_ENUM) {
      CreateAnonymousType(base);
    }
  }
}


static int CheckForNestedEnsembles(datatype_t *pdt) {
  int retval;
  symboltable_t *pst;

  switch (D_CLASS(pdt)) {
  case DT_ENSEMBLE:
    retval = 1;
    break;
  case DT_ARRAY:
    retval = CheckForNestedEnsembles(D_ARR_TYPE(pdt));
    break;
  case DT_STRUCTURE:
    retval = 0;
    pst = D_STRUCT(pdt);
    while (pst != NULL) {
      retval |= CheckForNestedEnsembles(S_DTYPE(pst));
      pst = S_SIBLING(pst);
    }
    break;
  default:
    retval = 0;
    break;
  }

  return retval;
}


/* build record or union type */
datatype_t *build_record_type(symbolclass class) {
  datatype_t	*new;

  DB1(30, "build_record_type - %d\n", class);

  if (class == S_STRUCTURE) {
      new = alloc_dt(DT_STRUCTURE);
    }

  D_STAG(new) = NULL;
  D_STRUCT(new) = getlevel(current_level); /* collect fields in scope */
  return new;
}



/****************************************************************
 * define_ functions:
 ****************************************************************/


/* define record/union component */
static void define_component(char *id,datatype_t *type) {
  symboltable_t *new;
  
  DB1(30, "define_component - %s\n", id);

  new = LU_INS(id);
  S_CLASS(new)     = S_COMPONENT;
  S_DTYPE(new)     = type;
}

void define_componentlist(symlist_t *ids,datatype_t *type) {
  char *id;
  symlist_t *old;
  
  while (ids != NULL) {
    id = SYM_IDENT(ids);
    
    define_component( id, type);
    
    old = ids;
    ids = SYM_NEXT(ids);
    PFREE(old,sizeof(symlist_t));
  }
}


symboltable_t* create_distribution(char* name, char* grid, initial_t* init, int lineno, char* filename) {
  symboltable_t* distribution;
  datatype_t* pdt;
  char* namecopy;
  int rank;
  dimension_t* tmp;

  use_default_distribution = FALSE;
  if (T_TYPE(IN_VAL(init)) != SBE) {
      rank = D_DIST_NUM(S_DTYPE(T_IDENT(IN_VAL(init))));
  }
  else {
    for (rank = 0, tmp = T_SBE(IN_VAL(init)); tmp != NULL; tmp = DIM_NEXT(tmp)) {
      rank++;
    }
  }
  namecopy = (char*)malloc((strlen(name)+1)*sizeof(char));
  strcpy(namecopy, name);
  distribution = lu_insert(namecopy);
  S_IDENT(distribution) = namecopy;
  S_LINENO(distribution) = lineno;
  S_FILENAME(distribution) = filename;
  S_CLASS(distribution) = S_VARIABLE;
  S_STYPE(distribution) |= SC_CONSTANT;
  pdt = alloc_dt(DT_DISTRIBUTION);
  S_DTYPE(distribution) = pdt;
  S_VAR_INIT(distribution) = init;
  if (grid) {
    D_DIST_GRID(pdt) = lu_pst(grid);
    if (rank != D_GRID_NUM(S_DTYPE(D_DIST_GRID(pdt)))) {
      USR_FATAL_CONTX(yylineno, filename, "Rank of distribution must match rank of grid");
    }
  }
  else {
    D_DIST_GRID(pdt) = NULL;
  }

  D_DIST_NUM(pdt) = rank;
  if (GlobalDistributions == NULL) {
    GlobalDistributions = glist_create(distribution, GLIST_NODE_SIZE);
  }
  else {
    glist_append(GlobalDistributions, distribution, GLIST_NODE_SIZE);
  }
  return distribution;
}


symboltable_t* create_grid(char* name, initial_t* init, int lineno, char* filename) {
  symboltable_t* grid;
  datatype_t* pdt;
  char* namecopy;
  int rank;
  dimension_t* tmp;

  use_default_distribution = FALSE;
  for (rank = 0, tmp = T_SBE(IN_VAL(init)); tmp != NULL; tmp = DIM_NEXT(tmp)) {
    rank++;
  }
  namecopy = (char*)malloc((strlen(name)+1)*sizeof(char));
  strcpy(namecopy, name);
  grid = lu_insert(namecopy);
  S_IDENT(grid) = namecopy;
  S_LINENO(grid) = lineno;
  S_FILENAME(grid) = filename;
  S_CLASS(grid) = S_VARIABLE;
  S_STYPE(grid) |= SC_CONSTANT;
  pdt = alloc_dt(DT_GRID);
  S_DTYPE(grid) = pdt;
  D_GRID_NUM(pdt) = rank;

  S_VAR_INIT(grid) = init;
  if (GlobalGrids == NULL) {
    GlobalGrids = glist_create(grid, GLIST_NODE_SIZE);
  }
  else {
    glist_append(GlobalGrids, grid, GLIST_NODE_SIZE);
  }
  return grid;
}


static datatype_t* convert_sbe_to_direction_type(expr_t* expr) {
  datatype_t* pdt;
  dimension_t* dimlist;
  int numdims;
  expr_t* valexpr;
  int intval;

  if (T_TYPE(expr) == SBE) {
    pdt = alloc_dt(DT_DIRECTION);
    
    numdims = 0;
    dimlist = T_SBE(expr);
    while (dimlist != NULL) {
      if (DIM_TYPE(dimlist) == DIM_FLAT) {
	valexpr = DIM_LO(dimlist);
	if (expr_computable_const(valexpr)) {
	  intval = expr_intval(valexpr);
	  if (intval < 0) {
	    if (intval == -1) {
	      D_DIR_SIGN(pdt,numdims) = SIGN_NEGONE;
	    } else {
	      D_DIR_SIGN(pdt,numdims) = SIGN_NEG;
	    }
	  } else if (intval == 0) {
	    D_DIR_SIGN(pdt,numdims) = SIGN_ZERO;
	  } else if (intval > 0) {
	    if (intval == 1) {
	      D_DIR_SIGN(pdt,numdims) = SIGN_POSONE;
	    } else {
	      D_DIR_SIGN(pdt,numdims) = SIGN_POS;
	    }
	  }
	} else {
	  D_DIR_SIGN(pdt,numdims) = SIGN_UNKNOWN;
	}
      } else {
	USR_FATAL_CONTX(yylineno, in_file, 
			"Dimension %d is not a valid direction component",
			numdims+1);
      }
      
      numdims++;
      dimlist = DIM_NEXT(dimlist);
    }
    D_DIR_NUM(pdt) = numdims;

    if (numdims > MAXRANK) {
      USR_FATAL(NULL, "Directions of rank > %d are not supported", MAXRANK);
    }
    return pdt;
  }


  if (T_TYPE(expr) == VARIABLE) {
    if (D_CLASS(S_DTYPE(T_IDENT(expr))) == DT_DIRECTION) {
      return S_DTYPE(T_IDENT(expr));
    }
    else {
      USR_FATALX(yylineno, in_file, "Direction expected in place of %s", 
		 S_IDENT(T_IDENT(expr)));
    }
  }

  USR_FATALX(yylineno, in_file, "Expression is not a direction");
  return NULL;
}


static void check_region_bound(expr_t* expr, int lineno, char* filename) {
  typeinfo_expr(expr);
  if (!datatype_int(T_TYPEINFO(expr))) {
    USR_FATALX(lineno, filename, "Region bounds must be integers");
  } else if (T_TYPEINFO_REG(expr)) {
    USR_FATALX(lineno, filename, "Region bounds must be integer scalars");
  }
}


datatype_t* convert_sbe_to_region_type(expr_t* expr, char* distname, char* name, int lineno, char* filename) {
  datatype_t* baseregdt;
  datatype_t* pdt;
  dimension_t* dimlist;
  int numdims;
  int i;

  switch (T_TYPE(expr)) {
  case SBE:
    if (DIM_TYPE(T_SBE(expr)) == DIM_FLAT &&
	(T_TYPE(DIM_LO(T_SBE(expr))) == SBE ||
	 D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) == DT_REGION)) {
      return convert_sbe_to_region_type(DIM_LO(T_SBE(expr)), distname, name, lineno, filename);
    }
    pdt = alloc_dt(DT_REGION);
    
    numdims = 0;
    dimlist = T_SBE(expr);
    while (dimlist != NULL) {
      D_REG_DIM_TYPE(pdt, numdims) = DIM_TYPE(dimlist);

      switch (DIM_TYPE(dimlist)) {
      case DIM_RANGE:
	check_region_bound(DIM_HI(dimlist), lineno, filename);
	/* fall through */
      case DIM_FLAT:
	check_region_bound(DIM_LO(dimlist), lineno, filename);
	break;
      case DIM_INHERIT:
	if (current_level == 0) {
	  USR_FATAL_CONTX(lineno, filename, 
			  "Can't use blank dimensions in definition of static region %s",
			 name);
	}
	break;
      default:
	break;
      }

      numdims++;
      dimlist = DIM_NEXT(dimlist);
    }
    D_REG_NUM(pdt) = numdims;
    if (numdims > maxdim) {
      maxdim = numdims;
    }
    {
      int i;
      for (i=0; i<numdims; i++) {
	D_REG_NEED_IND(pdt, i) = 0;
	D_REG_NEED_NEXT(pdt, i) = 0;
	D_REG_NEED_PREV(pdt, i) = 0;
	D_REG_NEED_DENSE_DIR(pdt, i) = 0;
      }
    }

    if (numdims > MAXRANK) {
      USR_FATALX(lineno, filename, "Regions of rank > %d are not supported.", MAXRANK);
    }

    if (distname != NULL) {
      D_REG_DIST(pdt) = lu_pst(distname);
    }
    else {
      D_REG_DIST(pdt) = NULL;
    }

    return pdt;
  case CONSTANT:
  case VARIABLE:
    if (D_CLASS(S_DTYPE(T_IDENT(expr))) == DT_REGION) {
      pdt = copy_dt(S_DTYPE(T_IDENT(expr)));
      if (distname != NULL) {
	D_REG_DIST(pdt) = lu_pst(distname);
      }
      else {
	D_REG_DIST(pdt) = NULL;
      }
      return pdt;
    } else {
      USR_FATALX(lineno, filename, "Region expected in place of %s",
		 S_IDENT(T_IDENT(expr)));
    }
    break;
  case BIPREP:
    typeinfo_expr(expr);
    pdt = copy_dt(T_TYPEINFO(expr));
    if (distname != NULL) {
      D_REG_DIST(pdt) = lu_pst(distname);
    }
    else {
      D_REG_DIST(pdt) = NULL;
    }
    return pdt;
    break;
  case BIWITH:
    if (T_SUBTYPE(expr) == SPARSE_WITH_REGION || 
	T_SUBTYPE(expr) == SPARSE_WITHOUT_REGION) {
      baseregdt = convert_sbe_to_region_type(T_OPLS(expr), distname, name, lineno, filename);
      numdims = D_REG_NUM(baseregdt);
      if (numdims == 0) {
	typeinfo_expr(T_NEXT(T_OPLS(expr)));
	numdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(T_NEXT(T_OPLS(expr)))));
      }
      pdt = alloc_dt(DT_REGION);
      for (i=0; i<numdims; i++) {
	D_REG_DIM_TYPE(pdt, i) = DIM_SPARSE;
      }
      D_REG_NUM(pdt) = numdims;
      if (distname != NULL) {
	D_REG_DIST(pdt) = lu_pst(distname);
      }
      else {
	D_REG_DIST(pdt) = NULL;
      }
    } else {
      pdt = convert_sbe_to_region_type(T_OPLS(expr), distname, name, lineno, filename);
    }
    return pdt;
    break;
  default:
    INT_FATAL(NULL,"unexpected case in convert_sbe_to_region_type");
  }

  USR_FATALX(lineno, filename, "Expression is not a region");

  return NULL;
}


symboltable_t* define_direction(char* name, initial_t* init, 
				int lineno, char* filename) {
  datatype_t* pdt;
  expr_t* initexpr;
  symboltable_t* newdir;
  int anondir = 0;

  if (IN_BRA(init)) {
    INT_FATAL(NULL, 
	      "Wasn't expected initializers in braces in define_direction");
  } else {
    initexpr = IN_VAL(init);
  }


  if (T_TYPE(initexpr) == SBE) {
    pdt = convert_sbe_to_direction_type(initexpr);
  } else {
    typeinfo_expr(initexpr);
    pdt = T_TYPEINFO(initexpr);
  }

  if (name == NULL) {
    name = (char *)malloc(16*sizeof(char));
    sprintf(name, "_AnonDir%d",dircounter++);
    anondir = 1;
  } 
  
  newdir = define_varconst(S_VARIABLE, TRUE, FALSE, FALSE, FALSE, FALSE, name, pdt, init, lineno, 
			   filename);
  
  if (anondir) {
    S_SETUP(newdir) = 0;
  }

  return newdir;
}


static char* nameRegion(expr_t* regexpr) {
  char* newname;
  char* subname;
  symboltable_t* dirpst;
  static int dynamic_reg_counter = 1;
  char* dirname;
  char* prepname;
  int regfirst;

  switch (T_TYPE(regexpr)) {
  case VARIABLE:
  case CONSTANT:
    if (symtab_is_qreg(T_IDENT(regexpr))) {
      return "_CurrentRegion";
    } else if (T_IDENT(regexpr) != NULL) {
      return S_IDENT(T_IDENT(regexpr));
    }
    /* fall through */
  case SBE:
    newname = (char*)malloc(16*sizeof(char));
    sprintf(newname, "_AnonReg%d", dynamic_reg_counter++);
    break;
  case ARRAY_REF:
  case BIDOT:
    return nameRegion(T_OPLS(regexpr));
    break;
  case BIPREP:
    subname = nameRegion(T_OPLS(regexpr));
    dirpst = expr_find_root_pst(T_NEXT(T_OPLS(regexpr)));
    dirname = S_IDENT(dirpst);
    newname = (char*)malloc(strlen(subname) + strlen(dirname) + 16);
    switch (T_SUBTYPE(regexpr)) {
    case OF_REGION:
      prepname = "of";
      regfirst = 1;
      break;
    case IN_REGION:
      prepname = "in";
      regfirst = 1;
      break;
    case AT_REGION:
      prepname = "at";
      regfirst = 0;
      break;
    case BY_REGION:
      prepname = "by";
      regfirst = 0;
      break;
    default:
      INT_FATAL(NULL, "unexpected prepositional region type");
    }
    if (regfirst) {
      sprintf(newname, "_%s_%s_%s_%d", dirname, prepname, subname, 
	      dynamic_reg_counter++);
    } else {
      sprintf(newname, "_%s_%s_%s_%d", subname, prepname, dirname, 
	      dynamic_reg_counter++);
    }
    break;
  default:
    INT_FATAL(NULL, "unexpected region type");
    break;
  }

  return newname;
}


static symboltable_t* define_region_guts(int constant, char* name,
					 initial_t* init, datatype_t* pdt,
					 int lineno, char* filename) {
  int anonreg = 0;
  symboltable_t* newreg;

  if (name == NULL) {
    name = nameRegion(IN_VAL(init));
    anonreg = 1;
  }

  newreg = define_varconst(S_VARIABLE, constant, FALSE, FALSE, FALSE, FALSE, name, pdt, init, lineno,
			 filename);

  if (anonreg) {
    S_ANON(newreg) = 1;
    S_SETUP(newreg) = 0;
  }

  return newreg;
}


static expr_t* ensure_region_named_setup(expr_t* reg, int setup, int lineno, char* filename) {
  symboltable_t* newreg;
  expr_t* dir;
  datatype_t* regdt;

  /* this filters out expressions that we don't want to name and setup */
  switch (T_TYPE(reg)) {
    /* These things may all be regions */
  case SBE:
  case VARIABLE:
  case CONSTANT:
  case BIPREP:
  case BIWITH:
    break;
    /* These are not */
  default:
    return reg;
  }
  if (T_TYPE(reg) == BIPREP) {
    dir = T_NEXT(T_OPLS(reg));
    T_OPLS(reg) = ensure_region_named_setup(T_OPLS(reg), setup, lineno, filename);
    T_NEXT(T_OPLS(reg)) = dir;
  }
  if (T_IDENT(reg) == NULL) {
    if (T_TYPE(reg) == SBE) {
      regdt = convert_sbe_to_region_type(reg, NULL, "", lineno, filename);
    } else {
      typeinfo_expr(reg);
      regdt = T_TYPEINFO(reg);
    }
    newreg = define_region_guts(TRUE, NULL, build_init(reg, NULL),
				regdt, lineno, filename);
    S_SETUP(newreg) = setup;
    reg = build_0ary_op(CONSTANT, newreg);
  } else {
    /* note that we never want to change a 1 into a 0,  that I'm aware of... */
    if (S_SETUP(T_IDENT(reg)) == 0) {
      S_SETUP(T_IDENT(reg)) = setup;
    }
  }

  return reg;
}


static expr_t* build_binary_region_expr(exprtype type, regionclass subtype, 
					expr_t* reg, expr_t* otherexpr,
					int numdims, int lineno, char* filename) {
  expr_t* new;

  if (otherexpr == NULL) {
    INT_FATAL(NULL, "other expr is null in build_prep_region_expr");
  }
  if (reg == NULL) {
    INT_FATAL(NULL, "reg expr is null in build_prep_region_expr");
  }

  reg = convert_sbe_to_region(reg, "", lineno, filename);
  if (T_IDENT(reg) == pst_qreg[0]) {
    T_IDENT(reg) = pst_qreg[numdims];
  }
  new = alloc_expr(type);
  T_SUBTYPE(new) = subtype;
  T_OPLS(new) = reg;
  T_NEXT(reg) = otherexpr;
  return new;
}


expr_t* build_prep_region_expr(regionclass prep, expr_t* dir, expr_t* reg, int lineno, char* filename) {
  typeinfo_expr(dir);
  if (D_CLASS(T_TYPEINFO(dir)) != DT_DIRECTION) {
    if (T_IDENT(dir)) {
      USR_FATALX(lineno, filename, "Direction expected in place of %s", 
		 S_IDENT(T_IDENT(dir)));
    } else {
      USR_FATALX(lineno, filename, "Direction expected in region definition");
    }
  }
  if (T_TYPE(reg) == BIWITH &&
      (T_SUBTYPE(reg) == WITH_REGION ||
       T_SUBTYPE(reg) == WITHOUT_REGION)) {
    USR_FATALX(lineno, filename, "Cannot apply region operators to masked region expressions");
  }
  if (T_TYPE(reg) != SBE) {
    reg = ensure_region_named_setup(reg, 0, lineno, filename);
    typeinfo_expr(reg);
    if (D_CLASS(T_TYPEINFO(reg)) != DT_REGION) {
      USR_FATALX(lineno, filename, "Expression is not a region");
    }
    if (D_REG_NUM(T_TYPEINFO(reg)) != D_DIR_NUM(T_TYPEINFO(dir)) &&
	!expr_is_qreg(reg)) {
      USR_FATALX(lineno, filename, 
		 "direction and region must match in rank.");
    }
  }
  return build_binary_region_expr(BIPREP, prep, reg, dir, 
				  D_DIR_NUM(T_TYPEINFO(dir)), lineno, filename);
}


static expr_t* replace_qreg_with_qmask(expr_t* mask) {
  if (T_IDENT(mask) && (T_IDENT(mask) == pst_qreg[0])) {
    return gen_quotemask;
  } else {
    return mask;
  }
}


expr_t* build_with_region_expr(regionclass with, expr_t* mask, expr_t* reg, int lineno, char* filename) {
  int numdims;
  int sparse;

  sparse = (with == SPARSE_WITH_REGION || with == SPARSE_WITHOUT_REGION);
  if (!sparse && mask == pexprUNKNOWN) {
    USR_FATALX(lineno, filename, "cannot use '?' as mask");
  }
  mask = replace_qreg_with_qmask(mask);
  typeinfo_expr(mask);
  if (T_TYPEINFO_REG(mask)) {
    numdims = D_REG_NUM(T_TYPEINFO(T_TYPEINFO_REG(mask)));
  } else {
    numdims = 0;
  }
  reg = ensure_region_named_setup(reg, 0, lineno, filename);
  typeinfo_expr(reg);
  return build_binary_region_expr(BIWITH, with, reg, mask, numdims, lineno, filename);
}


/* var k : [R] real */
/* var k : [,,] real */

datatype_t *build_ensemble_type(datatype_t *base,expr_t *region, int setup, int dnr, int lineno, char* filename) {
  datatype_t *new;

  DB1(30, "build_ensemble_type -  %s\n", S_IDENT(T_IDENT(region)));
  typeinfo_expr(region);
  if (region == NULL || D_CLASS(T_TYPEINFO(region)) != DT_REGION) {
    INT_FATAL(NULL, "must pass in a region");
  }

  /* make sure region is named and set up for array variables */
  if (T_TYPE(region) == CONSTANT ||
      T_TYPE(region) == VARIABLE ||
      T_TYPE(region) == BIPREP) {
    region = ensure_region_named_setup(region, setup, lineno, filename);
  }

  typeinfo_expr(region);

  HandleAnonymousRecords(base);

  if (base == pdtFILE) {
    USR_FATAL_CONTX(lineno, filename, 
		    "Parallel arrays may not be of type 'file'");
  }

  if (CheckForNestedEnsembles(base)) {
    USR_FATAL_CONTX(lineno, filename,
		    "Parallel arrays may not be nested");
  }

  new = alloc_dt(DT_ENSEMBLE);
  D_ENS_TYPE(new) = base;

  D_ENS_REG(new)  = region;
  D_ENS_NUM(new)  = D_REG_NUM(T_TYPEINFO(region));

  D_ENS_DNR(new) = dnr;

  D_ENS_COPY(new) = NULL;

  return new;
}


int equiv_datatypes(datatype_t *pdt1, datatype_t *pdt2) {
  symboltable_t *s1, *s2;

  if (pdt1 == NULL || pdt2 == NULL) {
    return FALSE;
  }

  if (D_CLASS(pdt1) == DT_GENERIC || D_CLASS(pdt1) == DT_GENERIC_ENSEMBLE ||
      D_CLASS(pdt2) == DT_GENERIC || D_CLASS(pdt2) == DT_GENERIC_ENSEMBLE) {
    return TRUE;
  }

  if (D_CLASS(pdt1) != D_CLASS(pdt2)) {
    return FALSE;
  }

  switch (D_CLASS(pdt1)) {
  case DT_INTEGER:
  case DT_REAL:
  case DT_BOOLEAN:     
  case DT_CHAR:     
  case DT_SHORT:
  case DT_LONG:
  case DT_DOUBLE:
  case DT_QUAD:
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_LONG:
  case DT_UNSIGNED_BYTE:
  case DT_SIGNED_BYTE:
  case DT_ENUM:
  case DT_VOID:
  case DT_SUBPROGRAM:
  case DT_PROCEDURE:
  case DT_FILE:
  case DT_STRING:
      return TRUE;

  case DT_ARRAY:
    if (D_ARR_NUM(pdt1) != D_ARR_NUM(pdt2)) {
      return FALSE;
    }
    return equiv_datatypes(D_ARR_TYPE(pdt1), D_ARR_TYPE(pdt2));

  case DT_STRUCTURE:
    for (s1 = D_STRUCT(pdt1), s2 = D_STRUCT(pdt2);
	 s1 != NULL;
	 s1 = S_SIBLING(s1),  s2 = S_SIBLING(s2)) {
      if (!equiv_datatypes(S_DTYPE(s1), S_DTYPE(s2))) {
	return FALSE;
      }
    }
    if (s2 != NULL) {
      return FALSE;		/* pdt2 has more fields than pdt1 */
    } else {
      return TRUE;
    }

  case DT_ENSEMBLE:
    if (D_ENS_NUM(pdt1) != D_ENS_NUM(pdt2)) {
      return FALSE;
    }
    return equiv_datatypes(D_ENS_TYPE(pdt1), D_ENS_TYPE(pdt2));

  case DT_OPAQUE:
    return (D_NAME(pdt1) == D_NAME(pdt2));

  default:
    INT_FATAL(NULL,"<UNKNOWN TYPECLASS %d in equiv_datatypes()>", 
	      D_CLASS(pdt1));
    break;
  }
  return FALSE;
}

/*
 * prototype_flag is 0 if this is a function definition, 1 if this is
 * a function prototype and 2 if this is an external prototype.
 */
symboltable_t *define_functionhead(char *id, datatype_t* returntype, int lineno, char *filename,
				   int prototype_flag, int free) {
  symboltable_t *fn;
  symboltable_t *decls;
  symboltable_t *temp;
  genlist_t	*params;	/* genlist of parameters only */
  genlist_t	*glp;
  genlist_t     *temp2;		/* list of prototype's parameters */
  int           num_params;
  int differ;
  static int overload_number = 1;
  function_t* prev;
  function_t* next;
  symboltable_t* this;

  decls = getlevel(1);

  fn = lu(id);
  if (fn == NULL) { /* if no function/prototype of this name exists */
    fn = LU_INS(id);
    S_CLASS(fn) = S_SUBPROGRAM; 
    if (prototype_flag == 2) {
      S_STYPE(fn) |= SC_EXTERN;
    }
    S_DTYPE(fn) = alloc_dt(DT_SUBPROGRAM);
    S_STD_CONTEXT(fn) = FALSE;
    S_FUN_BODY(fn) = NULL;
    S_FG_SHADOW(fn) = 0;
  }
  if (S_CLASS(S_DTYPE(fn)) != DT_SUBPROGRAM) {
    YY_FATALX(yylineno, in_file, "function cannot overload non-function: %s", id);
  }

  if (S_FUN_BODY(fn) != NULL) { /* if function/prototype of this name exists */
    function_t* fun;

    for (fun = S_FUN_BODY(fn); fun != NULL; fun = T_OVERLOAD_NEXT(fun)) {
      if ((S_STYPE(T_FCN(fun)) & SC_EXTERN) || prototype_flag == 2) {
	differ = 0;
	temp = decls;
	while (temp != NULL && S_CLASS(temp) != S_PARAMETER) {
	  temp = S_SIBLING(temp);
	}
	temp2 = T_PARAMLS(fun);
	while (temp != NULL && temp2 != NULL) {
	  if (!equiv_datatypes(S_PAR_TYPE(temp), S_PAR_TYPE(G_IDENT(temp2)))) {
	    differ = 1;
	  }
	  temp2 = G_NEXT(temp2);
	  temp = S_SIBLING(temp);
	  while (temp != NULL && S_CLASS(temp) != S_PARAMETER) {
	    temp = S_SIBLING(temp);
	  }
	}
	if (temp != NULL || temp2 != NULL) {
	  differ = 1;
	}
	if (differ) {
	  YY_FATAL_CONTX(yylineno, in_file, "extern functions cannot be overloaded: %s", id);
	}
	else {
	  if ((!(S_STYPE(T_FCN(fun)) & SC_EXTERN)) || (prototype_flag != 2)) {
	    YY_FATAL_CONTX(yylineno, in_file, "extern functions cannot be overloaded: %s", id);
	  }
	}
      }
    }

    for (fun = S_FUN_BODY(fn); fun != NULL; fun = T_OVERLOAD_NEXT(fun)) {
      differ = 0;
      temp = decls;
      while (temp != NULL && S_CLASS(temp) != S_PARAMETER) {
		temp = S_SIBLING(temp);
      }
      temp2 = T_PARAMLS(fun);
      while (temp != NULL && temp2 != NULL) {
		if (/* S_PAR_CLASS(temp) != S_PAR_CLASS(G_IDENT(temp2)) || */
			!equiv_datatypes(S_PAR_TYPE(temp), S_PAR_TYPE(G_IDENT(temp2)))) {
		  differ = 1;
		}
		temp2 = G_NEXT(temp2);
		temp = S_SIBLING(temp);
		while (temp != NULL && S_CLASS(temp) != S_PARAMETER) {
		  temp = S_SIBLING(temp);
		}
      }
      if (temp != NULL || temp2 != NULL) {
		differ = 1;
      }
      if (!differ) {
		break;
      }
    }

    /** differ is 0 iff the function is prototyped **/
    if (differ == 0) {
      if (T_STLS(fun) != NULL) {
	YY_FATALX(lineno, filename, "function redefinition: %s", id);
      }
      if (!equiv_datatypes(S_FUN_TYPE(T_FCN(fun)), returntype)) {
	YY_FATALX(lineno, filename,
		  "Function, '%s', prototype and function declaration have different return types", id);
      }

      temp = decls;
      while (temp != NULL && S_CLASS(temp) != S_PARAMETER) {
	temp = S_SIBLING(temp);
      }
      temp2 = T_PARAMLS(fun);
      while (temp != NULL && temp2 != NULL) {
	if (S_PAR_CLASS(temp) != S_PAR_CLASS(G_IDENT(temp2))) {
	  YY_FATALX(lineno, filename,
		    "Value/reference parameter mismatch between function, '%s', prototype and declaration", id);
	}
	temp2 = G_NEXT(temp2);
	temp = S_SIBLING(temp);
	while (temp != NULL && S_CLASS(temp) != S_PARAMETER) {
	  temp = S_SIBLING(temp);
	}
      }

      this = T_FCN(fun);
      prev = T_OVERLOAD_PREV(S_FUN_BODY(this));
      next = T_OVERLOAD_NEXT(S_FUN_BODY(this));
      PFREE(S_FUN_BODY(this), sizeof(function_t));
    }

    /** differ is 1 iff the function is not prototyped **/
    if (differ == 1) {
      id = (char*)malloc(256*sizeof(char));
      sprintf(id, "_ZPL%dOVERLOAD_%s", overload_number++, S_IDENT(fn));
      this = lu(id);
      if (this != NULL) {
	INT_FATAL(NULL, "overload uniqueness error");
      }
      this = LU_INS(id);
      S_CLASS(this) = S_SUBPROGRAM;
      S_DTYPE(this) = alloc_dt(DT_SUBPROGRAM);
      S_STD_CONTEXT(this) = FALSE;
      S_FUN_BODY(this) = NULL;
      S_FG_SHADOW(this) = 0;
      S_LINENO(this) = lineno;
      S_FILENAME(this) = filename;

      for (prev = S_FUN_BODY(fn);
	   T_OVERLOAD_NEXT(prev) != NULL;
	   prev = T_OVERLOAD_NEXT(prev));
      next = NULL;
    }

  }
  else {
    this = fn;
    prev = NULL;
    next = NULL;
  }

  /* Fish out the parameters from the declaration list: */
  params = NULL;
  num_params = 0;
  for (temp = decls; temp != NULL; temp = S_SIBLING(temp)) {
    if (S_CLASS(temp) == S_PARAMETER) {
      glp = alloc_gen();
      G_IDENT(glp) = temp;
      params = cat_genlist_ls( params, glp);
      num_params++;
      DB1(27, "define_function2 - %s is a parameter", S_IDENT(temp));
    } else  {
      DB1(27, "define_function2 - %s isn't a parameter", S_IDENT(temp));
    }
    if (S_PARENT(temp) == NULL) {
      S_PARENT(temp) = this;
    }
  }

  S_FUN_BODY(this) = build_function(this, decls, params, NULL, NULL);

  T_OVERLOAD_PREV(S_FUN_BODY(this)) = prev;
  if (prev != NULL) {
    T_OVERLOAD_NEXT(prev) = S_FUN_BODY(this);
  }

  T_OVERLOAD_NEXT(S_FUN_BODY(this)) = next;
  if (next != NULL) {
    T_OVERLOAD_PREV(next) = S_FUN_BODY(this);
  }
  if (free) {
    S_STYPE(this) |= SC_FREE;
  }
  S_FUN_TYPE(this) = returntype;
  T_NUM_PARAMS(S_FUN_BODY(this)) = num_params;

  S_LINENO(this) = lineno;
  S_FILENAME(this) = filename;

  return this;
}

symboltable_t* define_functionbody(symboltable_t* fn, statement_t *stmt,
				   int lineno, char *filename)
{
  statement_t   *comp_stmt;
  symboltable_t *decls;

  decls = getlevel(1);

  /* The AST invariant seems to be that the statement body is a comp_stmt 
   * with the local declarations defined within that comp_stmt.
   */
  if (T_TYPE(stmt) != S_COMPOUND) {
    comp_stmt = build_compound_statement( NULL, stmt, lineno, filename);
  } else {
    decls = cat_symtab_ls(decls, T_DECL(T_CMPD(stmt)));
    comp_stmt = stmt;
  }

  T_DECL(S_FUN_BODY(fn)) = decls;
  T_DECL(T_CMPD(comp_stmt)) = decls;
  T_STLS(S_FUN_BODY(fn)) = comp_stmt;
  return fn;
}


static void squelch_setup_for_anonymous_regions(datatype_t* type) {
  expr_t* reg;
  symboltable_t* regpst;

  if (D_CLASS(type) == DT_ENSEMBLE) {
    reg = D_ENS_REG(type);
    regpst = T_IDENT(reg);
    if (regpst) {
      if (S_ANON(regpst)) {
	S_SETUP(regpst) = 0;
      }
    }
  }
}


/* define a parameter to a function */
static symboltable_t* define_parameter(subclass mode,int free,char *id, datatype_t *type) { 
  /* taken directly from build_idlist() */

  symboltable_t *new;
  int level = current_level;
  
  DB2(30, "define_parameter - %d %s\n", mode, id);
  
  current_level = 1;
  new = LU_INS(id);
  S_CLASS(new) = S_PARAMETER;
  S_PAR_TYPE(new) = type;
  S_PAR_CLASS(new) = mode;
  S_PARENT(new) = NULL;
  S_FG_PARAM(new) = 1;
  if (mode == SC_CONST) {
    S_STYPE(new) |= SC_CONSTANT;
  }
  if (free) {
    S_STYPE(new) |= SC_FREE;
  }
  if (S_LEVEL(new) != 1) {
    INT_FATAL(NULL, "Parameter level not 1");
  }
  /* for reference parameters, we don't need to setup their dynamic
     regions because we'll never use them; for value parameters,
     we don't set them up so that it will be set up at the callsite
     where the paramtemp is set up.

     BUT... the declaration of the region happens within the function
     rather than outside it where it's used, so this is still a
     problem; could fix by making assignment to temp occur within
     function rather than without... */
  if (mode == SC_CONST || mode == SC_IN || mode == SC_OUT || mode == SC_INOUT) {
    squelch_setup_for_anonymous_regions(type);
  }
  current_level = level;

  return new;
}


/* define list of param string names */
void define_parameterlist(subclass mode,int free,symlist_t *ids,datatype_t *type) {
  char *id;
  symlist_t *old;
  symboltable_t* newparam;

  while (ids != NULL) {
    id = SYM_IDENT(ids);
    
    newparam = define_parameter( mode, free, id, type);
    
    old = ids;
    ids = SYM_NEXT(ids);
    PFREE(old,sizeof(symlist_t));
  }
}


static void check_direction_value(expr_t* expr) {
  dimension_t* dimlist;
  int numdims;

  if (T_TYPE(expr) == SBE) {
    numdims = 0;
    dimlist = T_SBE(expr);

    while (dimlist != NULL) {
      if (DIM_TYPE(dimlist) != DIM_FLAT) {
	USR_FATAL_CONTX(yylineno, in_file, 
			"Dimension %d is not a valid direction component",
			numdims+1);
      }
      numdims++;
      dimlist = DIM_NEXT(dimlist);
    }
    if (numdims > MAXRANK) {
      USR_FATAL(NULL, "Directions of rank > %d are not supported", MAXRANK);
    }
    return;
  }
  else if (T_TYPE(expr) == VARIABLE || T_TYPE(expr) == CONSTANT) {
    if (D_CLASS(S_DTYPE(T_IDENT(expr))) != DT_DIRECTION) {
      USR_FATALX(yylineno, in_file, "Direction expected in place of %s", 
		 S_IDENT(T_IDENT(expr)));
    }
    return;
  }

  USR_FATALX(yylineno, in_file, "Expression is not a direction");
}


static int sbe_is_qreg(expr_t* expr) {
  return dimlist_is_qreg(T_SBE(expr));
}


expr_t* convert_sbe_to_region(expr_t* expr, char* name, int lineno, char* filename) {
  expr_t* regexpr;
  symboltable_t* pst;
  int qreg;
  datatype_t* regdt;

  if (T_TYPE(expr) == SBE) {
    qreg = sbe_is_qreg(expr);
    if (qreg) {
      return buildqregexpr(qreg);
    }
    if (DIM_TYPE(T_SBE(expr)) == DIM_FLAT &&
	(T_TYPE(DIM_LO(T_SBE(expr))) == SBE ||
	 D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) == DT_REGION)) {
	 
      return convert_sbe_to_region(DIM_LO(T_SBE(expr)), name, lineno, filename);
    }
    /* doing this so anonymous regions get named; others shouldn't get here */
    regdt = convert_sbe_to_region_type(expr, NULL, name, lineno, filename);
    pst = define_region_guts(!datatype_reg_dynamicdim(regdt), NULL, build_init(expr, NULL), regdt, 
			     lineno, filename);
    regexpr = build_0ary_op(CONSTANT, pst);
    return regexpr;
  }

  if (D_CLASS(typeinfo_expr(expr)) == DT_REGION) {
    return expr;
  } else {
    if (T_IDENT(expr) && S_IDENT(T_IDENT(expr))) {
      USR_FATALX(lineno, filename, "Region expected in place of %s",
		 S_IDENT(T_IDENT(expr)));
    } else {
      USR_FATALX(lineno, filename, "Expression is not a region");
    }
  }

  return NULL;
}


statement_t* convert_sbe_to_region_spec(expr_t* expr) {
  expr_t* reg;
  statement_t* stmt;
  expr_t* mask;
  int masktype;

  if (T_TYPE(expr) == SBE) {
    if (DIM_TYPE(T_SBE(expr)) == DIM_FLAT &&
	(T_TYPE(DIM_LO(T_SBE(expr))) == SBE ||
	 D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) == DT_REGION)) {
      return convert_sbe_to_region_spec(DIM_LO(T_SBE(expr)));
    }
    reg = ensure_region_named_setup(expr, 0, yylineno, in_file);
    stmt = build_reg_mask_scope(reg, NULL, MASK_NONE, NULL, yylineno, in_file);
    return stmt;
  } else if (D_CLASS(typeinfo_expr(expr)) == DT_REGION) {
    if (T_TYPE(expr) == BIWITH && (T_SUBTYPE(expr) == WITH_REGION ||
				   T_SUBTYPE(expr) == WITHOUT_REGION)) {
      mask = T_NEXT(T_OPLS(expr));
      if (T_SUBTYPE(expr) == WITH_REGION) {
	masktype = MASK_WITH;
      } else {
	masktype = MASK_WITHOUT;
      }
      expr = T_OPLS(expr);
    } else {
      masktype = MASK_NONE;
      mask = NULL;
    }
    reg = ensure_region_named_setup(expr, 0, yylineno, in_file);
    stmt = build_reg_mask_scope(reg, mask, masktype, NULL, yylineno, in_file);
    return stmt;
  }
  else if (expr_find_root_pst(expr) != NULL &&
	   D_CLASS(S_DTYPE(expr_find_root_pst(expr))) == DT_DIRECTION) {
    USR_FATALX(yylineno, in_file, "Cannot use direction in region scope");
  }

  INT_FATAL(NULL, "Failure in convert_sbe_to_region_spec");
  return NULL;
}


static int anonymous_sbe(expr_t* expr) {
  if (T_TYPE(expr) == SBE) {
    if (DIM_TYPE(T_SBE(expr)) == DIM_FLAT &&
	((T_TYPE(DIM_LO(T_SBE(expr))) == SBE) ||
	 (D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) == DT_GRID) ||
	 (D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) == DT_DISTRIBUTION) ||
	 (D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) == DT_REGION))) {
      return anonymous_sbe(DIM_LO(T_SBE(expr)));
    }
  }
  return T_TYPE(expr) == SBE || T_TYPE(expr) == BIPREP;
}


symboltable_t* build_grid_decl_list(expr_t* expr, int lineno, char* filename) {
  symboltable_t* pst;
  datatype_t* pdt;
  int rank;
  static int num = 0;
  char* name;
  dimension_t* tmp;

  if (!anonymous_sbe(expr)) {
    return NULL;
  }
  
  if (T_TYPE(expr) == BIPREP) {
    INT_FATAL(NULL, "Preposition in anonymous grid");
  }

  for (rank = 0, tmp = T_SBE(expr); tmp != NULL; tmp = DIM_NEXT(tmp)) {
    rank++;
  }

  pdt = alloc_dt(DT_GRID);
  D_GRID_NUM(pdt) = rank;
  name = (char*)malloc(128*sizeof(char));
  sprintf(name, "AnonymousGrid%d", ++num);
  pst = alloc_loc_st(S_VARIABLE, name);
  S_STYPE(pst) |= SC_CONSTANT;
  S_DTYPE(pst) = pdt;
  S_VAR_INIT(pst) = build_init(expr, NULL);
  S_SIBLING(pst) = NULL;
  S_LINENO(pst) = lineno;
  S_FILENAME(pst) = filename;
  return pst;
}


symboltable_t* build_distribution_decl_list(expr_t* expr, int lineno, char* filename) {
  symboltable_t* pst;
  datatype_t* pdt;
  int rank;
  static int num = 0;
  char* name;
  dimension_t* tmp;

  if (!anonymous_sbe(expr)) {
    return NULL;
  }
  
  if (T_TYPE(expr) == BIPREP) {
    INT_FATAL(NULL, "Preposition in anonymous distribution");
  }

  for (rank = 0, tmp = T_SBE(expr); tmp != NULL; tmp = DIM_NEXT(tmp)) {
    rank++;
  }

  pdt = alloc_dt(DT_DISTRIBUTION);
  D_DIST_GRID(pdt) = NULL;
  D_DIST_NUM(pdt) = rank;
  name = (char*)malloc(128*sizeof(char));
  sprintf(name, "AnonymousDistribution%d", ++num);
  pst = alloc_loc_st(S_VARIABLE, name);
  S_STYPE(pst) |= SC_CONSTANT;
  S_DTYPE(pst) = pdt;
  S_VAR_INIT(pst) = build_init(expr, NULL);
  S_SIBLING(pst) = NULL;
  S_LINENO(pst) = lineno;
  S_FILENAME(pst) = filename;
  return pst;
}


symboltable_t* build_region_decl_list(expr_t* expr, int lineno, char* filename) {
  symboltable_t* pst;
  datatype_t* pdt;
  symboltable_t* pstls = NULL;

  if (T_TYPE(expr) == BIPREP) {
    pstls = build_region_decl_list(T_OPLS(expr), lineno, filename);
  }
  if (T_TYPE(expr) == SBE) {
    pdt = convert_sbe_to_region_type(expr, NULL, "", lineno, filename);
  }
  else {
    typeinfo_expr(expr);
    pdt = T_TYPEINFO(expr);
  }
  if (!datatype_find_region(pdt)) {
    USR_FATALX(lineno, filename, "Region type expected");
  }
  if (!anonymous_sbe(expr)) {
    return NULL;
  }
  pst = alloc_loc_st(S_VARIABLE, nameRegion(expr));
  S_STYPE(pst) |= SC_CONSTANT;
  S_ANON(pst) = 1;
  S_DTYPE(pst) = pdt;
  S_VAR_INIT(pst) = build_init(expr, NULL);
  S_SIBLING(pst) = pstls;
  D_REG_DIST(pdt) = NULL;
  return pst;
}

static void set_region_distribution(symboltable_t* pst, statement_t* stmt, int lineno, char* filename) {
  expr_t* ens;

  if (use_default_distribution) {
    return;
  }
  if (D_REG_DIST(datatype_find_region(S_DTYPE(pst)))) {
    return;
  }
  ens = stmtls_find_ens_expr(stmt);
  if (ens) {
    symboltable_t* dist = D_REG_DIST(S_DTYPE(expr_find_root_pst(D_ENS_REG(datatype_find_ensemble(S_DTYPE(expr_find_ens_pst(ens)))))));
    if (dist) {
      D_REG_DIST(S_DTYPE(pst)) = dist;
    }
    else {
      D_REG_DIST(S_DTYPE(pst)) = expr_find_ens_pst(ens);
    }
  }
  else {
    USR_FATALX(lineno, filename, "Cannot determine distribution for region scope");
  }
}

/*
 * IN: expr is a region expression scope around stmt
 * OUT: expr is a constant expression of a region
 *      returned statement is a compound with local variables that define the region
 */
static statement_t* build_region_compound_statement(expr_t** expr, statement_t* stmt,
						    int lineno, char* filename) {
  symboltable_t* pst;

  pst = build_region_decl_list(*expr, lineno, filename);
  if (stmt) {
    set_region_distribution(pst, stmt, lineno, filename);
  }
  *expr = build_0ary_op(CONSTANT, pst);
  return build_compound_statement(pst, stmt, lineno, filename);
}


statement_t* build_region_scope(expr_t* expr, int lineno, char* filename) {
  statement_t* stmt;
  expr_t* mask = NULL;
  int masktype = MASK_NONE;

  if (T_TYPE(expr) != SBE && D_CLASS(typeinfo_expr(expr)) != DT_REGION) {
    if (expr_find_root_pst(expr) != NULL && D_CLASS(S_DTYPE(expr_find_root_pst(expr))) == DT_DIRECTION) {
      USR_FATALX(lineno, filename, "Cannot use direction in region scope");    
    }
    INT_FATAL(T_STMT(expr), "Failure in build_region_scope");
  }
  if (T_TYPE(expr) == SBE) {
    if (DIM_TYPE(T_SBE(expr)) == DIM_FLAT &&
	(T_TYPE(DIM_LO(T_SBE(expr))) == SBE ||
	 D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) == DT_REGION)) {
      return build_region_scope(DIM_LO(T_SBE(expr)), lineno, filename);
    }
  } else if (D_CLASS(typeinfo_expr(expr)) == DT_REGION) {
    if (T_TYPE(expr) == BIWITH && (T_SUBTYPE(expr) == WITH_REGION ||
				   T_SUBTYPE(expr) == WITHOUT_REGION)) {
      mask = T_NEXT(T_OPLS(expr));
      masktype = (T_SUBTYPE(expr) == WITH_REGION) ? MASK_WITH : MASK_WITHOUT;
      expr = T_OPLS(expr);
    }
  }
  if (anonymous_sbe(expr)) { /* build compound statement with local region variables */
    stmt = build_region_compound_statement(&expr, NULL, lineno, filename);
    T_STLS(T_CMPD(stmt)) = build_reg_mask_scope(expr, mask, masktype, NULL, lineno, filename);
  }
  else {
    stmt = build_reg_mask_scope(expr, mask, masktype, NULL, lineno, filename);
  }
  return stmt;
}


statement_t* insert_statements_into_region_scope(statement_t* reg, statement_t* stmt) {
  symboltable_t* pst;

  if (T_TYPE(reg) == S_COMPOUND) {
    T_BODY(T_REGION(T_STLS(T_CMPD(reg)))) = stmt;
    pst = expr_find_root_pst(T_REGION_SYM(T_REGION(T_STLS(T_CMPD(reg)))));
  }
  else {
    T_BODY(T_REGION(reg)) = stmt;
    pst = expr_find_root_pst(T_REGION_SYM(T_REGION(reg)));
  }
  set_region_distribution(pst, stmt, yylineno, in_file);
  return reg;
}


/* typedef a type_denoter */
void define_type(char *id,datatype_t *type,int extrn,int lineno,
		 char *filename) {
  symboltable_t *new;
  datatype_t *newdt;

  DB4(30, "define_type - %s %d %d %s\n", id, extrn, lineno, filename);

  new = LU_INS(id);
  S_CLASS(new)     = S_TYPE;
  if (extrn) {
    S_STYPE(new) |= SC_EXTERN;
  }
  if (type == pdtOPAQUE) {
    newdt = (datatype_t *)malloc(sizeof(datatype_t));
    *newdt = *type;
    type = newdt;
  }
  S_DTYPE(new)     = type;
  if (D_NAME(type) == NULL) {	/* if it doesn't already have a name, name it */
    D_NAME(type)   = new;	/* TODO: TEST this w/ code generator. */
  }
  S_LINENO(new)    = lineno;
  S_FILENAME(new)  = filename;
}


/*
 * define list of strings 
 */
/* 
 * define a const|[config] var 
 */
symboltable_t* define_varconst(symbolclass kind, int constant, int config, int extrn, 
			       int free, int statc, char *id, datatype_t *type,
			       initial_t *init,int lineno, char *filename) {
  symboltable_t *new;
  expr_t* reg;

  DB5(20, "define_varconst - %d %d %s %d %s\n", kind, config, id, lineno, 
      filename);


  /*** create an anonymous type for all unnamed types ***/
  HandleAnonymousRecords(type);

  if (config) {
    if (D_CLASS(type) == DT_STRUCTURE) {
      USR_FATAL_CONTX(lineno, filename, 
		      "Config vars may not be record types");
    }
  }

  new = LU_INS(id);
  S_CLASS(new)     = kind;
  if (constant) {
    S_STYPE(new) |= SC_CONSTANT;
  }
  if (config) {
    S_STYPE(new) |= SC_CONFIG;
  }
  if (extrn) {
    S_STYPE(new) |= SC_EXTERN;
  }
  if (free) {
    S_STYPE(new) |= SC_FREE;
  }
  if (statc) {
    S_STYPE(new) |= SC_STATIC;
  }
  S_DTYPE(new)     = type;
  S_LINENO(new)    = lineno;
  S_FILENAME(new)  = filename;

  if (D_CLASS(type) == DT_DIRECTION) {
    if (init) {
      check_direction_value(IN_VAL(init));
    }
  }
  S_VAR_INIT(new)  = init;

  if (D_CLASS(type) == DT_ENSEMBLE) {
    reg = D_ENS_REG(type);
  }
  return new;
}


void define_varconstlist(symbolclass kind,int constant,int config,int extrn,int free,int statc,symlist_t *ids,
			 datatype_t *type,initial_t *init,int lineno,
			 char *filename) {
  char *id;
  symlist_t *old;
  
  while (ids != NULL) {
    id = SYM_IDENT(ids);

    define_varconst( kind, constant, config, extrn, free, statc, id, type, init, lineno, filename);
    
    old = ids;
    ids = SYM_NEXT(ids);
    PFREE(old,sizeof(symlist_t));
  }
}






/****************************************************************
 * "look up" functions:
 ****************************************************************/


symboltable_t *lookup(symbolclass class,char *id) {
  symboltable_t *pst = lu_only(id);

  DB1(55, "lookup - %d\n", class);

  if (pst == NULL) {
    YY_FATALX(yylineno,in_file,"%s is undeclared",id);
  }
  else if (S_CLASS(pst) != class) {
    if (S_CLASS(pst) == S_PARAMETER && class == S_VARIABLE) {
      /* This is special cased, because variables and parameters are
	 usually interchangeable */
    } else {
      YY_FATALX(yylineno,in_file,"expected %s to be a %s, but it is a %s",
		id, symbolnames[class], symbolnames[S_CLASS(pst)]);
    }
  }

  return pst;
}


datatype_t *HandleUnknownTypes(char *ident) {
  datatype_t *retval=NULL;
  int preflen;

  preflen = strlen(ZPLprefix);

  if (!strncmp(ident,ZPLprefix,preflen)) {
    ident+=preflen;
  }

  if (!strcmp(ident, "real")) {
    YY_FATAL_CONTX(yylineno, in_file, "type 'real' does not exist; "
		   "assuming you wanted a 'float'");
    retval = pdtFLOAT; 
  } else if (!strcmp(ident, "int")) {
    YY_FATAL_CONTX(yylineno, in_file, "type 'int' does not exist; "
		   "assuming you wanted an 'integer'");
    retval = pdtINT;
  } else if (!strcmp(ident, "long")) {
    YY_FATAL_CONTX(yylineno, in_file, "type 'long' does not exist; "
		   "assuming you wanted a 'longint'");
    retval = pdtLONG;
  } else if (!strcmp(ident, "short")) {
    YY_FATAL_CONTX(yylineno, in_file, "type 'short' does not exist; "
		   "assuming you wanted a 'shortint'");
    retval = pdtSHORT;
  } else if (!strcmp(ident, "bool")) {
    YY_FATAL_CONTX(yylineno, in_file, "type 'bool' does not exist; "
		   "assuming you wanted a 'boolean'");
    retval = pdtBOOLEAN;
  } else if (!strcmp(ident, "struct")) {
    YY_FATALX(yylineno, in_file, "type constructor 'struct' does not exist; "
	      "you probably want a 'record'");
  } else {
    YY_FATALX(yylineno, in_file, "unknown type '%s'", ident);
  }

  return retval;
}


static int implicit_castable(typeclass t) {
  return (t == DT_INTEGER ||
	  t == DT_REAL ||
	  t == DT_CHAR ||
	  t == DT_ENUM ||
	  t == DT_SHORT ||
	  t == DT_LONG ||
	  t == DT_DOUBLE ||
	  t == DT_QUAD ||
	  t == DT_UNSIGNED_INT ||
	  t == DT_UNSIGNED_SHORT ||
	  t == DT_UNSIGNED_LONG ||
	  t == DT_SIGNED_BYTE ||
	  t == DT_UNSIGNED_BYTE ||
	  t == DT_BOOLEAN ||
	  t == DT_COMPLEX ||
	  t == DT_DCOMPLEX ||
	  t == DT_QCOMPLEX);
}

/*  0 if unacceptable (e.g. different number of parameters, int->array)       *
 *  7 if parameter mismatch is castable (e.g. int->double, double->int)       *
 *  8 if parameter mismatch is well-intentioned castable (e.g. float->double) *
 *  9 if there is no parameter mismatch (e.g. float->float)                   *
 * and subtract 6 if the function is promoted, 3 if the parameter is          */
static int parameter_rating(datatype_t* ftype, datatype_t* atype, expr_t* reg) {
  switch (D_CLASS(ftype)) {

  case DT_GENERIC:
    return (reg == NULL) ? 9 : 3;

  case DT_GENERIC_ENSEMBLE:
    return (reg == NULL) ? 6 : 9;

  case DT_REGION:
  case DT_INTEGER:
  case DT_REAL:
  case DT_BOOLEAN:     
  case DT_CHAR:     
  case DT_SHORT:
  case DT_LONG:
  case DT_DOUBLE:
  case DT_QUAD:
  case DT_COMPLEX:
  case DT_DCOMPLEX:
  case DT_QCOMPLEX:
  case DT_UNSIGNED_INT:
  case DT_UNSIGNED_SHORT:
  case DT_UNSIGNED_LONG:
  case DT_UNSIGNED_BYTE:
  case DT_SIGNED_BYTE:
  case DT_ENUM:
  case DT_VOID:
  case DT_SUBPROGRAM:
  case DT_PROCEDURE:
  case DT_FILE:
  case DT_STRING:
    if (D_CLASS(ftype) == D_CLASS(atype)) {
      return (reg != NULL) ? 3 : 9;
    }
    else if ((D_CLASS(ftype) == DT_QUAD    && D_CLASS(atype) == DT_DOUBLE ) ||
			 (D_CLASS(ftype) == DT_QUAD    && D_CLASS(atype) == DT_REAL   ) ||
			 (D_CLASS(ftype) == DT_DOUBLE  && D_CLASS(atype) == DT_REAL   ) ||
			 (D_CLASS(ftype) == DT_LONG    && D_CLASS(atype) == DT_INTEGER) ||
			 (D_CLASS(ftype) == DT_LONG    && D_CLASS(atype) == DT_SHORT  ) ||
			 (D_CLASS(ftype) == DT_LONG    && D_CLASS(atype) == DT_BOOLEAN) ||
			 (D_CLASS(ftype) == DT_INTEGER && D_CLASS(atype) == DT_SHORT  ) ||
			 (D_CLASS(ftype) == DT_INTEGER && D_CLASS(atype) == DT_BOOLEAN) ||
			 (D_CLASS(ftype) == DT_SHORT   && D_CLASS(atype) == DT_BOOLEAN)) {
      return (reg != NULL) ? 2 : 8;
    }
    else if (implicit_castable(D_CLASS(ftype)) && implicit_castable(D_CLASS(atype))) {
      return (reg != NULL) ? 1 : 7;
    }
    else if (D_CLASS(atype) == DT_ENSEMBLE) {
      return parameter_rating(ftype, D_ENS_TYPE(atype), NULL) - 6;
    }
    else {
      return 0;
    }

  case DT_ENSEMBLE:
    if (D_CLASS(atype) == DT_ARRAY) {
      return parameter_rating(ftype, D_ARR_TYPE(atype), reg);
    }
    else if (D_CLASS(atype) == DT_ENSEMBLE) {
      return 9;
    }
    if (reg != NULL) {
      return parameter_rating(D_ENS_TYPE(ftype), atype, NULL);
    }
    else {
      return parameter_rating(D_ENS_TYPE(ftype), atype, NULL) - 3;
    }

  case DT_ARRAY:
	if (D_CLASS(atype) == DT_ARRAY) {
	  return parameter_rating(D_ARR_TYPE(ftype), D_ARR_TYPE(atype), reg);
	}
	else {
	  return 0;
	}

  case DT_STRUCTURE:
  case DT_OPAQUE:
    if (D_NAME(ftype) == D_NAME(atype)) {
      return 9;
    }
    else {
      return 0;
    }

  default:
    INT_FATAL(NULL,"unknown typeclass %d in parameter rating", D_CLASS(ftype));
    break;
  }
  return 0;
}

char* get_function_name(symboltable_t* fn, expr_t* paramls, int silent) {
  symboltable_t* formals;
  expr_t* actuals;

  function_t* fun;       /* current function under scrutiny                               */
  int rating, newrating; /* quantitatively, how good of a match is the current function?  */
  function_t* bestfun;   /* the best matching function                                    */
  int bestrating;        /* quantitatively, how good of a match is the best function?     */
  int ambiguous;         /* if there are multiple best functions, resolution is ambiguous */

  /* resolve non-overloaded functions quickly and easily */
  if (S_FUN_BODY(fn) == NULL || T_OVERLOAD_NEXT(S_FUN_BODY(fn)) == NULL) {
    return S_IDENT(fn);
  }

  ambiguous = 0;
  bestrating = 0;
  bestfun = NULL;
  for (fun = S_FUN_BODY(fn); fun != NULL; fun = T_OVERLOAD_NEXT(fun)) {
    rating = 10;
    formals = T_DECL(fun);
    while (formals && !S_FG_PARAM(formals)) {
      formals = S_SIBLING(formals);
    }
    actuals = paramls;
    while (actuals && formals) {
      typeinfo_expr(actuals);

      newrating =
		parameter_rating(S_PAR_TYPE(formals), T_TYPEINFO(actuals), T_TYPEINFO_REG(actuals));
      if (newrating < rating) {
		rating = newrating;
      }
      actuals = T_NEXT(actuals);
      do {
		formals = S_SIBLING(formals);
      } while (formals && !S_FG_PARAM(formals));
    }
    if (actuals || formals) {
      rating = 0;
    }
    if (rating > bestrating) {
      bestfun = fun;
      bestrating = rating;
      ambiguous = 0;
    }
    else if (rating == bestrating) {
      ambiguous = 1;
    }
  }
  if (bestrating <= 0 || bestfun == NULL) {
    if (!silent) {
      YY_FATALX(yylineno, in_file, "call of function cannot be resolved: %s", S_IDENT(fn));
    }
    else {
      return NULL;
    }
  }
  if (ambiguous == 1) {
    if (!silent) {
      YY_FATAL_CONTX(yylineno, in_file, "overloaded function resolution is ambiguous: %s", S_IDENT(fn));
    }
    else {
      return NULL;
    }
  }
  return S_IDENT(T_FCN(bestfun));
}


expr_t* buildqregexpr(int rank) {
  expr_t* newexpr;

  newexpr = build_0ary_op(CONSTANT, pst_qreg[rank]);
  typeinfo_expr(newexpr);
  
  return newexpr;
}


expr_t *define_quote_region() {
  return buildqregexpr(0);
}


symboltable_t* find_distribution_pst_in_expr(expr_t* expr) {
  if (!(T_TYPE(expr) == SBE || T_TYPE(expr) == VARIABLE || T_TYPE(expr) == CONSTANT)) {
    INT_FATAL(T_STMT(expr), "Bad SBE defining variable region");
  }

  if (T_TYPE(expr) == SBE) {
    if (DIM_TYPE(T_SBE(expr)) != DIM_FLAT ||
	D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) != DT_DISTRIBUTION) {
      USR_FATALX(yylineno, in_file, "Distribution needed to define a region");
    }
    else {
      return T_IDENT(DIM_LO(T_SBE(expr)));
    }
  }

  if (!D_CLASS(typeinfo_expr(expr)) == DT_DISTRIBUTION) {
    USR_FATALX(yylineno, in_file, "Distribution needed to define a region");
  }

  return T_IDENT(expr);
}


symboltable_t* find_grid_pst_in_expr(expr_t* expr) {
  if (!(T_TYPE(expr) == SBE || T_TYPE(expr) == VARIABLE || T_TYPE(expr) == CONSTANT)) {
    INT_FATAL(T_STMT(expr), "Bad SBE defining variable region");
  }

  if (T_TYPE(expr) == SBE) {
    if (DIM_TYPE(T_SBE(expr)) != DIM_FLAT ||
	D_CLASS(typeinfo_expr(DIM_LO(T_SBE(expr)))) != DT_GRID) {
      USR_FATALX(yylineno, in_file, "Grid needed to define a distribution");
    }
    else {
      return T_IDENT(DIM_LO(T_SBE(expr)));
    }
  }
  
  if (!D_CLASS(typeinfo_expr(expr)) == DT_GRID) {
    USR_FATALX(yylineno, in_file, "Grid needed to define a distribution");
  }
  
  return T_IDENT(expr);
}
