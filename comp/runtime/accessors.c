/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/error.h"
#include "../include/runtime.h"
#include "../include/symbol.h"


static FILE* accheadfile;
static FILE *acccodefile;


char *gen_accessors(symboltable_t *arr,expr_t *reg,int numdims,
		    int arrtype,datatype_t *pdt) {
  char typename[256]="";
  char *name;
  int i;
  symboltable_t *accpst;
  datatype_t *elempdt;
  int innerdim;

  elempdt = D_ARR_TYPE(pdt);
  name = (char *)malloc(256*sizeof(char));
  sprintf(name,"_Access%dD_",numdims);
  switch (arrtype) {
  case ARR_NORMAL:
    sprintf(name,"%sDns",name);
    break;
  case ARR_STRIDED:
    sprintf(name,"%sStr",name);
    break;
  case ARR_SPARSE:
    sprintf(name,"%sSps",name);
    return_pdt(elempdt,PDT_NAME,typename);
    sprintf(name,"%s_%s",name,typename);
    break;
  default:
    INT_FATAL(NULL,"Unknown array type");
    break;
  }

  accpst = lu(name);
  if (accpst == NULL) {
    insert_function(name,pdtVOID,2,pdtGENERIC_ENSEMBLE,SC_INOUT,pdtGENERIC,SC_INOUT);
  } else {
    free(name);
    return S_IDENT(accpst);
  }

  fprintf(accheadfile,"static char* %s(_array,_vector);\n",name);
  fprintf(acccodefile,"static char* %s(_array arr,_vector i) {\n",name);
  
  switch (arrtype) {
  case ARR_NORMAL:
    fprintf(acccodefile,"  return _F_ACCESS_%dD(arr",numdims);
    for (i=0;i<numdims;i++) {
      fprintf(acccodefile,",i[%d]",i);
    }
    fprintf(acccodefile,");\n");
    break;
  case ARR_STRIDED:
    fprintf(acccodefile,"  return _ACCESS_%dD(arr",numdims);
    for (i=0;i<numdims;i++) {
      fprintf(acccodefile,",i[%d]",i);
    }
    fprintf(acccodefile,");\n");
    break;
  case ARR_SPARSE:
    innerdim = numdims - 1;

    fprintf(acccodefile,"  _region reg = _ARR_DECL_REG(arr);\n");
    fprintf(acccodefile,"  _SPS_DECL_INIT_ARR_ORIG_W(");
    gen_pdt(acccodefile,elempdt,PDT_PCST);
    fprintf(acccodefile,", arr);\n");
    fprintf(acccodefile,"  _SPS_DECL_INIT_REG(reg,%d);\n",innerdim);
    fprintf(acccodefile,"  _SPS_DECL_INNER_STOP(reg);\n");
    for (i=0; i<numdims; i++) {
      fprintf(acccodefile,"  const int _I(%d) = i[%d];\n",i,i);
    }
    fprintf(acccodefile,"\n");

    fprintf(acccodefile,"  _SPS_START_UP_INNER(reg,reg,%d,%d);\n",numdims,innerdim);
    fprintf(acccodefile,"  _SPS_CATCH_UP_INNER(reg,reg,%d);\n",innerdim);
    fprintf(acccodefile,"  return (char*)(_SPS_ORIG(arr) + "
	    "_SPSID_VAL(reg,reg,%d));\n",innerdim);

    break;
  }
  
  fprintf(acccodefile,"}\n\n\n");

  return name;
}


void StartAccessors() {
  char filename[FILEPATHLEN];

  sprintf(filename,"%s%s_%s",DESTPATH,trunc_in_filename,ACCCODEFILE);
  acccodefile=fopen(filename,"w");
  if (acccodefile == NULL) {
    USR_FATAL(NULL, "Cannot open file '%s'",filename);
  }
  fprintf(acccodefile,"/** accessors **/\n");
  sprintf(filename,"%s%s_%s",DESTPATH,trunc_in_filename,ACCHEADFILE);
  accheadfile=fopen(filename,"w");
  if (accheadfile == NULL) {
    USR_FATAL(NULL, "Cannot open file '%s'",filename);
  }
}


void EndAccessors() {
  fclose(acccodefile);
  fclose(accheadfile);
}


