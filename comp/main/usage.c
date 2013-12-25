/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


/***
 ***
 *** FILE: usage.c
 *** AUTHOR: Sung-Eun Choi (sungeun@cs.washington.edu)
 ***
 *** DESCRIPTION: Print out usage information for zc or zc0.
 ***
 *** NOTES: This file is shared by both zc (rzc) and zc0.  Use the
 ***        argument to usage to get the proper message.  See usage.h.
 ***
 ***        New flags should always be non-release (internal).
 ***
 ***        Put #if's around internal flags.  Also, prepend their
 ***        description with a "*."  See below for examples.
 ***
 ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/usage.h"
#include "../include/version.h"

void
use_usage(usage_t type)
{

  switch(type) {
  case FROM_ZC:
    printf("Use 'zc -h' ");
    break;
  case FROM_RZC:
    printf("Use 'rzc -h' ");
    break;
  case FROM_ZC0:
    printf("Use 'zc0 -h' ");
    break;
  case FROM_RZC0:
    printf("Use 'rzc0 -h' ");
    break;
  }
  printf("for help\n");

  exit(1);
}

void
usage(usage_t type)
{
  switch(type) {
  case FROM_ZC:
    printf("Usage: zc  ");
    break;
  case FROM_RZC:
    printf("Usage: rzc ");
    break;
  case FROM_ZC0:
    printf("Usage: zc0 ");
    break;
  case FROM_RZC0:
    printf("Usage: rzc0");
    break;
  }

  if (type == FROM_ZC || type == FROM_RZC) {
    printf(           " [-cpp] [-d <directory>] [-Dname|-Dname=val]\n"
	   "            [-fusion[0|1|2|3]] [-g] [-h] [-helppar] [-I <path>] \n"
	   "            [-l <lib>] [-L <path>] [-noaccessopt]\n"
	   "            [-nobuild] [-nocheck] [-nocommopt] [-noconstopt]\n"
	   "            [-nocontract] [-nofloodopt] [-noregopt]\n"
           "            [-nopermopts] [-norestrict]\n");
    printf("            [-nostrength] [-notile] [-o <filename>]\n"
	   "            [-O[0|1]] [-safe] [-savec] [-stencil=[none|vector\n"
           "             |scalar|unroll] [-Uname] [-v] [-verbose] [-V]\n"
           "            [<C files>] [<object files>] [<header files>]\n"
           "            <ZPL file>\n");
#if (!RELEASE_VER)
/*********************************************/
/*** PUT NON-RELEASE (INTERNAL) FLAGS HERE ***/
/*********************************************/
    printf("Usage: zc   [-compat] [-cmo]\n"
           "            [-dbgzc0] [-dbgparser] [-mloop_order=xyz]\n"
           "            [-mloop_up=xyz] [-mloop_unroll=xyz] [-mloop_tile=xyz]\n"
           "            [-mloop_tile_order=xyz] [-mloop_tile_up=xyz]\n"
	   "            [-mscanpeel] [-nobytewalkers] [-nocpplines] [-noprivate]\n"
	   "            [-nopermmapshare] [-nopermmapsave] [-nopermoverlap]\n"
	   "            [-nosparsememopt] [-nopermsavedataspace] [-nopermprocmap=0,1]\n"
	   "            [-nopermrleprocmap]\n");
    printf("            [-nopermrelindices] [-nopermloopdecode] [-p <passfile>]\n"
	   "            [-q[0|1]] [-report='opt'] [-sparse=[access|link]]\n"
	   "            [-stencil_adds=<double> -stencil_mems=<double>]\n"
	   "            [-stencil_muls=<double> -stencil_regs=<double>]\n"
	   "            [-stencil_bbar=<double> -stencil_debug] [-tokenize]\n"
           "            [-valgrind] [-zc0 <zc0 binary>]\n");
#endif
  }

  if (type == FROM_ZC0 || type == FROM_RZC0) {
    printf(           " [-d <directory>] [-fusion[0|1|2|3]] [-g] [-h]\n"
	   "            [-I <path>] [-l <lib>] [-L <path>] [-noaccessopt]\n"
	   "            [-nobuild] [-nocheck] [-nocommopt] [-noconstopt]\n"
           "            [-nocontract] [-nostrength] [-O[0|1]]\n"
	   "            [-strict] [-v] [-verbose] [-V]\n"
	   "            [<C files>] [<object files>] [<header files>]\n"
           "            <ZPL file>\n");
#if (!RELEASE_VER)
/*********************************************/
/*** PUT NON-RELEASE (INTERNAL) FLAGS HERE ***/
/*********************************************/
    printf("Usage: zc0  [-noprivate] [-p <passfile>] [-q[0|1]] [-zccommand <command>]\n");
#endif
  }
  printf("\n");

  printf("FLAGS\n");
  printf("  -autocheckpoint    Automatically checkpoint ZPL program (experimental)\n");
  printf("  -checkpoint        Checkpoint program from user inserted checkpoints (experimental)\n");
