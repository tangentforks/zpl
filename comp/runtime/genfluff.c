/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/Privgen.h"
#include "../include/error.h"
#include "../include/genlist.h"
#include "../include/runtime.h"
#include "../include/symboltable.h"

#define NOT(x) (((x) == LO) ? HI : LO)

static void gen_lohi(FILE* outfile,int lohi) {
  fprintf(outfile,"_");
  if (lohi == LO) {
    fprintf(outfile,"lo");
  } else {
    fprintf(outfile,"hi");
  }
}


static void gen_lohi_caps(FILE* outfile,int lohi) {
  if (lohi == LO) {
    fprintf(outfile,"LO");
  } else {
    fprintf(outfile,"HI");
  }
}


static void gen_bound(FILE *outfile,char* regname,int dim,int lohi) {
  fprintf(outfile,"_SPS_REG_FLUFF_");
  gen_lohi_caps(outfile, lohi);
  fprintf(outfile,"(%s, %d)",regname, dim);
}


static void gen_my_bound(FILE *outfile,int lohi,int dim) {
  fprintf(outfile,"_DIST_MY");
  gen_lohi_caps(outfile,lohi);
  fprintf(outfile,"(_DefaultDistribution, %d)",dim);
}


static void gen_reg_glb_lohi(FILE* outfile, char* regname, int lohi,
			     int dim) {
  fprintf(outfile,"_REG_GLOB_");
  gen_lohi_caps(outfile,lohi);
  fprintf(outfile,"(%s, %d)",regname, dim);
}


static void gen_reg_test(FILE *outfile,char* regname,int lohi,int dim) {
  fprintf(outfile,"if ((_");
  if (lohi == LO) {
    fprintf(outfile,"AFTER_BEG");
  } else {
    fprintf(outfile,"BEFOR_END");
  }
  fprintf(outfile,"_OF_REG(_DefaultGrid, %s, %d)) && (", regname, dim);
  gen_lohi(outfile,lohi);
  if (lohi == LO) {
    fprintf(outfile,"<=");
  } else {
    fprintf(outfile,">=");
  }
  gen_reg_glb_lohi(outfile,regname,NOT(lohi),dim);
  fprintf(outfile,")) {\n");
}


static void gen_unk_fluffval(FILE *outfile,int unk_fluff,int dim,
			     char* regname) {
  fprintf(outfile, "_REG_STRIDE(%s, %d)", regname, dim);
}


void gen_fluffval(FILE *outfile,int fluffval,int unk_fluff,int dim,
		  char* regname) {
  if (!unk_fluff && !fluffval) {
    fprintf(outfile,"0");
    return;
  }
  if (fluffval) {
    if (unk_fluff) {
      fprintf(outfile,"max(");
    }
    fprintf(outfile,"%d",fluffval);
  }
  if (unk_fluff) {
    if (fluffval) {
      fprintf(outfile,",");
    }
    gen_unk_fluffval(outfile,unk_fluff,dim, regname);
    if (fluffval) {
      fprintf(outfile,")");
    }
  }
}


static void gen_set_bound(FILE* outfile,char* regname,int dim,int lohi) {
  gen_bound(outfile,regname,dim,lohi);
  fprintf(outfile," = ");
  gen_lohi(outfile,lohi);
  fprintf(outfile,";\n");
}


static void gen_set_fluff(FILE *outfile, char* regname, int dim,
			  int fluffval,int unk_fluff,int lohi,int strided,
			  int wrapfluff) {
  /* generating fluff-modified bound */
  gen_lohi(outfile,lohi);
  fprintf(outfile," = ");
  gen_my_bound(outfile,lohi,dim);
  if (lohi == LO) {
    fprintf(outfile,"-");
  } else {
    fprintf(outfile,"+");
  }
  gen_fluffval(outfile,fluffval,unk_fluff,dim, regname);
  fprintf(outfile,";\n");

  if (!wrapfluff) {
    gen_reg_test(outfile,regname,lohi,dim);
  }

  /* generating other bound */
  gen_lohi(outfile,NOT(lohi));
  fprintf(outfile," = ");
  gen_my_bound(outfile,lohi,dim);
  if (lohi == LO) {
    fprintf(outfile,"-");
  } else {
    fprintf(outfile,"+");
  }
  fprintf(outfile,"1;\n");

  /* if strided we need to snap and make sure it's still a legal range */
  if (strided) {
    fprintf(outfile,"_lo = _SNAP_UP(_lo, %s, %d);\n", regname, dim);
    fprintf(outfile,"_hi = _SNAP_DN(_hi, %s, %d);\n", regname, dim);

    fprintf(outfile,"if (_lo <= _hi) {\n");
  }

  /* if we didn't already own this range, this will now be our range */
  fprintf(outfile,"if (");
  gen_bound(outfile,regname,dim,LO);
  fprintf(outfile," > ");
  gen_bound(outfile,regname,dim,HI);
  fprintf(outfile,") {\n");
  gen_set_bound(outfile,regname,dim,NOT(lohi));
  fprintf(outfile,"}\n");

  /* in any case, take the one we were assigned */
  gen_set_bound(outfile,regname,dim,lohi);

  /* close strided's legal range check */
  if (strided) {
    fprintf(outfile,"}\n");
  }
  if (!wrapfluff) {
    fprintf(outfile,"}\n");
  }
}


void gen_fluff_decls(FILE *outfile,expr_t *reg,int strided) {
  fprintf(outfile,"int _lo;\n");
  fprintf(outfile,"int _hi;\n");
}


static void gen_fluff_setup_onedim(FILE *outfile,char* regname,int strided,
				   int fluffval,int unk_fluff,int wrapfluff,
				   int lohi,int dim) {
  if (fluffval || unk_fluff) {
    gen_set_fluff(outfile,regname,dim,fluffval,unk_fluff,lohi,strided,wrapfluff);
  }
}


static void gen_bounds_setup(FILE *outfile, char* regname, int numdims) {
  int i;

  for (i=0;i<numdims;i++) {
    gen_bound(outfile,regname,i,LO);
    fprintf(outfile," = _REG_MYLO(%s, %d);\n", regname, i);

    gen_bound(outfile,regname,i,HI);
    fprintf(outfile," = _REG_MYHI(%s, %d);\n", regname, i);
  }
}
    

void gen_fluff_setup(FILE *outfile,char* regname,symboltable_t* reg,
		     int numdims,int strided) {
  int i;

  gen_bounds_setup(outfile,regname,numdims);

  for (i=0;i<numdims;i++) {
    gen_fluff_setup_onedim(outfile,regname,strided,S_FLUFF_LO(reg,i),
			   S_UNK_FLUFF_LO(reg,i),S_WRAPFLUFF_LO(reg,i),LO,i);
    gen_fluff_setup_onedim(outfile,regname,strided,S_FLUFF_HI(reg,i),
			   S_UNK_FLUFF_HI(reg,i),S_WRAPFLUFF_HI(reg,i),HI,i);
  }
}
