/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/** shift reduce errors:

User defined reductions and scans can sometimes work on a list of
expressions in parentheses. This conflicts with parentheses around
expressions. This should not be a problem.

**/

%{
#include <stdlib.h>
#include <stdio.h>
#include "../include/error.h"
#include "../include/dtype.h"
#include "../include/db.h"
#include "../include/dbg_code_gen.h"
#include "../include/const.h"
#include "../include/struct.h"
#include "../include/symtab.h"
#include "../include/symmac.h"
#include "../include/parsetree.h"
#include "../include/global.h"
#include "../include/cglobal.h"
#include "../include/treemac.h"
#include "../include/symbol.h"
#include "../include/buildsym.h"
#include "../include/buildsymutil.h"
#include "../include/buildstmt.h"
#include "../include/buildzplstmt.h"
#include "../include/stmtutil.h"
#include "../include/allocstmt.h"
#include "../include/scan.h"

#define YYMAXDEPTH 800
%}

%start program

%union	{
  int i;
  double r;
  char ch;
  char* str;
  int* pint;
  exprtype et;
  binop_t ot;
  permutetype_t pt;
  struct symboltable_struct *pst;
  struct dimension_struct *pdl;
  struct symlist_struct *psl;
  struct expr_struct *pe;
  struct datatype_struct *pdt;
  struct statement_struct *ps;
  struct initial_struct *pit;
  struct module_struct *pm;
  dimtypeclass dtc;
  regionclass rgc;
}

%token TARRAY TBEGIN TBLOCK TBOOLEAN TBREAD TBWRITE TCHAR TCOMPLEX TCONFIG TCONST
%token TCONTINUE TCYCLIC TDCOMPLEX TDIRECTION TDO TDISTRIBUTION TDOUBLE TDOWNTO TELSE TELSIF
%token TEND TEXIT TEXTERN TFILE TFOR TGENERIC TGENERICENSEMBLE TGRID THALT TIF
%token TINT TIRREGULAR TLONG TMSCAN TMULTIBLOCK TNONDIST TOPAQUE TPROCEDURE TPROGRAM TPROTOTYPE TQCOMPLEX
%token TQUAD TREAD TRECORD TREFLECT TREGION TREPEAT TRETURN TSBYTE TSCALEDBY
%token TSHORT TSTRING TTHEN TTO TTYPE TFREE TUNION TUNSBYTE TUNSINT
%token TUNSLONG TUNSSHORT TUNTIL TVAR TWHILE TWRAP TWRITE TWRITELN 
%token TINOUT TOUT TSTATIC
%token TLP TRP TLCBR TRCBR TLSBR TRSBR
%token TCOLON TCOMMA TDOTDOT TQUEST TQUOTE TSEMI
%token TNOT

%token TRGRID

%token <str> TIDENT TTYPEIDENT

%token <str> TUSRREDUCE TUSRSCAN

%token TANDREDUCE TMAXREDUCE  TMINREDUCE  TORREDUCE TPLUSREDUCE TTIMESREDUCE
%token TANDSCAN   TMAXSCAN    TMINSCAN    TORSCAN   TPLUSSCAN   TTIMESSCAN
%token TBORREDUCE TBANDREDUCE TBXORREDUCE
%token TBORSCAN   TBANDSCAN   TBXORSCAN

%token <str> TSTR
%token <i>   TINTEGER
%token <r>   TFLOAT
%token <ch>  TCHARACTER

%token TNOSEMICHECKS TNOENDCHECKS

%nonassoc  TPRESERVE TDESTROY TASSIGN TASSPLUS TASSMINUS TASSMULT TASSDIVIDE TASSMODULUS TASSAND TASSOR
%left      TREGIONAT TBY TWITH TWITHOUT TSWITH TSWITHOUT
%right     TOF TIN
%left      TOR
%left      TAND
%left      TLESS TGREATER TLESSEQUAL TGREATEREQUAL TEQUAL TNOTEQUAL
%left      TPLUS TMINUS
%left      TSTAR TDIVIDE TMODULUS
%right     TFLOOD TREDUCE TUNARY
%left      TEXP
%left      TPERMUTE TAT TWRAPAT TPRIME TDOT

%type <et> assignop
%type <ot> assignop_gets reductionop scanop
%type <i> integer tsemi tend
%type <pdl> dimlist
%type <pdl> sbe_item_list sbe_item
%type <pe> sbe
%type <pe> mrlist_elem mrlist mega_region
%type <pdt> proto_type_denoter ext_type_denoter type_denoter nonparallel_type_denoter returntype protoreturntype
%type <pdt> direction_type direction_type_component
%type <pdt> region_type distribution_type distribution_type_component grid_type grid_type_component
%type <dtc> region_type_component
%type <pe> lvalue base_lvalue funccall constval expr sbe_expr exprlist nonnull_exprlist assignment
%type <pe> ioexpr ioexprlist nonnull_ioexprlist
%type <pint> integerlist
%type <pit> init initlist
%type <pm> extdefs
%type <ps> stmt nonempty_stmtls stmtls
%type <ps> io_stmt compound_stmt mscan_stmt if_stmt elsif_stmt loop_stmt expr_stmt control_stmt wrapreflect_stmt
%type <psl> idlist ident
%type <pst> string enumlist enumerator
%type <ps>  region_spec
%type <pst> extdef fndef
%type <str> identifier
%type <pe> direction_ref
%type <pe> direction_init_ref
%type <pit> direction_init
%type <rgc> maskop
%type <i> is_free param_type

/* The following have been added to provide more accurate line numbers.
 * Each of the compound control flow statements now has a new rule whose
 * sole purpose is to record the line number that it starts on.
 * 
 * This makes the code really gross, but...
 */
%type <i>   tbread tbwrite
%type <i>   tif twhile trepeat tfor telsif tbegin tmscan trecord
%type <i>   tcontinue texit thalt treturn twrap treflect tread twrite twriteln
%type <i>   tequal tcolon tprocedure tprototype tprogram 

%%

/* To disable semicolon and end checking, switch the rules below:
tnosemichecks: TNOSEMICHECKS;
tnoendchecks: TNOENDCHECKS;
*/
tnosemichecks: ;
tnoendchecks: ;


program:	tprogram identifier 
			{
                          entry_name = $2; /* save name of entry point */
			}
		tsemi
                extdefs
			{ 
			  zpl_module = finish_module($5); 
			}
	;

extdefs:	extdef
			{ $$ = add_module(NULL, $1); }
	|	extdefs extdef
			{ $$ = add_module($1, $2); }
	;