#if (!RELEASE_VER)
  if (type == FROM_ZC || type == FROM_RZC) {
    printf("* -compat        Compatibility with old compiler interface.\n");
  }
  printf("* -cmo           Use column-major-allocation for arrays.\n");
#endif
  if (type == FROM_ZC || type == FROM_RZC) {
    printf("  -cpp           Use cpp to preprocess the input ZPL file.\n");
  }
  printf("  -d             Specify the directory in which to place generated C code.\n");
#if (!RELEASE_VER)
  if (type == FROM_ZC || type == FROM_RZC) {
    printf("* -dbgzc0        Run zc0 in a gdb session.\n");
    printf("* -dbgparser     Dump parser debug information.\n");
  }
#endif
  if (type == FROM_ZC || type == FROM_RZC) {
    printf("  -D             Preprocessor flag (passed directly to CPP).\n");
  }
  printf("  -fusion        Enable array loop fusion, same as -fusion3.\n");
  printf("  -fusion0       Disable compiler temp fusion.\n");
  printf("  -fusion1       Enable fusion for compiler temporaries.\n");
  printf("  -fusion2       1 + Enable fusion for contraction.\n");
  printf("  -fusion3       2 + Enable fusion for locality.\n");

  printf("  -g             Generate debug symbols, including ZGUARD support (out-of-date).\n");
  printf("  -h             Display this message.\n");
  printf("  -helppar       Display why functions are parallel.\n");
  printf("  -I             Add path to search for #include files.\n");
  printf("  -l             Link library during object code compilation.\n");
  printf("  -L             Add path to search for libraries.\n");
#if (!RELEASE_VER)
  printf("* -mloop_order=xyz  Sets default MLOOP iteration order.\n");
  printf("* -mloop_up=xyz     Sets default MLOOP iteration direction.\n");
  printf("* -mloop_unroll=xyz Sets default MLOOP unrolling.\n");
  printf("* -mloop_tile=xyz   Sets default MLOOP tile sizes.\n");
  printf("* -mloop_order=xyz  Sets default MLOOP tile iteration order.\n");
  printf("* -mloop_up=xyz     Sets default MLOOP tile iteration direction.\n");
  printf("* -mscanpeel     Enable mscan loop peeling (faster compile).\n");
#endif
  printf("  -noaccessopt   Disable efficient array access.\n");
  if (type == FROM_ZC || type == FROM_RZC) {
    printf("  -nobuild       Disable the building of the binary.\n");
  }
  printf("  -nocheck       Disable typechecking passes.\n");
  printf("  -nocommopt     Use naive communication insertion algorithm.\n");
  printf("  -noconstopt    Disable evaluation of constant initializers.\n");
  printf("  -nocontract    Disable array contraction.\n");
  printf("  -nocopts       Disable low-level C optimizations.\n");
#if (!RELEASE_VER)
  printf("* -nobytewalkers Declare walkers using <type>* rather than char*\n");
  printf("* -nocpplines    Disable printing of cpp line number information.\n");
  printf("* -noprivate     Disable private access macros.\n");
#endif
  printf("  -nopermopts    Disable permutation optimizations.\n");
#if (!RELEASE_VER)
  printf("* -nopermprocmap=0,1  Use permute procmap or not.\n");
  printf("* -nopermmapshare     Disable permute map sharing.\n");
  printf("* -nopermmapsave      Disable permute map saving.\n");
  printf("* -nopermoverlap      Disable permute comm/comp overlap.\n");
  printf("* -nopermrleprocmap   Disable permute procmap run length encoding.\n");
  printf("* -nopermrleindices   Disable permute indices run length encoding.\n");
  printf("* -nopermloopdecode   Disable permute loop decoding of rle indices.\n");
  printf("* -nopermsavedataspace   Disable permute use dest/src as data buckets.\n");
  printf("* -nopermdirect          Disable permute send data directly from dest to src.\n");
#endif
  printf("  -nofloodopt    Disable hoisting of flood expressions.\n");
  printf("  -noregopt      Disable region loop dimension reordering.\n");
  printf("  -norestrict    Disable use of the restrict keyword.\n");
  printf("  -nostrength    Disable strength reduction optimizations.\n");
