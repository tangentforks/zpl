/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***************************************************************************
	Center for Supercomputing Research and Development
	at the University of Illinois at Urbana-Champaign
		305 Talbot Laboratory
		104 South Wright Street
		Urbana, Illinois  61801-2932 USA

Write to the above address for information on the CSRD Software Catalog,
or send e-mail to:  softcat@csrd.uiuc.edu

Copyright (c) 1991 by the Board of Trustees of the University of Illinois
*unpublished*.  All rights reserved.  This proprietary created material is
provided under a license agreement which restricts its use and protects its
confidentiality.  No publication or waiver of confidentiality is implied by
this transfer under the license.
***************************************************************************/

#ifndef	SYMTAB_H
#define	SYMTAB_H

#include "const.h"
#include "struct.h"
#include "masktype.h"

typedef	struct	dimension_struct	dimension_t;
typedef	struct	initial_struct		initial_t;

/* If changed, update symbolnames[] in buildzplstmt.c */
typedef	enum	{
	S_VARIABLE,
	S_LITERAL,
	S_TYPE,
	S_SUBPROGRAM,
	S_PARAMETER,
	S_COMPONENT,
	S_ENUMELEMENT,
	S_UNKNOWN,
	S_ENUMTAG,
	S_STRUCTURE,
	S_VARIANT
} symbolclass;

typedef	enum	{
	DT_INTEGER,	
	DT_REAL,	
	DT_CHAR,	
	DT_ARRAY,	
	DT_FILE,
	DT_ENUM,
	DT_STRING,
	DT_STRUCTURE,	
	DT_VOID,	
	DT_SUBPROGRAM,	
	DT_GRID,
	DT_DISTRIBUTION,
	DT_REGION,
	DT_ENSEMBLE,
	DT_DIRECTION,
	DT_SHORT,
	DT_LONG,
	DT_DOUBLE,
	DT_QUAD,
	DT_UNSIGNED_INT,
	DT_UNSIGNED_SHORT,
	DT_UNSIGNED_LONG,
	DT_SIGNED_BYTE,
	DT_UNSIGNED_BYTE,
	/*** sungeun *** generic data types ***/
	/*** These are necessary for functions which are part of our ***/
	/*** standard context, such as print routines which can take ***/
	/*** any type of ensemble as an argument and also the abs() ***/
	/*** macro from macros.h which can take any type and return ***/
	/*** that type. ***/
	DT_GENERIC,		/*** any data type ***/
	DT_GENERIC_ENSEMBLE,	/*** an ENSEMBLE of any datatype ***/
	DT_PROCEDURE,	        /* procedure parameters for external C */
				/* functions -- not to be confused with */
				/* with a DT_SUBPROGRAM */
	DT_BOOLEAN,
	DT_OPAQUE,
	DT_COMPLEX,
	DT_DCOMPLEX,
	DT_QCOMPLEX
} typeclass;

#define	SC_EXTERN        01
#define SC_CONFIG	 02
#define SC_FREE          04
#define	SC_REGISTER	 010
#define	SC_STATIC	 020
#define SC_CONSTANT      040

typedef enum {
  NONSENSE_REGION,
  INTERNAL_REGION,
  SIMPLE_REGION,
  DYNAMIC_REGION,
  OF_REGION,
  IN_REGION,
  AT_REGION,
  BY_REGION,
  SPARSE_REGION,
  WITH_REGION,
  WITHOUT_REGION,
  SPARSE_WITH_REGION,
  SPARSE_WITHOUT_REGION
} regionclass;

typedef enum {
  SC_NORMAL,
  SC_EINIT,
  SC_CONST,
  SC_IN,
  SC_OUT,
  SC_INOUT
} subclass;

typedef enum {
  DIM_INHERIT=0,
  DIM_FLOOD,
  DIM_GRID,
  DIM_RANGE,
  DIM_FLAT,
  DIM_SPARSE,
  DIM_DYNAMIC  /* The ? case... */
} dimtypeclass;

typedef enum {
  SIGN_ZERO=0,
  SIGN_POS,
  SIGN_NEG,
  SIGN_UNKNOWN,
  SIGN_POSONE,
  SIGN_NEGONE
} signclass;