extdef: 	TDIRECTION   directiondefs
			{ $$ = NULL;  }
	|	TREGION      regiondefs
			{ $$ = NULL;  }
        |       TEXTERN TTYPE exttypedefs
                        { $$ = NULL;  }
	|	TTYPE        typedefs
			{ $$ = NULL;  }
        |       TEXTERN TVAR extvardefs
                        { $$ = NULL;  }
        |       TEXTERN TFREE TVAR extfreevardefs
                        { $$ = NULL;  }
	|	TVAR         vardefs
			{ $$ = NULL;  }
        |       TFREE TVAR freevardefs
                        { $$ = NULL;  }
	|	TCONFIG TCONST configconstdefs
			{ $$ = NULL;  }
        |       TCONFIG TVAR configdefs
			{ $$ = NULL;  }
	|	TCONST    constdefs
			{ $$ = NULL;  }
	|	TEXTERN TCONST extconstdefs
			{ $$ = NULL;  }
        |       TGRID        griddefs
                        { $$ = NULL;  }
        |       TDISTRIBUTION      distributiondefs
                        { $$ = NULL;  }
        |       fndef
                        { $$ = $1; }
;

/* One or more of each: */
constdefs:	constdefs	constdef	| constdef;
extconstdefs:   extconstdefs    extconstdef     | extconstdef;
directiondefs:	directiondefs	directiondef	| directiondef;
exttypedefs:	exttypedefs	exttypedef	| exttypedef;
typedefs:	typedefs	typedef		| typedef;
extvardefs:	extvardefs	extvardef	| extvardef;
extfreevardefs:	extfreevardefs	extfreevardef	| extfreevardef;
vardefs:	vardefs		vardef		| vardef;
freevardefs:     freevardefs      freevardef       | freevardef;
staticvardefs:	staticvardefs		staticvardef		| staticvardef;
staticfreevardefs:     staticfreevardefs      staticfreevardef       | staticfreevardef;
configdefs:	configdefs	configdef	| configdef;
configconstdefs: configconstdefs configconstdef | configconstdef;
regiondefs:	regiondefs	regiondef	| regiondef;
distributiondefs:     distributiondefs      distributiondef       | distributiondef;
griddefs:       griddefs        griddef         | griddef;

extvardef:	idlist tcolon ext_type_denoter tsemi
		{ 
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,TRUE,FALSE,FALSE,$1,$3,NULL,$2, in_file); 
		}
	;

extfreevardef:	idlist tcolon ext_type_denoter tsemi
		{ 
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,TRUE,TRUE,FALSE,$1,$3,NULL,$2, in_file); 
		}
	;

vardef:		idlist tcolon type_denoter tsemi
		{ 
		  check_name_list_conflict($1);
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,FALSE,FALSE,FALSE,$1,$3,NULL,$2, in_file); 
		}
        |
                idlist tcolon type_denoter tequal init tsemi
		{ 
		  check_name_list_conflict($1);
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,FALSE,FALSE,FALSE,$1,$3,$5,$2, in_file); 
		}
	;

freevardef:	idlist tcolon type_denoter tsemi
		{ 
		  check_name_list_conflict($1);
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,FALSE,TRUE,FALSE,$1,$3,NULL,$2, in_file); 
		}
        |
                idlist tcolon type_denoter tequal init tsemi
		{ 
		  check_name_list_conflict($1);
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,FALSE,TRUE,FALSE,$1,$3,$5,$2, in_file); 
		}
	;

staticvardef:		idlist tcolon type_denoter tsemi
		{ 
		  check_name_list_conflict($1);
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,FALSE,FALSE,TRUE,$1,$3,NULL,$2, in_file); 
		}
        |
                idlist tcolon type_denoter tequal init tsemi
		{ 
		  check_name_list_conflict($1);
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,FALSE,FALSE,TRUE,$1,$3,$5,$2, in_file); 
		}
	;

staticfreevardef:        idlist tcolon type_denoter tsemi
		{ 
		  check_name_list_conflict($1);
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,FALSE,TRUE,TRUE,$1,$3,NULL,$2, in_file); 
		}
        |
                idlist tcolon type_denoter tequal init tsemi
		{ 
		  check_name_list_conflict($1);
		  define_varconstlist(S_VARIABLE,FALSE,FALSE,FALSE,TRUE,TRUE,$1,$3,$5,$2, in_file); 
		}
	;

configdef:	TIDENT tcolon type_denoter tequal init tsemi
		{ 
		  check_name_conflict($1);
		  define_varconst(S_VARIABLE, FALSE, TRUE, FALSE, FALSE, FALSE, $1, $3, $5, $2, in_file); 
		}
	|       TIDENT tcolon type_denoter tsemi
		{ /* -- error rule -- */
		  YY_FATAL_CONTX(yylineno, in_file,
				 "config variables must be initialized");
		}
	;

configconstdef:	TIDENT tcolon type_denoter tequal init tsemi
		{ 
		  check_name_conflict($1);
		  define_varconst(S_VARIABLE, TRUE, TRUE, FALSE, FALSE, FALSE, $1, $3, $5, $2, in_file); 
		}
	|       TIDENT tcolon type_denoter tsemi
		{ /* -- error rule -- */
		  YY_FATAL_CONTX(yylineno, in_file,
				 "config constants must be initialized");
		}
	;

extconstdef:	TIDENT tcolon type_denoter tequal init tsemi
		{ 
		  check_name_conflict($1);
		  define_varconst(S_VARIABLE,TRUE, FALSE, TRUE, FALSE, FALSE, $1, $3, $5, $2, in_file);
		}
	|       TIDENT tcolon type_denoter tsemi
		{
		  check_name_conflict($1);
		  define_varconst(S_VARIABLE, TRUE, FALSE, TRUE, FALSE, FALSE, $1, $3, NULL, $2, in_file); 
		}
	;

constdef:	TIDENT tcolon type_denoter tequal init tsemi
		{ 
		  check_name_conflict($1);
		  define_varconst( S_VARIABLE, TRUE, FALSE, FALSE, FALSE, FALSE, $1, $3, $5, $2, in_file); 
		}
	|       TIDENT tcolon type_denoter tsemi
		{ /* -- error rule -- */
		  YY_FATAL_CONTX(yylineno, in_file,
				 "consts must be initialized");
		}
	;

distributiondef:  TIDENT TCOLON TIDENT tequal init tsemi
                {
		  create_distribution($1, $3, $5, yylineno, in_file);
		}
	|	
		TIDENT tequal init tsemi
                {
		  create_distribution($1, NULL, $3, yylineno, in_file);
		}
        ;

griddef:    TIDENT tequal init tsemi
                {
		  create_grid($1, $3, yylineno, in_file);
		}
        ;

