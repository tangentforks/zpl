/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/error.h"
#include "../include/db.h"
#include "../include/const.h"
#include "../include/struct.h"
#include "../include/symtab.h"
#include "../include/global.h"
#include "../include/stencil.h"
#ifdef DEBUG
int debug_flag = 0;
#endif

char *entry_name;
module_t *zpl_module = NULL;

int current_level = 0;
int enum_value = 0;
int yylineno = 1;
int verbose = FALSE;
int tokenize = FALSE;
int rank1only = TRUE;	/* rank1only becomes false if we ever see some with 
                           rank greater than 1.  This is used by Brad to
			   optimize the 1D distribution. */
symboltable_t *yystmtlab;
symboltable_t *last_ident;
symboltable_t *currlabel;
datatype_t *pdtINT, *pdtFLOAT, *pdtBOOLEAN, *pdtCHAR, *pdtVOID;
datatype_t *pdtLONG, *pdtSHORT, *pdtDOUBLE, *pdtQUAD, *pdtUNSIGNED_INT;
datatype_t *pdtCOMPLEX, *pdtDCOMPLEX, *pdtQCOMPLEX;
datatype_t *pdtGENERIC, *pdtGENERIC_ENSEMBLE;
datatype_t *pdtFILE, *pdtSTRING, *pdtOPAQUE;
datatype_t *pdtUNSIGNED_SHORT, *pdtUNSIGNED_LONG;
datatype_t *pdtUNSIGNED_BYTE, *pdtSIGNED_BYTE;
datatype_t *pdtGRID, *pdtDISTRIBUTION, *pdtREGION, *pdtDIRECTION, *pdtPROCEDURE;
datatype_t* pdtRANK_REGION[MAXRANK+1];
datatype_t* pdtRANK_ENSEMBLE[MAXRANK+1];

datatype_t* pdtGRID_SCALAR[MAXRANK+1];
symboltable_t* pstGRID_SCALAR[MAXRANK+1];

datatype_t *rmsframe;
symboltable_t *rmstack;

int	mem_usage = 0;
symboltable_t *pstMAIN;
FILE *zstdin = NULL;
FILE *zstdout = NULL;

char *zc_command = NULL; /*** zc command line ***/
                         /*** if zc0 was called from zc, NULL otherwise ***/
char *zbasename = NULL;  /* name of the final binary */
char *in_file = NULL;    /* name of the file being compiled */

int     error_detected = FALSE;
int     fatal_count;     /* count of fatal errors; used by FATALCONT macros */
int     new_access = 1;  /* bool set in cpp/main.c using command line arg -a */
                         /* 1 => use new ensemble access (bumper/walker)  */
                         /* 0 => use old ensemble access (ACCESS_xD...) */
int     new_amacro = 0;  /* bool set in cpp/main.c using command line arg -a */
                         /* 1 => use new access macros */
                         /* 0 => use old access macros */
int     fuse_level = 3;  /* fusion level, set using -fusion */
int     contract = TRUE; /* employ contraction */
int     safe_code = 0;   /* bool set in cpp/main.c using command line arg -s */
                         /* 1 => generate the safest code possible */
                         /* 0 => don't protect me.  (default) */
int	noclone = 0;     /* bool set in cpp/main.c using command line arg -w */
                         /* 1 => do not clone all functions */
                         /* 0 => clone all functions */
int typecheck_code = TRUE;/*** to type check or not ***/
int const_opt = TRUE;    /* by default, compiler can evaluate constant exprs */
int flood_opt = TRUE;    /* by default, compiler can optimize flood mloops */
int tile_opt = TRUE;     /* by default, compiler can tile mloops */
int mloop_opt = TRUE;    /* by default, compiler can reorder mloop dims */
int stencil_opt = STENCIL_UNROLL; /* default is unrolled stencil */
double stencil_adds = STENCIL_DEFADDS;
double stencil_mems = STENCIL_DEFMEMS;
double stencil_muls = STENCIL_DEFMULS;
double stencil_regs = STENCIL_DEFREGS;
double stencil_bbar = STENCIL_DEFBBAR;
int stencil_debug = STENCIL_DEFDBG;

