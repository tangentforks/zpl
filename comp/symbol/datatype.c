/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/const.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/cglobal.h"
#include "../include/struct.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/macros.h"
#include "../include/dtype.h"
#include "../include/buildzplstmt.h"
#include "../include/buildsymutil.h"

#define INT			1
#define FLOAT			2
#define CHAR			3
#define VOID			4
#define LONG			5
#define SHORT			6
#define DOUBLE			7
#define UNSIGNED_INT		8
#define	CONST			9
#define VOLATILE		10
#define GENERIC			11
#define GENERIC_ENSEMBLE	12
#define Z_FILE			13
#define STRING			14
#define UNSIGNED_SHORT		15
#define UNSIGNED_LONG		16
#define UNSIGNED_BYTE		17
#define SIGNED_BYTE		18
#define REGION		        19
#define PROCEDURE		20
#define BOOLEAN			21
#define QUAD			22
#define OPAQUE                  23
#define DIRECTION               24

datatype_t *alloc_dt(typeclass class) {
  datatype_t *new = (datatype_t *)PMALLOC(sizeof(datatype_t));

  STRUCTTYPE(new) = DATATYPE_T;
  D_CLASS(new) = class;
  D_NAME(new) = NULL;

  if (class == DT_SUBPROGRAM) {
    D_FUN_TYPE(new) = pdtINT; 
    D_FUN_BODY(new) = NULL;
  }

  return new;
}

symboltable_t *makepst(char *sb) {
  symboltable_t *pst;

  pst = alloc_st(S_TYPE, sb);
  S_PARENT(pst) = NULL;
  S_LEVEL(pst) = 0;
  S_PHE(pst) = alloc_he(sb);
  
  return pst;
}

static datatype_t *makedt(typeclass type,int sw) {
  datatype_t *dt = NULL;

  dt = alloc_dt(type);

  switch (sw) {
  case INT:
    D_NAME(dt) = makepst("int");
    break;
  case LONG:
    D_NAME(dt) = makepst("long");
    break;
  case SHORT:
    D_NAME(dt) = makepst("short");
    break;
  case UNSIGNED_INT:
    D_NAME(dt) = makepst("unsigned");
    break;
  case UNSIGNED_SHORT:
    D_NAME(dt) = makepst("unsigned short");
    break;
  case UNSIGNED_LONG:
    D_NAME(dt) = makepst("unsigned long");
    break;
  case SIGNED_BYTE:
    D_NAME(dt) = makepst("signed char");
    break;
  case UNSIGNED_BYTE:
    D_NAME(dt) = makepst("unsigned char");
    break;
  case FLOAT:
    D_NAME(dt) = makepst("float");
    break;
  case DOUBLE:
    D_NAME(dt) = makepst("double");
    break;
  case QUAD:
    D_NAME(dt) = makepst("_zquad");
    break;
  case BOOLEAN:
    D_NAME(dt) = makepst("boolean");
    break;
  case CHAR:
    D_NAME(dt) = makepst("char");
    break;
  case VOID:
    D_NAME(dt) = makepst("void");
    break;
  case CONST:
    D_NAME(dt) = makepst("const");
    break;
  case VOLATILE:
    D_NAME(dt) = makepst("volatile");
    break;
  case GENERIC:
    D_NAME(dt) = makepst("generic");
    break;
  case GENERIC_ENSEMBLE:
    D_NAME(dt) = makepst("generic_ensemble");
    break;
  case Z_FILE:
    D_NAME(dt) = makepst("file");
    break;
  case DIRECTION:
    D_NAME(dt) = makepst("direction");
    break;
  case STRING:
    D_NAME(dt) = makepst("char *");
    break;
  case REGION:
  default:
    D_NAME(dt) = NULL;
    break;
  }
  return dt;
}


static datatype_t *make_complex_type(datatype_t *basetype,typeclass type,
				     char *name) {
  datatype_t *retval;

  enter_block(NULL);
  define_componentlist(build_sym_list(build_sym_list(NULL,alloc_symlist("re")),
				      alloc_symlist("im")),basetype);
  retval = build_record_type(S_STRUCTURE);
  D_CLASS(retval) = type;
  D_NAME(retval) = makepst(name);
  exit_block();

  return retval;
}