direction_ref: sbe
		{
		  symboltable_t* newdir;
		  newdir = define_direction(NULL, 
					    build_init($1, NULL), yylineno, 
					    in_file);
		  $$ = build_0ary_op(VARIABLE, newdir);
		}
	|	base_lvalue
		{
		  $$ = $1;
		}
	;

direction_init_ref: sbe
		{
		  $$ = $1;
		}
	| base_lvalue
		{
		  $$ = $1;
		}
	;

direction_init: direction_init_ref
		{
		  $$ = build_init($1, NULL);
		}
	;


directiondef:	TIDENT tequal direction_init tsemi
		{ 
                  check_name_conflict($1);
		  define_direction($1, $3, $2, in_file);
		}
	;

regiondef:	TIDENT tequal sbe_expr tsemi
		{ 
		  check_name_conflict($1);
		  define_varconst(S_VARIABLE,TRUE, FALSE, FALSE, FALSE, FALSE, $1, 
		                  convert_sbe_to_region_type($3, NULL, $1, $2, in_file), 
		                  build_init($3, NULL), $2, in_file);
		}
        | 	TIDENT TCOLON TIDENT tequal sbe_expr tsemi
		{ 
		  check_name_conflict($1);
		  check_name_conflict($3);
		  define_varconst(S_VARIABLE,TRUE, FALSE, FALSE, FALSE, FALSE, $1, 
		                  convert_sbe_to_region_type($5, $3, $1, $4, in_file), 
		                  build_init($5, NULL), $4, in_file);
		}
	;

exttypedef:	TIDENT tequal ext_type_denoter tsemi
		{ 
		  define_type( $1, $3, TRUE, $2, in_file); 
		}
	;

typedef:	TIDENT tequal type_denoter tsemi
		{ 
		  check_name_conflict($1);
		  define_type( $1, $3, FALSE, $2, in_file); 
		}
	;

sbe_item:       /* nothing */
		{
		  $$ = build_dimension(DIM_INHERIT);
		}
        |       TSTAR
		{
		  $$ = build_dimension(DIM_FLOOD);
		}
	|       TRGRID
		{
		  $$ = build_dimension(DIM_GRID);
		}
        |       TQUEST
                {
		  $$ = build_dimension(DIM_DYNAMIC);
		}
	|       expr TDOTDOT expr
		{ $$ = build_dim_list( NULL, $1, $3); }
        |       sbe_expr
		{ $$ = build_dim_list( NULL, $1, $1); }
	;

sbe_item_list:  sbe_item
                {
                  $$ = $1;
		}
        |
                sbe_item_list TCOMMA sbe_item
                {
		  $$ = append_dim_list($1, $3);
		}
        ;

sbe:            TLSBR sbe_item_list TRSBR
                {
		  $$ = build_sbe_expr($2);
		}
        ;


		/* region_spec: A region specifier */
region_spec:    sbe
                {
		  $$ = build_region_scope($1, yylineno, in_file);
		}
	;


mrlist_elem:	/* nothing */
		{ $$ = build_0ary_op(VARIABLE,NULL); }
	| expr
		{ $$ = $1; }
	| TRGRID expr
		{ $$ = cat_expr_ls(build_0ary_op(VARIABLE,lu_pst("_rgridremap")), $2); }
	;


mrlist:		mrlist_elem
		{ $$ = cat_expr_ls(NULL,$1); }
	|	mrlist TCOMMA mrlist_elem
		{ $$ = cat_expr_ls($1,$3); }
	;


mega_region:	TLSBR mrlist TRSBR
	{ $$ = $2; }
	;


		/* dimlist: Specifies a dimension of an array */
dimlist:	expr TDOTDOT expr
		{ $$ = build_dim_list( NULL, $1, $3); }
	|	dimlist   TCOMMA  expr TDOTDOT expr
		{ $$ = build_dim_list( $1, $3, $5); }
	|       expr               	    /* allow degenerate dimensions */
		{
		  YY_WARNX(yylineno, in_file, "Indexed array has degenerate dimension");
		  $$ = build_dim_list( NULL, $1, $1); }
	|	dimlist   TCOMMA  expr
		{ $$ = build_dim_list( $1, $3, $3); }
	|	expr TCOLON expr
		{ 
		  YY_FATAL_CONTX(yylineno, in_file, "expected '..' "
				 "but found ':'");
		  $$ = build_dim_list( NULL, $1, $3);
		}
	;


/****************************************************************
 * TYPES:    (derived from ISO_PASCAL grammar)
 ****************************************************************/

trecord:  TRECORD	
			{ $$ = yylineno; }
	;

proto_type_denoter:	/* type denoter for function prototypes: 
			   allows regions, directions, and functions */
	  type_denoter
        | TGRID         { $$ = pdtGRID; }
	| TPROCEDURE    { $$ = pdtPROCEDURE; }
	;

 ext_type_denoter:
          type_denoter
        | TOPAQUE { $$ = pdtOPAQUE; }
        ;

direction_type_component:
	  TPLUS
          {
	    $$ = init_direction_type(SIGN_POS);
	  }
        |
	  TMINUS
          {
	    $$ = init_direction_type(SIGN_NEG);
	  }
        |
          integer
          {
	    if ($1 != 0) {
	      YY_FATALX(yylineno, in_file, "Invalid direction type");
	    }
	    $$ = init_direction_type(SIGN_ZERO);
	  }
        |
          {
	    $$ = init_direction_type(SIGN_UNKNOWN);
	  }
        ;

direction_type:
          direction_type_component TCOMMA direction_type
          {
	    $$ = compose_direction_type($1, $3);
	  }
        |
          direction_type_component
          {
	    $$ = $1;
	  }
        ;

grid_type_component: TDOTDOT
	  {
	    $$ = init_grid_type(DIM_RANGE);
	  }
        | TDOT
          {
	    $$ = init_grid_type(DIM_FLAT);
	  }
	;

grid_type: grid_type_component TCOMMA grid_type
	  {
	    $$ = compose_grid_type($1,$3);
	  }
	| grid_type_component
	  {
	    $$ = $1;
	  }
	;

distribution_type_component:
  	  TBLOCK
	  {
	    $$ = init_distribution_type(DIM_BLOCK);
	  }
	|	
  	  TCYCLIC
	  {
  	    YY_FATALX(yylineno, in_file, "Only block type distributions are supported");
	    $$ = init_distribution_type(DIM_CYCLIC);
	  }
	|
  	  TMULTIBLOCK
	  {
  	    YY_FATALX(yylineno, in_file, "Only block type distributions are supported");
	    $$ = init_distribution_type(DIM_MULTIBLOCK);
	  }
	|	
  	  TNONDIST
	  {
  	    YY_FATALX(yylineno, in_file, "Only block type distributions are supported");
	    $$ = init_distribution_type(DIM_NONDIST);
	  }
	|	
  	  TIRREGULAR
	  {
  	    YY_FATALX(yylineno, in_file, "Only block type distributions are supported");
	    $$ = init_distribution_type(DIM_IRREGULAR);
	  }
	;

