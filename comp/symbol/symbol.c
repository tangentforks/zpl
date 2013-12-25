/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "../include/Stackgen.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/dimension.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/macros.h"
#include "../include/runtime.h"
#include "../include/stack.h"
#include "../include/symmac.h"
#include "../include/treemac.h"
#include "../include/db.h"
#include "../include/buildsymutil.h"
#include "../include/buildsym.h"
#include "../include/stmtutil.h"

#ifdef DEBUG
#define DBLVL 30
#endif

symboltable_t *pstINDEX[MAXRANK+1];
symboltable_t* pstindex[MAXRANK+1] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
symboltable_t* pstCurrentRegion;
symboltable_t *pstSTRNCPY;
symboltable_t *pstSTRNCMP;
symboltable_t *pstSTRNCAT;
symboltable_t* pstCHR2STR;
expr_t *pexprNULLSTR;
symboltable_t *pstLSHIFT;
symboltable_t *pstRSHIFT;
symboltable_t *pstBOR;
symboltable_t *pstBAND;
symboltable_t *pstBXOR;
symboltable_t *pstBNOT;
symboltable_t *pst_NULL;
symboltable_t *pstCalcRedReg;
symboltable_t* pstAutoRedReg[MAXRANK+1];
symboltable_t *pst_qreg[MAXRANK+1];
symboltable_t* pst_free;
symboltable_t *pst_mfreg;
symboltable_t *pst_dcid, *pst_MSCANREG;
symboltable_t *pst_ms_tile_size[MAXRANK];
expr_t* pexprUNKNOWN;
symboltable_t *ExternalDep; /*** for universal sets for external functions ***/
expr_t *gen_quotemask;
expr_t *pexpr_qmask[MAXRANK+1];
symboltable_t *maplvl[MAXLEVEL];
datatype_t *def_type[MAXIDENT];

static hashentry_t *symtab[MAXHASH];	
static genlist_t* unresolved = NULL;

/* inserts a function with a NULL body */
symboltable_t *insert_function(char* name,
			       datatype_t* return_type,
			       int num_args,
			       ...) {
  symboltable_t *fcn;
  va_list ap;
  int i;

  enter_block(NULL);
  va_start(ap, num_args);
  for (i=0; i<num_args; i++) {
    datatype_t* pdt;
    subclass class;
    char* arg;

    pdt = va_arg(ap,datatype_t*);
    class = va_arg(ap,subclass);
    arg = malloc(128);
    sprintf(arg,"_arg%d",i);
    define_parameterlist(class,0,build_sym_list(NULL,alloc_symlist(arg)),pdt);
  }
  va_end(ap);
  pop();
  current_level--;
  fcn = define_functionhead(name,return_type,-1,NULL,0,FALSE);
  current_level++;
  push(fcn);
  fcn = define_functionbody(fcn,build_compound_statement(NULL,NULL,-1,NULL),-1,NULL);
  exit_block();
  S_STD_CONTEXT(fcn) = TRUE;
  return fcn;
}

static symboltable_t *alloc_st_help(symbolclass class,char *str, int level) {
  int i;
  symboltable_t *temp = (symboltable_t *)PMALLOC(sizeof(symboltable_t));
  
  STRUCTTYPE(temp) = SYMBOLTABLE_T;
  
  S_CLASS(temp) = class;
  S_LEVEL(temp) = level;
    
  if (str) {
    S_IDENT(temp) = (char *)PMALLOC(strlen(str)+1);
    strcpy(S_IDENT(temp), str);
  } else {
    S_IDENT(temp) = NULL;
  }
  S_PHE(temp) = NULL;
  S_PARENT(temp) = top();
  S_SIBLING(temp) = NULL;
  S_SIBPREV(temp) = NULL;
  S_NEXT(temp) = NULL;
  S_DTYPE(temp) = NULL;
  S_INLIST(temp) = NULL;
  S_OUTLIST(temp) = NULL;
  if (level == 0) {
    S_STYPE(temp) = SC_STATIC;
  } else {
    S_STYPE(temp) = 0;
  }
  S_INIT(temp) = NULL;
  S_SUBCLASS(temp) = SC_NORMAL;
  
  S_FG_ACTIVE(temp) = S_ACTIVE;
  S_FG_SHADOW(temp) = 0;
  S_FG_TYPE(temp) = 0;
  S_FG_DELETE(temp) = 0;
  S_FG_EQUIV(temp) = 0;
  S_FG_PARAM(temp) = 0;
  S_IS_USED(temp) = 1;
  S_FG_1(temp) = 0;
  S_FG_2(temp) = 0;
  S_FG_3(temp) = 0;
  S_FG_4(temp) = 0;
  S_FG_5(temp) = 0;
  S_FG_6(temp) = 0;
  S_FG_7(temp) = 1;	
  
  S_FG_8(temp) = 0;
  S_FG_9(temp) = 0;
  S_FG_0(temp) = 0;
  
  for (i=0;i<MAXRANK;i++) {
    S_FLUFF_LO(temp,i) = 0;
    S_FLUFF_HI(temp,i) = 0;
    S_UNK_FLUFF_LO(temp,i) = 0;
    S_UNK_FLUFF_HI(temp,i) = 0;
    S_WRAPFLUFF_LO(temp,i) = 0;
    S_WRAPFLUFF_HI(temp,i) = 0;
  }

  S_MASK(temp) = 0;
  S_STD_CONTEXT(temp) = FALSE;

  S_LINENO(temp) = 0;
  S_FILENAME(temp) = NULL;

  S_SETUP(temp) = 1;
  S_ANON(temp) = 0;

  S_REG_MASK(temp) = NULL;
  S_REG_WITH(temp) = MASK_NONE;

  S_NUM_INSTANCES(temp) = 0;
  S_INSTANCE_LOOP(temp) = -1;

  S_PROHIBITIONS(temp) = 0;

  return temp;
}

