/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "../include/IronCCgen.h"
#include "../include/Spsgen.h"
#include "../include/cglobal.h"
#include "../include/dtype.h"
#include "../include/error.h"
#include "../include/getopt.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/main.h"
#include "../include/parsetree.h"
#include "../include/scan.h"
#include "../include/struct.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/treemac.h"
#include "../include/usage.h"
#include "../include/version.h"

extern int yydebug;

static void signal_handler(int sig) {
  switch (sig) {
  case SIGTRAP:      /*** trace trap ***/
    INT_FATAL(NULL, "Trace trap.");
  case SIGFPE:       /*** floating point exception ***/
    INT_FATAL(NULL, "Floating point exception.");
  case SIGBUS:       /*** bus error ***/
    INT_FATAL(NULL, "Bus error.");
  case SIGSEGV:      /*** segmentation violation ***/
    INT_FATAL(NULL, "Segmentation violation.");
  default:
    INT_FATAL(NULL, "Unrecognized signal.");
  }
}


static void initialize(void) {
  initialize_lexer();	/* linc 4/22/93 */
  initialize_datatypes();
  initialize_symtab();
  initialize_pstack();
}

static void check_prototypes(void) {
  int i;
  hashentry_t *phe;
  symboltable_t *pst;

  for (i = 0; i < MAXHASH; i++) {
    phe = getsymtab(i);
    while (phe != NULL) {
      pst = HE_SYM(phe);
      while (pst != NULL) {
	if (S_CLASS(pst) == S_SUBPROGRAM) {
	  if (T_STLS(S_FUN_BODY(pst)) == NULL &&
	      (!(S_STYPE(pst) & SC_EXTERN))) {
	    USR_FATAL_CONTX(S_LINENO(pst), S_FILENAME(pst),
			    "non-extern function prototype, %s, has no definition", S_IDENT(pst));
	  }
	}
	pst = S_NEXT(pst);
      }
      phe = HE_NEXT(phe);
    }
  }
}