distribution_type: distribution_type_component TCOMMA distribution_type
	  {
	    $$ = compose_distribution_type($1,$3);
	  }
	| distribution_type_component
	  {
	    $$ = $1;
	  }
	;

region_type_component: TDOTDOT
          {
	    $$ = DIM_RANGE;
	  }
        | TDOT
          {
	    $$ = DIM_FLAT;
	  }
        | TSTAR
          {
	    $$ = DIM_FLOOD;
	  }
	| TGRID
	  {
	    $$ = DIM_GRID;
	  }
	| /* nothing */
	  {
	    $$ = DIM_INHERIT;
	  }
	;

region_type: region_type_component TCOMMA region_type
	  {
	    $$ = compose_region_type($1,$3);
	  }
	| region_type_component
	  {
	    $$ = compose_region_type($1,NULL);
	  }
	;

nonparallel_type_denoter:
	  TINT		{ $$ = pdtINT; }
	| TBOOLEAN	{ $$ = pdtBOOLEAN; }
	| TCHAR		{ $$ = pdtCHAR; }
	| TSHORT	{ $$ = pdtSHORT; }
	| TLONG		{ $$ = pdtLONG; }
	| TFLOAT	{ $$ = pdtFLOAT; }
	| TDOUBLE	{ $$ = pdtDOUBLE; }
	| TQUAD		{ $$ = pdtQUAD; }
	| TFILE		{ $$ = pdtFILE; }
	| TSTRING	{ $$ = pdtSTRING; }
	| TUNSINT	{ $$ = pdtUNSIGNED_INT; }
	| TUNSSHORT	{ $$ = pdtUNSIGNED_SHORT; }
	| TUNSLONG	{ $$ = pdtUNSIGNED_LONG; }
	| TUNSBYTE	{ $$ = pdtUNSIGNED_BYTE; }
	| TSBYTE	{ $$ = pdtSIGNED_BYTE; }
	| TCOMPLEX      { $$ = pdtCOMPLEX; }
	| TDCOMPLEX	{ $$ = pdtDCOMPLEX; }
	| TQCOMPLEX	{ $$ = pdtQCOMPLEX; }
        | TGENERIC      { $$ = pdtGENERIC; }
	| TTYPEIDENT					/* existing type */
		{ $$ = S_DTYPE( lookup( S_TYPE, $1)); }
	| TLP enumlist TRP
		{ $$ = build_enum( NULL, $2); }

	| TARRAY TLSBR dimlist TRSBR TOF type_denoter
		{ $$ = build_array_type($3, $6); }

	| trecord { enter_block(NULL); } record_list tend
		{ 
		  $$ = build_record_type(S_STRUCTURE); 
		  exit_block(); 
		}
	| TUNION { enter_block(NULL); } record_list tend
		{ $$ = build_record_type(S_VARIANT); 
		  exit_block(); 
		}
	| TIDENT
		{
                  $$ = HandleUnknownTypes($1);
		}
	| error
		{ YY_FATALX(yylineno, in_file, "error in type specification");}
	;

type_denoter:
	  nonparallel_type_denoter
	| TDIRECTION TLESS direction_type TGREATER
          {
	    $$ = $3;
          }
        | TREGION TLESS region_type TGREATER
	  {
	    $$ = $3;
	  }
	| sbe TREGION TLESS region_type TGREATER
	  {
	    use_default_distribution = FALSE;
	    D_REG_DIST($4) = find_distribution_pst_in_expr($1);
	    $$ = $4;
	  }
	| TDISTRIBUTION TLESS distribution_type TGREATER
	  {
	    use_default_distribution = FALSE;
	    $$ = $3;
	  }
	| sbe TDISTRIBUTION TLESS distribution_type TGREATER
	  {
	    use_default_distribution = FALSE;
	    D_DIST_GRID($4) = find_grid_pst_in_expr($1);
	    $$ = $4;
	  }
	| TGRID TLESS grid_type TGREATER
	  {
	    use_default_distribution = FALSE;
	    $$ = $3;
	  }
	| TREGION	{ $$ = pdtREGION; }
        | TDISTRIBUTION       { $$ = pdtDISTRIBUTION; }
        | TGENERICENSEMBLE { $$ = pdtGENERIC_ENSEMBLE; }
	| sbe nonparallel_type_denoter 			/* parallel array */
		{ 
		  $$ = build_ensemble_type( $2, convert_sbe_to_region($1, "", yylineno, in_file), 1, 0, yylineno, in_file);
		}
	;


enumlist: { enum_value = 0; } enumerator
		{ $$ = cat_symtab_ls(NULL, $2); }
	| enumlist TCOMMA enumerator
		{ $$ = cat_symtab_ls($1, $3); }
	| enumlist TCOMMA
		{ $$ = $1; }
	;

enumerator:
	  TIDENT
		{ check_name_conflict($1);
		  $$ = build_enumelem(LU_INS($1), NULL); 
		}
	| TIDENT TEQUAL expr
		{ check_name_conflict($1);
		  $$ = build_enumelem(LU_INS($1), $3);
		}
	;


record_list: 	record_component 
	|	record_list  record_component 
	;

record_component:
	  idlist tcolon type_denoter tsemi
		{ 
		  define_componentlist( $1, $3); 
		}
	;



/****************************************************************
 * PROCEDURE/FUNCTION:
 ****************************************************************/

