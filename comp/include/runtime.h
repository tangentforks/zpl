/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __RUNTIME_H_
  
#define __RUNTIME_H_
#include <stdio.h>
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/symmac.h"
#include "../include/treemac.h"

#ifndef EXTERN
#define EXTERN extern
#endif

#ifndef FILEPATHLEN
#define FILEPATHLEN    256
#endif

#define ACCHEADFILE    "acc.h"
#define ACCHEADFILESTR "_ACC_H_"
#define ACCCODEFILE    "acc.c"

#define REGSPSFILE     "sps.c"

#define CFGCODEFILE    "cfg.c"
#define CFGCODEFILE2   "pcfg.c"

#define INITCODEFILE   "init.c"

#define ARR_NORMAL  0
#define ARR_STRIDED 1
#define ARR_SPARSE  2


/* accessor prototypes */

void StartAccessors(void);
void EndAccessors(void);
char *gen_accessors(symboltable_t *,expr_t *,int,int,datatype_t *);


/* config prototypes */

void StartConfig(void);
void EndConfig(FILE*);
void DoConfig(FILE*, symboltable_t*);


/* direction prototypes */

void gen_dir_name_as_ident(FILE*, expr_t*);
void direxpr_gen_comp(FILE*, expr_t*, int);
void dir_gen_comp(FILE*, expr_t*, int);        /* print +%dir_i */
void dir_gen_comp_offset(FILE*, expr_t*, int); /* print +%dir_i */
long dir_comp_to_int(expr_t*, int, int*);
signclass dir_get_sign(expr_t*, int);

/* genfluff prototypes */

void gen_fluffval(FILE *,int,int,int,char*);
void gen_fluff_decls(FILE *,expr_t *,int);

void gen_fluff_setup(FILE *,char*,symboltable_t *,int, int);


/* make prototypes */

void GenerateZMakefile(void);

/* perregion prototypes */

void GenGlobalSetup(FILE*, symboltable_t*, datatype_t*, expr_t*, char*, int);
void GenLocalSetup(FILE*, symboltable_t*, datatype_t*, expr_t*, char*, int);


/* perregion_sparse prototypes */

void gen_sps_loc_decls(FILE*,char*, datatype_t*);
void gen_sps_special_mloop_stmt(FILE*,statement_t*, int);
void gen_sparse_fluff(FILE*, char*, symboltable_t*, datatype_t*);
void gen_sparse_structure(FILE*, symboltable_t *, char*, datatype_t*, expr_t*);


/* runtime prototypes */

void StartRuntime(void);
void EndRuntime(void);

/* typeensemble prototypes */

void GenAllocEnsemble(FILE*, symboltable_t*, char*, datatype_t*, int, int);
void GenFreeEnsemble(FILE*, symboltable_t*, char*, datatype_t*, int, int);

#endif