typedef enum {
  DIM_BLOCK=0,
  DIM_CYCLIC,
  DIM_MULTIBLOCK,
  DIM_NONDIST,
  DIM_IRREGULAR
} distributionclass;

struct	dimension_struct	{
  structtype	st_type;

  dimtypeclass  dimtype;
  expr_t	*lo_var;	
  expr_t	*hi_var;	

  dimension_t	*nextdim;
};

struct	symlist_struct {
  structtype st_type;	
  char* entry;
  symlist_t* next_p;
};

struct	initial_struct {
	structtype	st_type;	
	int		brace;		
					
	int		repeat;
	union	{
		expr_t		*value;	
		initial_t	*list;	
		int		ivalue;	
	} u;
	initial_t	*next;
};


struct hashentry_struct {
	char	*ident;		
	symboltable_t	*pst;	
				
	symboltable_t	*last;	
				
				
					
	hashentry_t	*he_next;	
				
};


struct	symboltable_struct	{
  structtype	st_type;		
  struct	{	
    unsigned active_bit : 1;	
    unsigned delete_bit : 1;	
    unsigned equiv_bit  : 1;	
    unsigned shadow_bit : 1;
    unsigned type_bit   : 1;
    unsigned param_bit : 1;
    unsigned future_6 : 1;		
    unsigned future_7 : 1;		
    unsigned future_8 : 1;
    unsigned future_9 : 1;
    unsigned future_0 : 1;
    unsigned flag_1 : 1;
    unsigned flag_2 : 1;
    unsigned flag_3 : 1;
    unsigned flag_4 : 1;
    unsigned flag_5 : 1;
    unsigned used : 1;
  } flags;
  symbolclass	sclass;
  int		level;
  char		*ident;		
  hashentry_t	*phe;		
  symboltable_t	*parent;	
  symboltable_t	*sib_prev;	
  symboltable_t	*sib_next;	
  symboltable_t	*next;
  datatype_t* type;
  stmtlist_t	*inlist;        
  stmtlist_t	*outlist;       
  int		storagetype; 
  initial_t	*init_list;
  int		offset;

  subclass	sub_class;

  int           fluff[MAXRANK][2];
  int           unknown_fluff[MAXRANK][2];
  int           wrap_fluff[MAXRANK][2];
  int		mask;  /* if ensemble, true if used as mask */
  int		std_context;		/*** sungeun ***/
  int		lineno;		/* source line number of declaration */
  char		*filename_p;    /* source file name of declatation */
  int           setup;          /* use to suppress setup */
  int           anon;           /* use to mark anonymous */
  genlist_t     *actuals;       /* used to store aliases */
  expr_t        *mask_expr;     /* used to store the mask expression */
  masktype      mask_with;      /* mask type */
  int           numinstances;   /* number of instances */
  int           inst_loop_num;  /* associate inst variable with loop #... */
  int           nogen;          /* restrict where MLOOP vars are genned */
};

struct	datatype_struct	{
  structtype	st_type;	
  symboltable_t	*pstName;
  typeclass     sclass;
  union	{
    struct {
      dimtypeclass griddims[MAXRANK];
      int num_dim;
    } grid;
    struct {
      int num_dim;
      distributionclass dims[MAXRANK];
      symboltable_t* grid;
    } distribution;
    struct {
      int num_dim;
      dimtypeclass dimtype[MAXRANK];
      symboltable_t* distribution;
      
      /* for optimizing sparse region storage */
      int           need_ind[MAXRANK];   /* need indices? */
      int           need_next[MAXRANK];
      int           need_prev[MAXRANK];
      int           need_dense_dir[MAXRANK];
    } region;
    struct {
      datatype_t* type;
      int num_dim;
      expr_t* region;
      int do_not_register;
      int mask;
      symboltable_t* copyfn;
    } ensemble;
    struct {
      int num_dim;
      signclass sign[MAXRANK];
    } direction;
    struct {
      datatype_t* type;
      int num_dim;
      dimension_t* dim_list;
      int array_type;
    } array;
    struct {
      function_t* body;
      datatype_t* return_type;
    } function;
    struct {
      symboltable_t* tag;
      symboltable_t* list;
    } record;
  } u;
};

#endif