fndef:   tprocedure TIDENT
                {
		  check_name_conflict($2);
                  enter_block(NULL);
                }
         TLP formals TRP returntype tsemi
	 locals
		{
		  pop();
		  current_level--;
		  $<pst>$ = define_functionhead($2, $7, $1, in_file, 0, FALSE);
		  current_level++;
		  push($<pst>$);
		}
         compound_stmt tsemi
                {
                  $<pst>$ = define_functionbody($<pst>10, $11, $1, in_file);
		  exit_block();
		}
        |
	 tprototype TIDENT
		{
		  enter_block(NULL);
		}
	 TLP protoformals TRP protoreturntype tsemi
		{
		  pop();
		  current_level--;
		  $<pst>$ = define_functionhead($2, $7, $1, in_file, 1, FALSE);
		  current_level++;
		  push($<pst>$);
		  exit_block();
		}
        |
         TFREE tprocedure TIDENT
                {
		  check_name_conflict($3);
                  enter_block(NULL);
                }
         TLP formals TRP returntype tsemi
	 locals
		{
		  pop();
		  current_level--;
		  $<pst>$ = define_functionhead($3, $8, $2, in_file, 0, TRUE);
		  current_level++;
		  push($<pst>$);
		}
         compound_stmt tsemi
                {
                  $<pst>$ = define_functionbody($<pst>11, $12, $2, in_file);
		  exit_block();
		}
        |
	 TFREE tprototype TIDENT
		{
		  enter_block(NULL);
		}
	 TLP protoformals TRP protoreturntype tsemi
		{
		  pop();
		  current_level--;
		  $<pst>$ = define_functionhead($3, $8, $2, in_file, 1, TRUE);
		  current_level++;
		  push($<pst>$);
		  exit_block();
		}
        |
	 TEXTERN tprototype TIDENT
		{
		  enter_block(NULL);
		}
	 TLP protoformals TRP protoreturntype tsemi
		{ 
		  pop();
		  current_level--;
		  $<pst>$ = define_functionhead($3, $8, $2, in_file, 2, FALSE);
		  current_level++;
		  push($<pst>$);
		  exit_block();
		}
	|
	 TEXTERN TFREE tprototype TIDENT
		{
		  enter_block(NULL);
		}
	 TLP protoformals TRP protoreturntype tsemi
		{ 
		  pop();
		  current_level--;
		  $<pst>$ = define_functionhead($4, $9, $3, in_file, 2, TRUE);
		  current_level++;
		  push($<pst>$);
		  exit_block();
		}
	;


protoreturntype:
		{ $$ = pdtVOID; }
	|	tcolon proto_type_denoter
		{ $$ = $2; }
	;

returntype: /* none */
		{ $$ = pdtVOID; }
	| tcolon type_denoter
		{ $$ = $2; }
	;

is_free: /* nothing */
		{ $$ = FALSE; }
	|	TFREE
		{ $$ = TRUE; }
	;

param_type:	/* nothing */
		{ $$ = SC_CONST; }
	|	TCONST
		{ $$ = SC_CONST; }
	|	TVAR               /* to maintain half-baked backwards
		                      compatibility */
		{
		  YY_WARNX(yylineno, in_file, "'var' parameter is antiquated; "
		"Use const, in, out, or inout");
		  $$ = SC_INOUT;
		}
	|	TIN
		{ $$ = SC_IN; }
	|	TOUT
		{ $$ = SC_OUT; }
	|	TINOUT
		{ $$ = SC_INOUT; }
	;

formals:  /* 	no formals */
	| 	nonnullformals
		;

nonnullformals:
		formal
        | 	nonnullformals tsemi formal
		;

formal:   	is_free param_type idlist tcolon type_denoter
		{ 
		check_name_list_conflict($3);
		define_parameterlist($2, $1, $3, $5);
		}
	;

protoformals: /* no protoformals */
        | nonnullprotoformals
	;

nonnullprotoformals:
		protoformal
        | 	nonnullprotoformals tsemi protoformal
        ;

protoformal: 	is_free param_type idlist tcolon proto_type_denoter
		{ define_parameterlist($2, $1, $3, $5); }
		;

locals: /* 	no locals */
        | nonnulllocals
        ;

nonnulllocals:
        local
        | nonnulllocals local
        ;

local:  TVAR vardefs
		{ }
        | TFREE TVAR freevardefs
                { }
	| TSTATIC TVAR staticvardefs
		{ }
	| TSTATIC TFREE TVAR staticfreevardefs
		{ }
	;

reductionop:
	  TPLUSREDUCE 	{ $$ = PLUS;	}
	| TTIMESREDUCE	{ $$ = TIMES;	}
	| TMINREDUCE 	{ $$ = MIN;	}
	| TMAXREDUCE 	{ $$ = MAX;	}
	| TANDREDUCE 	{ $$ = AND;	}
	| TORREDUCE 	{ $$ = OR;	}
	| TBANDREDUCE	{ $$ = BAND;	}
	| TBORREDUCE	{ $$ = BOR;	}
	| TBXORREDUCE	{ $$ = BXOR;	}
	;

scanop:   TPLUSSCAN 	{ $$ = PLUS;	}
	| TTIMESSCAN	{ $$ = TIMES;	}
	| TMINSCAN 	{ $$ = MIN;	}
	| TMAXSCAN 	{ $$ = MAX;	}
	| TANDSCAN 	{ $$ = AND;	}
	| TORSCAN 	{ $$ = OR;	}
	| TBANDSCAN  	{ $$ = BAND;	}
	| TBORSCAN  	{ $$ = BOR;	}
	| TBXORSCAN  	{ $$ = BXOR;	}
	;

assignop: TASSIGN 	{ $$ = BIASSIGNMENT;	}
	| TEQUAL	{ YY_FATAL_CONTX(yylineno, in_file, "'=' cannot be used for assignment, expected ':='");
			  $$ = BIASSIGNMENT;
			}
	;

assignop_gets: 
	  TASSPLUS 	{ $$ = PLUS;	}
	| TASSMINUS 	{ $$ = MINUS;	}
	| TASSMULT 	{ $$ = TIMES;	}
	| TASSDIVIDE 	{ $$ = DIVIDE;	}
	| TASSMODULUS 	{ $$ = MOD;	}
	| TASSAND 	{ $$ = AND;	}
	| TASSOR 	{ $$ = OR;	}
	;

base_lvalue:  TIDENT
		{ $$ = build_0ary_op(VARIABLE,check_var($1)); }
        | base_lvalue TLSBR nonnull_exprlist TRSBR   		     %prec TDOT
		{ $$ = build_Nary_op( ARRAY_REF, $1, $3); 
		   T_SUBTYPE($$) = ARRAY_IND;
		}
        | base_lvalue TLSBR TRSBR   				     %prec TDOT
		{  $$ = build_Nary_op( ARRAY_REF, $1, NULL); 
		   T_SUBTYPE($$) = ARRAY_IND;
		}
	| base_lvalue TLSBR error            TRSBR   		     %prec TDOT
		{  YY_FATAL_CONTX(yylineno, in_file,
				  "expecting an array reference"); 
                  $$ = build_null_op();
		}
	| base_lvalue TDOT identifier
		{ $$ = build_struct_op(BIDOT,$1, findstructmember($1,$3,0)); }
	| base_lvalue TRGRID mega_region
		{ $$ = build_randacc_op($1, $3); }
        ;

