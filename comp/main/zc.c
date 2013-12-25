/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** FILE:   zc.c
 *** AUTHOR: Sung-Eun Choi (sungeun@cs.washington.edu)
 ***
 *** NOTES:  This file is somewhat of a mess (by necessity), but the
 ***         comments associated with variable name should help.
 ***
 ***         Make sure any tmp files have a clean routine (which is
 ***         called by the signal handler) so these file can be cleaned
 ***         up in case there is an unexpected exit.
 ***
 ***/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "../include/version.h"
#include "../include/error.h"
#include "../include/usage.h"
#include "../include/const.h"
#include "../include/macros.h"

static char* exe_suffix;

/*** stuff to make this compile! ***/
/*** to resolve errors when linking with error.o ***/
int fatal_count;
char prevline[MAXBUFF];  /* should match scan.h */
char currline[MAXBUFF];  /* should match scan.h */
int column;
int ntokens;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

#define _ZC0 "zc0"
#define _RZC0 "rzc0"

#define _GDB "gdb"

#define _VALGRIND "valgrind --quiet"

#define _VALGRINDGDB "valgrind --gdb-attach=yes"

#define DEFAULT_TMPDIR "/tmp"
#define COMPAT_PATHBASE "./output"

#define CPP "cpp"

static char *zc0_name(char *);
static usage_t set_zc_type(char *);
static char *re_scratch(char *, int);
static void make_command(char *, int, const char *, ...);
static void exec_command(const char *, const char *, ...);
static void zc_signal_handler(int);
static void clean_and_exit(int);
static void clean_cpp(void);
static void clean_passfile(void);
static void clean_outputdir(void);
static void clean_buildoutput(void);
static char *get_unique_name(char *, char *);
static void print_command(const char *);

static char *zpl_home = NULL;       /*** strings for required env vars ***/
static char *zpl_target = NULL;
static char *zpl_commlayer = NULL;
static int print_commands = FALSE;  /*** print compiler passes? ***/
static char *full_zc0_name;         /*** zc0 command ***/
static usage_t zc_type;             /*** from zc or rzc? ***/

/*** global variables needed for signal handler ***/
static int cpp = FALSE;             /*** run cpp? ***/
static char *cpp_filename = NULL;   /*** name of the cpp'ed file ***/
static int user_passfile = FALSE;   /*** name of user pass file ***/
static char *tmp_passfile = NULL;   /*** name of cpp'ed pass file ***/
static int user_outputdir = FALSE;  /*** user specified an output dir? ***/
static char *outputdir = NULL;      /*** name of output dir ***/
static char *tmp_buildoutput = NULL;/*** output from zmake ***/
static int build_error = FALSE;     /*** zmake error ***/
static int compatibility = FALSE;   /*** compatibility with old interface? ***/
static int savec = FALSE;           /*** save zc0 generated code? ***/
static char *out_basename = NULL;   /*** base name of output ***/
static char **other_files = NULL;   /*** other .c or .o files ***/
static int num_other = 0;           /*** number of other files ***/
static int rm_binaries = TRUE;      /*** internal flag (see below) ***/
static char *src_dir = NULL;        /*** location of .z file ***/
static int src_dir_len = 0;         /*** length of src_dir ***/