#if (!RELEASE_VER)
  printf("* -nosparsememopt Disable optimization of sparse region storage\n");
#endif
  printf("  -notile        Disable tiling of region loops.\n");

  if (type == FROM_ZC || type == FROM_RZC) {
    printf("  -o             Specify output file name.\n");
  }
  printf("  -O             Compile with full optimizations, same as -O1.\n");
  printf("  -O0            Compile with no optimizations.\n");
  printf("  -O1            Compile with full optimizations. (default)\n");
  printf("                  - optimized array accesses\n");
  printf("                  - optimized communication generation\n");
  printf("                  - evaluate constant initializers\n");
  printf("                  - array loop fusion (-fusion3)\n");
  printf("                  - array contraction\n");
  printf("                  - strength reduction\n");
  printf("                  - flood and tiling optimizations\n");
  printf("                  - loop ordering optimizations\n");
  printf("                  - stencil optimizations\n");

#if (!RELEASE_VER)
  printf("* -p <pass file> Specify pass file <pass file> to be used by the compiler.\n");
  printf("* -Pcommstats    Profile time spent performing Ironman communication.\n");
  printf("* -Pcompstats    Profile time spent performing computation.\n");
  printf("*                NOTE: Only one of the above two flags may be used.\n");

  printf("* -report        Return info on compiler progress, good for regression tests.\n");
#endif

  printf("  -safe          Compile with runtime checks like bounds checking (not implemented).\n");
  if (type == FROM_ZC || type == FROM_RZC) {
    printf("  -savec         Save the generated C code.  An output directory\n");
    printf("                 (using -d) must be specified with this flag.\n");
  }
  printf("  -seqonly        Optimize C for sequential use ONLY (experimental).\n");

#if (!RELEASE_VER)
  printf("* -sparse=access  Use dense access for sparse region iteration.\n");
  printf("* -sparse=link    Use link structure for sparse region iteration.\n");
#endif

  printf("  -stencil        Enable unroll stencil optimization.\n");
  printf("  -stencil=none   Disable stencil optimization.\n");
  printf("  -stencil=unroll Enable unroll stencil optimization.\n");
  printf("  -stencil=scalar Enable scalar stencil optimization.\n");
  printf("  -stencil=vector Enable vector stencil optimization (not implemented).\n");
#if (!RELEASE_VER)
  printf("* -stencil_adds=<double>  Benefit of eliminating an add.\n");
  printf("* -stencil_mems=<double>  Benefit of eliminating a mem ref.\n");
  printf("* -stencil_muls=<double>  Benefit of eliminating a multiply.\n");
  printf("* -stencil_regs=<double>  Benefit of using a register.\n");
  printf("* -stencil_bbar=<double>  Benefit threshold bar.\n");
#endif
  if (type == FROM_ZC0 || type == FROM_RZC0) {
    printf("  -strict        Employ strict typechecking.\n");
  }
#if (!RELEASE_VER)
  printf("* -tokenize      Show the program's lexical tokens and exit.\n");
#endif
  if (type == FROM_ZC || type == FROM_RZC) {
    printf("  -U             Preprocessor flag (passed directly to CPP).\n");
  }
  if (type == FROM_ZC || type == FROM_RZC) {
    printf("  -v             Print out passes as they execute.\n");
  }
  printf("  -verbose       Verbose error reporting.\n");
  printf("  -V             Display version information and quits.\n");
#if (!RELEASE_VER)
  printf("* -q             Run in quiet mode, same as -q0.\n");
  printf("* -q0            Do not print pass information (default).\n");
  printf("* -q1            Print pass information.\n");
  printf("* -valgrind      Run zc0 under valgrind.\n");

  if (type == FROM_ZC || type == FROM_RZC) {
    printf("* -zc0           Specify the zc0 binary to use.\n");
  }
  if (type == FROM_ZC0 || type == FROM_RZC0) {
    printf("* -zccommand     Specify the zc command line used to call zc.\n");
    printf("                 This must be the FIRST argument to zc0 if it\n");
    printf("                 is used.\n");
  }

  printf("\n* Non-release (internal) flags.\n");
#endif
  printf("\n");
  exit(1);
}


void print_version() {
  printf("zc ZPL Compiler Version ");
  version_fprint(stdout);
  printf("\n");
  printf(COPYRIGHT);
  printf("\n");
  printf(BANNER);
  printf("\n");
}