lvalue:  base_lvalue
          { $$ = $1; }
        | lvalue TPERMUTE mega_region
	  { $$ = build_permute_op(P_PERMUTE,$3,$1); }
	| lvalue TAT direction_ref
		{ 
		  $$ = build_unary_at_op( $1, $3,AT_NORMAL); 
		}
	| lvalue TWRAPAT direction_ref
		{ 
		  $$ = build_unary_at_op( $1, $3,AT_WRAP); 
		}
	| lvalue TPRIME TAT direction_ref
		{
		  $$ = build_unary_at_op($1,$4,AT_PRIME);
		}
	;

funccall: TIDENT TLP exprlist TRP   			%prec TDOT
            {
	      symboltable_t *fpst;
	      char *id;
	      
	      fpst = lu($1);
	      if (fpst == NULL || S_CLASS(fpst) != S_SUBPROGRAM) {
		YY_FATALX(yylineno, in_file, "undefined function: %s", $1);
	      }
	      else {
		id = get_function_name(fpst, $3, 0);
		$$ = build_Nary_op(FUNCTION, build_0ary_op(VARIABLE,check_var(id)), $3);
	      }
	    };

constval: TINTEGER
		{ $$ = build_0ary_op(CONSTANT,build_int($1)); }
	| TFLOAT
		{ $$ = build_0ary_op(CONSTANT,build_real($1)); }
	| TCHARACTER
		{ $$ = build_0ary_op(CONSTANT,build_char($1)); }
	| string
		{ $$ = build_0ary_op(CONSTANT,$1); }
	;

ioexpr:   expr
		{ $$ = T_MAKE_LIST($1); }
        | expr TCOLON expr	/* force colons to sit between two expr's */
		{ $$ = cat_io_expr($1, $3); }
        ;

nonnull_ioexprlist:
	  nonnull_ioexprlist TCOMMA ioexpr
		{ $$ = cat_expr_ls($1, $3); }
	| ioexpr
		{ $$ = $1; }
	;

ioexprlist:	{ $$ = NULL; }	/* similar to exprlist but may contain colons */
	| nonnull_ioexprlist
		{ $$ = $1; }
	;

nonnull_exprlist:
	  sbe_expr
		{ $$ = T_MAKE_LIST($1); }
	| nonnull_exprlist TCOMMA sbe_expr
		{ $$ = cat_expr_ls($1,$3); }
	;

exprlist:	{ $$ = NULL; }
	| nonnull_exprlist
		{ $$ = $1; }
	;

expr:   constval
		{ $$ = $1; }
	| funccall
		{ $$ = $1; }
        | base_lvalue
                { $$ = $1; }
        | expr TPERMUTE mega_region
	  { $$ = build_permute_op(P_PERMUTE,$3,$1); }
	| expr TAT direction_ref
		{ 
		  $$ = build_unary_at_op( $1, $3,AT_NORMAL); 
		}
	| expr TWRAPAT direction_ref
		{ 
		  $$ = build_unary_at_op( $1, $3,AT_WRAP); 
		}
	| expr TPRIME TAT direction_ref
		{
		  $$ = build_unary_at_op($1,$4,AT_PRIME);
		}
	| TLP expr TRP
		{ $$ = $2; }
        | TMINUS expr                                           %prec TUNARY
                { $$ = build_unary_op(UNEGATIVE, $2); }
        | TPLUS expr                                            %prec TUNARY
                { $$ = build_unary_op(UPOSITIVE, $2); }
        | TNOT expr                                             %prec TUNARY
                { $$ = build_unary_op(UCOMPLEMENT, $2); }
	| reductionop expr  					%prec TREDUCE
		{ $$ = build_reduction_op($1,NULL,$2); }
	| reductionop sbe expr				        %prec TREDUCE
		{ $$ = build_reduction_op($1,$2,$3); }
	| scanop expr						%prec TREDUCE
		{ $$ = build_scan_op($1,NULL,$2); }
	| scanop TLSBR integerlist TRSBR expr			%prec TREDUCE
                { $$ = build_scan_op($1,$3,$5); }
        | TUSRREDUCE expr                              	        %prec TREDUCE
                { $$ = build_usrreduction_op($1,NULL,$2); }
        | TUSRREDUCE sbe expr                          	        %prec TREDUCE
                { $$ = build_usrreduction_op($1,$2,$3); }
        | TUSRREDUCE TLP nonnull_exprlist TRP                   %prec TREDUCE
	        { $$ = build_usrreduction_op($1,NULL,$3); }
        | TUSRREDUCE sbe TLP nonnull_exprlist TRP               %prec TREDUCE
                { $$ = build_usrreduction_op($1,$2,$4); }
        | TUSRSCAN expr                                         %prec TREDUCE
                { $$ = build_usrscan_op($1,NULL,$2); }
        | TUSRSCAN TLSBR integerlist TRSBR expr                 %prec TREDUCE
                { $$ = build_usrscan_op($1,$3,$5); }
        | TUSRSCAN TLP nonnull_exprlist TRP                     %prec TREDUCE
	{ $$ = build_usrscan_op($1,NULL,$3); }          
        | TUSRSCAN TLSBR integerlist TRSBR TLP nonnull_exprlist TRP        %prec TREDUCE
	  { $$ = build_usrscan_op($1,$3,$6); }
	| TFLOOD sbe expr
		{ $$ = build_flood_op(FLOOD,$2,$3); }
        | expr TFLOOD
                { $$ = NULL; YY_FATALX(yylineno, in_file, "flood operator unexpected"); }
	| expr TPLUS      expr  { $$ = build_binary_op(BIPLUS       ,$3,$1); }
	| expr TMINUS     expr  { $$ = build_binary_op(BIMINUS      ,$3,$1); }
	| expr TSTAR      expr  { $$ = build_binary_op(BITIMES      ,$3,$1); }
	| expr TDIVIDE    expr  { $$ = build_binary_op(BIDIVIDE     ,$3,$1); }
	| expr TMODULUS   expr  { $$ = build_binary_op(BIMOD        ,$3,$1); }
        | expr TEXP       expr  { $$ = build_binary_op(BIEXP        ,$3,$1); }
	| expr TLESS      expr  { $$ = build_binary_op(BILESS_THAN  ,$3,$1); }
	| expr TLESSEQUAL expr  { $$ = build_binary_op(BIL_THAN_EQ  ,$3,$1); }
	| expr TGREATER   expr  { $$ = build_binary_op(BIGREAT_THAN ,$3,$1); }
	| expr TGREATEREQUAL expr {$$= build_binary_op(BIG_THAN_EQ  ,$3,$1); }
	| expr TEQUAL     expr  { $$ = build_binary_op(BIEQUAL      ,$3,$1); }
	| expr TNOTEQUAL  expr  { $$ = build_binary_op(BINOT_EQUAL  ,$3,$1); }
	| expr TAND       expr  { $$ = build_binary_op(BILOG_AND    ,$3,$1); }
	| expr TOR        expr  { $$ = build_binary_op(BILOG_OR     ,$3,$1); }
        ;