symboltable_t *alloc_st(symbolclass class,char *str) {
  return alloc_st_help(class, str, current_level);
}


symboltable_t* alloc_loc_st(symbolclass class,char* str) {
  return alloc_st_help(class, str, 1);
}



symlist_t *alloc_symlist(char* name) {
  symlist_t *new = (symlist_t *)PMALLOC(sizeof(symlist_t));
  
  STRUCTTYPE(new) = SYMLIST_T;
  SYM_IDENT(new) = name;
  SYM_NEXT(new) = NULL;

  return new;
}


hashentry_t *alloc_he(char *ident) {
  hashentry_t *new = (hashentry_t *)PMALLOC(sizeof(hashentry_t));
  
  HE_IDENT(new) = (char *)PMALLOC(strlen(ident)+1);
  strcpy(HE_IDENT(new), ident);
  HE_SYM(new) = NULL;
  HE_LAST(new) = NULL;
  
  return new;
}

initial_t *alloc_initial() {
  initial_t *new = (initial_t *)PMALLOC(sizeof(initial_t));
  
  IN_BRA(new) = 0;
  IN_REP(new) = 0;
  STRUCTTYPE(new) = INITIAL_T;
  S_NEXT(new) = NULL;
  return new;
}


symboltable_t *getlevel(int level) {
  symboltable_t *pst, *head;
  
  pst = maplvl[level];
  head = pst;
  while (pst != NULL && S_SIBPREV(pst) != NULL) {
    IFDB(30) {
      if (level == 0)
	DBS1(10, "\t\tgetlevel::%s::\n", S_IDENT(pst));
    }
    pst = S_SIBPREV(pst);
    IFDB(30) {
      if (pst == head)
	DBS0(30, "\t===================\n");
    }
  }
  
  return pst;
}


hashentry_t *getsymtab(int index) {
  return symtab[index];
}


static int hash(char *ident) {
  int sum = 0;
  
  while (*ident != '\0')
    sum = (2 * sum + *ident++) % MAXHASH;
  return(sum);
}

static void insert_phe(hashentry_t *phe) {
  int hv;
  
  DB1(20, "insert_phe - %s\n", HE_IDENT(phe));
  hv = hash(HE_IDENT(phe));
  HE_NEXT(phe) = symtab[hv];
  symtab[hv] = phe;
}

static hashentry_t *get_he(char *ident) {
  hashentry_t *phe;
  
  DB1(50, "get_he - %s\n", ident);
  
  if (ident == NULL) {
    INT_FATAL(NULL, "get_he called with NULL symbol");
  }

  phe = symtab[hash(ident)];
  while (phe != NULL) {
    if (strcmp(ident, HE_IDENT(phe)) == 0)
      return phe;
    phe = HE_NEXT(phe);
  }
  
  return NULL;
}

symboltable_t* lu_pst(char* name, ...) {
  static char lookup_name[128];
  va_list ap;

  va_start(ap,name);
  vsprintf(lookup_name,name,ap);
  va_end(ap);
  return lu_only(lookup_name);
}

