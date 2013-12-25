/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef FILE
#include <stdio.h>
#endif

#include "db.h"
#include "const.h"
#include "glistmac.h"
#include "struct.h"
#include "symtab.h"

extern int firstsemi; /* when we see the first semicolon, presumably
			 after program, we switch over to reading the
			 standard context, stdzpl.z */
extern  FILE    *fnamesave;

extern	FILE	*fname;
extern	char	buffer[];	
extern  module_t *zpl_module;

extern int verbose;	/* use verbose mode */
extern int tokenize;    /* use tokenize mode */
extern char *in_file;
extern char* base_in_file;  /* in_file without the path */
extern char *zc_command;
extern char *zbasename;
extern char *trunc_in_file;
extern char *trunc_in_filename;
extern int yylineno;
extern symboltable_t *pstMAIN;
extern int current_level;
extern FILE *zstdin, *zstdout;
extern int error_detected;
extern char *entry_name;		/* name of entry point procedure */
extern int fatal_count;  /* count of fatal errors; used by FATALCONT macros */
extern int typecheck_code;/* to type check or not (cmd line arg -nocheck */
extern int new_access;   /* boolean set in cpp/main.c using cmd line arg -a */
extern int fuse_level;   /* fusion level, set using -fusion */
extern int contract;     /* employ contraction */
extern int safe_code;    /* boolean set in cpp/main.c using command line -S */
extern int strict_check; /* boolean set in cpp/main.c using command line -s */
extern char *outputdir;  /* char buffer to put the output dir from comlin */
extern int quiet;        /* run in quiet mode */
extern int debug;        /* TRUE => enable ZGUARD compiler support */
extern int print_cpp_lines; /* TRUE => print "# 34 "goop.z" stuff */
extern int const_opt;    /* TRUE => allow compiler to evaluate constant
			    initialization expressions */
extern int flood_opt;    /* TRUE => allow compiler to optimize flood mloops */
extern int tile_opt;	 /* TRUE => allow compiler to tile mloops */
extern int c_opt;        /* TRUE => do low-level C optimizations */

#define STENCIL_NONE 0      /* do not run stencil opt */
#define STENCIL_UNROLL 1    /* run unroll version of stencil opt */
#define STENCIL_SCALAR 2    /* run scalar version of stencil opt */
#define STENCIL_VECTOR 3    /* run vector version of stencil opt */
extern int stencil_opt;     /* set to value of above definitions */
extern double stencil_adds; /* benefit of eliminating one addition */
extern double stencil_mems; /* benefit of eliminating one mem ref */
extern double stencil_muls; /* benefit of eliminating one multiplication */
extern double stencil_regs; /* benefit of using one register (negative) */
extern double stencil_bbar; /* benefit function must exceed this bbar */
extern int stencil_debug;   /* debug the stencil pass */

extern int mloop_opt;    /* TRUE => allow compiler to reorder mloop dims */
extern int cache_line_size; /* number of bytes in a cache line */
extern int cmo_alloc;	 /* TRUE => implies are allocated CMO */
extern int sr_opt;       /* TRUE => enable strength reduction */
extern int priv_access;  /* TRUE => emit private access macros */
/*** only one of these can be TRUE (i.e, !(commstats && compstats)) ***/
extern int commstats;    /* TRUE => emit macros for timing comm */
extern int compstats;    /* TRUE => emit macros for timing comp */
extern int paranal_help; /* TRUE => have paranal explain its actions */
extern int nomscanpeel;	 /* TRUE => do not peel mscan tiles */

extern int permmapshare;      /* TRUE => share permmap between permutes                */
extern int permmapsave;       /* TRUE => save permmaps for possible reuse              */
extern int permoverlap;       /* TRUE => overlap computation with permute              */
extern int permprocmap;       /* use procmap? -1, 0, 1 => compiler decides, no if possible, yes */
extern int permrleprocmap;    /* TRUE => run length encode proc map                    */
extern int permrleindices;    /* TRUE => run length encode indices                     */
extern int permloopdecode;    /* TRUE => use loops to decode rle indices               */
extern int permsavedataspace; /* TRUE => try to use dst and src as the data buckets    */
extern int permdirect;        /* TRUE => try to send data directly from dest to source */

/***
 *** compiler auto report flag, use -report=OPTIMIZATION
 ***
 *** use this flag to generate some info on an optimization and how it
 *** fired on a given program.  this is for the automatic testing
 *** system.
 ***
 *** to add an optimization to this auto test system, add a flag to
 *** this array and update NUM_REPORTS.  Modify the array
 *** initialization line in global.c.  Update main.c so that your
 *** report can be set.  Use if REPORT(num) to do stuff if your flag
 *** is used.  e.g. if (REPORT(REPORT_STENCIL)) print ("bla");
 ***/
#define NUM_REPORTS 3
#define REPORT_STENCIL 0
#define REPORT_NUDGE 1
#define REPORT_TEMPS 2
extern int makereport[NUM_REPORTS];
#define REPORT(num) (makereport[(num)])

extern int ran_a2nloops; /* TRUE => we've run a2nloops already */

typedef enum insertion_type {
  NAIVE_COMM=0,
  OPTIMIZED_COMM
  } insertion_type;
