/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** file:          passes.h
 *** description:   This file contains the list of compiler passes
 *** author:        Sung-Eun Choi
 *** date:          19 January 1996
 *** modifications:
 ***
 *** NOTES:
 ***    For an example of a pass that traverses the AST, see
 ***     r2mloops/r2mloops.c or comm/comm.c.
 ***
 *** HOW TO ADD A NEW PASS TO THE ZPL COMPILER:
 ***    Check out the following files:
 ***       /projects/orca3/zpl/src/include/passes.h
 ***       /projects/orca3/zpl/src/include/passlist.h
 ***
 ***    Create a function called "call_mypass" in the file where the
 ***    optimization will be placed.  This function calls your pass
 ***    with two argument: the module being compiled (module_t *) and
 ***    the arguments (char *.)  Add the prototype to this file below.
 ***
 ***    Add your pass to the passes[] data structure below.  You need
 ***    to add an entry composed of the following: the name for the
 ***    pass (char *), the name of the function created above, and a
 ***    short description (char *.)  Please keep it alphabetical and
 ***    name the passes intelligently -- e.g. name related passes
 ***    similarly (temp insertion passes start with the word "temps".)
 ***
 ***    Call your pass in the passlist array (in passlist.h)
 ***
 ***    In addition, you will need to recompile all of main, in
 ***    particular main/runpasses.c.
 ***
 ***/

#ifndef __PASSES_H_
#define __PASSES_H_

#include "../include/struct.h"

typedef struct pass_t {
  char *name;
  int (*fun)(module_t *, char *);
  char *description;
} pass_t;

extern int call_a2nloops(module_t *, char *);
extern int call_callanal(module_t *,char *);
extern int call_callgraph(module_t *, char *);
extern int call_cda_temps(module_t *, char *);
extern int call_cganal(module_t *, char *);
extern int call_live(module_t *, char *);
extern int call_insert_checkpoints(module_t *, char *);
extern int call_checkpoint(module_t *, char *);
extern int call_codegen(module_t *, char *);
extern int call_commdep(module_t *, char *);
extern int call_commstats(module_t *, char *);
extern int call_constrainmloops(module_t *, char *);
extern int call_contraction(module_t *, char *);
extern int call_copts(module_t *, char *);
extern int call_debug_codegen(module_t *, char *);
extern int call_dep(module_t *, char *);
extern int call_depend(module_t *, char *);
extern int call_ensparam(module_t *, char *);
extern int call_fixup(module_t *, char *);
extern int call_floodopt(module_t *, char *);
extern int call_floodtile(module_t *, char *);
extern int call_fluff(module_t *,char *);
extern int call_function_temps(module_t *, char *);
extern int call_hierarchy_aggregate(module_t*, char*);
extern int call_insert_comm(module_t *, char *);
extern int call_mscan(module_t *, char *);
extern int call_paranal(module_t *, char *);
extern int call_perm2mloops(module_t *, char *);
extern int call_permcomm(module_t *, char *);
extern int call_r2mloops(module_t *, char *);
extern int call_red2mloops(module_t*,char*);
extern int call_rank_res(module_t *, char *);
extern int call_return_ensembles(module_t *, char *);
extern int call_runtime_checks(module_t *, char *);
extern int call_scan_temps(module_t *, char *);
extern int call_scope(module_t *, char *);
extern int call_setcid(module_t *,char *);
extern int call_stencil(module_t *, char *);
extern int call_strength(module_t *, char *);
extern int call_test_scale(module_t*, char*);
extern int call_typecheck(module_t *, char *);
extern int call_typeinfo(module_t *, char *);
extern int call_uve(module_t *, char *);

#if (!RELEASE_VER)
extern int call_mem_usage(module_t *, char *);
#endif

#ifdef __PASSES_C_

#define NO_MORE_PASSES ""

pass_t passes[] = {

  {"aggregate_hierarchy_assignments", call_hierarchy_aggregate, "Aggregate hierarchy assignment statements"},
  {"callanal", call_callanal, "Analyze function calls"},
  {"callgraph", call_callgraph, "Build the call graph"},
  {"cganal", call_cganal, "Identify call graph node positions"},
  {"checkpoint", call_checkpoint, "Add checkpointing to code"},
  {"codegen", call_codegen, "Generate C code"},
  {"constrainmloops", call_constrainmloops, "Set mloop iteration order according to constraints"},
  {"contraction", call_contraction, "Array contraction"},
  {"copts", call_copts, "Low-level C optimizations"},
  {"covanal", call_rank_res, "Determine region coverage"},
  {"depend_array", call_commdep, "Generate specialized in/out set for array variables"},
  {"depgraph", call_dep, "Build data dependence graph"},
  {"fixup", call_fixup, "Fixup the AST"},
  {"floodopt", call_floodopt, "Replace expressions floodable in inner dimension"},
  {"floodtile", call_floodtile, "Decide whether to tile based on data flooding"},
  {"fluff", call_fluff, "Allocate fluff for array variables"},
  {"insert_comm", call_insert_comm, "Insert communication"},
  {"insert_checkpoints", call_insert_checkpoints, "Insert checkpoints"},
  {"insert_mloops", call_r2mloops, "Insert mloops"},
  {"insert_nloops", call_a2nloops, "Insert nloops"},
  {"live", call_live, "Live variable analysis"},
  {"mscan", call_mscan, "Pipeline wavefronts (e.g., primed @s)"},
  {"paranal", call_paranal, "Identify parallel functions"},
  {"perm2mloops", call_perm2mloops, "Convert permutations to mloops"},
  {"permcomm", call_permcomm, "Separate permutation communication to overlap with computation"},
  {"pr_commstats", call_commstats, "Insert communication profiling information"},
  {"red2mloops", call_red2mloops, "Convert reductions to MLOOPs"},
  {"return_ens", call_return_ensembles, "Optimize arrays returned from procedures"},
  {"runtime_checks", call_runtime_checks, "Insert runtime checks, like bounds checks"},
  {"scope", call_scope, "Per statement scope resolution"},
  {"setcid", call_setcid, "Sets up Ironman Communication IDs"},
  {"stencil", call_stencil, "Stencil redundancy elimination"},
  {"strength", call_strength, "Strength reduction"},
  {"temps_cda", call_cda_temps, "Insert temps for lhs-rhs aliasing"},
  {"temps_ensparam", call_ensparam, "Insert temps for array parameters"},
  {"temps_function", call_function_temps, "Insert temps for function calls"},
  {"temps_scan", call_scan_temps, "Insert temps for reductions and scans"},
  {"typecheck", call_typecheck, "Perform typechecking"},
  {"typeinfo", call_typeinfo, "Determine type information for expressions"},
  {"deadvarelim", call_uve, "Unused variable elimination"},
#if (!RELEASE_VER)
  {"debug_codegen", call_debug_codegen, "Debug code generation"},
  {"mem_usage", call_mem_usage, "Report compiler memory usage"},
  {"testscale", call_test_scale, "Prints out scale information for all BIATs"},
#endif
  {NO_MORE_PASSES, NULL, ""}
};

#else

extern int run_pass(module_t *, char *, FILE *);
extern pass_t passes[];

#endif
#endif