symboltable_t *lu_only(char *ident) {
  hashentry_t	*phe;
  symboltable_t	*pst;
  
  phe = get_he(ident);
  if (phe == NULL) { 
    phe = alloc_he(ident);
    insert_phe(phe);
  }
  pst = HE_SYM(phe);
  while (pst != NULL) {
    DB3(DBLVL, "lu_only checking %s, level %d, active %d\n",
	S_IDENT(pst), S_LEVEL(pst), S_FG_ACTIVE(pst));
    if (strcmp(ident,S_IDENT(pst)) == 0 &&
	S_FG_ACTIVE(pst) == S_ACTIVE &&
	S_LEVEL(pst) <= current_level)
      return pst;
    pst = S_NEXT(pst);
  }
  DB1(DBLVL, "lu_only: %s not found\n", ident);
  pst = alloc_st(S_UNKNOWN, ident);
  S_DTYPE(pst) = pdtINT;
  S_CLASS(pst) = S_VARIABLE;
  if (lu(ident) == NULL) {
    YY_FATALX(yylineno, in_file, "Undeclared identifier %s", ident);
  }
  S_PHE(pst) = phe;
  insert_pst(pst);
  return pst;
}


symboltable_t *lu(char *ident) {
  hashentry_t	*phe;
  symboltable_t	*pst;
  
  phe = get_he(ident);
  if (phe == NULL)
    return NULL;
  pst = HE_SYM(phe);
  while (pst != NULL) {
    DB2(30, "checking %s %d\n", S_IDENT(pst), S_LEVEL(pst));
    if (strcmp(ident,S_IDENT(pst)) == 0 &&
	S_FG_ACTIVE(pst) == S_ACTIVE &&
	S_LEVEL(pst) <= current_level)
      return pst;
    pst = S_NEXT(pst);
  }
  return NULL;
}


symboltable_t *lu_insert(char *ident) {
  symboltable_t	*temp;
  hashentry_t *phe;
  
  DB0(50, "lu_insert entered - ");
  
  phe = get_he(ident);
  if (phe == NULL) {
    phe = alloc_he(ident);
    insert_phe(phe);
    temp = NULL;
  } else
    temp = HE_SYM(phe);
  
  while (temp != NULL) {
    
    DB0(25, "lu_insert(symbol.c):checking...\n");
    DB2(25, "%s ?= %s\n", ident, S_IDENT(temp));
    
    if (strcmp(ident,S_IDENT(temp)) == 0) {
      
      if (S_LEVEL(temp) == current_level &&
	  S_FG_ACTIVE(temp) == S_ACTIVE) {
	YY_FATALX(yylineno, in_file, "duplicate definition of %s", ident);
      }
    }
    temp = S_NEXT(temp);
  }
  
  
  temp = alloc_st(S_VARIABLE, ident);

  S_PHE(temp) = phe;
  insert_pst(temp);
  
  DB0(50, "lu_insert exited\n");
  return temp;
}


symboltable_t *add_constant(char *ident) {
  symboltable_t *new;
  hashentry_t *phe;
  
  phe = get_he(ident);
  if (phe == NULL) {
    phe = alloc_he(ident);
    insert_phe(phe);
  }
  new = lu(ident);
  if (new == NULL) {
    new = alloc_st(S_LITERAL, ident);
    S_PHE(new) = phe;
    S_LEVEL(new) = 0;
    S_PARENT(new) = NULL;
    
    insert_pst(new);
  }
  return new;
}


static void link_sym(symboltable_t *pst) {
  S_SIBPREV(pst) = maplvl[ S_LEVEL(pst) ];
  if (maplvl[S_LEVEL(pst)] != NULL)
    S_SIBLING(maplvl[S_LEVEL(pst)]) = pst;
  maplvl[S_LEVEL(pst)] = pst;
  if (S_SIBPREV(pst) != NULL) {
    DB1(27, "sibling %s\n", S_IDENT(S_SIBPREV(pst)));
  } else {
    DB0(27, "no siblings\n");
  }
}


void insert_pst(symboltable_t *pst) {
  hashentry_t *phe;
  
  DB2(27, "insert_pst - %s - level %d -", S_IDENT(pst), S_LEVEL(pst));
  
  link_sym(pst);
  phe = get_he(S_IDENT(pst));
  if (phe == NULL) {	
    phe = alloc_he(S_IDENT(pst));
    insert_phe(phe);
  }
  
  S_PHE(pst) = phe;
  S_NEXT(pst) = HE_SYM(phe);
  HE_SYM(phe) = pst;
  if (HE_LAST(phe) == NULL)
    HE_LAST(phe) = pst;
}