int
main(int argc, char ** argv)
{
  int i, size;
  char *ch;
  char zmake_flags[512]="";    /*** buffer for flags to zmake ***/
  char small_buffer[512];      /*** scratch buffer, for small things ***/
  char *scratch = NULL;        /*** generic scratch buffer ***/
  int new_i;                   /*** argc for zc0 ***/
  char **newargv;              /*** argv for zc0 ***/
  char *new_path;              /*** corrected path for -I and -L flags ***/
  char *tmpdir;                /*** temporary directory to use ***/
  char *cpp_flags = NULL;      /*** user specified cpp flags (-D and -U) ***/
  char *include_path = NULL;   /*** include path for (all) calls to cpp ***/
  char *zc_command = NULL;     /*** zc command (passed to zc0) ***/
  int zc_size = 0;             /*** zc of zc command ***/
  char *zc0_binary = NULL;     /*** user specified zc0 binary ***/
  char *zc0_command = NULL;    /*** zc0 command line ***/
  int zc0_size = 0;            /*** size of zc0 command line ***/
  char *passfile = NULL;       /*** name of user specified pass file ***/
  char *in_basename = NULL;    /*** base name of original input file ***/
  char *in_dirname = NULL;     /*** name of directory for zc0 output ***/
  int valgrindzc0 = FALSE;     /*** run zc0 using valgrind? ***/
  int debugzc0 = FALSE;        /*** run zc0 using gdb? ***/
  char *debugzc0usage = NULL;  /*** usage string for gdb session ***/
  int build = TRUE;            /*** build the binary ***/
  int zfile_arg_no = 0;        /*** arg # (in argv) of the .z ***/
  struct stat status0, status1;/*** status variable for calls to stat() ***/

  /*** 50 extra args (should be enough) ***/
  newargv = (char **) malloc((argc+50)*sizeof(char *));

  set_zc_type(argv[0]);

  /* save space for a -I. and -L. -style flag */
  new_i = 5;

  /*** process all zc arguments and pass others on to zc0 ***/
  for (i = 1; i < argc;
       i++ /*** notice that this gets incremented in the loop, too ***/) {
    if (*argv[i] == '-') {
      /*** parse command line flags ***/
      switch(argv[i][1]) {
      case 'c':
	if (!strcmp(argv[i], "-cpp")) {
	  /*** run cpp on the input zpl file ***/
	  cpp = TRUE;
	}
	else if (!strcmp(argv[i], "-compat")) {
	  /*** mimic the behaviour of the old compiler (zpl) ***/
	  compatibility = TRUE;
	  user_outputdir = TRUE;
	  build = FALSE;
	  savec = TRUE;
	}
	else {
	  newargv[new_i++] = argv[i];
	}
	break;
      case 'd':
	if (!strcmp(argv[i], "-d")) {
	  /*** specify an output directory ***/
	  /*** this flag also gets passed to zc0 ***/
	  newargv[new_i++] = argv[i];
	  /*** check if there are more args ***/
	  if (++i >= argc) {
	    printf("ERROR: Incorrect use of -d\n");
	    use_usage(zc_type);
	  }
	  outputdir = (char *) malloc((strlen(argv[i])+1)*sizeof(char));
	  strcpy(outputdir, argv[i]);
	  if (stat(outputdir, &status0) == -1) {
	    /*** specified output directory does not exist ***/
	    USR_FATAL(NULL, "Directory '%s' does not exist.", outputdir);
	  }
	  newargv[new_i++] = outputdir;
	  user_outputdir = TRUE;
	}
	else if (!strcmp(argv[i], "-dbgzc0")) {
	  /* run zc0 in gdb */
	  debugzc0 = TRUE;
	}
	else {
	  newargv[new_i++] = argv[i];
	}
	break;
      case 'D':
      case 'U':
	/*** pass these flags along to cpp ***/
	if (cpp_flags == NULL) {
	  cpp_flags = (char *) malloc((strlen(argv[i])+1)*sizeof(char));
	  cpp_flags[0] = '\0';
	}
	else {
	  cpp_flags = (char *) realloc(cpp_flags,
				       strlen(cpp_flags)+strlen(argv[i])+2);
	  strcat(cpp_flags, " ");
	}
	strcat(cpp_flags, argv[i]);
	break;
      case 'g':
	if (!strcmp(argv[i], "-g")) {
	  /*** link with debug libraries -- for now, do nothing ***/
	  /*** this flag also gets passed to zc0 ***/
	  newargv[new_i++] = argv[i];
	}
	else {
	  newargv[new_i++] = argv[i];
	}
	break;
      case 'h':
	if (!strcmp(argv[i], "-helppar")) {
	  newargv[new_i++] = argv[i];
	} else {
	  /*** print out usage information and quit ***/
	  usage(zc_type);
	}
	break;
      case 'I':
      case 'L':
	/*** zc currently allow a space after the -I or -L flag ***/
	/*** specify include paths or library paths ***/
	/*** these flags also get passed to zc0 ***/
	if (argv[i][2] == '\0') {
	  /*** there is a space between the flag and the name ***/
	  newargv[new_i++] = argv[i];
	  /*** check if there are more args ***/
	  if (++i >= argc) {
	    printf("ERROR: Incorrect use of -%c\n", argv[i-1][1]);
	    use_usage(zc_type);
	  }
	  if (argv[i][0] == '-') {
	    printf("ERROR: Incorrect use of -%c\n", argv[i-1][1]);
	    use_usage(zc_type);
	  }
	  /*** point to the path name ***/
	  ch = argv[i];
	}
	else {
	  /*** name is right after flag ***/
	  /*** pass the flag along ***/
	  ch = (char *) malloc(3*sizeof(char));
	  strncpy(ch, argv[i], 2);
	  ch[2] = '\0';
	  newargv[new_i++] = ch;
	  /*** point to the path name ***/
	  ch = &(argv[i][2]);
	}
	/*** correct relative path names ***/
	if (*ch != '/') {
	  new_path = (char *) malloc((PATH_MAX+1+strlen(ch)+1+2)*
				     sizeof(char));
	  getcwd(new_path, PATH_MAX);
	  strcat(new_path, "/");
	  strcat(new_path, ch);
	}
	else {
	  new_path = ch;
	}
	newargv[new_i++] = new_path;
	if (newargv[new_i-2][1] == 'I') {
	  /*** copy the include paths for calls to cpp ***/
	  if (include_path == NULL) {
	    include_path = malloc((3+strlen(new_path)+1+2)*sizeof(char));
	    include_path[0] = '\0';
	  }
	  else {
	    include_path = realloc(include_path,
				   (strlen(include_path)+1 +
				    3+strlen(new_path)+1+2)*sizeof(char));
	    strcat(include_path, " ");
	  }
	  /*** cpp doesn't like spaces ***/
	  strcat(include_path, "-I\"");
	  strcat(include_path, new_path);
	  strcat(include_path, "\"");
	}
	break;
      case 'n':
	if (!strcmp(argv[i], "-nobuild")) {
	  /*** don't build the binary ***/
	  build = FALSE;
	  savec = TRUE;
	}
	else {
	  newargv[new_i++] = argv[i];
	}
	break;
      case 'o':
	/*** specify the name of the final binary ***/
	/** this flag also gets paased to zc0 ***/
	if (!strcmp(argv[i], "-o")) {
	  /*** there is a space between -o and the output file name ***/
	  newargv[new_i++] = argv[i];
	  /*** check if there are more args ***/
	  if (++i >= argc) {
	    printf("ERROR: Incorrect use of -o\n");
	    use_usage(zc_type);
	  }
	  /*** pass along the argument ***/
	  newargv[new_i++] = argv[i];
	  /*** and copy it for later use ***/
	  out_basename = (char *) malloc((strlen(argv[i])+1)*sizeof(char));
	  strcpy(out_basename, argv[i]);
	}
	else {
	  /*** output file name is right after -o ***/
	  out_basename = (char *) malloc((strlen(argv[i])-2+1)*sizeof(char));
	  strcpy(out_basename, argv[i]+2);
	  newargv[new_i++] = argv[i];
	}
	break;
      case 'p':
	/*** specify pass file (run cpp on the pass file) ***/
	/** this flag also gets paased to zc0 ***/
	if (!strcmp(argv[i], "-p")) {
	  /*** there is a space between -p and the pass file name ***/
	  newargv[new_i++] = argv[i++];
	  if (argv[i][0] == '-') {
	    /*** improper argument to -p ***/
	    printf("ERROR: Incorrect use of -p\n");
	    use_usage(zc_type);
	  }
	  passfile = (char *) malloc((strlen(argv[i])+1)*sizeof(char));
	  strcpy(passfile, argv[i]);
	}
	else {
	  /*** pass file name is right after -p ***/
	  ch = (char *) malloc(3*sizeof(char));
	  strncpy(ch, argv[i], 2);
	  ch[2] = '\0';
	  newargv[new_i++] = ch;
	  passfile = (char *) malloc((strlen(argv[i])-2+1)*sizeof(char));
	  strcpy(passfile, &(argv[i][2]));
	}
	user_passfile = new_i;
	/*** use as place holder until we get name of preproccesd file ***/
	newargv[new_i++] = passfile;
	break;
      case 's':
	if (!strcmp(argv[i], "-savec")) {
	  /*** save the generated C files ***/
	  savec = TRUE;
	}
	else {
	  newargv[new_i++] = argv[i];
	}
	break;
      case 'v':
	if (!strcmp(argv[i], "-valgrind")) {
	  /* run zc0 with valgrind */
	  valgrindzc0 = TRUE;
	}
	else if (!strcmp(argv[i], "-v")) {
	  print_commands = TRUE;
	}
	else {
	  newargv[new_i++] = argv[i];
	}
	break;
      case 'V':
	if (!strcmp(argv[i], "-V")) {
	  print_version();
	  exit(1);
	}
	else {
	  newargv[new_i++] = argv[i];
	}
	break;
      case 'z':
	if (!strcmp(argv[i], "-zc0")) {
	  /*** get the name of the binary ***/
	  zc0_binary = argv[++i];
	}
	else {
	  newargv[new_i++] = argv[i];
	}
	break;
      /******************************************************/
      /*** Must recognize any flags that require args and ***/
      /***  pass them along appropriately:                ***/
      /***      -I <path>                                 ***/
      /***      -l <library>                              ***/
      /***      -L <path>                                 ***/
      /******************************************************/
      case 'l':
	/*** zc currently allow a space after the -l flag ***/
	/*** specify libraries ***/
	if (!strcmp(argv[i], "-l")) {
	  /*** there is a space between -l and the library name ***/
	  newargv[new_i++] = argv[i];
	  if (++i >= argc) {
	    printf("ERROR: Incorrect use of -l\n");
	    use_usage(zc_type);
	  }
	  if (argv[i][0] == '-') {
	    /*** improper argument to -l ***/
	    printf("ERROR: Incorrect use of -l\n");
	    use_usage(zc_type);
	  }
	  /*** pass along the argument ***/
	  newargv[new_i++] = argv[i];
	}
	else {
	  /*** library name is right after -l ***/
	  newargv[new_i++] = argv[i];
	}
	break;
      default:
	newargv[new_i++] = argv[i];
	break;
      }
    }
    else {
      /*** just pass the arguments along to zc0 ***/

      if (argv[i][strlen(argv[i])-2] == '.') {
	if (argv[i][strlen(argv[i])-1] == 'z') {
	  zfile_arg_no = new_i;

	  /*** get the basename of the zpl file ***/
	  ch = strrchr(argv[i], '/');
	  if (ch != NULL) {
	    /*** save the source directory for use with -I, -L ***/
	    src_dir_len = ch - argv[i] + 2;
	    src_dir = (char *)malloc(src_dir_len*sizeof(char));
	    strncpy(src_dir,argv[i],src_dir_len-1);
	    src_dir[src_dir_len-1] = '\0';
	    ch++;
	  }
	  else {
	    ch = argv[i];
	  }
	  in_basename = (char *) malloc((strlen(ch)-2+1)*sizeof(char));
	  strncpy(in_basename, ch, strlen(ch)-2);
	  in_basename[strlen(ch)-2] = '\0';
	}
	else if ((argv[i][strlen(argv[i])-1] == 'c') ||
		 (argv[i][strlen(argv[i])-1] == 'o')) {
	  /*** pass the .c and .o files along to zc ***/
	  /*** also, copy them into the output directory ***/
	  if (other_files == NULL) {
	    other_files = (char **) malloc(1*sizeof(char *));
	  }
	  else {
	    other_files = (char **) realloc(other_files,
					    (num_other+1)*sizeof(char *));
	  }
	  ch = (char *) malloc((strlen(argv[i])+1)*sizeof(char));
	  strcpy(ch, argv[i]);
	  other_files[num_other] = ch;
	  num_other++;
	}
      }

      newargv[new_i++] = argv[i];
    }
  }
 
  /*** check that ZPLHOME, ZPLTARGET, ZPLCOMMLAYER are set ***/
  zpl_home = getenv("ZPLHOME");
  if (zpl_home == NULL) {
    USR_FATAL(NULL,
	      "Environment variable ZPLHOME must be set to ZPL home directory.");
  }
  zpl_target = getenv("ZPLTARGET");
  if (zpl_target == NULL) {
    USR_FATAL(NULL,
	      "Environment variable ZPLTARGET must be set to target machine type.");
  }
  if (strcmp(zpl_target, "x86-cygwin") == 0) {
    exe_suffix = ".exe";
  } else {
    exe_suffix = "";
  }
  zpl_commlayer = getenv("ZPLCOMMLAYER");
  if (zpl_commlayer == NULL) {
    USR_FATAL(NULL,
	      "Environment variable ZPLCOMMLAYER must be set to communication layer type.");
  }

 /*** get info about how this binary was called (i.e. from zc or rzc) ***/
  full_zc0_name = zc0_name(argv[0]);

  if ((savec && !compatibility) && (outputdir == NULL)) {
    /*** can't save C code without specifying an output directory ***/
    printf("ERROR: An output directory must specified with the -nobuild or -savec flag\n");
    use_usage(zc_type);
  }

  if (zfile_arg_no == 0) {
    /*** no zpl file named ***/
    printf("ERROR: No .z file specified\n");
    use_usage(zc_type);
  }

  /*** Fill in default -I, -L paths ***/
  if (src_dir == NULL || src_dir[0] != '/') { /* need cwd in -I, -L */
    int new_path_len;

    new_path_len = PATH_MAX+1;
    if (src_dir != NULL) {
      new_path_len+=strlen(src_dir)+1;
    }
    new_path = (char *)malloc(new_path_len*sizeof(char));
    getcwd(new_path,PATH_MAX);
    if (src_dir != NULL) {
      strcat(new_path,"/");
      strcat(new_path,src_dir);
    }
  } else {
    new_path = src_dir;
  }
  newargv[1] = "-I";
  newargv[2] = (char *)malloc((strlen(new_path)+3)*sizeof(char));
  sprintf(newargv[2], "\"%s\"", new_path);
  newargv[3] = "-L";
  newargv[4] = (char *)malloc((strlen(new_path)+3)*sizeof(char));
  sprintf(newargv[4], "\"%s\"", new_path);

  /* add standard -I to cpp include path */
  if (include_path == NULL) {
    include_path = malloc((3+strlen(new_path)+1+2)*sizeof(char));
    include_path[0] = '\0';
  } else {
    include_path = realloc(include_path,(strlen(include_path)+1 +
					 3+strlen(new_path)+1+2)*sizeof(char));
    strcat(include_path," ");
  }
  strcat(include_path,"-I\"");
  strcat(include_path,new_path);
  strcat(include_path,"\"");

  /*** create a new process group so that kill does't kill parents */
  setpgid(0,0);  /* equivalent to setpgrp() */

  /*** register signal handler for the following signals ***/
  for (i = SIGHUP; i <= SIGTERM; i++) {
    /*** can't catch SIGKILL ***/
    if (i != SIGKILL) {
      signal(i, zc_signal_handler);
    }
  }

  /*** put temporary files in TMPDIR if the environment variable ***/
  /*** is defined, else put it in DEFAULT_TMPDIR ***/
  tmpdir = getenv("TMPDIR");
  if (tmpdir == NULL) {
    tmpdir = DEFAULT_TMPDIR;
  }

  if (cpp) {
    /**********************************************/
    /*** run the preprocessor on the input file ***/
    /**********************************************/

    cpp_filename = get_unique_name(tmpdir, "cpp.z");

    size = strlen(CPP)+1 + (cpp_flags ? strlen(cpp_flags)+1 : 2) +
           (include_path ? strlen(include_path)+1 : 2) +
	     strlen(newargv[zfile_arg_no])+1 + 2 + strlen(cpp_filename)+1;
    scratch = re_scratch(scratch, size);
    make_command(scratch, size, "%s %s %s %s > %s",
		 CPP, (cpp_flags ? cpp_flags : " "),
		 (include_path ? include_path: " "),
		 newargv[zfile_arg_no], cpp_filename);
    exec_command(scratch, "Error preprocessing '%s'", newargv[zfile_arg_no]);

    newargv[zfile_arg_no] = cpp_filename;
    if (out_basename == NULL) {
      /*** set the output file name (name of the final ***/
      /***  binary) to be that of the original .z file ***/
      newargv[new_i++] = "-o";
      newargv[new_i++] = in_basename;
    }

  }

  if (user_passfile) {
    /********************************/
    /*** run cpp on the pass file ***/
    /********************************/

    tmp_passfile = get_unique_name(tmpdir, "passfile");

    size = strlen(CPP)+1 + (cpp_flags ? strlen(cpp_flags)+1 : 1) +
            (include_path ? strlen(include_path)+1 : 1) +
            strlen(passfile)+1 + 2 + strlen(tmp_passfile)+1;
    scratch = re_scratch(scratch, size);
    make_command(scratch, size, "%s %s %s %s > %s",
		 CPP, (cpp_flags ? cpp_flags : ""),
		 (include_path ? include_path: ""),
		 passfile, tmp_passfile);
    exec_command(scratch, "Error preprocessing passfile '%s'", passfile);
    newargv[user_passfile] = tmp_passfile;
  }

  if (outputdir == NULL) {
    /*******************************************/
    /*** create a temporary output directory ***/
    /*******************************************/
    if (compatibility) {
      /*** create the base directory, if it doesn't exist ***/
      {
	char command[256];

	sprintf(command, "making directory %s", COMPAT_PATHBASE);
	print_command(command);
      }
      if (mkdir(COMPAT_PATHBASE, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)==-1) {
	switch (errno) {
	case EEXIST:
	  break;
	default:
	  USR_FATAL(NULL, "Cannot create directory '%s'", COMPAT_PATHBASE);
	}
      }

      in_dirname = in_basename;
      outputdir = (char *) malloc((strlen(COMPAT_PATHBASE)+1 +
				   strlen(in_dirname)+1)*sizeof(char));
      sprintf(outputdir, "%s/%s", COMPAT_PATHBASE, in_dirname);
    }
    else {
      in_dirname = get_unique_name(NULL, in_basename);

      outputdir = (char *) malloc((strlen(tmpdir)+1+
				   strlen(in_dirname)+1)*sizeof(char));

      sprintf(outputdir, "%s/%s", tmpdir, in_dirname);
    }

    /*** create the directory, if it doesn't exist ***/
    {
      char command[256];

      sprintf(command, "making directory %s", outputdir);
      print_command(command);
    }
    if (mkdir(outputdir,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)==-1) {
      switch (errno) {
      case EEXIST:
	break;
      default:
	USR_FATAL(NULL, "Cannot create directory '%s'", outputdir);
      }
    }

    newargv[new_i++] = "-d";
    newargv[new_i++] = outputdir;
  }

  /*** if the binary name is not specfied, use basename of input file ***/
  if (out_basename == NULL) {
    out_basename = in_basename;
  }

  /**********************************************************/
  /*** copy other .c files and libraries to the outputdir ***/
  /**********************************************************/
  for (i = 0; i < num_other; i++) {
    size =  (3*sizeof(char)+
	     strlen(other_files[i])+1*sizeof(char)+
	     strlen(outputdir)+1*sizeof(char));
    scratch = re_scratch(scratch, size);
    make_command(scratch, size, "cp %s %s", other_files[i], outputdir);
    exec_command(scratch, "Error copying '%s'", other_files[i]);

    ch = strrchr(other_files[i], '/');
    if (ch != NULL) {
      ch++;
    }
    else {
      ch = other_files[i];
    }

    /*** make sure we have write permission ***/
    size = 10 + strlen(outputdir)+1 + strlen(ch)+1;
    scratch = re_scratch(scratch, size);
    make_command(scratch, size, "chmod u+w %s/%s", outputdir, ch);
    exec_command(scratch, "Error changing permissions for '%s/%s'",
		 outputdir, ch);
  }

  /*** specify the zc0 binary to use ***/
  if (zc0_binary == '\0') {
    /*** not specified by the user, so use the default ***/
    zc0_binary = full_zc0_name;
  }
  newargv[0] = zc0_binary;

  /*** copy the zc command line to pass it along to zc0 ***/
  /*** using the -zccommand flag (IT MUST BE THE FIRST ***/
  /***  COMMAND LINE FLAG TO ZC0, IF IT IS USED AT ALL ***/
  zc_size = 6;
  for (i = 0; i < argc; i++) {
    zc_size += strlen(argv[i])+1;
  }
  zc_command = (char *) malloc((zc_size+1)*sizeof(char));
  if (debugzc0 && !valgrindzc0) {
    sprintf(zc_command, "\'\"%s ", argv[0]);
  } 
  else {
    sprintf(zc_command, "\"\\\"%s ", argv[0]);
  }
  for (i = 1; i < argc; i++) {
    strcat(zc_command, argv[i]);
    if (i != argc-1) {
      strcat(zc_command, " ");
    }
    else {
      if (debugzc0 && !valgrindzc0) {
	strcat(zc_command,"\"\'");
      }
      else {
	strcat(zc_command, "\\\"\"");
      }
    }
  }
  if (strlen(zc_command) >= zc_size) {
    fprintf(stderr, "Buffer overflow!  (unrecoverable) -- exiting.\n");
    clean_and_exit(1);
  }

  /*** allocate the buffer for the zc0 command line ***/
  zc0_size = strlen(zc0_binary)+1+10+1+strlen(zc_command)+1;
  for (i = 0; i < new_i; i++) {
    zc0_size += strlen(newargv[i])+1;
  }
  if (debugzc0 && valgrindzc0) {
    zc0_size += strlen(_VALGRINDGDB)+1;
  }
  else if (debugzc0) {
    zc0_size += strlen(_GDB)+1;
  }
  else if (valgrindzc0) {
    zc0_size += strlen(_VALGRIND)+1;
  }
  zc0_command = (char *) malloc((zc0_size+1)*sizeof(char));
  if (debugzc0 && valgrindzc0) {
    sprintf(zc0_command,"%s %s ",_VALGRINDGDB, zc0_binary);
  } else if (debugzc0) {
    sprintf(zc0_command,"%s %s ",_GDB, zc0_binary);
  } else if (valgrindzc0) {
    sprintf(zc0_command,"%s %s ",_VALGRIND, zc0_binary);
  } else {
    sprintf(zc0_command, "%s ", zc0_binary);
  }
  strcat(zc0_command, "-zccommand ");
  strcat(zc0_command, zc_command);
  strcat(zc0_command, " ");
  /*** copy the arguments to the command buffer ***/
  for (i = 1; i < new_i; i++) {
    strcat(zc0_command, newargv[i]);
    strcat(zc0_command, " ");
  }
  if (debugzc0 && !valgrindzc0) {
    /*** brad wrote this, and i don't understand, so ask him ***/
    debugzc0usage = zc0_command+strlen(_GDB)+strlen(zc0_binary)+1;
    *debugzc0usage = '\0';
    debugzc0usage++;
    printf("use: run %s\n",debugzc0usage);
    fflush(stdout);
  }
  if (strlen(zc0_command) >= zc0_size) {
    fprintf(stderr, "Buffer overflow in zc!  (unrecoverable) -- exiting.\n");
    clean_and_exit(1);
  }
  /****************/
  /*** call zc0 ***/
  /****************/
  exec_command(zc0_command, "ZPL compile failed");

  /************************/
  /*** build the binary ***/
  /************************/
  if (build) {
    if (print_commands == FALSE) {
      /*** get a tmp file for the zmake output ***/
      tmp_buildoutput = get_unique_name(tmpdir, "build");
      size = strlen(tmp_buildoutput);
    }
    else {
      size = 0;
    }
    size += (3+strlen(zmake_flags)+1+strlen(outputdir)+2+8+3+5+1)*sizeof(char);
    scratch = re_scratch(scratch, size);
    if (print_commands == FALSE) {
      make_command(scratch, size, "cd %s; ./zmake %s 1> %s 2>&1",
		   outputdir, zmake_flags, tmp_buildoutput);
    }
    else {
      make_command(scratch, size, "cd %s; ./zmake %s",
		   outputdir, zmake_flags);
    }
    build_error = TRUE;  /*** hack for clean_buildoutput() ***/
    exec_command(scratch, "Build failed");
    build_error = FALSE;

    /*** move the binary to the current directory ***/

    /*** do not move binaries if already exists in this directory ***/
    sprintf(small_buffer, "%s/%s", outputdir, out_basename);
    stat(small_buffer, &status0);
    sprintf(small_buffer, "./%s", out_basename);
    if ((stat(small_buffer, &status1) == -1) ||
	(memcmp(&status0, &status1, sizeof(struct stat)))) {
      /*** this file should exist upon successful compilation ***/
      size = (3+strlen(outputdir)+1+strlen(out_basename)+2+strlen(exe_suffix)+1);
      scratch = re_scratch(scratch, size);
      make_command(scratch, size, "mv %s/%s%s .", outputdir, out_basename,
		   exe_suffix);
      exec_command(scratch, "Move failed");

      /*** the _real file may or may not exist ***/
      sprintf(small_buffer, "%s/%s_real", outputdir, out_basename);
      if (stat(small_buffer, &status0) != -1) {
	size += 5;
	scratch = re_scratch(scratch, size);
	make_command(scratch, size, "mv %s/%s_real%s .",
		     outputdir, out_basename, exe_suffix);
	exec_command(scratch, "Move failed");
      }

      /*** the _dbg file may or may not exist ***/
      sprintf(small_buffer, "%s/%s_dbg", outputdir, out_basename);
      if (stat(small_buffer, &status0) != -1) {
	size += 4;
	scratch = re_scratch(scratch, size);
	make_command(scratch, size, "mv %s/%s_dbg%s .",
		     outputdir, out_basename, exe_suffix);
	exec_command(scratch, "Move failed");
      }
    }

    /*** don't remove the binaries because either ***/
    /*** we've already moved them, or they should be left ***/
    rm_binaries = FALSE;

  }

  clean_and_exit(0);

  return 0;
}