maskop: TWITH
		{ $$ = WITH_REGION; }
	| TWITHOUT
		{ $$ = WITHOUT_REGION; }
	| TSWITH
		{ $$ = SPARSE_WITH_REGION; }
	| TSWITHOUT
		{ $$ = SPARSE_WITHOUT_REGION; }
	;


sbe_expr:
          expr {$$ = $1; }
        | sbe {$$ = $1; }
        | TQUOTE { $$ = define_quote_region(); }
        | direction_ref TOF        sbe_expr { $$ = build_prep_region_expr(OF_REGION, $1, $3, yylineno, in_file); }
        | direction_ref TIN        sbe_expr { $$ = build_prep_region_expr(IN_REGION, $1, $3, yylineno, in_file); }
        | sbe_expr TREGIONAT  direction_ref { $$ = build_prep_region_expr(AT_REGION, $3, $1, yylineno, in_file); }
        | sbe_expr TBY        direction_ref { $$ = build_prep_region_expr(BY_REGION, $3, $1, yylineno, in_file); }
        | sbe_expr maskop  sbe_expr %prec TWITH { $$ = build_with_region_expr($2, $3, $1, yylineno, in_file); }
        | sbe_expr maskop  TQUEST   %prec TWITH { $$ = build_with_region_expr($2, pexprUNKNOWN, $1, yylineno, in_file); }
        ;

assignment:
	  lvalue assignop sbe_expr
		{ $$ = build_binary_op($2,$3,$1); }
        | lvalue assignop_gets sbe_expr
                { $$ = build_binary_op_gets(BIOP_GETS,$2,$3,$1); }
	| lvalue TDESTROY sbe_expr
		{
		  $$ = build_Nary_op(FUNCTION, build_0ary_op(VARIABLE, check_var("_DESTROY")), cat_expr_ls($1, $3));
		}
	| lvalue TPRESERVE sbe_expr
		{
		  $$ = build_Nary_op(FUNCTION, build_0ary_op(VARIABLE, check_var("_PRESERVE")), cat_expr_ls($1, $3));
		}
	;


/****************************************************************
 * STATEMENTS:
 ****************************************************************/

tprogram: TPROGRAM   
		{ $$ = yylineno; } ;
tif: 	  TIF   
		{ $$ = yylineno; } ;
twhile:	  TWHILE
		{ $$ = yylineno; } ;
trepeat:  TREPEAT
		{ $$ = yylineno; } ;
tfor: 	  TFOR  
		{ $$ = yylineno; } ;
telsif:   TELSIF  
		{ $$ = yylineno; } ;
tbegin:   TBEGIN  
		{ $$ = yylineno; } ;
tmscan:   TMSCAN
		{ $$ = yylineno; } ;
tcontinue:TCONTINUE  
		{ $$ = yylineno; } ;
texit:	  TEXIT  
		{ $$ = yylineno; } ;
thalt:	  THALT  
		{ $$ = yylineno; } ;
treturn:  TRETURN  
		{ $$ = yylineno; } ;
twrap:    TWRAP  
		{ $$ = yylineno; } ;
treflect: TREFLECT  
		{ $$ = yylineno; } ;
tread:    TREAD  
		{ $$ = yylineno; } ;
twrite:   TWRITE  
		{ $$ = yylineno; } ;
twriteln: TWRITELN
		{ $$ = yylineno; } ;
tbread:   TBREAD
                { $$ = yylineno; } ;
tbwrite:  TBWRITE
                { $$ = yylineno; } ;
tequal:   TEQUAL
		{ $$ = yylineno; }
	| TASSIGN
		{ 
		  YY_FATAL_CONTX(yylineno, in_file, 
				 "expected '=' but found ':='");
		  $$ = yylineno; 
		}
	;

tcolon:   TCOLON
		{ $$ = yylineno; } 
	| TEQUAL
		{ 
		  YY_FATAL_CONTX(yylineno, in_file,
				 "expected ':' but found '='");
		  $$ = yylineno; 
		}
	| TASSIGN
		{ 
		  YY_FATAL_CONTX(yylineno, in_file,
				 "expected ':' but found ':='");
		  $$ = yylineno; 
		}
	;

tprocedure: TPROCEDURE
		{ $$ = yylineno; } ;

tprototype: TPROTOTYPE
		{ $$ = yylineno; } ;

tsemi: TSEMI
                { $$ = 1; }
       | tnosemichecks
                { $$ = 0; YY_FATAL_CONTX(yylineno, in_file, "';' expected"); }
       ;

tend:  TEND
                { $$ = 1; }
       | tnoendchecks
                { $$ = 0; YY_FATAL_CONTX(yylineno, in_file, "end expected"); }
       ;

stmtls:	
                { $$ = alloc_statement(S_NULL, yylineno, in_file); }
        |
        nonempty_stmtls
                { $$ = $1; }
        ;

nonempty_stmtls: stmt tsemi
	  	{ $$ = $1;
		  IFDB(70) {
		    DBS0(70, "stmt : dbg_gen_stmt() :\n");
		    dbg_gen_stmt(stdout,$1);
		  }
		}
	| nonempty_stmtls stmt tsemi
		{ $$ = cat_stmt_ls($1,$2);
		  IFDB(70) {
		    DBS0(70, "stmtls stmt : dbg_gen_stmt() :\n");
		    dbg_gen_stmt(stdout,$2);
		  }
		}
        | TSEMI
                {
		  $$ = alloc_statement(S_NULL, yylineno, in_file);
		}
        | error TSEMI
                {
		  $$ = NULL;
		  YY_FATAL_CONTX(yylineno, in_file, "bad statement");
		}
	;

stmt:  	compound_stmt      { $$ = $1; }
        | expr_stmt        { $$ = $1; } 
	| if_stmt          { $$ = $1; }
	| loop_stmt        { $$ = $1; }
        | control_stmt     { $$ = $1; }
        | io_stmt          { $$ = $1; }
	| mscan_stmt       { $$ = $1; }
        | wrapreflect_stmt { $$ = $1; }
	;

compound_stmt:
	tbegin stmtls tend
                {
		  $$ = build_compound_statement(NULL,$2,$1,in_file); 
		}
	| region_spec stmt
		{ 
		  $$ = insert_statements_into_region_scope($1, $2);
		}
        ;


expr_stmt: assignment
		{ 
		  $$ = build_expr_statement($1, yylineno, in_file);
		}
 	| funccall
		{
		  $$ = build_expr_statement($1,yylineno,in_file);
		}
        ;