void unlink_sym(symboltable_t *pst,int level) {
  symboltable_t *temp;
  
  temp = maplvl[level];
  while (temp != NULL && temp != pst)
    temp = S_SIBPREV(temp);
  if (temp == NULL) {
    DB1(DBLVL, "did not unlink %s\n", S_IDENT(pst));
  } else {
    DB1(DBLVL, "unlinking %s", S_IDENT(pst));
    if (temp == maplvl[level]) {
      DBS0(DBLVL, " - first one\n");
      maplvl[level] = S_SIBPREV(temp);
      if (maplvl[level] != NULL)
	S_SIBLING(maplvl[level]) = NULL;
    } else {
      DBS0(DBLVL, " - not first one\n");
      if (S_SIBLING(temp))
	S_SIBPREV(S_SIBLING(temp)) = S_SIBPREV(temp);
      if (S_SIBPREV(temp))
	S_SIBLING(S_SIBPREV(temp)) = S_SIBLING(temp);
    }
  }
}


void deactivate(symboltable_t *pst) {
  hashentry_t *phe;
  symboltable_t *pstTemp;
  
  DB1(30, "deactivating %s\n", S_IDENT(pst));
  phe = S_PHE(pst);
  if (phe == NULL) {
    DB0(50, "pst has no phe - aborting deactivate");
    return;
  }
  
  if (S_FG_ACTIVE(pst) == S_INACTIVE) {
    DB1(50, "attempted to deactivate an inactive symbol %s",
	S_IDENT(pst));
    return;
  } else 
    S_FG_ACTIVE(pst) = S_INACTIVE;
  
  if (HE_SYM(phe) != pst) {
    DB1(50, "deactivating non-head symbol %s", S_IDENT(pst));
    pstTemp = HE_SYM(phe);
    while (pstTemp != NULL && S_NEXT(pstTemp) != pst)
      pstTemp = S_NEXT(pstTemp);
    S_NEXT(pstTemp) = S_NEXT(pst);
    if (S_NEXT(pst) == NULL)
      HE_LAST(phe) = pstTemp;
    
    return;
  }
  
  if (S_NEXT(pst) != NULL) {
    HE_SYM(phe) = S_NEXT(pst);
    S_NEXT(HE_LAST(phe)) = pst;
    HE_LAST(phe) = pst;
    S_NEXT(pst) = NULL;
  }
}

symboltable_t *add_unresolved(symboltable_t *pst) {
  genlist_t* new;
  genlist_t* current;
  
  DB0(20, "enter add_unresolved -");
  current = unresolved;
  while (current != NULL) {
    if (strcmp(S_IDENT(G_IDENT(current)), S_IDENT(pst)) == 0) {
      DB1(20, "found %s\n", S_IDENT(pst));
      return G_IDENT(current);
    }
    current = G_NEXT(current);
  }
  
  DB1(20, "adding %s\n", S_IDENT(pst));
  new = alloc_gen();
  G_IDENT(new) = pst;
  G_NEXT(new) = unresolved;
  unresolved = new;
  return pst;
}


static symboltable_t *insert_define(char *name,symbolclass class,
				    datatype_t *type) {
  symboltable_t *pst;

  DB1(DBLVL, "Inserting variable %s as part of the standard context.\n", name);
 
  pst = LU_INS(name);
  S_CLASS(pst) = class;
  S_STD_CONTEXT(pst) = TRUE;
  S_DTYPE(pst) = type;

  return pst;
}

static symboltable_t* insert_constant(char* name, datatype_t* type) {
  symboltable_t* pst;

  DB1(DBLVL, "Inserting constant %s as part of the standard context.\n", name);
 
  pst = LU_INS(name);
  S_CLASS(pst) = S_VARIABLE;
  S_STYPE(pst) |= SC_CONSTANT;
  S_STD_CONTEXT(pst) = TRUE;
  S_DTYPE(pst) = type;

  return pst;
}


static symboltable_t* insert_variable(char* name, datatype_t* type) {
  symboltable_t* pst;

  DB1(DBLVL, "Inserting variable %s as part of the standard context.\n", name);
 
  pst = LU_INS(name);
  S_CLASS(pst) = S_VARIABLE;
  S_STD_CONTEXT(pst) = TRUE;
  S_DTYPE(pst) = type;

  return pst;
}