/*
 * FUNCTION: set_zc_type - Set the variable zc_type which tells zc how
 *                         it was called so that it can print out the
 *                         correct error string.
 * AUTHOR: sungeun
 * DATE:   7 January 1998
 *
 */
static usage_t
set_zc_type(char *str)
{
  char *zc_name, *ch;

  /*** copy the argument ***/
  zc_name = (char *) malloc(strlen(str)+1);
  strcpy(zc_name, str);

  ch = strrchr(zc_name, '/');
  if (ch != NULL) {
    ch++;
  }
  else {
    ch = zc_name;
  }

  if (strncasecmp("rzc", ch, 3) == 0) {
    /*** use rzc0 ***/
    zc_type = FROM_RZC;
  }
  else {
    /*** use zc0 ***/
    zc_type = FROM_ZC;
  }

  free(zc_name);

  return zc_type;
}

/* FUNCTION: zc0_name - return appropriate string name for zc0, based on
 *           argument string; if arument ends in "rzc" use _RZC0, otherwise
 *           use _ZC0;
 * AUTHOR:   echris
 * MODIFIED: sungeun (6 June 1997)
 *           Changed search for zc0 (or rzc0) binary.
 *                - Use path to zc (or rzc) if specified
 *                - otherwise, assume zc0 (or rzc0) will be found in
 *                  the path (hopefully in the * same place as zc or
 *                  rzc is).
 *           Also, set the zc_type (to FROM_ZC or FROM_RZC).
 *
 *           sungeun (9 June 1997)
 *           Change search for zc0 (or rzc0), AGAIN.
 *                - If path to zc (or rzc) is specified, check for zc0
 *                  (or rzc0) in the same path.  If it exists, use it.
 *                  Otherwise, treat as if no path was specified.
 *                - If no path was specified, add $ZPLHOME/bin/$ZPLTARGET
 *                  and $ZPLHOME/bin to the user's PATH and assume zc0
 *                  (or rzc0) will be in the PATH.
 *
 *           sungeun (7 January 1998)
 *           Moved the setting of zc_type to the routine set_zc_type()
 *            since it had to occur earlier.
 *
 * RETURNS:  Dynamically allocated charater string that contains the
 *           name of the binary to use.
 * */

