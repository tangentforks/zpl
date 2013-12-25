/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "../include/allocstmt.h"
#include "../include/error.h"
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/traverse.h"
#include "../include/treemac.h"

#if (!RELEASE_VER)
int call_mem_usage(module_t *mod, char *s) {
  fprintf(zstdout, "Total memory allocated so far : %d bytes\n",mem_used());
  fprintf(zstdout, "Net memory allocated so far : %d bytes\n",net_mem_used());
  return 0;
}

#define MAXMEM 3000000

static int	mem_usage = 0;
static int	net_mem_usage = 0;

int net_mem_used() {
  return net_mem_usage;
}

int mem_used() {
  return mem_usage;
}
#endif

char *our_malloc(int size) {
  char *point;
	
  if (size == 0) {
    INT_FATAL(NULL, "memory allocation error : size == 0");
  }

#if (!RELEASE_VER)
  mem_usage += size;
  net_mem_usage += size;

  DB1(100,"DBG 100 : mem_usage:%d\n", mem_usage);
  DB1(100,"net_mem_usage:%d\n", net_mem_usage);
#endif
  /*  point = (char *)calloc(size, sizeof(char)); */ /*echris*/
  point = (char *)malloc(size); /*echris*/
  if (point == NULL) {
    INT_FATAL(NULL, "Memory allocation error");
  }

  /* echris */
  memset(point, 0, size);

  DB1(200,"allocated %p\n", point);
	
  return point;
}

void our_free(char *p, int size) {
#if (!RELEASE_VER)
  net_mem_usage -= size;
#endif

  if (p == NULL) {
    return;
  }
  
  if (size <= 0) {
    INT_FATAL(NULL, "our_free freeing size <= 0");
  }
  
#if (!RELEASE_VER)
  DB1(100,"mem_usage:%d\n", mem_usage);
  DB1(100,"net_mem_usage:%d\n", net_mem_usage);
#endif

  DB1(200,"freeing   %p\n", p);

  free(p);
}


expr_t	*alloc_expr(exprtype type) {
  expr_t	*new;
  /*** sungeun *** int i; ***/

  DB1(70,"alloc_expr(%d)\n",(int)type);
  
  new = (expr_t *)PMALLOC(sizeof(expr_t));
  /*** sungeun *** this assumes the PMALLOC() zero's memory ***/
  ST_TYPE(new) = EXPR_T;
  T_CLEAR1(new);
  T_CLEAR2(new);
  T_SETUP1(new,NULL);
  T_SETUP2(new,NULL);
  T_CLR_FLAG(T_FLAG1(new));
  T_CLR_FLAG(T_FLAG2(new));
  T_CLR_FLAG(T_FLAG3(new));
  T_CLR_FLAG(T_FLAG4(new));
  /*** sungeun *** T_DUMMY(new) = 0; ***/
  T_TYPE(new) = type;
  /*** sungeun ***
  T_SUBTYPE(new) = 0;
  T_TYPEINFO(new) = NULL;
  T_TYPEINFO_REG(new) = NULL;
  T_OPLS(new) = NULL;
  T_IDENT(new) = NULL;
  T_REGMASK(new) = NULL;
  T_REGMASK2(new) = NULL;
  T_MAP(new) = NULL;
  T_MAPSIZE(new) = 0;
  T_AT(new) = NULL;

  for (i = 0; i < MAXRANK; i++) {
    T_NUDGE (new)[i] = 0;
  }

  T_INSTANCE_NUM(new) = 0;
  T_STMT(new) = NULL;
  T_PARENT(new) = NULL;
  T_PREV(new) = NULL;
  T_NEXT(new) = NULL;

  T_MEANING(new) = NULL;
  ***/

  return new;
}


loop_t *alloc_loop(looptype type) {
  loop_t *new;

  DB1(70,"alloc_loop(%d)\n",(int)type);

  new = (loop_t *)PMALLOC(sizeof(loop_t));
  ST_TYPE(new) = LOOP_T;
  T_CLEAR1(new);
  T_CLEAR2(new);
  T_TYPE(new) = type;
  T_BODY(new) = NULL;

  switch (type) {
  case L_DO:
    T_START_VAL(new) = 0;
    T_STOP_VAL(new) = 0;
    T_STEP_VAL(new) = 0;
    T_UPDOWN(new) = L_DO_UP;
    T_IVAR(new) = NULL;
    T_ENDDO(new) = NULL;
    T_START(new) = NULL;
    T_STOP(new) = NULL;
    T_STEP(new) = NULL;
    T_LNEXTDO(new) = NULL;
    T_PDECL(new) = NULL;
    T_PINIT(new) = NULL;
    T_PPOST(new) = NULL;
    break;
  case L_WHILE_DO:
  case L_REPEAT_UNTIL:
    T_LOOPCOND(new) = NULL;
    break;
  default:
    INT_FATAL(NULL, "illegal loop type (%d) in alloc_loop()",(int)type);
  }
  return new;
}


