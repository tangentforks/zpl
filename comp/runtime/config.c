/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Privgen.h"
#include "../include/datatype.h"
#include "../include/db.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/runtime.h"
#include "../include/typeinfo.h"

static FILE *cfgcodefile=NULL;
static FILE *cfgcodefile2=NULL;

static int cfgvarnum=0;

void StartConfig() {
  char buffer[FILEPATHLEN];
  
  sprintf(buffer,"%s%s_%s",DESTPATH,trunc_in_filename,CFGCODEFILE);
  if ((cfgcodefile=fopen(buffer,"w"))==NULL) {
    USR_FATAL(NULL, "Cannot open file '%s'",buffer);
  }
  /*  fprintf(cfgcodefile,"# 2 \"%s_%s\"\n",trunc_in_filename,CFGCODEFILE);*/
  fprintf(cfgcodefile,"void _AssignToConfig(char *_name,char *_val) {\n");
  priv_tid_decl(cfgcodefile);

  sprintf(buffer,"%s%s_%s",DESTPATH,trunc_in_filename,CFGCODEFILE2);
  if ((cfgcodefile2=fopen(buffer,"w"))==NULL) {
    USR_FATAL(NULL, "Cannot open file '%s'",buffer);
  }
  /*  fprintf(cfgcodefile2,"# 2 \"%s_%s\"\n",trunc_in_filename,CFGCODEFILE2);*/
  fprintf(cfgcodefile2,"#include <stdio.h>\n");
  fprintf(cfgcodefile2,"#define _PRINT_OK_\n");
  fprintf(cfgcodefile2,"#include \"md_zinc.h\"\n");
  fprintf(cfgcodefile2,"#include \"zc_proto.h\"\n");
  fprintf(cfgcodefile2,"\n");

  fprintf(cfgcodefile2,"const int _MAXDIM = %d;\n",maxdim);

  fprintf(cfgcodefile2,"void _PrintConfigs(void) {\n");
}  


static void DoConfigAssign(symboltable_t *p) {
  if (cfgcodefile!=NULL) {
    if (cfgvarnum!=0) {
      fprintf(cfgcodefile," else");
    } else {
      fprintf(cfgcodefile," ");
    }
    fprintf(cfgcodefile," if (!strcmp(_name,\"%s\")) {\n",S_IDENT(p));
    switch (D_CLASS(S_DTYPE(p))) {
    case DT_CHAR:
      fprintf(cfgcodefile,"    %s=*_val;\n",S_IDENT(p));
      break;
    case DT_SHORT:
    case DT_INTEGER:
    case DT_UNSIGNED_BYTE:
    case DT_SIGNED_BYTE:
      fprintf(cfgcodefile,"    %s=atoi(_val);\n",S_IDENT(p));
      break;
    case DT_DOUBLE:
    case DT_REAL:
    case DT_QUAD:
      fprintf(cfgcodefile,"    %s=atof(_val);\n",S_IDENT(p));
      break;
    case DT_COMPLEX:
    case DT_DCOMPLEX:
    case DT_QCOMPLEX:
      fprintf(cfgcodefile,"    sscanf(_val,\"{%%f,%%f\",&(%s.re),&(%s.im));\n",
	      S_IDENT(p),S_IDENT(p));
      break;
    case DT_UNSIGNED_INT:
    case DT_UNSIGNED_SHORT:
    case DT_UNSIGNED_LONG:
    case DT_LONG:
      fprintf(cfgcodefile,"    %s=atol(_val);\n",S_IDENT(p));
      break;
    case DT_STRING:
      fprintf(cfgcodefile,"    strcpy(%s,_val);\n",S_IDENT(p));
      break;
    case DT_BOOLEAN:
      fprintf(cfgcodefile,"    if (strcmp(_val,\"true\") == 0) {\n");
      fprintf(cfgcodefile,"      %s = 1;\n",S_IDENT(p));
      fprintf(cfgcodefile,"    } else if (strcmp(_val,\"false\") == 0) {\n");
      fprintf(cfgcodefile,"      %s = 0;\n",S_IDENT(p));
      fprintf(cfgcodefile,"    } else {\n");
      fprintf(cfgcodefile,"      _RT_ALL_FATAL0(\"Attempting to assign config "
	      "var \\\"%s\\\" a value other than true/false.\\n\");\n",
	      S_IDENT(p));
      fprintf(cfgcodefile,"    }\n");
      break;
    case DT_FILE:
      fprintf(cfgcodefile,"    if (strcmp(_val,\"zin\") == 0) {\n");
      fprintf(cfgcodefile,"      %s = zin;\n",S_IDENT(p));
      fprintf(cfgcodefile,"    } else if (strcmp(_val,\"zout\") == 0) {\n");
      fprintf(cfgcodefile,"      %s = zout;\n",S_IDENT(p));
      fprintf(cfgcodefile,"    } else if (strcmp(_val,\"zerr\") == 0) {\n");
      fprintf(cfgcodefile,"      %s = zerr;\n",S_IDENT(p));
      fprintf(cfgcodefile,"    } else if (strcmp(_val,\"znull\") == 0) {\n");
      fprintf(cfgcodefile,"      %s = znull;\n",S_IDENT(p));
      fprintf(cfgcodefile,"    } else {\n");
      fprintf(cfgcodefile,"      _RT_ALL_FATAL0(\"Attempting to assign config "
	      "var \\\"%s\\\" a value other than zin/zout/zerr.\\n\");\n",
	      S_IDENT(p));
      fprintf(cfgcodefile,"    }\n");
      break;
    case DT_ARRAY:
      fprintf(cfgcodefile, "_RT_ALL_FATAL0(\"Cannot assign config vars of type"
	      " indexed array from the command-line, currently\\n\");\n");
      break;
    case DT_DIRECTION:
      fprintf(cfgcodefile, "_RT_ALL_FATAL0(\"Cannot assign config vars of type"
	      " 'direction' from the command-line, currently\\n\");\n");
      break;
    case DT_ENUM:
      {
        symboltable_t* pst;

        pst = D_STRUCT(S_DTYPE(p));
	fprintf(cfgcodefile, "    ");
        while (pst != NULL) {
          fprintf(cfgcodefile, "if (strcmp(_val,\"%s\") == 0) {\n", 
		  S_IDENT(pst));
	  fprintf(cfgcodefile, "      %s = %s;\n", S_IDENT(p), S_IDENT(pst));
	  fprintf(cfgcodefile, "    } else ");
	  
	  pst = S_SIBLING(pst);
	}
	fprintf(cfgcodefile, "{\n");
	fprintf(cfgcodefile, "_RT_ALL_FATAL0(\"Attempting to assign config "
		             "var \\\"%s\\\" a value that's not part of its "
                             "enumerated type.\\n\");\n", S_IDENT(p));
	fprintf(cfgcodefile, "    }\n");
      }
      break;
    default:
      INT_FATALX(S_LINENO(p), S_FILENAME(p),
		 "Unsupported config var type: %d",D_CLASS(S_DTYPE(p)));
    }
    fprintf(cfgcodefile,"    _cfginit[%d]=1;\n",cfgvarnum);
    fprintf(cfgcodefile,"  }");
  }
}