static char *
zc0_name(char *str)
{
  int use_path;
  struct stat status;
  char *name, *ch;
  char *path, *newpath;

  /*** copy the argument ***/
  name = (char *) malloc(strlen(str)+max(strlen(_ZC0),strlen(_RZC0))+1);
  strcpy(name, str);

  ch = strrchr(name, '/');
  if (ch != NULL) {
    /*** use specified path ***/
    ch++;
    use_path = TRUE;
  }
  else {
    /*** no path specified, just use the name of the binary ***/
    ch = name;
    use_path = FALSE;
  }

  if (strncasecmp("rzc", ch, 3) == 0) {
    /*** use rzc0 ***/
    strcpy(ch, _RZC0);
  }
  else {
    /*** use zc0 ***/
    strcpy(ch, _ZC0);
  }

  if (use_path) {
    /*** check to see if zc0/rzc0 is in the given directory ***/
    if (stat(name, &status) != -1) {
      /*** the file exists ***/
      return name;
    }
    else {
      /*** assume it will be found in the search path ***/
      name = ch;  /*** notice that we waste some space, oh well! ***/
    }
  }

  /*** add $ZPLHOME/bin/$ZPLTARGET and $ZPLHOME/bin to the user's PATH ***/
  path = getenv("PATH");
  newpath = (char *) malloc((4+1+strlen(path)+1+
			     strlen(zpl_home)+1+3+1+strlen(zpl_target)+1+
			     strlen(zpl_home)+1+3+1)*sizeof(char));
  sprintf(newpath, "PATH=%s:%s/bin/%s:%s/bin",
	  path, zpl_home, zpl_target, zpl_home);
  /*** WARNING *** WARNING *** WARNING *** WARNING ***/
  /*** The putenv function is NOT ANSI, but seems  ***/
  /***  to exist on most UNIX platforms.  If there ***/
  /***  compile time error, check this line.       ***/
  putenv(newpath);

  return name;

}

