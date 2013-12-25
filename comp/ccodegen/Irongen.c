/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Irongen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/datatype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/genlist.h"
#include "../include/global.h"
#include "../include/runtime.h"
#include "../include/struct.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/treemac.h"


/* variables declared in global.h */

int *max_numensembles;


static int max_numdirs=0;
static int max_numwrapdirs=0;


void StartIronmanCodegen(int numcomms) {
  int i;

  numcomms++;
  max_numensembles = (int *)malloc(numcomms*sizeof(int));
  for (i=0;i<numcomms;i++) {
    max_numensembles[i] = 0;
  }
  max_numensembles[numcomms-1] = 1; /* collective comm */
}


void EndIronmanCodegen(FILE *outfile,int numcomms) {
  int i;

  fprintf(outfile,"_CONST int _numcomms = %d;\n",numcomms+1);
  fprintf(outfile,"_CONST int _enspercommid[%d] = {%d",numcomms+1,
	  max_numensembles[0]);
  for (i=1;i<numcomms;i++) {
    fprintf(outfile,",%d",max_numensembles[i]);
  }
  fprintf(outfile,",1"); /* collective comm */
  fprintf(outfile,"};\n");
  fprintf(outfile,"_CONST int _max_numdirs = %d;\n",max_numdirs);
  fprintf(outfile,"_CONST int _max_numwrapdirs = %d;\n",max_numwrapdirs);
  fprintf(outfile,"_CONST int _numremapmaps = %d;\n", numperms-numshare);
  fprintf(outfile,"\n");
}


