/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/statement.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/treemac.h"


statement_t *stmt_in_loop(statement_t *stmt) {
  statement_t *stmtptr;

  stmtptr = stmt;
  while (T_PARENT(stmtptr) != NULL) {
    stmtptr = T_PARENT(stmtptr);
    if (T_TYPE(stmtptr) == S_LOOP) {
      return stmtptr;
    }
  }
  return NULL;
}

/* function: stmtls_length - returns the number of statements in stmtls;
 *                           this code follows T_NEXT, not T_LEX_NEXT
 * date: 29 April 1997
 * creator: echris
 */

int stmtls_length(statement_t *stmtls)
{
  int length = 0;

  while (stmtls) {
    length++;
    stmtls = T_NEXT(stmtls);
  }

  return (length);
}


int stmt_rank(statement_t *stmt) {
  int rank=0;
  int parallel=0;
  expr_t *expr;

  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_HALT:
  case S_CONTINUE:
  case S_ZPROF:
    return 0;
  case S_WRAP:
  case S_REFLECT:
    return expr_rank(T_OPLS(T_WRAP(stmt)));
  case S_COMM:
    return 0;
  case S_REGION_SCOPE:
    return expr_rank(T_MASK_EXPR(T_REGION(stmt)));
  case S_MLOOP:
    return 0;
  case S_NLOOP:
    return stmtls_rank(T_NLOOP_BODY(T_NLOOP(stmt)));
  case S_MSCAN:
    return stmtls_rank(T_STLS(T_CMPD(stmt)));
  case S_COMPOUND:
    return 0;
  case S_EXPR:
    return expr_rank(T_EXPR(stmt));
  case S_IF:
    {
      if_t *ifstuff;
      
      ifstuff = T_IF(stmt);
      expr = T_IFCOND(ifstuff);
      rank = expr_rank(expr);
      parallel = expr_parallel(expr);
      if (rank) {
	return rank;
      }
      if (parallel) {
	rank = stmtls_rank(T_THEN(ifstuff));
	rank = rank ? rank : stmtls_rank(T_ELSE(ifstuff));
	return rank;
      }
    }
    return 0;
  case S_LOOP:
    {
      loop_t *loop;
      
      loop = T_LOOP(stmt);
      if (T_TYPE(loop) == L_DO) {
	expr = T_IVAR(loop);
	rank = expr_rank(expr);
	parallel = expr_parallel(expr);

	expr = T_START(loop);
	rank = rank ? rank : expr_rank(expr);
	parallel = parallel || expr_parallel(expr);

	expr = T_STOP(loop);
	rank = rank ? rank : expr_rank(expr);
	parallel = parallel || expr_parallel(expr);

	expr = T_STEP(loop);
	rank = rank ? rank : expr_rank(expr);
	parallel = parallel || expr_parallel(expr);
      } else {
	expr = T_LOOPCOND(loop);
	rank = expr_rank(expr);
	parallel = expr_parallel(expr);
      }
      if (rank) {
	return rank;
      }
      if (parallel) {
	return stmtls_rank(T_BODY(loop));
      }
      return 0;
    }
  case S_RETURN:
    rank = expr_rank(T_RETURN(stmt));
    if (!rank) {
      datatype_t *retdt;

      retdt = S_FUN_TYPE(T_FCN(T_PARFCN(stmt)));
      if (S_CLASS(retdt) == DT_ENSEMBLE) {
	rank = D_ENS_NUM(retdt);
      }
    }
    return rank;
  case S_IO:
    return expr_rank(IO_EXPR1(T_IO(stmt)));
  default:
    INT_FATAL(stmt,"Unknown statement type in stmt_rank(): %d\n",T_TYPE(stmt));
    return 0;  /* won't get here, but compiler doesn't know that */
  }
}


int stmtls_rank(statement_t *stmtls) {
  int rank=0;

  while (stmtls) {
    rank = stmt_rank(stmtls);
    if (rank) {
      return rank;
    }
    stmtls = T_NEXT(stmtls);
  }
  return 0;
}


static expr_t *stmtls_find_reg(statement_t *stmt) {
  expr_t *reg;

  while (stmt != NULL) {
    reg = stmt_find_reg(stmt);
    if (reg != NULL) {
      return reg;
    }
    stmt = T_NEXT(stmt);
  }
  return NULL;
}