/*
 * FUNCTION: re_scratch
 *
 * PURPOSE:  reallocate a buffer to be size
 *
 * RETURNS:  piece of memory of size
 *
 * */
static char *
re_scratch(char *scratch, int size)
{
  if (scratch == NULL) {
    scratch = (char *) malloc(size*sizeof(char));
  }
  else {
    scratch = (char *) realloc(scratch, size*sizeof(char));
  }
  return scratch;
}

/*
 * FUNCTION: make_command
 *
 * PURPOSE:  copy the command into the provided buffer and check for
 *           overflow.
 *
 * */
static void
make_command(char *buffer, int size, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  if (strlen(buffer) >= size) {
    /***
    fprintf(stderr, "%s\n", buffer);
    fprintf(stderr, "size=%d, strlen=%d\n", size, strlen(buffer));
    ***/
    fprintf(stderr, "Buffer overflow!  (unrecoverable) -- exiting.\n");
    clean_and_exit(1);
  }
}

/*
 * FUNCTION: exec_command
 *
 * PURPOSE:  execute the command in the buffer using system().
 *           print out message and exiti (clean up also ) if there is
 *           an error.
 *
 * */
static void
exec_command(const char *command, const char *format, ...)
{
  int exit_status;
  va_list args;

  print_command(command);
  exit_status = system(command);
  if (exit_status) {

    if (build_error && WEXITSTATUS(exit_status) == 2 && print_commands==FALSE) {
      char command[256];

      sprintf(command, "cat %s", tmp_buildoutput);
      print_command(command);
      system(command);
    }

    if (print_commands == TRUE) {
      fprintf(stderr, "\n");
    }
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, " -- exiting.\n");

    if (strcmp(format, "ZPL compile failed") != 0) {
      if (print_commands == FALSE) {
	fprintf(stderr," * Try compiling with the -v flag for more information.\n");
      }
      if (getenv("ZMAKEVERBOSE") == NULL ||
	  strcmp(getenv("ZMAKEVERBOSE"),"yes") != 0) {
	fprintf(stderr, " * You might ");
	if (print_commands == FALSE) {
	  fprintf(stderr, "also ");
	}
	fprintf(stderr,
		"try setting the ZMAKEVERBOSE environment variable to\n"
		"   'yes' for further details.\n");
      }
      fprintf(stderr, 
	      " * Please contact zpl-info@cs.washington.edu if you are stuck or believe\n"
              "   that you've found a bug.\n");
    }
    clean_and_exit(1);
  }

}