int main(int argc, char **argv) {
  usage_t from_name = FROM_ZC0;
  char *real_in_file = NULL;
  char *arg, ch;
  char *pch;
  int zbasenamelen;
  FILE *fpPASS = NULL;
  int flag = 0;

  signal(SIGTRAP, signal_handler);      /*** trace trap ***/
  signal(SIGFPE,  signal_handler);      /*** floating point exception ***/
  signal(SIGBUS,  signal_handler);      /*** bus error ***/
  signal(SIGSEGV, signal_handler);      /*** segmentation violation ***/

  init_args(argc, argv);

  /*** go past first argument ***/
  arg = get_arg(&flag, &ch);

  arg = get_arg(&flag, &ch);
  /*** zc will pass in the -zccommand flag as the *first* arg. ***/
  /***  This will allow us to print out the correct usage info ***/
  /***  and print out the string in the generated code.        ***/
  if (flag && (ch == 'z')) {
    if (!strcmp(arg, "ccommand")) {
      from_name = FROM_ZC;
    }
    /*** the next argument should be the zc command line ***/
    arg = get_arg(&flag, &ch);
    if (flag) {
      printf("ERROR: Improper use of -zccomand flag.  There may be a bug in zc.\n");
      use_usage(from_name);
    }
    zc_command = (char *) PMALLOC((strlen(argv[2])+1)*sizeof(char));
    /*** strip off the quotes ***/
    strcpy(zc_command, argv[2]+1);
    zc_command[strlen(zc_command)-1] = '\0';
    arg = get_arg(&flag, &ch);
  }

  /************************************************************************/
  /*** WARNING: When adding flags that take arguments, you must make zc ***/
  /***          understand them also!   Please see zc.c for more info   ***/
  /************************************************************************/
  if (argc > 1) {
    do {
      if (flag) {
	switch(ch) {
	case 'a':
	  if (!strcmp(arg, "utocheckpoint")) {
	      do_insert_checkpoints = TRUE;
	  } else {
	    printf("ERROR: Unrecognized flag (-a%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	case 'c':
	  if (!strcmp(arg, "mo")) {
	      cmo_alloc = TRUE;
	  } else if (!strcmp(arg, "heckpoint")) {
	    do_checkpoint = TRUE;
	  } else {
	    printf("ERROR: Unrecognized flag (-c%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	case 'd':
	  if (!strcmp(arg, "bgparser")) {
	    yydebug = 1;
	    break;
	  }
	  if (*arg == '\0') {
	    /*** skip over the space ***/
	    arg = get_arg(&flag, &ch);
	  }
	  else {
	    flag = 0;
	  }
	  if ((arg != NULL) && (flag == 0)) {
	    outputdir = (char *) PMALLOC((strlen(arg)+1)*sizeof(char));
	    strcpy(outputdir, arg);
	  }
	  else {
	    printf("ERROR: Incorrect use of -d\n");
	    use_usage(from_name);
	  }
	  break;
	case 'f':
	  if (!strncmp(arg, "usion", 5)) {
	    arg += 5;    /*** move past "usion" ***/
	    if (*arg == '\0') {
	      fuse_level = 3;
	    }
	    else if (*arg == '0') {
	      fuse_level = 0;
	    }
	    else if (*arg == '1') {
	      fuse_level = 1;
	    }
	    else if (*arg == '2') {
	      fuse_level = 2;
	    }
	    else if (*arg == '3') {
	      fuse_level = 3;
	    }
	    else {
	      printf("ERROR: Incorrect use of -fusion\n");
	      use_usage(from_name);
	    }
	  }
	  else {
	    printf("ERROR: Unrecognized flag (-f%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	case 'g':
	  /* enable zguard compiler support */
	  debug = 1;
	  print_cpp_lines = TRUE;
	  break;
	case 'I':
	  if (*arg == '\0') {
	    /*** skip over the space ***/
	    arg = get_arg(&flag, &ch);
	  }
	  else {
	    flag = 0;
	  }
	  if ((arg != NULL) && (flag == 0)) {
	    /*** skip over it and let codegen take care of it ***/
	    /*** i don't like this, but i don't want to fix it now ***/
	  }
	  else {
	    printf("ERROR: Incorrect use of -I\n");
	    use_usage(from_name);
	  }
	  break;
	case 'l':
	case 'L':
          if (!strcmp(arg,"ine_size")) {
	    arg = get_arg(&flag,&ch);
	    cache_line_size = atoi(arg);
            printf("line size is _%s_ %d\n",arg,cache_line_size);
          }
	  if (*arg == '\0') {
	    /*** skip over the space ***/
	    arg = get_arg(&flag, &ch);
	  }
	  else {
	    flag = 0;
	  }
	  if ((arg != NULL) && (flag == 0)) {
	    /*** skip over it and let codegen take care of it ***/
	    /*** i don't like this, but i don't want to fix it now ***/
	  }
	  else {
	    printf("ERROR: Incorrect use of -%c\n", ch);
	    use_usage(from_name);
	  }
	  break;
	case 'm':
	  if (!strncmp(arg,"loop_",5)) {
	    int i=0;

	    arg+= 5;

	    if (!strncmp(arg,"unroll=",7)) {
	      char* argptr = arg+7;

	      while (*argptr!='\0' && i<MAXRANK) {
		default_mloop_unroll[i] = *argptr - '0';

		argptr++;
		i++;
	      }
	    } else if (!strncmp(arg,"order=",6)) {
	      char* argptr = arg+6;

	      while (*argptr!='\0' && i<MAXRANK) {
		default_mloop_order[i] = *argptr - '0';

		argptr++;
		i++;
	      }
	    } else if (!strncmp(arg,"up=",3)) {
	      char* argptr = arg+3;

	      while (*argptr!='\0' && i<MAXRANK) {
		if (*argptr == '0') {
		  default_mloop_up[i] = -1;
		} else {
		  default_mloop_up[i] = 1;
		}

		argptr++;
		i++;
	      }
	    } else if (!strncmp(arg,"tile=",5)) {
	      char* argptr = arg+5;

	      while (*argptr!='\0' && i<MAXRANK) {
		default_mloop_tile[i] = *argptr - '0';

		argptr++;
		i++;
	      }
	    } else if (!strncmp(arg,"tile_order=",11)) {
	      char* argptr = arg+11;

	      while (*argptr!='\0' && i<MAXRANK) {
		default_mloop_tile_order[i] = *argptr - '0';

		argptr++;
		i++;
	      }
	    } else if (!strncmp(arg,"tile_up=",8)) {
	      char* argptr = arg+8;

	      while (*argptr!='\0' && i<MAXRANK) {
		if (*argptr == '0') {
		  default_mloop_tile_up[i] = -1;
		} else {
		  default_mloop_tile_up[i] = 1;
		}

		argptr++;
		i++;
	      }
	    }
	  }
	  else if (!strcmp(arg, "scanpeel")) {
	    nomscanpeel = FALSE;
	  }
	  break;
	case 'n' :
	  if (!strcmp(arg, "ocheck")) {
	    typecheck_code = FALSE;
	  }
	  else if (!strcmp(arg, "oaccessopt")) {
	    new_access = 0;
	  }
	  else if (!strcmp(arg, "obytewalkers")) {
	    use_charstar_pointers = 0;
	  }
	  else if (!strcmp(arg, "ocommopt")) {
	    comm_insert = NAIVE_COMM;
	  }
	  else if (!strcmp(arg, "ocontract")) {
	    contract = FALSE;
	  }
	  else if (!strcmp(arg, "oconstopt")) {
	    const_opt = FALSE;
	  }
	  else if (!strcmp(arg, "ocopts")) {
	    c_opt = FALSE;
	  }
	  else if (!strcmp(arg, "ocpplines")) {
	    print_cpp_lines = FALSE;
	  }
	  else if (!strcmp(arg, "ofloodopt")) {
	    flood_opt = FALSE;
	  }
	  else if (!strcmp(arg, "oregopt")) {
	    mloop_opt = FALSE;
	  }
	  else if (!strcmp(arg, "orestrict")) {
	    restrictkw = FALSE;
	  }
	  else if (!strcmp(arg, "otile")) {
	    tile_opt = FALSE;
	  }
	  else if (!strcmp(arg, "oprivate")) {
	    priv_access = FALSE;
	  }
	  else if (!strcmp(arg, "opermopts")) {
	    permmapshare = FALSE;
	    permmapsave = FALSE;
	    permoverlap = FALSE;
	    permrleprocmap = FALSE;
	    permrleindices = FALSE;
	    permloopdecode = FALSE;
	    permsavedataspace = FALSE;
	    permdirect = FALSE;
	  }
	  else if (!strcmp(arg, "opermmapshare")) {
	    permmapshare = FALSE;
	  }
	  else if (!strcmp(arg, "opermmapsave")) {
	    permmapsave = FALSE;
	  }
	  else if (!strcmp(arg, "opermoverlap")) {
	    permoverlap = FALSE;
	  }
	  else if (!strncmp(arg, "opermprocmap=0", 14)) {
	    permprocmap = 0;
	  }
	  else if (!strncmp(arg, "opermprocmap=1", 14)) {
	    permprocmap = 1;
	  }
	  else if (!strcmp(arg, "opermrleprocmap")) {
	    permrleprocmap = FALSE;
	  }
	  else if (!strcmp(arg, "opermprocmap")) {
	    permprocmap = FALSE;
	  }
	  else if (!strcmp(arg, "opermrleindices")) {
	    permrleindices = FALSE;
	    permloopdecode = FALSE;
	  }
	  else if (!strcmp(arg, "opermloopdecode")) {
	    permloopdecode = FALSE;
	  }
	  else if (!strcmp(arg, "opermsavedataspace")) {
	    permsavedataspace = FALSE;
	  }
	  else if (!strcmp(arg, "opermdirect")) {
	    permdirect = FALSE;
	  }
	  else if (!strcmp(arg, "osparsememopt")) {
	    optimize_sparse_storage = 0;
	  }
	  else if (!strcmp(arg, "ostrength")) {
	    sr_opt = FALSE;
	  }
	  else {
	    printf("ERROR: Unrecognized flag (-n%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	case 'o':
	  /*** specify the final binary name ***/
	  if (*arg == '\0') {
	    /*** skip over the space ***/
	    arg = get_arg(&flag, &ch);
	  }
	  else {
	    flag = 0;
	  }
	  if ((arg != NULL) && (flag == 0)) {
	    zbasename = (char *) PMALLOC((strlen(arg)+1)*sizeof(char));
	    strcpy(zbasename, arg);
	  }
	  else {
	    printf("ERROR: Incorrect use of -o\n");
	    use_usage(from_name);
	  }
	  break;
	case 'O':
	  if (*arg == '\0') {
	    /*** full optimizations ***/
	    new_access = 1;
	    fuse_level = 3;
	    contract = TRUE;
	    comm_insert = OPTIMIZED_COMM;
	    const_opt = TRUE;
	    flood_opt = TRUE;
	    tile_opt = TRUE;
	    mloop_opt = TRUE;
	    sr_opt = TRUE;
	    stencil_opt = STENCIL_UNROLL;
	  }
	  else if (*arg == '0') {
	    /*** no optimizations ***/
	    new_access = 0;
	    fuse_level = 0;
	    contract = FALSE;
	    comm_insert = NAIVE_COMM;
	    const_opt = FALSE;
	    flood_opt = FALSE;
	    tile_opt = FALSE;
	    mloop_opt = FALSE;
	    sr_opt = FALSE;
	    stencil_opt = STENCIL_NONE;
	  }
	  else if (*arg == '1') {
	    /*** full optimizations ***/
	    new_access = 1;
	    fuse_level = 3;
	    contract = TRUE;
	    comm_insert = OPTIMIZED_COMM;
	    const_opt = TRUE;
	    flood_opt = TRUE;
	    tile_opt = TRUE;
	    mloop_opt = TRUE;
	    sr_opt = TRUE;
	    stencil_opt = STENCIL_UNROLL;
	  }
	  else {
	    /*** full optimizations ***/
	    USR_WARN(NULL, "Optimization level -O%c not supported, using -O1", 
		     *arg);
	    new_access = 1;
	    fuse_level = 3;
	    contract = TRUE;
	    comm_insert = OPTIMIZED_COMM;
	    const_opt = TRUE;
	    flood_opt = TRUE;
	    tile_opt = TRUE;
	    mloop_opt = TRUE;
	    sr_opt = TRUE;
	    stencil_opt = STENCIL_UNROLL;
	  }
	  break;
	case 'p' :
	  if (*arg == '\0') {
	    /*** skip over the space ***/
	    arg = get_arg(&flag, &ch);
	  }
	  else {
	    flag = 0;
	  }
	  if ((arg != NULL) && (flag == 0)) {
	    if ((fpPASS = fopen(arg, "r")) == NULL) {
	      USR_FATAL(NULL, "Error opening file pass file '%s'", arg);
	    }
	  }
	  else {
	    printf("ERROR: Incorrect use of -p\n");
	    use_usage(from_name);
	  }
	  break;
	case 'P' :
	  if (!strcmp(arg, "commstats")) {
	    if (compstats == TRUE) {
	      USR_WARN(NULL, "Cannot profile both communication and computation.  Ignoring -P%s.\n", arg);
	    }
	    else {
	      /*** profile time spent performing Ironman communication ***/
	      commstats = TRUE;
	    }
	  }
	  else if (!strcmp(arg, "compstats")) {
	    if (commstats == TRUE) {
	      USR_WARN(NULL, "Cannot profile both communication and computation.  Ignoring -P%s.\n", arg);
	    }
	    else {
	      /*** profile time spent performing computation ***/
	      compstats = TRUE;
	    }
	  }
	  else {
	    printf("ERROR: Unrecognized flag (-P%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	case 's':
	  if (!strcmp(arg, "trict")) {
	    strict_check = TRUE;
	  }
	  else if (!strncmp(arg, "eqonly", 6)) {
	    seqonly = TRUE;
	  }
	  else if (!strncmp(arg, "parse", 5)) {
	    arg += 5;
	    if (!strcmp(arg,"=access")) {
	      sparsity_type = _DIR_DNS;
	    } else if (!strcmp(arg,"=link")) {
	      sparsity_type = _DIR_SPS;
	    } else {
	      printf("ERROR: Incorrect use of -sparse\n");
	      use_usage(from_name);
	    }
	  }
	  else if (!strcmp(arg, "afe")) {
	    do_bounds_checking = TRUE;
	  }
	  else if (!strncmp(arg, "tencil", 6)) {
	    arg += 6;
	    if (*arg == '\0') {
	      stencil_opt = STENCIL_UNROLL;
	    }
	    else if (!strcmp(arg, "=unroll")) {
	      stencil_opt = STENCIL_UNROLL;
	    }
	    else if (!strcmp(arg, "=scalar")) {
	      stencil_opt = STENCIL_SCALAR;
	    }
	    else if (!strcmp(arg, "=vector")) {
	      stencil_opt = STENCIL_VECTOR;
	    }
	    else if (!strcmp(arg, "=none")) {
	      stencil_opt = STENCIL_NONE;
	    }
	    else if (!strncmp(arg, "_adds=", 6)) {
	      arg += 6;
	      stencil_adds = atof(arg);
	    }
	    else if (!strncmp(arg, "_mems=", 6)) {
	      arg += 6;
	      stencil_mems = atof(arg);
	    }
	    else if (!strncmp(arg, "_muls=", 6)) {
	      arg += 6;
	      stencil_muls = atof(arg);
	    }
	    else if (!strncmp(arg, "_regs=", 6)) {
	      arg += 6;
	      stencil_regs = atof(arg);
	    }
	    else if (!strncmp(arg, "_bbar=", 6)) {
	      arg += 6;
	      stencil_bbar = atof(arg);
	    }
	    else if (!strncmp(arg, "_debug", 6)) {
	      arg += 6;
	      stencil_debug = TRUE;
	    }
	    else {
	      printf("ERROR: Incorrect use of -stencil\n");
	      use_usage(from_name);
	    }
	  }
	  else {
	    printf("ERROR: Unrecognized flag (-s%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	case 't':
	  if (!strcmp(arg, "okenize")) {
	    tokenize = TRUE;
	  }
	  break;
	case 'V':			/*** version ***/
	  print_version();
	  exit(1);
	case 'v':
	  if (!strcmp(arg, "erbose")) {
	    verbose = TRUE;
	  }
	  else {
	    printf("ERROR: Unrecognized flag (-v%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	case 'q':
	  if (*arg == '\0') {
	    quiet = TRUE;
	  }
	  else if (*arg == '0') {
	    quiet = TRUE;
	  }
	  else if (*arg == '1') {
	    quiet = FALSE;
	  }
	  else {
	    printf("ERROR: Unrecognized flag (-q%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	case 'h':
	  if (!strcmp(arg,"elppar")) {
	    paranal_help = 1;
	  } else {
	    /*** emit usage information for zc0 ***/
	    usage(FROM_ZC0);
	  }
	  break;
	case 'r':
	  if (!strncmp(arg, "le", 2)) {
	    arg += 2;
	    if (!strncmp(arg, "indices=", 8)) {
	      arg += 8;
	      if (!strcmp(arg, "rank")) {
		permrleindices = -1;
	      }
	      else {
		permrleindices = atoi(arg);
	      }
	    }
	    else if (!strncmp(arg, "procmap=", 8)) {
	      arg += 8;
	      if (!strcmp(arg, "rank")) {
		permrleprocmap = -1;
	      }
	      else {
		permrleprocmap = atoi(arg);
	      }
	    }
	    else {
	      printf ("ERROR: Unrecognized flag (-rle%s)\n", arg);
	      use_usage(from_name);
	    }
	  }
	  else if (!(strcmp(arg,"eport=stencil"))) {
	    makereport[REPORT_STENCIL] = TRUE;
	  }
	  else if (!(strcmp(arg,"eport=nudge"))) {
	    makereport[REPORT_NUDGE] = TRUE;
	  }
	  else if (!(strcmp(arg,"eport=temps"))) {
	    makereport[REPORT_TEMPS] = TRUE;
	  }
	  else {
	    printf ("ERROR: Unrecognized flag (-r%s)\n", arg);
	    use_usage(from_name);
	  }
	  break;
	default:
	  printf("ERROR: Unrecognized flag (-%c%s)\n", ch, arg);
	  use_usage(from_name);
	}
      } else {
	if (arg[strlen(arg)-2]=='.') {
	  switch (arg[strlen(arg)-1]) {
	  case 'z':
	    if (real_in_file == NULL) {
	      real_in_file = arg;
	    } else {
	      USR_FATAL(NULL, "zpl cannot currently handle mutiple .z files");
	    }
	    break;
	  case 'o':
	  case 'c':
	  case 'h':
	    break;
	  default:
	    USR_FATAL(NULL, "zpl can only handle files with .c, .o, .h, "
		      "and .z extensions");
	    break;
	  }
	} else {
	  USR_FATAL(NULL, "zpl can only handle files with .c, .o, .h, "
		    "and .z extensions");
	}
      }
    } while ((arg = get_arg(&flag, &ch)) != NULL);
  }

  if (real_in_file != NULL) {
    if ((fname = fopen(real_in_file, "r")) == NULL) {
      USR_FATAL(NULL, "Error opening input file '%s'", real_in_file);
    }

    if (in_file == NULL) {
      in_file = real_in_file;
      base_in_file = strrchr(in_file,'/');
      if (base_in_file == NULL) {
	base_in_file = in_file;
      } else {
	base_in_file++;
      }
    }

    if (zbasename == NULL) {
      /*** get the basename for code gen ***/
      zbasenamelen = strlen(in_file)-1;
      zbasename = (char *) PMALLOC(zbasenamelen*sizeof(char));

      /*** strip off the .z ***/
      strncpy(zbasename, in_file, zbasenamelen-1);
      zbasename[zbasenamelen-1] = '\0';

      /*** remove path from basename ***/
      pch = strrchr(zbasename, '/');
      if (pch!=NULL) {
	zbasename = pch + 1;
      }
    }

  } else {
    fname = stdin;
    in_file = "stdin";
  }

  initialize();
  FATALCONTINIT();
  if (yyparse() != 0) {
    USR_FATAL_CONT(NULL, "Parse errors detected");
  }
  FATALCONTDONE();
  FATALCONTINIT();
  check_prototypes();
  FATALCONTDONE();
  fclose(fname);

  {
    symboltable_t* pst;
    pst = lu("CheckTimer");
    T_PARALLEL(S_FUN_BODY(pst)) = TRUE;
    pst = lu("ResetTimer");
    T_PARALLEL(S_FUN_BODY(pst)) = TRUE;
  }

  pstMAIN = lu(entry_name);
  if (pstMAIN == NULL || S_FUN_BODY(pstMAIN) == NULL ||
      T_STLS(S_FUN_BODY(pstMAIN)) == NULL) {
    USR_FATAL(NULL, "Expecting entry procedure named '%s' but none was found",
	       entry_name);
  }

  return runpasses(zpl_module, fpPASS);
}