if_stmt: tif expr TTHEN stmtls tend
		{
		  $$ = build_if_statement($2,$4,NULL,NULL,$1,in_file);
                }
	| tif expr TTHEN stmtls TELSE stmtls tend
		{
		  $$ = build_if_statement($2,$4,$6,NULL,$1,in_file); 
		}
	| tif expr TTHEN stmtls elsif_stmt
		{ 
		  $$ = build_if_statement($2,$4,$5,NULL,$1,in_file); 
		}
        | tif expr error
                {
		  YY_FATAL_CONTX(yylineno, in_file, "'if' statement is missing a 'then'"); 
		  $$ = build_if_statement($2,NULL,NULL,NULL,$1,in_file);
		}
        ;


elsif_stmt:	  telsif expr TTHEN stmtls tend
		{ 
		  $$ = build_if_statement($2,$4,NULL,NULL,$1,in_file); 
		  T_SUBTYPE($$) = C_ELSIF;
		}
	|
	  telsif expr TTHEN stmtls TELSE stmtls tend
		{ 
		  $$ = build_if_statement($2,$4,$6,NULL,$1,in_file); 
		  T_SUBTYPE($$) = C_ELSIF;
		}
	|
	  telsif expr TTHEN stmtls elsif_stmt
		{
		  $$ = build_if_statement($2,$4,$5,NULL,$1,in_file);
		  T_SUBTYPE($$) = C_ELSIF;
		}
	|
	  telsif expr error
		{
		  YY_FATAL_CONTX(yylineno, in_file, "'elsif' statement is missing a 'then'"); 
		  $$ = build_if_statement($2,NULL,NULL,NULL,$1,in_file); 
		  T_SUBTYPE($$) = C_ELSIF;
		}
	;


loop_stmt: twhile expr TDO stmtls tend
		{ 
		  $$ = build_loop_statement(L_WHILE_DO,$2,$4,$1,in_file); 
		}
	| twhile expr error
		{
		  YY_FATAL_CONTX(yylineno, in_file, "'while' statement is missing a 'do'"); 
		  $$ = build_loop_statement(L_WHILE_DO,$2,NULL,$1,in_file); 
		}
	| trepeat stmtls TUNTIL expr
		{ 
		  $$ = build_loop_statement(L_REPEAT_UNTIL,$4,$2,$1,in_file); 
		}
	| tfor expr TASSIGN expr TTO expr          TDO stmtls tend
		{ 
		  $$ = build_do_statement(L_DO,L_DO_UP,NULL,$2,$4,$6,NULL,$8,$1,in_file);
		}
	| tfor expr TASSIGN expr TTO expr TBY expr TDO stmtls tend
		{ 
		  $$ = build_do_statement(L_DO,L_DO_UP,NULL,$2,$4,$6,$8,$10,$1,in_file);
		}
	| tfor expr TASSIGN expr TDOWNTO expr      TDO stmtls tend 
		{
		  $$ = build_do_statement(L_DO,L_DO_DOWN,NULL,$2,$4,$6,NULL,$8,$1,in_file);
		}
	| tfor expr TASSIGN expr TDOWNTO expr TBY expr TDO stmtls tend 
		{
		  $$ = build_do_statement(L_DO,L_DO_DOWN,NULL,$2,$4,$6,$8,$10,$1,in_file);
		}
        ;


control_stmt: tcontinue 
		{
		  $$ = alloc_statement(S_CONTINUE, yylineno, in_file); 
		}
	| texit 
		{ 
		  $$ = alloc_statement(S_EXIT, yylineno, in_file); 
		}
	| treturn
		{
		  $$ = build_return_statement(NULL, yylineno, in_file); 
		}
	| treturn expr 
		{ 
		  $$ = build_return_statement($2, yylineno, in_file); 
		}
        ;

io_stmt: tread TLP nonnull_ioexprlist TRP 
		{
		  $$ = build_io_statements( IO_READ, $3, yylineno, in_file); 
		}
	| twrite TLP nonnull_ioexprlist TRP 
		{
		  $$ = build_io_statements( IO_WRITE, $3, yylineno, in_file); 
		}
	| twriteln TLP ioexprlist TRP 
		{
		  $$ = build_io_statements( IO_WRITELN, $3, yylineno,in_file); 
		}
	| tbread TLP nonnull_ioexprlist TRP 
		{
		  $$ = build_io_statements( IO_BREAD, $3, yylineno, in_file); 
		}
	| tbwrite TLP nonnull_ioexprlist TRP 
		{
		  $$ = build_io_statements( IO_BWRITE, $3, yylineno, in_file); 
		}
	| thalt 
		{
		  $$ = build_halt_statement(NULL, yylineno, in_file); 
		}
        | thalt TLP nonnull_ioexprlist TRP 
		{
		  $$ = build_halt_statement($3, yylineno, in_file);
		}
	;

mscan_stmt: tmscan stmtls tend
                {
		  $$ = build_mscan_statement(NULL,$2,$1,in_file);
		}
	;


wrapreflect_stmt: twrap nonnull_exprlist
		{
		  $$ = build_wrap_statement(S_WRAP,$2,yylineno,in_file); 
		}
	| treflect nonnull_exprlist
		{
		  $$ = build_wrap_statement(S_REFLECT,$2,yylineno,in_file); 
		}

/****************************************************************
 * MISC HELPERS:
 ****************************************************************/

init:	  sbe_expr
		{ $$ = build_init($1, NULL); }
	| TLCBR initlist        TRCBR
		{ $$ = build_init(NULL, $2); }
	| TLCBR initlist TCOMMA TRCBR
		{ $$ = build_init(NULL, $2); }
	;

initlist: init
		{ $$ = build_initlist(NULL, $1); }
	| initlist TCOMMA init
		{ $$ = build_initlist($1, $3); }
	;

string:	  TSTR
		{ $$ = combine(NULL, $1); }
	| string TSTR
		{ $$ = combine($1,$2); }
	;

identifier:
	  TIDENT
	| TTYPEIDENT
	;

ident:
	identifier
	        { $$ = alloc_symlist($1); }
	;

idlist:	
	  ident
		{ $$ = build_sym_list( NULL, $1); }
	| idlist TCOMMA ident
		{ $$ = build_sym_list( $1, $3); }
	;

integerlist:
	  integer
		{ $$ = build_intlist( NULL,$1); }
	| integerlist TCOMMA integer
		{ $$ = build_intlist( $1, $3); }
	;

integer:         TINTEGER	{ $$ =   $1; }
	| TPLUS  TINTEGER	{ $$ =   $2; }
	| TMINUS TINTEGER	{ $$ = - $2; }
	;
%%