static void
zc_signal_handler(int sig)
{
  switch (sig) {
  case SIGSEGV:
    fprintf(stderr, "Segmentation violation in ZPL compiler!\n"
	    "Please contact zpl-info@cs.washington.edu.\n");
    break;
  case SIGBUS:
    fprintf(stderr, "Bus error in ZPL compiler!\n"
	    "Please contact zpl-info@cs.washington.edu.\n");
    break;
  default:
    fprintf(stderr, "Signal %d caught -- exiting.\n", sig);
    break;
  }
  kill(0,SIGTERM);      /* termitate subprocess */
  clean_and_exit(sig);
}

static void
clean_and_exit(int exit_status)
{

  clean_cpp();
  clean_passfile();
  clean_buildoutput();

  if ((compatibility == FALSE) && /*** not "compatibility" AND ***/
      ((savec == FALSE))) {        /*** don't save the C code OR ***/
    clean_outputdir();
  }

  exit(exit_status);

}

static void
clean_cpp()
{
  char command[256];

  if (cpp) {
    sprintf(command, "rm -f %s", cpp_filename);

    print_command(command);
    system(command);
  }

}

static void
clean_passfile()
{
  char command[256];

  if (user_passfile) {
    sprintf(command, "rm -f %s", tmp_passfile);

    print_command(command);
    system(command);
  }

}

