/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** file:          passlist.h
 *** description:   This file contains the default list of the compiler
 ****               passes to be run during compilation
 *** author:        Sung-Eun Choi
 *** date:          19 January 1996
 *** modifications:
 ***
 *** PASS LIST format (for parsing as an external pass file):
 ***    The first item in the list must be the macro FIRST
 ***    The last item in the list must be the macro LAST
 ***    Each pass name and associated flags are enclosed in a single
 ***     set of double quotes (e.g. "my_pass -d 1")
 ***    Each line may only contain the name of a single pass
 ***    Each line may not exceed 80 characters
 ***
 *** IMPORTANT NOTE = if you change this list, change all passlists in
 ***                  the test directory structure.
 ***
 ***/

#ifndef __PASSLIST_H_
#define __PASSLIST_H_

#define FIRST "first_pass"
#define LAST "last_pass"

static char passlist[128][80] = {
  FIRST,
  "fixup",
  "typeinfo",
  "callgraph",
  "cganal",
  "paranal",
  "temps_scan",
  "fixup",
  "typeinfo",
  "temps_ensparam",
  "fixup",
  "typeinfo",
  "temps_function",
  "fixup",
  "fixup",
  "typeinfo",
  "insert_nloops",
  "typeinfo",
  "typecheck",
  "fixup",
  "aggregate_hierarchy_assignments",
  "fixup",
  "typeinfo",
  "fixup",
  "callgraph",
  "cganal",
  "fixup",
  "covanal",
  "callanal",
  "strength",
  "fixup",
  "fixup",
  "callgraph",
  "paranal",
  "typeinfo",
  "temps_cda",
  "fixup",
  "typeinfo",
  "fluff",
  "return_ens",
  "fixup",
  "depend_array",
  "live",
  "perm2mloops",
  "red2mloops",
  "fixup",
  "typeinfo",
  "insert_nloops",
  "typeinfo",
  "fixup",
  "insert_mloops",
  "fixup",
  "scope",
  "depend_array",
  "depgraph",
  "fixup",
  "fixup",
  "contraction",
  "fixup",
  "depend_array",
  "fixup",
  "constrainmloops",
  "floodtile",
  "floodopt",
  "stencil",
  "fixup",
  "depend_array",
  "live",
  "insert_checkpoints",
  "fixup",
  "depend_array",
  "insert_comm",
  "insert_comm -L",
  "fixup",
  "depend_array",
  "depgraph",
  "permcomm",
  "fixup",
  "typeinfo",
  "setcid",
  "fixup",
  "pr_commstats",
  "fixup",
  "mscan",
  "constrainmloops",
  "fixup",
  "runtime_checks",
  "fixup",
  "copts",
  "fixup",
  "typeinfo",
  "fixup",
  "checkpoint",
  "fixup",
  "codegen",
  LAST,
};

#endif