if_t *alloc_if() {
  if_t *new;

  DB0(70,"alloc_if()\n");
  
  new = (if_t *)PMALLOC(sizeof(if_t));
  ST_TYPE(new) = IF_T;
  T_CLEAR1(new);
  T_CLEAR2(new);
  T_IFCOND(new) = NULL;
  T_PDP(new) = 0;
  T_THEN(new) = NULL;
  T_ELSE(new) = NULL;
  
  return new;
}

stmtlist_t *alloc_stmtlist(statement_t *s,stmtlist_t *next) {
  stmtlist_t *new;

  new = (stmtlist_t *) PMALLOC(sizeof(stmtlist_t));

  LIST_STMT(new) = s;
  LIST_NEXT(new) = next;

  return new;
}


compound_t *alloc_compound() {
  compound_t *new;

  DB0(70,"alloc_compound()\n");

  new = (compound_t *)PMALLOC(sizeof(compound_t));
  ST_TYPE(new) = COMPOUND_T;
  T_CLEAR1(new);
  T_CLEAR2(new);
  T_DECL(new) = NULL;
  T_STLS(new) = NULL;
  return new;
}


statement_t *alloc_statement(stmnttype type,int lineno,char *filename) {
  statement_t *new;

  DB3(70,"alloc_statement(%d,%d,%s)\n",(int)type,lineno,filename);

  new = (statement_t *)PMALLOC(sizeof(statement_t));

  ST_TYPE(new) = STATEMENT_T;
  T_CLEAR1(new);
  T_CLEAR2(new);
  T_CLEAR3(new);
  T_CLEAR4(new);
  T_SETUP1(new,NULL);
  T_SETUP2(new,NULL);
  T_SETUP3(new,NULL);
  T_SETUP4(new,NULL);

  T_TYPE(new) = type;
  T_SUBTYPE(new) = 0;
  T_LINENO(new) = lineno;
  T_FILENAME(new) = filename;
  T_LABEL(new) = NULL;

  /*** initialize the union field ***/
  T_EXPR(new) = NULL;
  
  T_SEMANTICS(new) = SEM_NONE;
  
  T_PARENT(new) = NULL;
  
  T_PARFCN(new) = NULL;

  T_PREV(new) = NULL;
  T_NEXT(new) = NULL;
  T_NEXT_LEX(new) = NULL;
  T_PREV_LEX(new) = NULL;
  
  T_IN(new) = NULL;
  T_OUT(new) = NULL;
  T_LIVE(new) = NULL;

  T_FUNC_CALLS(new) = NULL;
  T_PARALLEL(new) = FALSE;

  T_IRON_COMMID(new) = 0;

  T_SCOPE(new) = NULL;
  T_OUTDEP(new) = NULL;
  T_INDEP(new) = NULL;
  T_IN_MLOOP(new) = NULL;
  T_VISITED(new) = FALSE;

  T_PRE(new) = NULL;
  T_POST(new) = NULL;

  T_IS_SHARD(new) = 0;

  T_PROHIBITIONS(new) = 0;

  return new;
}


function_t *alloc_function() {
  function_t *new;
  int i;

  DB0(70,"alloc_function()\n");

  new = (function_t *)PMALLOC(sizeof(function_t));
  ST_TYPE(new) = FUNCTION_T;
  T_CLEAR1(new);
  T_CLEAR2(new);
  T_FCN(new) = NULL;
  T_SEMANTICS(new) = SEM_NONE;
  T_DECL(new) = NULL;
  T_PARAMLS(new) = NULL;
  T_INVOCATIONS(new) = NULL;
  T_STLS(new) = NULL;
  T_FIRSTDO(new) = NULL;
  T_PREV(new) = NULL;
  T_NEXT(new) = NULL;
  T_CGNODE(new) = NULL;
  T_LOOPS(new) = NULL;
  T_G_REFS(new) = NULL;
  T_G_MODS(new) = NULL;
  T_G_USES(new) = NULL;
  T_IS_SUMMARIZED(new) = FALSE;
  T_BLOCK(new) = NULL;

  T_PARALLEL(new) = FALSE;
  T_REACHABLE(new) = FALSE;
  T_MODULE(new) = NULL;
  for (i=0;i<=MAXRANK;i++) {
    T_UNC_STMT(new,i) = NULL;
    T_UNC_FUNC(new,i) = NULL;
  }
  T_LO_COMMID(new) = -1;
  T_NUM_PARAMS(new) = -1;
  T_CALLINFO(new) = (callsite_t *)-1;

  return new;
}