void gen_iron_comm(FILE* outfile,statement_t *stmt) {
  comm_info_t * comm_info;
  genlist_t * dirs;
  genlist_t* dirtypes;
  datatype_t * dptr;
  int numarrs;
  int commid;
  int numdirs;

  commid = T_IRON_COMMID(stmt);

  if (T_COMM_TYPE(stmt) == C_NEW) {

    /* Write out the declarations for the call */
    fprintf(outfile,"{ /* NEW ");
#if (!RELEASE_VER)
    fprintf(outfile, " cid:%d", T_COMM_ID(stmt));
#endif
    fprintf(outfile," (near line %d) */\n",T_LINENO(stmt));

    /* Count total number of arrays being sent */
    numarrs=0;
    comm_info = T_COMM_INFO(stmt);
    while (comm_info != NULL) {
      numarrs++;
      comm_info = T_COMM_INFO_NEXT(comm_info);
    }
    if (numarrs > max_numensembles[commid]) {
      max_numensembles[commid] = numarrs;
    }

    /* Declare our main data structure */
    fprintf(outfile,"_arrcomminfo _atcomminfo[%d];\n",numarrs);


    /* Declare the structures for the direction vectors */
    comm_info = T_COMM_INFO(stmt);
    numarrs=0;
    while (comm_info != NULL) {
      dirs = T_COMM_INFO_DIR(comm_info);
      numdirs = 0;
      while (dirs != NULL) {
	numdirs++;
	dirs = G_NEXT(dirs);
      }
      if (numdirs > max_numdirs) {
	max_numdirs = numdirs;
      }
      fprintf(outfile,"_direction_pnc _subdirvect%d[%d];\n",numarrs,
	      numdirs);
      numarrs++;
      comm_info = T_COMM_INFO_NEXT(comm_info);
    }


    /* Loop over each array, filling in appropriate fields */
    numarrs = 0;
    comm_info = T_COMM_INFO(stmt);
    while (comm_info != NULL) {
      /* Generate element size */
      fprintf(outfile,"_atcomminfo[%d].elemsize = ",numarrs);
      dptr = datatype_find_ensemble(S_DTYPE(expr_find_ens_pst(T_COMM_INFO_ENS(comm_info))));
      if (D_CLASS(dptr) != DT_ENSEMBLE) {
	INT_FATAL(NULL, "dptr not ensemble");
      }
      find_element_size(dptr,outfile);
      fprintf(outfile,";\n");

      /* Generate array reference */
      fprintf(outfile,"_atcomminfo[%d].arr = ",numarrs);
      brad_no_access++;
      gen_expr(outfile, expr_find_ensemble_root(T_COMM_INFO_ENS(comm_info)));
      brad_no_access--;
      fprintf(outfile,";\n");

      /*** for lhs @'s ***/
      fprintf(outfile,"_atcomminfo[%d].lhscomm = %d;\n",
	      numarrs, T_COMM_LHSCOMM(stmt));

      /* Generate wrap directions */
      numdirs = 0;
      dirs = T_COMM_INFO_DIR(comm_info);
      dirtypes = T_COMM_INFO_DIRTYPES(comm_info);
      while (dirs != NULL) {
	if (G_ATTYPE(dirtypes) == AT_WRAP) {
	  expr_t* dir = G_EXPR(dirs);

	  if (T_IDENT(dir) && S_SETUP(T_IDENT(dir)) == 0) {
	    force_pst_init(outfile, T_IDENT(dir));                /** need finalize and done if directions have that -sjd */
	  }
	  fprintf(outfile,"_subdirvect%d[%d] = ",numarrs,numdirs);

	  gen_expr(outfile, dir);

	  fprintf(outfile,";\n");
	  numdirs++;
	}
	dirs = G_NEXT(dirs);
	dirtypes = G_NEXT(dirtypes);
      }
      if (numdirs > max_numwrapdirs) {
	max_numwrapdirs = numdirs;
      }
      /* Generate number of wrap directions */
      fprintf(outfile,"_atcomminfo[%d].numwrapdirs = %d;\n",numarrs,numdirs);

      /* Generate normal directions */
      dirs = T_COMM_INFO_DIR(comm_info);
      dirtypes = T_COMM_INFO_DIRTYPES(comm_info);
      while (dirs != NULL) {
	if (G_ATTYPE(dirtypes) != AT_WRAP) {
	  expr_t* dir = G_EXPR(dirs);

	  if (T_IDENT(dir) && S_SETUP(T_IDENT(dir)) == 0) {
	    force_pst_init(outfile, T_IDENT(dir));
	  }
	  fprintf(outfile,"_subdirvect%d[%d] = ",numarrs,numdirs);

	  gen_expr(outfile, dir);

	  fprintf(outfile,";\n");
	  numdirs++;
	}
	dirs = G_NEXT(dirs);
	dirtypes = G_NEXT(dirtypes);
      }

      /* Generate total number of directions and assign the list */
      fprintf(outfile,"_atcomminfo[%d].numdirs = %d;\n",numarrs,numdirs);
      fprintf(outfile,"_atcomminfo[%d].dirlist = _subdirvect%d;\n",numarrs,
	      numarrs);

      numarrs++;
      comm_info = T_COMM_INFO_NEXT(comm_info);
    }
  }

  /* emit the call into the ironman interface */
  fprintf(outfile,"_AtComm_");
  switch (T_COMM_TYPE(stmt)) {
  case C_NEW:
    fprintf(outfile,"New");
    break;
  case C_DR:
    fprintf(outfile,"DR");
    break;
  case C_SR:
    fprintf(outfile,"SR");
    break;
  case C_DN:
    fprintf(outfile,"DN");
    break;
  case C_SV:
    fprintf(outfile,"SV");
    break;
  case C_OLD:
    fprintf(outfile,"Old");
    break;
  default:
    break;
  }
  fprintf(outfile,"(");

  if (T_COMM_TYPE(stmt) == C_NEW) {
    if (T_COMM_REG(stmt)) {
      priv_reg_access(outfile, T_COMM_REG(stmt));
    } else {
      genStackReg(outfile);
    }
    fprintf(outfile,",%d,_atcomminfo,",numarrs);
  }
  if (T_COMM_TYPE(stmt) == C_DR ||
      T_COMM_TYPE(stmt) == C_SR ||
      T_COMM_TYPE(stmt) == C_DN ||
      T_COMM_TYPE(stmt) == C_SV ||
      T_COMM_TYPE(stmt) == C_OLD) {
    if (T_COMM_REG(stmt)) {
      priv_reg_access(outfile, T_COMM_REG(stmt));
    } else {
      genStackReg(outfile);
    }
    fprintf(outfile,",");
  }
  fprintf(outfile,"%d);",commid);

  if (T_COMM_TYPE(stmt) == C_NEW) {
    fprintf(outfile,"\n}\n");
  }
  else {
    /* Generate comment for communication */
    switch (T_COMM_TYPE(stmt)) {
    case C_DR:
      fprintf(outfile,"       /* Dest Ready");
      break;
    case C_SR:
      fprintf(outfile,"      /* Source Ready ");
      break;
    case C_DN:
      fprintf(outfile,"     /* Dest Needed");
      break;
    case C_SV:
      fprintf(outfile,"      /* Source Volatile");
      break;
    case C_OLD:
      fprintf(outfile,"       /* OLD");
      break;
    default:
      break;
    }
#if (!RELEASE_VER)
    fprintf(outfile, " cid:%d", T_COMM_ID(stmt));
#endif
    fprintf(outfile," (near line %d) */\n",T_LINENO(stmt));
  }

}