int cache_line_size = 0; /* # of bytes in a cache line...used to compute */
			 /* tile size */
int cmo_alloc = FALSE;	 /* by default, arrays are allocated in RMO */
int sr_opt = TRUE;       /* by default, compiler perform strength reduction */
int priv_access = FALSE;  /* by default, do not emit private access macros */
int commstats = FALSE;   /* by default, do not emit macros for timing comm */
int compstats = FALSE;   /* by default, do not emit macros for timing comp */
int nomscanpeel = TRUE;  /* by default, don't peel mscan tiles */

insertion_type comm_insert = OPTIMIZED_COMM; /* comm insertion type */
int buffer_opt = TRUE;	 /* try to optimize comm buffer usage, when possible */

int strict_check = FALSE;/* boolean set in cpp/main.c using command line -s */
char *outputdir = NULL;  /* char buffer to put the C output from comlin -d */

int quiet = TRUE;	 /* run in quite mode */
int debug = FALSE;      /* TRUE => enable ZGUARD compiler support */
int print_cpp_lines = FALSE;  /* TRUE => litter code with # 23 "foo.z" */
int paranal_help = FALSE; /* TRUE => have paranal explain its decisions */

int permmapshare    = TRUE;   /* TRUE => share permmap between permutes   */
int permmapsave     = TRUE;   /* TRUE => save permmaps for possible reuse */
int permoverlap     = TRUE;   /* TRUE => overlap computation with permute */
int permprocmap     = -1;   /* -1 ==> heuristic on per remap basis; 1 => use procmap; 0 => avoid if possible */
int permrleprocmap  = -1;     /* run length encode procmap / indices      */
int permrleindices  = -1;     /*     -1, use level = rank, else level = # */
int permloopdecode  = TRUE;   /* TRUE => use loops to decode rle indices  */
int permsavedataspace = TRUE; /* TRUE => use source/dest as buckets       */
int permdirect = TRUE;

int seqonly = FALSE; /* TRUE => optimize knowing program is sequential */

int c_opt = TRUE; /* TRUE => do low-level C optimizations */

int makereport[NUM_REPORTS] = {FALSE, FALSE, FALSE}; /* documented in global.h */

/* default MLOOP properties... */

int default_mloop_tile[MAXRANK] = {0,0,0,0,0,0}; 
int default_mloop_unroll[MAXRANK] = {1,1,1,1,1,1}; 
int default_mloop_order[MAXRANK] = {0,1,2,3,4,5}; 
int default_mloop_up[MAXRANK] = {1,1,1,1,1,1};
int default_mloop_tile_order[MAXRANK] = {0,1,2,3,4,5};
int default_mloop_tile_up[MAXRANK] = {1,1,1,1,1,1};

int optimize_sparse_storage = 1;

int do_insert_checkpoints = FALSE; /* by default, do not insert checkpoints */
int do_checkpoint = FALSE; /* by default, do not checkpoint */

symboltable_t* chkpoint;
symboltable_t* chklabel;
symboltable_t* checkpointing;
symboltable_t* recovering;
symboltable_t* recoverfilename;
symboltable_t* chksave;
symboltable_t* chkrecover;
symboltable_t* ckpt_interval; /*** sungeun ***/

int numperms;
int numshare;

int restrictkw = TRUE; /* TRUE ==> use keyword restrict where applicable */

int use_default_distribution = TRUE; /* TRUE ==> no user supplied distributions or grids */

glist GlobalGrids = NULL;
glist GlobalDistributions = NULL;

int do_bounds_checking = FALSE; /* TRUE ==> do array bounds checking */

/* X1 likes this:
int hoist_access_ptrs = HOIST_PTRS_TO_SUBPROG;
int hoist_access_mults = 1;
*/

int hoist_access_ptrs = 0;
int hoist_access_mults = 0;