module_t *alloc_module() {
  module_t *new;

  DB0(70,"alloc_module()\n");

  new = (module_t *)PMALLOC(sizeof(module_t));
  ST_TYPE(new) = MODULE_T;
  T_DECL(new) = NULL;
  T_FCNLS(new) = NULL;
  T_NEXT(new) = NULL;

  return new;
}

io_t	*alloc_io(iotype type) {
  io_t *new = (io_t *)PMALLOC(sizeof(io_t));

  DB1(70,"alloc_io(%d)\n",(int)type);
  

  ST_TYPE(new) = IO_T;
  IO_TYPE(new) = type;

  IO_EXPR1(new) = NULL;
  IO_FILE(new) = NULL;
  IO_CONTROL(new) = NULL;

  return new;
}

genlist_t *freelist_gen = NULL;

genlist_t *alloc_gen() {
  genlist_t *new;
        
  if (!freelist_gen) {
    new = ((genlist_t *) PMALLOC (sizeof(genlist_t)));
  } else {
    new = freelist_gen;
    freelist_gen = G_NEXT(freelist_gen);
  }

  G_NEXT(new) = NULL;
  return new;
}

void free_gen(genlist_t *glp) {
  G_NEXT(glp) = freelist_gen;
  freelist_gen = glp;
}

void free_expr(expr_t *e) {
  DB0(50,"free_expr()\n");

  if (e) {
    PFREE(e,sizeof(expr_t));
  }
}


void free_stmt(statement_t *s) {
  DB0(30,"free_stmt()\n");

  assert(s);

  DB1(50,"Freeing statement, type == %d\n",T_TYPE(s));

  switch(T_TYPE(s)) {
  case S_NULL:		
  case S_END:		
  case S_ZPROF:  /* MDD */
    break;
  case S_EXPR:
    destroy_expr(T_EXPR(s));
    break;
  case S_RETURN:		
    destroy_expr(T_RETURN(s));
    break;
  case S_COMPOUND:
  case S_MSCAN:
    free_compound(T_CMPD(s));
    break;
  case S_IF:		
    free_if(T_IF(s));
    break;
  case S_LOOP:		
    free_loop(T_LOOP(s));
    break;
  case S_IO:		
    DB0(10,"Warning : free_stmt not yet implemented for S_IO statements, "
	"leaves garbage\n");
    break;
  case S_REGION_SCOPE:		
  case S_MLOOP:
  case S_NLOOP:
  case S_WRAP:		
  case S_REFLECT:		
  case S_COMM:
    break;
  default:
    INT_FATAL(s, "bad statement type (%d) in free_stmt()",T_TYPE(s));
  }

  PFREE(s,sizeof(statement_t));
}

void free_compound(compound_t *c) {
  DB0(80,"free_compound()\n");
  PFREE(c,sizeof(compound_t));
}

void free_if(if_t *c) {
  DB0(80,"free_if()\n");
  PFREE(c,sizeof(if_t));
}

void free_loop(loop_t *c) {
  DB0(80,"free_loop()\n");
  PFREE(c,sizeof(loop_t));
}

void free_stmtlist(stmtlist_t *s) {
  if (s) {
    free_stmtlist(LIST_NEXT(s));
    PFREE(s, sizeof(stmtlist_t));
  }
}

void destroy_exprls(expr_t *e) {
  DB0(70,"destroy_exprls()\n");
  traverse_exprls(e,1,free_expr);
}


void destroy_expr(expr_t *e) {
  DB0(70,"destroy_expr()\n");
  traverse_expr(e,1,free_expr);
}


void destroy_stmtls(statement_t *s) {
  DB0(70,"destroy_stmtls()\n");
  traverse_stmtls(s,1,NULL,destroy_expr,NULL);
}


void destroy_stmt(statement_t *s) {
  DB0(70,"destroy_stmt()\n");
  traverse_stmt(s,1,NULL,destroy_expr,NULL);
}