static void add_internals(void) {
  char name[80];
  int i;

  /* Insert symbols for strings. */
  pstSTRNCPY = insert_function("_ZPL_STRCPY",pdtSTRING,2,pdtSTRING,SC_INOUT,pdtSTRING,SC_CONST);
  pstSTRNCMP = insert_function("_ZPL_STRCMP",pdtINT,2,pdtSTRING,SC_CONST,pdtSTRING,SC_CONST);
  pstSTRNCAT = insert_function("_ZPL_STRCAT",pdtSTRING,2,pdtSTRING,SC_INOUT,pdtSTRING,SC_CONST);
  pstCHR2STR = insert_function("_ZPL_CHR2STR",pdtSTRING,1,pdtCHAR,SC_CONST);
  pexprNULLSTR = build_0ary_op(VARIABLE,insert_constant("_ZPL_NULLSTR",pdtSTRING));
  pexprUNKNOWN = build_0ary_op(VARIABLE,insert_constant("???",pdtSTRING));
  insert_constant("_ZPL_STRLEN",pdtINT);

  ExternalDep = insert_variable("ExternalDep",pdtGENERIC);



  /* just some things that Brad was rude enough to insert */
  insert_function("_T_GOOD_ARR",pdtINT,1,pdtGENERIC_ENSEMBLE,SC_INOUT);
  pst_NULL = insert_variable("_ZPL_NULL",pdtLONG);

  sprintf(name, "CurrentRegion");
  pstCurrentRegion = insert_constant(name,  pdtRANK_REGION[0]);

  /* quote regions/masks */
  for (i=0;i<=MAXRANK;i++) {
    sprintf(name,"%s.%s",RMS_NAME,RMS_REG);
    pst_qreg[i] = alloc_st(S_VARIABLE, name);
    S_STD_CONTEXT(pst_qreg[i]) = TRUE;
    S_DTYPE(pst_qreg[i]) = pdtRANK_REGION[i];
  }
  
  sprintf(name, "/* free region */");
  pst_free = alloc_st(S_VARIABLE, name);
  S_DTYPE(pst_free) = pdtRANK_REGION[0];

  sprintf(name,"_MagicFloodReg");
  pst_mfreg = insert_variable(name,pdtGENERIC);

  sprintf(name,"_QUOTE_MASK@");
  gen_quotemask = build_0ary_op(VARIABLE,
				insert_constant(name,
					      pdtGENERIC));


  for (i=0; i<=MAXRANK; i++) {
    sprintf(name,"_RGridScalar%dD",i+1);
    pstGRID_SCALAR[i] = insert_constant(name,pdtGRID_SCALAR[i]);
  }

  for (i=0;i<=MAXRANK;i++) {
    symboltable_t* maskpst;

    sprintf(name, "%s.%s", RMS_NAME, RMS_MASK);

    maskpst = alloc_st(S_VARIABLE, name);
    S_STYPE(maskpst) |= SC_CONSTANT;
    S_DTYPE(maskpst) = pdtRANK_ENSEMBLE[i];
    S_STD_CONTEXT(maskpst) = TRUE;
    pexpr_qmask[i] = build_0ary_op(VARIABLE, maskpst);

    D_ENS_REG(pdtRANK_ENSEMBLE[i]) = buildqregexpr(i);
    D_ENS_COPY(pdtRANK_ENSEMBLE[i]) = NULL;
  }

  enter_block(NULL);
  define_componentlist
    (build_sym_list
     (build_sym_list
      (build_sym_list(NULL, alloc_symlist("reg")),
       alloc_symlist("mask")),
      alloc_symlist("withflag")),pdtINT);

  rmsframe = build_record_type(S_STRUCTURE);
  D_NAME(rmsframe) = makepst("_rmsframe");
  exit_block();

  for (i=0; i<MAXRANK; i++) {
    int tile = 16;
    
    sprintf(name, "_ms_ts%d", i);
    pst_ms_tile_size[i] = alloc_loc_st(S_VARIABLE, name); 
    S_DTYPE(pst_ms_tile_size[i]) = pdtINT; 
    sprintf (buffer, "%d", tile); /* for build_int */
    S_INIT(pst_ms_tile_size[i]) = 
      build_init(build_0ary_op(CONSTANT,build_int(tile)),NULL);
  }

  pst_dcid = alloc_st(S_VARIABLE, "_dcid"); 
  S_LEVEL(pst_dcid) = 1;
  S_DTYPE(pst_dcid) = pdtINT; 

  pst_MSCANREG = alloc_st(S_VARIABLE, "_MSCANREG"); 
  S_LEVEL(pst_MSCANREG) = 1;
  S_DTYPE(pst_MSCANREG) = alloc_dt(DT_REGION);

  for (i=1; i<MAXRANK; i++) {
    char autoregname[16];
    sprintf(autoregname, "_AutoRedReg%d", i);
    pstAutoRedReg[i] = alloc_st(S_VARIABLE, autoregname); 
    S_DTYPE(pstAutoRedReg[i]) = alloc_dt(DT_REGION);
    D_REG_NUM(S_DTYPE(pstAutoRedReg[i])) = i;
    S_SETUP(pstAutoRedReg[i]) = -1;
  }
}


