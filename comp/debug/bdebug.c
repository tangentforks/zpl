/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#if (!RELEASE_VER)
#include <stdio.h>
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/global.h"
#include "../include/symmac.h"
#include "../include/treemac.h"


static void print_typeclass(typeclass typ) {
  switch(typ) {
  case DT_INTEGER:
    printf("INTEGER");
    break;
  case DT_REAL:
    printf("REAL");
    break;
  case DT_CHAR:
    printf("CHAR");
    break;
  case DT_ARRAY:
    printf("ARRAY");
    break;
  case DT_ENSEMBLE:
    printf("ENSEMBLE");
    break;
  case DT_FILE:
    printf("FILE");
    break;
  case DT_ENUM:
    printf("ENUM");
    break;
  case DT_STRING:
    printf("STRING");
    break;
  case DT_STRUCTURE:
    printf("STRUCTURE");
    break;
  case DT_VOID:
    printf("VOID");
    break;
  case DT_SUBPROGRAM:
    printf("SUBPROGRAM");
    break;
  case DT_REGION:
    printf("REGION");
    break;
  case DT_SHORT:
    printf("SHORT");
    break;
  case DT_LONG:
    printf("LONG");
    break;
  case DT_DOUBLE:
    printf("DOUBLE");
    break;
  case DT_QUAD:
    printf("QUAD");
    break;
  case DT_UNSIGNED_INT:
    printf("UNSIGNED_INT");
    break;
  case DT_UNSIGNED_SHORT:
    printf("UNSIGNED_SHORT");
    break;
  case DT_UNSIGNED_LONG:
    printf("UNSIGNED_LONG");
    break;
  case DT_SIGNED_BYTE:
    printf("SIGNED_BYTE");
    break;
  case DT_UNSIGNED_BYTE:
    printf("UNSIGNED_BYTE");
    break;
  case DT_GENERIC:
    printf("GENERIC");
    break;
  case DT_GENERIC_ENSEMBLE:
    printf("GENERIC_ENSEMBLE");
    break;
  case DT_PROCEDURE:
    printf("PROCEDURE");
    break;
  case DT_BOOLEAN:
    printf("BOOLEAN");
    break;
  case DT_OPAQUE:
    printf("OPAQUE");
    break;
  case DT_COMPLEX:
    printf("COMPLEX");
    break;
  case DT_DCOMPLEX:
    printf("DCOMPLEX");
    break;
  case DT_QCOMPLEX:
    printf("QCOMPLEX");
    break;

  default:
    printf("<UNKNOWN %d>", typ);
  }
}


static void print_datatype_entry(datatype_t *pdt) {
  if (pdt == NULL) {
    printf("NULL pointer passed to print_datatype_entry\n");
    return;
  }
  print_typeclass(D_CLASS(pdt));
  printf(",NAME:");
  if (D_CLASS(pdt) == DT_ENSEMBLE) {
    printf("[%s]%s\n", S_IDENT(T_IDENT(D_ENS_REG(pdt))),
	   S_IDENT(D_NAME(D_ENS_TYPE(pdt))));
  } else {
    if (D_NAME(pdt) != NULL) {
      printf("%s\n", S_IDENT(D_NAME(pdt)));
    }
    else {
      printf("null\n");
    }
  }
}


static void print_symbolclass(symboltable_t *pst)
{
  symbolclass sym;

  if (pst == NULL) {
    printf("NULL pointer passed to print_symbolclass\n");
    return;
  }
  sym = S_CLASS(pst);
  switch(sym) {
  case S_VARIABLE:
    if (S_IS_CONSTANT(pst)) {
      printf("CONSTANT-");
    }
    printf("VARIABLE");	
    if (S_IS_ENSEMBLE(pst)) {
      printf("-ENSEMBLE");
    }
    if (D_CLASS(S_DTYPE(pst)) == DT_DIRECTION) {
      printf("-DIRECTION");
    }

    break;
  case S_TYPE:
    printf("TYPE");
    break;
  case S_LITERAL:
    printf("LITERAL");
    break;
  case S_SUBPROGRAM:
    printf("SUBPROGRAM");
    break;
  case S_PARAMETER:
    printf("PARAMETER");
    break;
  case S_COMPONENT:
    printf("COMPONENT");
    break;
  case S_ENUMELEMENT:
    printf("ENUMELEMENT");
    break;
  case S_ENUMTAG:
    printf("ENUMTAG");
    break;
  case S_STRUCTURE:
    printf("STRUCTURE");
    break;
  case S_VARIANT:
    printf("VARIANT");
    break;
  case S_UNKNOWN:
    printf("UNKNOWN");
    break;
  default:
    printf("<UNKNOWN CLASS %d>", sym);
  }
}

void print_symtab_entry(symboltable_t * pst) {
  if (pst == NULL) {
    printf("NULL pointer passed to print_symtab_entry\n");
    return;
  }
  printf("NAME:%s, LVL:%d, FLG:active-%o, ",
	   S_IDENT(pst), S_LEVEL(pst), S_FG_ACTIVE(pst));
  printf("PAR:");
  if (S_PARENT(pst) == NULL)
    printf("null");
  else
    printf("%s", S_IDENT(S_PARENT(pst)));
  printf(", SIB:");
  if (S_SIBLING(pst) == NULL)
    printf("null");
  else
    printf("%s", S_IDENT(S_SIBLING(pst)));
  printf(", PREV:");
  if (S_SIBPREV(pst) == NULL)
    printf("null");
  else
    printf("%s", S_IDENT(S_SIBPREV(pst)));
  printf(", NXT:");
  if (S_NEXT(pst) == NULL)
    printf("null, ");
  else
    printf("%s, ", S_IDENT(S_NEXT(pst)));
  printf("\n");
  printf("CL:");
  print_symbolclass(pst);
  printf(",");
  print_datatype_entry(S_DTYPE(pst));

}

#endif