static void
clean_buildoutput()
{
  char command[256];

  if (tmp_buildoutput != NULL) {
    /*** remove the file ***/
    sprintf(command, "rm -f %s", tmp_buildoutput);
    print_command(command);
    system(command);
  }
}

static void
clean_outputdir()
{
  int i;
  char command[256];
  char *ch;

  if ((outputdir != NULL) && (out_basename != NULL)) {
    if (user_outputdir == TRUE) {
      /*** just remove the code in the directory ***/
      if (rm_binaries == TRUE) {
	sprintf(command, "rm -f %s/%s", outputdir, out_basename);
	print_command(command);
	system(command);
	sprintf(command, "rm -f %s/%s_real", outputdir, out_basename);
	print_command(command);
	system(command);
	sprintf(command, "rm -f %s/%s_dbg", outputdir, out_basename);
	print_command(command);
	system(command);
      }
      sprintf(command, "rm -f %s/%s.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_acc.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_cat.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_cat.o", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_cfg.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_cfg2.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_cfg3.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_arr.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_sps.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_reg.c", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_reg.h", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/%s_usr.h", outputdir, out_basename);
      print_command(command);
      system(command);
      sprintf(command, "rm -f %s/zmake", outputdir);
      print_command(command);
      system(command);

      if (num_other != 0) {
	/*** remove the user specified files that were copied here ***/
	for (i = 0; i < num_other; i++) {
	  ch = strrchr(other_files[i], '/');
	  if (ch != NULL) {
	    ch++;
	  }
	else {
	  ch = other_files[i];
	}
	  sprintf(command, "rm -f %s/%s", outputdir, ch);
	  print_command(command);
	  system(command);

	  if (ch[strlen(ch)-1] == 'c') {
	    ch[strlen(ch)-1] = 'o';
	    sprintf(command, "rm -f %s/%s", outputdir, ch);
	    print_command(command);
	    system(command);
	  }
	}
      }
    }
    else {
      /*** zc created directory.. remove the entire directory ***/
      sprintf(command, "rm -rf %s", outputdir);
      print_command(command);
      system(command);
    }
  }
}

/*** get a unique name (given path and "tail) based on pid and uid ***/
/*** both path and tail may be NULL ***/
static char *
get_unique_name(char *path, char *tail)
{
  char *name;
  int length;
  pid_t pid;
  uid_t uid;

  length = (path ? strlen(path) : strlen(".")) + 1
    + 3 + 10 + 10 + (tail ? strlen(tail) : strlen("pooh")) + 1;
  name = (char *) malloc(length*sizeof(char));

  pid = getpid();
  uid = geteuid();

  sprintf(name, "%s/zc-%ld-%ld-%s", (path ? path : "."),
	  (long)pid, (long)uid, (tail ? tail : "pooh"));

  if (length < strlen(name)+1) {
    fprintf(stderr, "Buffer overflow (unrecoverable)!\n");
    exit(1);
  }

  return(name);
}

static void
print_command(const char *s)
{
  if (print_commands) {
    printf("\n%s\n", s);
  }
}


