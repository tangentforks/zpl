/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __DGEN_H_
#define __DGEN_H_

#include <stdio.h>
#include "struct.h"
#include "symtab.h"

#ifndef DGEN_DECL
#define DGEN_DECL extern
#endif

#define DTF_SPACE 0x0000001 /* type should end with a space */
#define DTF_SHORT 0x0000010 /* type should use short form if available */
#define DTF_PARAM 0x0000100 /* type is a parameter */
#define DTF_RECRS 0x0001000 /* type is part of a recursive type */
#define DTF_ONEWD 0x0010000 /* type should be one word if available */
#define DTF_SETUP 0x0100000 /* type is part of ensemble setup */
#define DTF_CSTBL 0x1000000 /* type should be a legal cast */

#define PDT_TYPE  0x0000001 /* typedef definition */
#define PDT_CAST  0x1000010 /* cast */
#define PDT_PCST  0x0000010 /* pointer cast */
#define PDT_GLOB  0x0000011 /* global variable declaration */
#define PDT_PARM  0x0000110 /* parameter declaration */
#define PDT_EGLB  0x0100011 /* ensemble global w/ initializer */
#define PDT_EPRM  0x0100111 /* ensemble setup parameter */
#define PDT_LOCL  0x0000011 /* local variable declaration */
#define PDT_RVAL  0x1000011 /* return value of a function */
#define PDT_COMP  0x0001011 /* component of a structure */
#define PDT_SIZE  0x0000010 /* sizeof() expression */
#define PDT_SRED  0x0010000 /* scan/reduce routines */
#define PDT_NAME  0x0010000 /* used for generating names with types embedded */

DGEN_DECL function_t *codegen_fn;

/* Dgen.c routines */

void StartDgen(void);
void EndDgen(void);

void GenDistributionSetup(FILE*, symboltable_t*, datatype_t*, char*, expr_t*, int);
void GenGridSetup(FILE*, symboltable_t*, datatype_t*, char*, expr_t*, int);
void gen_function_header(FILE*, symboltable_t*, int);
dimension_t *gen_pdt(FILE *,datatype_t *,int);
char* return_pdt(datatype_t *,int, char *);
void gen_name(FILE*, expr_t*);
void gen_pst(FILE*,symboltable_t*);
void gen_pst_init(FILE*, symboltable_t*);
void force_pst_init(FILE*, symboltable_t*);
void gen_pst_finalize(FILE*, symboltable_t*);
void force_pst_finalize(FILE*, symboltable_t*);
void gen_pst_done(FILE*, symboltable_t*);
void force_pst_done(FILE*, symboltable_t*);
void gen_pst_ls(FILE*,symboltable_t*);
void gen_pst_init_ls(FILE*,symboltable_t*);
void gen_pst_finalize_ls(FILE*,symboltable_t*);
void gen_pst_done_ls(FILE*,symboltable_t*);
void gen_psc(FILE*,int);
void gen_array_size(FILE*,dimension_t*,datatype_t*);
void gen_array_sizes(FILE*,dimension_t*,datatype_t*);
void gen_subprog_decls(void);
void gen_init_list(FILE*,initial_t *);
void gen_finalize(module_t*);

/* Dgenhelp.c routines */

void gen_dual_ensemble_pst(FILE*,symboltable_t*,int);
dimension_t* gen_dual_ensemble_pdt(FILE*,datatype_t*,int);

#endif