extern insertion_type comm_insert;	/*** use specified comm insertion ***/
extern int buffer_opt;			/*** try to optimize comm buffer usage, when possible ***/
extern int dynamic_reg_counter;  

extern datatype_t *pdtBOOLEAN;
extern datatype_t *pdtCHAR;
extern datatype_t *pdtINT;
extern datatype_t *pdtSHORT;
extern datatype_t *pdtLONG;
extern datatype_t *pdtFLOAT;
extern datatype_t *pdtDOUBLE;
extern datatype_t *pdtQUAD;
extern datatype_t *pdtVOID;
extern datatype_t *pdtGENERIC;
extern datatype_t *pdtGENERIC_ENSEMBLE;
extern datatype_t *pdtFILE;
extern datatype_t *pdtSTRING;
extern datatype_t *pdtSIGNED_BYTE;
extern datatype_t *pdtUNSIGNED_BYTE;
extern datatype_t *pdtUNSIGNED_SHORT;
extern datatype_t *pdtUNSIGNED_LONG;
extern datatype_t *pdtUNSIGNED_INT;
extern datatype_t *pdtOPAQUE;
extern datatype_t *pdtCOMPLEX;
extern datatype_t *pdtDCOMPLEX;
extern datatype_t *pdtQCOMPLEX;

extern datatype_t *pdtGRID_SCALAR[MAXRANK+1];
extern symboltable_t* pstGRID_SCALAR[MAXRANK+1];

extern datatype_t* pdtGRID;        /* only allowed in procedure prototypes */
extern datatype_t* pdtDISTRIBUTION;      /* only allowed in procedure prototypes */
extern datatype_t *pdtREGION;      /* only allowed in procedure prototypes */
extern datatype_t* pdtDIRECTION;
extern datatype_t *pdtPROCEDURE;   /* only allowed in procedure prototypes */

extern datatype_t* pdtRANK_REGION[MAXRANK+1];
extern datatype_t* pdtRANK_ENSEMBLE[MAXRANK+1];

extern symboltable_t *pstINDEX[MAXRANK+1];
extern symboltable_t *pstindex[MAXRANK+1];
extern symboltable_t *pstSTRNCPY;
extern symboltable_t *pstSTRNCMP;
extern symboltable_t *pstSTRNCAT;
extern symboltable_t* pstCHR2STR;
extern expr_t *pexprNULLSTR;
extern symboltable_t *pstLSHIFT;
extern symboltable_t *pstRSHIFT;
extern symboltable_t *pstBOR;
extern symboltable_t *pstBAND;
extern symboltable_t *pstBXOR;
extern symboltable_t *pstBNOT;
extern symboltable_t *pst_NULL;
extern expr_t* pexprUNKNOWN;
extern symboltable_t* pstCalcRedReg;
extern symboltable_t* pstAutoRedReg[MAXRANK+1];

extern symboltable_t *pst_qreg[MAXRANK+1];
extern symboltable_t* pst_free;
extern symboltable_t *pst_mfreg;  /* magic flood region */
extern symboltable_t *pst_dcid;
extern symboltable_t *pst_MSCANREG;
extern symboltable_t *pst_ms_tile_size[MAXRANK];

extern datatype_t *rmsframe;
extern symboltable_t *rmstack;

extern expr_t *gen_quotemask;
extern expr_t *pexpr_qmask[MAXRANK+1];
extern char* DESTPATH;
extern int codegen_imaginary;
extern int printing_string;
extern int brad_no_access;
extern int disable_new_access;
extern int argc;
extern char** argv;
extern symboltable_t* ExternalDep;
extern symboltable_t* maplvl[MAXLEVEL];
extern int GlobalTempCounter;
extern int* max_numensembles;
extern int mask_no_access;
extern int no_iterate;
extern glist GlobalSetup;
extern glist GlobalDestroy;
extern int retpt_used;
extern symboltable_t* mi_reg;
extern int maxdim;

extern int default_mloop_tile[MAXRANK];
extern int default_mloop_unroll[MAXRANK];
extern int default_mloop_order[MAXRANK];
extern int default_mloop_up[MAXRANK];
extern int default_mloop_tile_order[MAXRANK];
extern int default_mloop_tile_up[MAXRANK];

extern int sparsity_type;

extern int seqonly;

extern int optimize_sparse_storage;

extern int do_insert_checkpoints;
extern int do_checkpoint;

extern symboltable_t* chkpoint;
extern symboltable_t* chklabel;
extern symboltable_t* checkpointing;
extern symboltable_t* recovering;
extern symboltable_t* recoverfilename;
extern symboltable_t* chksave;
extern symboltable_t* chkrecover;
extern symboltable_t* ckpt_interval; /*** sungeun *** exported config var for checkpointing ***/

extern int numperms; /* number of permutations */
extern int numshare; /* number of permutation maps shared */

extern int restrictkw; /* 1 ==> use keyword restrict where applicable */

extern int use_default_distribution;

extern glist GlobalDistributions;
extern glist GlobalGrids;

extern int use_charstar_pointers;

extern int do_bounds_checking;

extern expr_t* currentDirection;

#define HOIST_PTRS_TO_MLOOP   1
#define HOIST_PTRS_TO_SUBPROG 2

extern int hoist_access_ptrs;
extern int hoist_access_mults;

#endif