expr_t *stmt_find_reg(statement_t *stmt) {
  expr_t *reg;

  if (stmt == NULL) {
    return NULL;
  }
  switch (T_TYPE(stmt)) {
  case S_NULL:
  case S_EXIT:
  case S_END:
  case S_HALT:
  case S_CONTINUE:
  case S_ZPROF:
    return NULL;
  case S_WRAP:
  case S_REFLECT:
    return T_TYPEINFO_REG(T_OPLS(T_WRAP(stmt)));
  case S_COMM:
    return NULL;
  case S_REGION_SCOPE:
    if (T_MASK_EXPR(T_REGION(stmt))) {
      return T_TYPEINFO_REG(T_MASK_EXPR(T_REGION(stmt)));
    } else {
      return NULL;
    }
  case S_MLOOP:
    return NULL;
  case S_NLOOP:
    return NULL;
  case S_MSCAN:
  case S_COMPOUND:
    return NULL;
  case S_EXPR:
    return expr_find_reg(T_EXPR(stmt));
  case S_IF:
    reg = T_TYPEINFO_REG(T_IFCOND(T_IF(stmt)));
    if (reg == NULL) {
      reg = stmtls_find_reg(T_THEN(T_IF(stmt)));
    }
    if (reg == NULL) {
      reg = stmtls_find_reg(T_ELSE(T_IF(stmt)));
    }
    return reg;
  case S_LOOP:
    {
      loop_t *loop;
      
      loop = T_LOOP(stmt);
      if (T_TYPE(loop) == L_DO) {
	reg = T_TYPEINFO_REG(T_IVAR(loop));
	reg = reg ? reg : T_TYPEINFO_REG(T_START(loop));
	reg = reg ? reg : T_TYPEINFO_REG(T_STOP(loop));
	reg = (reg || !T_STEP(loop)) ? reg : T_TYPEINFO_REG(T_STEP(loop));
      } else {
	reg = T_TYPEINFO_REG(T_LOOPCOND(loop));
      }
      if (reg == NULL) {
	reg = stmtls_find_reg(T_BODY(loop));
      }
      return reg;
    }
  case S_RETURN:
    reg = T_TYPEINFO_REG(T_RETURN(stmt));
    if (reg == NULL) {
      datatype_t *retdt;
     
      retdt = S_FUN_TYPE(T_FCN(T_PARFCN(stmt)));
      if (S_CLASS(retdt) == DT_ENSEMBLE) {
	reg = D_ENS_REG(retdt);
      }
    }
    return reg;
  case S_IO:
    return T_TYPEINFO_REG(IO_EXPR1(T_IO(stmt)));
  default:
    INT_FATAL(stmt,"Unknown statement type in stmt_find_ens(): %d\n",
	      T_TYPE(stmt));
    return NULL;  /* won't get here, but compiler doesn't know that */
  }
}


int stmt_is_shard(statement_t *stmt) {
  expr_t* expr;
  loop_t* loop;
  expr_t* reg;
  int retval;

  if (T_IS_SHARD(stmt) != 0) {
    return T_IS_SHARD(stmt);
  }

  switch (T_TYPE(stmt)) {
  case S_IF:
    expr = T_IFCOND(T_IF(stmt));
    break;
  case S_LOOP:
    loop = T_LOOP(stmt);
    switch (T_TYPE(loop)) {
    case L_WHILE_DO:
    case L_REPEAT_UNTIL:
      expr = T_LOOPCOND(loop);
      break;
    case L_DO:
      expr = T_IVAR(loop);
      break;
    }
    break;
  default:
    return 0;
  }

  reg = T_TYPEINFO_REG(expr);
  if (reg == NULL && T_TYPE(stmt) == S_LOOP && T_TYPE(loop) == L_DO) {
    reg = T_TYPEINFO_REG(T_START(loop));
    if (reg == NULL) {
      reg = T_TYPEINFO_REG(T_STOP(loop));
    }
  }
  if (reg == NULL) {
    retval = 0;
  } else if (expr_is_qreg(reg) == -1) {
    /* unresolved quote region */
    retval = -1;
  } else {
    retval = D_REG_NUM(T_TYPEINFO(reg));
  }
  if (retval != 0) {
    T_IS_SHARD(stmt) = retval;
  }
  return retval;
}


int stmts_are_ancestors(statement_t* stmt1, statement_t* stmt2) {
  statement_t* stmtptr;

  stmtptr = stmt1;
  while (stmtptr) {
    if (stmtptr == stmt2) {
      return 1;
    }
    stmtptr = T_PARENT(stmtptr);
  }

  stmtptr = stmt2;
  while (stmtptr) {
    if (stmtptr == stmt1) {
      return 1;
    }
    stmtptr = T_PARENT(stmtptr);
  }

  return 0;
}