static void DoConfigDefaultExpression(FILE* outfile, symboltable_t *pst) {
  fprintf(outfile,"  if (_cfginit[%d]==0) {\n",cfgvarnum);
}


static void DoConfigPrint(symboltable_t *pst) {
  if (cfgvarnum == 0) {
    fprintf(cfgcodefile2,"  printf(\"\\nCONFIG VARS:\\n\");\n");
  }
  fprintf(cfgcodefile2,"  printf(\"%14s = ",S_IDENT(pst));
  printing_string++;
  gen_init_list(cfgcodefile2, S_VAR_INIT(pst));
  fprintf(cfgcodefile2,"\\n\");\n");
  printing_string--;
}


void DoConfig(FILE* initfile, symboltable_t *pst) {
  DoConfigAssign(pst);
  DoConfigPrint(pst);
  DoConfigDefaultExpression(initfile, pst);
  cfgvarnum++;
}


void EndConfig(FILE* outfile) {
  int i;
  
  if (cfgvarnum != 0) {
    fprintf(cfgcodefile," else {\n");
  }
  fprintf(cfgcodefile,"    if (_INDEX == 0) {\n");
  fprintf(cfgcodefile,"      printf(\"There is no assignable config ");
  fprintf(cfgcodefile,"var named \\\"%%s\\\"\\n\",_name);\n");
  fprintf(cfgcodefile,"    }\n");
  fprintf(cfgcodefile,"    exit(222);\n");
  if (cfgvarnum != 0) {
    fprintf(cfgcodefile,"  }\n");
  }
  fprintf(cfgcodefile,"}\n\n\n");
  fclose(cfgcodefile);

  if (cfgvarnum>0) {
    /*** config vars are not private *** we'll let p=1 initialize them ***/
    fprintf(outfile,"\nint _cfginit[%d] = { 0",cfgvarnum);
    for (i=1;i<cfgvarnum;i++) {
      fprintf(outfile,", 0");
    }
    fprintf(outfile,"};\n");
  }

  fprintf(cfgcodefile2,"}\n\n\n");
  fclose(cfgcodefile2);
}