void initialize_datatypes() {
  int i, j;

  pdtINT = makedt(DT_INTEGER, INT);
  pdtFLOAT = makedt(DT_REAL, FLOAT);
  pdtBOOLEAN = makedt(DT_BOOLEAN, BOOLEAN);
  pdtCHAR = makedt(DT_CHAR, CHAR);
  pdtVOID = makedt(DT_VOID, VOID);
  pdtLONG = makedt(DT_LONG, LONG);
  pdtSHORT = makedt(DT_SHORT, SHORT);
  pdtDOUBLE = makedt(DT_DOUBLE, DOUBLE);
  pdtQUAD = makedt(DT_QUAD, QUAD);
  pdtUNSIGNED_INT = makedt(DT_UNSIGNED_INT, UNSIGNED_INT);
  pdtUNSIGNED_SHORT = makedt(DT_UNSIGNED_SHORT, UNSIGNED_SHORT);
  pdtUNSIGNED_LONG = makedt(DT_UNSIGNED_LONG, UNSIGNED_LONG);
  pdtUNSIGNED_BYTE = makedt(DT_UNSIGNED_BYTE, UNSIGNED_BYTE);
  pdtSIGNED_BYTE = makedt(DT_SIGNED_BYTE, SIGNED_BYTE);
  pdtGENERIC = makedt(DT_GENERIC, GENERIC);
  pdtGENERIC_ENSEMBLE = makedt(DT_GENERIC_ENSEMBLE, GENERIC_ENSEMBLE);
  pdtFILE = makedt(DT_FILE, Z_FILE);
  pdtSTRING = makedt(DT_STRING, STRING);
  pdtREGION = makedt(DT_REGION, REGION);
  pdtDIRECTION = makedt(DT_DIRECTION, DIRECTION);
  pdtDISTRIBUTION = makedt(DT_DISTRIBUTION, REGION);
  pdtGRID = makedt(DT_GRID, REGION);
  pdtPROCEDURE = makedt(DT_PROCEDURE, PROCEDURE);
  pdtOPAQUE = makedt(DT_OPAQUE, OPAQUE);
  for (i=0; i<=MAXRANK; i++) {
    pdtGRID_SCALAR[i] = makedt(DT_REGION,REGION);
    D_REG_NUM(pdtGRID_SCALAR[i]) = i;
    D_REG_DIM_TYPE(pdtGRID_SCALAR[i], i) = DIM_GRID;
    D_REG_DIST(pdtGRID_SCALAR[i]) = NULL;
  }

  pdtCOMPLEX = make_complex_type(pdtFLOAT,DT_COMPLEX,"fcomplex");
  pdtDCOMPLEX = make_complex_type(pdtDOUBLE,DT_DCOMPLEX,"dcomplex");
  pdtQCOMPLEX = make_complex_type(pdtQUAD,DT_QCOMPLEX,"qcomplex");

  for (i=0;i<=MAXRANK;i++) {
    pdtRANK_REGION[i] = makedt(DT_REGION, REGION);
    D_REG_NUM(pdtRANK_REGION[i]) = i;
    D_REG_DIST(pdtRANK_REGION[i]) = NULL;
    for (j=0; j<i; j++) {
      D_REG_DIM_TYPE(pdtRANK_REGION[i], j) = DIM_INHERIT;
    }

    pdtRANK_ENSEMBLE[i] = makedt(DT_ENSEMBLE,GENERIC_ENSEMBLE);
    D_ENS_NUM(pdtRANK_ENSEMBLE[i]) = i;
    D_ENS_TYPE(pdtRANK_ENSEMBLE[i]) = pdtBOOLEAN;
  }
}

datatype_t *copy_dt(datatype_t *old_dt) {
  datatype_t *new_dt = alloc_dt(D_CLASS(old_dt));

  memcpy(new_dt, old_dt, sizeof(datatype_t));

  return(new_dt);
}

datatype_t* init_direction_type(signclass sign) {
  datatype_t* pdt;

  pdt = alloc_dt(DT_DIRECTION);
  D_DIR_SIGN(pdt, 0) = sign;
  D_DIR_NUM(pdt) = 1;
  return pdt;
}

