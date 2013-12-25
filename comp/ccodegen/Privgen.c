/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/symboltable.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/treemac.h"

/***
 *** priv_access_begin(FILE *)
 *** priv_access_end(FILE *)
 ***
 *** begin and end private access in compiler output (to file)
 ***
 *** generate variable name between begin and end
 ***/

void priv_access_begin(FILE *outfile) {
  if (priv_access) {
    fprintf(outfile, "_PRIV_ACCESS(");
  }
}

void priv_access_end(FILE *outfile) {
  if (priv_access) {
    fprintf(outfile, ")");
  }
}

/***
 *** priv_named_access(FILE *, char *)
 ***
 *** private access in compiler output (to file) for named string
 ***
 *** generate variable name between begin and end
 ***/

void priv_named_access(FILE *outfile, char *name) {
  if (priv_access) {
    fprintf(outfile, "_PRIV_ACCESS(%s)", name);
  } else {
    fprintf(outfile, "%s", name);
  }
}

/***
 *** priv_decl_begin(FILE *)
 *** priv_decl_end(FILE *)
 ***
 *** begin and end private declaration in compiler output (to file)
 ***
 *** generate extern or static (or blank), type, and variable name
 ***  between begin and end (separated by a comma)
 ***/

void priv_decl_begin(FILE *outfile) {
  fprintf(outfile, "_PRIV_DECL(");
}

void priv_decl_end(FILE *outfile) {
  fprintf(outfile, ");\n");
}


/***
 *** priv_reg_access(symboltable_t *)
 ***
 *** access the region
 ***/

void priv_reg_access(FILE *outfile, expr_t *reg) {
  gen_name(outfile, reg);
  return;

  if (D_CLASS(T_TYPEINFO(reg)) != DT_REGION) {
    INT_FATAL(NULL, "reg is not a region.");
  }

  if (expr_is_qreg(reg)) {
    /*** SERIOUS HACK ***/
    fprintf(outfile, "%s.%s", RMS, RMS_REG);
  } else if (expr_dyn_reg(reg) || expr_contains_qreg(reg) || !priv_access) {
    /*** dynamic regions are by definition private ***/
    if (T_IDENT(reg) == NULL || S_IDENT(T_IDENT(reg)) == NULL) {
      fprintf(outfile, "/* unknown region name */");
      return;
      INT_FATAL(NULL,"Trying to codegen an unnamed region in priv_reg_access");
    }
    fprintf(outfile, "%s", S_IDENT(T_IDENT(reg)));
  } else {
    if (S_IDENT(T_IDENT(reg)) == NULL) {
      INT_FATAL(NULL,"Trying to codegen an unnamed region in priv_reg_access");
    }
    priv_access_begin(outfile);
    fprintf(outfile, "%s", S_IDENT(T_IDENT(reg)));
    priv_access_end(outfile);
  }
}

/***
 *** priv_alloc_begin(FILE *)
 *** priv_alloc_end(FILE *)
 *** priv_reg_alloc(FILE *, char *)
 *** priv_ens_alloc(FILE *, char *)
 ***
 *** begin and end private allocation in compiler output (to file)
 ***
 *** generate type and variable name between begin and end
 ***  (separated by a comma)
 ***/

void priv_alloc_begin(FILE *outfile) {
  fprintf(outfile, "_PRIV_ALLOC(");
}

void priv_alloc_end(FILE *outfile) {
  fprintf(outfile, ");\n");
}

void priv_reg_alloc(FILE *outfile, symboltable_t* region) {
  if (priv_access) {
    fprintf(outfile, "_PRIV_REG_ALLOC(%s);\n", S_IDENT(region));
  }
}


/***
 *** priv_tid_decl(FILE *)
 ***
 *** declare tid
 ***/
void priv_tid_decl(FILE *outfile) {
  if (priv_access) {
    fprintf(outfile, "  _PRIV_TID_DECL;\n");
  }
}