static void add_standard_context(void) {
  int i;
  char s[] = "IndexXXXRMStack[X]";

  pstCalcRedReg = insert_function("_CalcRedReg",pdtREGION,3,pdtREGION,SC_INOUT,pdtREGION,SC_INOUT,pdtREGION,SC_INOUT);
  insert_function("_MSCAN_REGINIT", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtINT, SC_CONST);
  insert_function("_MSCAN_REGINIT_DIM_UP", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtINT, SC_CONST);
  insert_function("_MSCAN_REGINIT_DIM_DN", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtINT, SC_CONST);
  insert_function("_MSCAN_INIT", pdtGENERIC,0);
  insert_function("_MSCAN_POST", pdtGENERIC,0);
  insert_function("_CC_INBUFFER", pdtGENERIC,1,pdtGENERIC, SC_CONST);
  insert_function("_CC_INBUFFER_I", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_CC_INT_INBUFFER_I", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_CC_CHAR_INBUFFER_I", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_CC_DOUBLE_INBUFFER_I", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);

  insert_function("_CC_OUTBUFFER", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_CC_OUTBUFFER_I", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_CC_INT_OUTBUFFER_I", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_CC_CHAR_OUTBUFFER_I", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_CC_DOUBLE_OUTBUFFER_I", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);

  insert_function("_COPY_INT_STMT", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_COPY_DOUBLE_STMT", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_COPY_CHAR_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_COPY_STMT", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_ADD_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_MULT_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_MAX_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_MIN_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_AND_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_OR_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_XOR_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_BAND_STMT", pdtGENERIC,2,  pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("_BOR_STMT", pdtGENERIC,2, pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);

  for (i=0; i<MAXRANK; i++) {
    /* Insert symbols for Index1, Index2, etc.. */
    sprintf(s, "Index%d", i+1);
    pstINDEX[i+1] = insert_constant(s,  pdtINT);
  }

  {
    expr_t* pexprOne;
    expr_t* pexprZero;
    datatype_t* pdtINTARR;

    sprintf(buffer, "1");
    pexprOne = build_0ary_op(CONSTANT, build_int(1));
    sprintf(buffer, "0");
    pexprZero = build_0ary_op(CONSTANT, build_int(0));

    pdtINTARR = build_array_type(build_dim_list(NULL,pexprOne, pexprZero),
				 pdtINT);

    for (i=0; i<MAXRANK; i++) {
      sprintf(s, "index%d", i+1);
      pstindex[i+1] = insert_constant(s,  pdtINT);
    }
  }

  /*** from /usr/include/limits.h and float.h ***/
  insert_constant("CHAR_BIT",  pdtINT);
  insert_constant("CHAR_MAX",  pdtCHAR);
  insert_constant("CHAR_MIN",  pdtCHAR);
  insert_constant("INT_MAX",  pdtINT);
  insert_constant("INT_MIN",  pdtINT);
  insert_constant("LONG_MAX",  pdtLONG);
  insert_constant("LONG_MIN",  pdtLONG);
  insert_constant("SBYTE_MAX",  pdtSIGNED_BYTE);
  insert_constant("SBYTE_MIN",  pdtSIGNED_BYTE);
  insert_constant("SHRT_MAX",  pdtSHORT);
  insert_constant("SHRT_MIN",  pdtSHORT);
  insert_constant("UBYTE_MAX",  pdtUNSIGNED_BYTE);
  insert_constant("UINT_MAX",  pdtUNSIGNED_INT);
  insert_constant("ULONG_MAX",  pdtUNSIGNED_LONG);
  insert_constant("USHRT_MAX",  pdtUNSIGNED_SHORT);

  insert_constant("FLT_RADIX",  pdtINT);
  insert_constant("FLT_DIG",  pdtINT);
  insert_constant("FLT_EPSILON",  pdtFLOAT);
  insert_constant("FLT_MANT_DIG",  pdtINT);
  insert_constant("FLT_MAX",  pdtFLOAT);
  insert_constant("FLT_MIN",  pdtFLOAT);

  insert_constant("DBL_RADIX",  pdtINT);
  insert_constant("DBL_DIG",  pdtINT);
  insert_constant("DBL_EPSILON",  pdtDOUBLE);
  insert_constant("DBL_MANT_DIG",  pdtINT);
  insert_constant("DBL_MAX",  pdtDOUBLE);
  insert_constant("DBL_MIN",  pdtDOUBLE);

  /*** leave these out for now, don't know ANSI for these ***/
  /*** insert_constant("QUAD_RADIX",  pdtINT);
  insert_constant("LDBL_DIG",  pdtINT);
  insert_constant("LDBL_EPSILON",  pdtQUAD);
  insert_constant("LDBL_MANT_DIG",  pdtINT); ***/
  insert_constant("LDBL_MAX",  pdtQUAD);
  insert_constant("LDBL_MIN",  pdtQUAD);

  /*** from /usr/include/math.h ***/
  insert_constant("M_E",  pdtDOUBLE);
  insert_constant("M_LOG2E",  pdtDOUBLE);
  insert_constant("M_LOG10E",  pdtDOUBLE);
  insert_constant("M_LN2",  pdtDOUBLE);
  insert_constant("M_PI",  pdtDOUBLE);
  insert_constant("M_PI_2",  pdtDOUBLE);
  insert_constant("M_PI_4",  pdtDOUBLE);
  insert_constant("M_1_PI",  pdtDOUBLE);
  insert_constant("M_2_PI",  pdtDOUBLE);
  insert_constant("M_2_SQRTPI",  pdtDOUBLE);
  insert_constant("M_SQRT2",  pdtDOUBLE);
  insert_constant("M_SQRT_2",  pdtDOUBLE);

  /*** constants defined for boolean type ***/
  insert_constant( "true",  pdtBOOLEAN );
  insert_constant( "false",  pdtBOOLEAN );

  /*** from /usr/include/macros.h ***/
  insert_function("max", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("min", pdtGENERIC,2,pdtGENERIC, SC_CONST, pdtGENERIC, SC_CONST);
  insert_function("abs", pdtGENERIC,1,pdtGENERIC, SC_CONST);
  insert_function("sqr", pdtDOUBLE,1,pdtGENERIC,SC_CONST);
  pstLSHIFT = insert_function("bsl",pdtINT,2,pdtINT,SC_CONST,pdtINT,SC_CONST);
  pstRSHIFT = insert_function("bsr",pdtINT,2,pdtINT,SC_CONST,pdtINT,SC_CONST);
  pstBOR = insert_function("bor", pdtINT,2,pdtINT,SC_CONST,pdtINT,SC_CONST);
  pstBAND = insert_function("band", pdtINT,2,pdtINT,SC_CONST,pdtINT,SC_CONST);
  pstBXOR = insert_function("bxor", pdtINT,2,pdtINT,SC_CONST,pdtINT,SC_CONST);
  pstBNOT = insert_function("bnot", pdtINT,1,pdtINT,SC_CONST);
  insert_function("bpop", pdtINT,1,pdtGENERIC,SC_CONST);
  insert_function("cube", pdtDOUBLE,1,pdtGENERIC,SC_CONST);
  insert_function("trunc", pdtDOUBLE,1,pdtGENERIC,SC_CONST);

  /*** from /usr/include/math.h ***/
  /* double precision */
  insert_function("acos", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("asin", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("atan", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("atan2", pdtDOUBLE,2, pdtDOUBLE, SC_CONST, pdtDOUBLE, SC_CONST);
  insert_function("ceil", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("cos", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("cosh", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("exp", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("fabs", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("floor", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("fmod", pdtDOUBLE,2, pdtDOUBLE, SC_CONST, pdtDOUBLE, SC_CONST);
  insert_function("ldexp", pdtDOUBLE,2, pdtDOUBLE, SC_CONST, pdtINT, SC_CONST);
  insert_function("log", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("log10", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("pow", pdtDOUBLE,2, pdtDOUBLE, SC_CONST, pdtDOUBLE, SC_CONST);
  insert_function("sin", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("sinh", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("sqrt", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("tan", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);
  insert_function("tanh", pdtDOUBLE,1, pdtDOUBLE, SC_CONST);

  /* single precision */
  insert_function("acosf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("asinf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("atanf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("atan2f", pdtFLOAT,2, pdtFLOAT, SC_CONST, pdtFLOAT, SC_CONST);
  insert_function("ceilf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("cosf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("coshf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("expf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("fabsf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("floorf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("fmodf", pdtFLOAT,2, pdtFLOAT, SC_CONST, pdtFLOAT, SC_CONST);
  insert_function("ldexpf", pdtFLOAT,2, pdtFLOAT, SC_CONST, pdtINT, SC_CONST);
  insert_function("logf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("log10f", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("powf", pdtFLOAT,2, pdtFLOAT, SC_CONST, pdtFLOAT, SC_CONST);
  insert_function("sinf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("sinhf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("sqrtf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("tanf", pdtFLOAT,1, pdtFLOAT, SC_CONST);
  insert_function("tanhf", pdtFLOAT,1, pdtFLOAT, SC_CONST);

  insert_constant("zin",pdtFILE);
  insert_constant("zout",pdtFILE);
  insert_constant("zerr",pdtFILE);
  insert_constant("znull",pdtFILE);

  insert_constant("file_read",pdtSTRING);
  insert_constant("file_write",pdtSTRING);

  /*** add checkpoint function to the standard context ***/
  chkpoint = insert_function("checkpoint",pdtGENERIC,0);

  if ((do_insert_checkpoints == TRUE) || (do_checkpoint == TRUE)) {

    /*** add internal variable declarations ***/
    chksave = insert_function("_chksave",pdtGENERIC,0);
    chkrecover = insert_function("_chkrecover",pdtGENERIC,0);
    chklabel = insert_function("_chklabel",pdtGENERIC,1,pdtUNSIGNED_LONG,SC_CONST);
    checkpointing = insert_variable("_checkpointing",  pdtINT);
    recovering = insert_variable("_recovering",  pdtINT);
    recoverfilename = insert_constant("_recoverfilename", pdtSTRING);
  }

  insert_function("to_boolean",pdtBOOLEAN,1,pdtGENERIC,SC_CONST);
  insert_function("to_integer",pdtINT,1,pdtGENERIC,SC_CONST);
  insert_function("to_float",pdtFLOAT,1,pdtGENERIC,SC_CONST);
  insert_function("to_longint",pdtLONG,1,pdtGENERIC,SC_CONST);
  insert_function("to_shortint",pdtSHORT,1,pdtGENERIC,SC_CONST);
  insert_function("to_double",pdtDOUBLE,1,pdtGENERIC,SC_CONST);
  insert_function("to_quad",pdtQUAD,1,pdtGENERIC,SC_CONST);
  insert_function("to_char",pdtCHAR,1,pdtGENERIC,SC_CONST);
  insert_function("to_ubyte",pdtUNSIGNED_BYTE,1,pdtGENERIC,SC_CONST);
  insert_function("to_sbyte",pdtSIGNED_BYTE,1,pdtGENERIC,SC_CONST);
  insert_function("to_uinteger",pdtUNSIGNED_INT,1,pdtGENERIC,SC_CONST);
  insert_function("to_ulongint",pdtUNSIGNED_LONG,1,pdtGENERIC,SC_CONST);
  insert_function("to_ushortint",pdtUNSIGNED_SHORT,1,pdtGENERIC,SC_CONST);

  add_internals();

  rmstack = insert_define("_RMStack", S_STRUCTURE, rmsframe);
}


void initialize_symtab() {
  int	i;
  
  for (i = 0; i < MAXHASH; i++) {
    symtab[i] = NULL;
  }
  
  for (i = 0; i < MAXLEVEL; i++)
    maplvl[i] = NULL;
  
  add_standard_context();
  
}

symboltable_t* get_blank_arrayref_index(int index, int depth) {
  char str[256];
  symboltable_t** pstptr;
  int i;

  if (index > MAXRANK) {
    INT_FATAL(NULL, "MAXRANK exceeded for blank array ref");
  }
  if (index == 0) {
    INT_FATAL(NULL, "Wasn't expecting anyone to want an index=0 index\n");
  }
  if (depth < 0) {
    INT_FATAL(NULL, "Depth must be positive\n");
  }
  pstptr = &(pstindex[index]);
  for (i=0; i<depth; i++) {
    if (*pstptr == NULL) {
      sprintf(str, "_ia%d_%d", index, i+1);
      *pstptr = alloc_st(S_VARIABLE,str);
      S_LEVEL(*pstptr) = -1;
      S_DTYPE(*pstptr) = pdtINT;
    }
    if (i != depth-1) {
      pstptr = &(S_NEXT(*pstptr));
    }
  }
  return *pstptr;
}