datatype_t* compose_direction_type(datatype_t* pdt1, datatype_t* pdt2) {
  datatype_t* pdt;
  int i;

  if (D_CLASS(pdt1) != DT_DIRECTION || D_CLASS(pdt2) != DT_DIRECTION) {
    INT_FATAL(NULL, "Datatype passed to compose_direction_type is not a direction");
  }

  pdt = alloc_dt(DT_DIRECTION);
  D_DIR_NUM(pdt) = D_DIR_NUM(pdt1) + D_DIR_NUM(pdt2);
  for (i = 0; i < D_DIR_NUM(pdt1); i++) {
    D_DIR_SIGN(pdt, i) = D_DIR_SIGN(pdt1, i);
  }
  for (i = D_DIR_NUM(pdt1); i < D_DIR_NUM(pdt); i++) {
    D_DIR_SIGN(pdt, i) = D_DIR_SIGN(pdt2, i - D_DIR_NUM(pdt1));
  }
  free(pdt1);
  free(pdt2);
  return pdt;
}


datatype_t* init_grid_type(dimtypeclass dimdist) {
  datatype_t* pdt;

  pdt = alloc_dt(DT_GRID);
  D_GRID_DIMS(pdt, 0) = dimdist;
  D_GRID_NUM(pdt) = 1;
  return pdt;
}


datatype_t* compose_grid_type(datatype_t* pdt1, datatype_t* pdt2) {
  datatype_t* pdt;
  int i;

  if (D_CLASS(pdt1) != DT_GRID || D_CLASS(pdt2) != DT_GRID) {
    INT_FATAL(NULL, "Datatype passed to compose_grid_type is not a grid");
  }

  pdt = alloc_dt(DT_GRID);
  D_GRID_NUM(pdt) = D_GRID_NUM(pdt1) + D_GRID_NUM(pdt2);
  for (i = 0; i < D_GRID_NUM(pdt1); i++) {
    D_GRID_DIMS(pdt, i) = D_GRID_DIMS(pdt1, i);
  }
  for (i = D_GRID_NUM(pdt1); i < D_GRID_NUM(pdt); i++) {
    D_GRID_DIMS(pdt, i) = D_GRID_DIMS(pdt2, i - D_GRID_NUM(pdt1));
  }
  free(pdt1);
  free(pdt2);
  return pdt;
}


datatype_t* init_distribution_type(distributionclass dimdist) {
  datatype_t* pdt;

  pdt = alloc_dt(DT_DISTRIBUTION);
  D_DIST_DIMS(pdt, 0) = dimdist;
  D_DIST_NUM(pdt) = 1;
  D_DIST_GRID(pdt) = NULL;
  return pdt;
}


datatype_t* compose_distribution_type(datatype_t* pdt1, datatype_t* pdt2) {
  datatype_t* pdt;
  int i;

  if (D_CLASS(pdt1) != DT_DISTRIBUTION || D_CLASS(pdt2) != DT_DISTRIBUTION) {
    INT_FATAL(NULL, "Datatype passed to compose_distribution_type is not a distribution");
  }

  pdt = alloc_dt(DT_DISTRIBUTION);
  D_DIST_NUM(pdt) = D_DIST_NUM(pdt1) + D_DIST_NUM(pdt2);
  for (i = 0; i < D_DIST_NUM(pdt1); i++) {
    D_DIST_DIMS(pdt, i) = D_DIST_DIMS(pdt1, i);
  }
  for (i = D_DIST_NUM(pdt1); i < D_DIST_NUM(pdt); i++) {
    D_DIST_DIMS(pdt, i) = D_DIST_DIMS(pdt2, i - D_DIST_NUM(pdt1));
  }
  free(pdt1);
  free(pdt2);
  return pdt;
}


datatype_t* compose_region_type(dimtypeclass dimtype, datatype_t* pdt) {
  int numdims;
  int i;

  if (pdt == NULL) {
    pdt = alloc_dt(DT_REGION);
    numdims = 0;
  } else {
    numdims = D_REG_NUM(pdt);
    for (i=numdims; i>0; i--) {
      D_REG_DIM_TYPE(pdt, i) = D_REG_DIM_TYPE(pdt, i-1);
    }
  }

  D_REG_DIM_TYPE(pdt, 0) = dimtype;
  D_REG_NUM(pdt) = numdims+1;
  maxdim = numdims+1;

  return pdt;
}
