/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** FILE:            Profgen.c
 *** AUTHOR:          Sung-Eun Choi (sungeun@cs.washington.edu)
 *** DESCRIPTION:     Generate code for profiling
 ***
 ***
 ***/

#include <stdio.h>
#include "../include/Cgen.h"
#include "../include/Privgen.h"
#include "../include/Profgen.h"
#include "../include/error.h"
#include "../include/global.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"

static void GenCommstatInit(FILE *);

/*** generate any initialization necessary (globals, macros, etc..) ***/

void GenProfInit(FILE *f) {
  fprintf(f, "\n\n");

  GenCommstatInit(f);

  fprintf(f, "\n\n");
}


/*** output macros for timing communication or computation ***/

static void GenCommstatInit(FILE *f) {
  if (commstats || compstats) {
    priv_decl_begin(f);
    fprintf(f, "static, double, _PRtime");
    priv_decl_end(f);

    if (commstats) {
      fprintf(f, "#define START_COMM {ResetTimer();}\n");
      fprintf(f, "#define END_COMM {");
      priv_named_access(f, "_PRtime");
      fprintf(f, "+= CheckTimer();}\n");
    }
    else {
      fprintf(f, "#define START_COMM {");
      priv_named_access(f, "_PRtime");
      fprintf(f, "+= CheckTimer();}\n");
      fprintf(f, "#define END_COMM {ResetTimer();}\n");
    }

    fprintf(f, "#define START_COMM_TIMING _start_timing()\n");
    fprintf(f, "static void _start_timing()\n");
    fprintf(f, "{\n");
    priv_tid_decl(f);
    priv_alloc_begin(f);
    fprintf(f, "double, _PRtime");
    priv_alloc_end(f);
    priv_named_access(f, "_PRtime");
    fprintf(f, "= 0L;\n");
    fprintf(f, "ResetTimer();\n");
    fprintf(f, "}\n");

    fprintf(f, "#define END_COMM_TIMING _end_timing()\n");
    fprintf(f, "static void _end_timing()\n");
    fprintf(f, "{\n");
    fprintf(f, "}\n");

    fprintf(f, "#define PRINT_COMM_TIMING _print_timing()\n");
    fprintf(f, "static void _print_timing()\n");
    fprintf(f, "{\n");
    fprintf(f, "int i;\n");
    priv_tid_decl(f);
    fprintf(f, "for (i = 0; i < _PROCESSORS; i++) {\n");
    fprintf(f, "if (i == _INDEX) {\n");
    fprintf(f, "fprintf(stdout, \"[%%d] time = %%.6f seconds\\n\", _INDEX, ");
    priv_named_access(f, "_PRtime");
    fprintf(f, ");\n");
    fprintf(f, "}\n");
    fprintf(f, "_BarrierSynch();\n");
    fprintf(f, "}\n");
    fprintf(f, "}\n");
  }
}

void gen_zprof(FILE *f, statement_t *stmt) {
  switch (T_ZPROFTYPE(stmt)) {
  case ZPROF_COMMSTAT:
    switch (T_COMMSTAT(stmt)) {
    case START_COMM_TIMING:
      fprintf(f, "START_COMM_TIMING;\n");
      break;
    case START_COMM:
      fprintf(f, "START_COMM;\n");
      break;
    case END_COMM:
      fprintf(f, "END_COMM;\n");
      break;
    case END_COMM_TIMING:
      fprintf(f, "END_COMM_TIMING;\n");
      break;
    case PRINT_COMM_TIMING:
      fprintf(f, "PRINT_COMM_TIMING;\n");
      break;
    }
    break;
  default:
    INT_FATAL(NULL, "Bad zproftype.");
  }
}

