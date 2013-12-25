/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/IOgen.h"
#include "../include/Privgen.h"
#include "../include/Spsgen.h"
#include "../include/datatype.h"
#include "../include/error.h"
#include "../include/expr.h"
#include "../include/global.h"
#include "../include/iostmt.h"
#include "../include/parsetree.h"
#include "../include/rmstack.h"
#include "../include/symmac.h"
#include "../include/symtab.h"
#include "../include/treemac.h"


#define EXPR_IS_BYTE(expr) (D_CLASS(T_TYPEINFO(expr)) == DT_SIGNED_BYTE || \
			    D_CLASS(T_TYPEINFO(expr)) == DT_UNSIGNED_BYTE)
#define EXPR_IS_BOOL(expr) (D_CLASS(T_TYPEINFO(expr)) == DT_BOOLEAN)


static void gen_io_pdt(FILE *outfile,datatype_t *pdt) {
  switch (D_CLASS(pdt)) {
  case DT_BOOLEAN:
    fprintf(outfile,"bool");
    break;
  case DT_CHAR:
    fprintf(outfile,"char");
    break;
  case DT_SIGNED_BYTE:
    fprintf(outfile,"sbyte");
    break;
  case DT_UNSIGNED_BYTE:
    fprintf(outfile,"ubyte");
    break;
  case DT_SHORT:
    fprintf(outfile,"short");
    break;
  case DT_UNSIGNED_SHORT:
    fprintf(outfile,"ushort");
    break;
  case DT_ENUM:
  case DT_INTEGER:
    fprintf(outfile,"int");
    break;
  case DT_UNSIGNED_INT:
    fprintf(outfile,"uint");
    break;
  case DT_LONG:
    fprintf(outfile,"long");
    break;
  case DT_UNSIGNED_LONG:
    fprintf(outfile,"ulong");
    break;
  case DT_REAL:
    fprintf(outfile,"float");
    break;
  case DT_DOUBLE:
    fprintf(outfile,"double");
    break;
  case DT_QUAD:
    fprintf(outfile,"quad");
    break;
  case DT_STRING:
    fprintf(outfile,"string");
    break;
  case DT_STRUCTURE:
    fprintf(outfile,"record");
    break;
  case DT_ARRAY:
    fprintf(outfile,"array");
    break;
  case DT_COMPLEX:
    fprintf(outfile,"fcomplex");
    break;
  case DT_DCOMPLEX:
    fprintf(outfile,"dcomplex");
    break;
  case DT_QCOMPLEX:
    fprintf(outfile,"qcomplex");
    break;
  case DT_REGION:
    fprintf(outfile,"reg");
    break;
  case DT_VOID:
    USR_FATAL(NULL, "Can't write/writeln expressions of void type");
  default:
    INT_FATAL(NULL,"Unknown datatype in IOgen.c");
    break;
  }
}


static void gen_file_expr(FILE *outfile,io_t *io) {
  if (IO_FILE(io) == NULL) {
    if (IO_IS_READ(io)) {
      fprintf(outfile,"zin");
    } else {
      fprintf(outfile,"zout");
    }
  } else {
    gen_expr(outfile, IO_FILE(io));
  }
}


static void gen_inout(FILE *outfile,io_t *io) {
  if (IO_IS_READ(io)) {
    fprintf(outfile,"In");
  } else {
    fprintf(outfile,"Out");
  }
}


static void gen_printscan(FILE *outfile,io_t *io,int numargs) {
  fprintf(outfile,"_SCALAR_F");
  if (IO_IS_READ(io)) {
    fprintf(outfile,"SCAN");
  } else {
    fprintf(outfile,"PRINT");
  }
  fprintf(outfile,"F%d",numargs);
}


static void gen_region_base(FILE* outfile, expr_t* reg) {
  priv_reg_access(outfile, reg);
  mark_region_needs(reg, D_REG_NUM(T_TYPEINFO(reg))-1, 0, 1);
}


static void gen_region(FILE *outfile) {
  expr_t *reg;

  reg = RMSCurrentRegion();
  gen_region_base(outfile,reg);
}



static void gen_io_expr_size(FILE *outfile,expr_t *expr) {
  fprintf(outfile,"sizeof(");
  gen_expr(outfile,expr);
  fprintf(outfile,")");
}


static void gen_io_expr_ref(FILE *outfile,expr_t *expr) {
  int isstring;

  isstring = (S_CLASS(T_TYPEINFO(expr)) == DT_STRING);
  if (!isstring) {
    fprintf(outfile,"&(");
  }
  gen_expr(outfile,expr);
  if (!isstring) {
    fprintf(outfile,")");
  }
}


static void gen_io_expr_str(FILE *outfile,expr_t *expr) {
  int isstring;

  isstring = (S_CLASS(T_TYPEINFO(expr)) == DT_STRING);
  if (!isstring) {
    fprintf(outfile,"\"");
  }
  gen_expr(outfile,expr);
  if (!isstring) {
    fprintf(outfile,"\"");
  }
}


static void gen_scalar_io_expr(FILE *outfile,io_t *io,expr_t *expr) {
  if (IO_IS_READ(io)) {
    gen_io_expr_ref(outfile,expr);
  } else {
    gen_expr(outfile,expr);
  }
  if (datatype_complex(T_TYPEINFO(expr))) {
    if (codegen_imaginary == 0) {
      fprintf(outfile,",");
      codegen_imaginary = 1;
      gen_scalar_io_expr(outfile,io,expr);
      codegen_imaginary = 0;
    }
  }
}


static void gen_rw(FILE *outfile,io_t *io) {
  if (IO_IS_READ(io)) {
    fprintf(outfile,"R");
  } else {
    fprintf(outfile,"W");
  }
}


static void gen_read_write(FILE *outfile,io_t *io) {
  fprintf(outfile,"_ZIO_");
  gen_rw(outfile,io);
}


static void gen_print_func(FILE *outfile,datatype_t *pdt,io_t *io) {
  fprintf(outfile,"_Stream");
  gen_inout(outfile,io);
  fprintf(outfile,"_");
  gen_io_pdt(outfile,pdt);
}


static void gen_control_string(FILE *outfile,datatype_t *pdt,io_t *io) {
  if (IO_CONTROL(io)) {
    fprintf(outfile,"%s",S_IDENT(T_IDENT(IO_CONTROL(io))));
  } else {
    fprintf(outfile,"_Z_");
    gen_rw(outfile,io);
    fprintf(outfile,"_");
    gen_io_pdt(outfile,pdt);
  }
}


static void gen_linefeed(FILE *outfile,io_t *io) {
  fprintf(outfile,"_SCALAR_LINEFEED(");
  gen_file_expr(outfile,io);
  fprintf(outfile,");\n");
}


static void gen_ensemble_io_gen(FILE *outfile,io_t *io,expr_t *expr) {
  int indexi;

  gen_file_expr(outfile,io);
  fprintf(outfile,",");
  gen_region(outfile);
  fprintf(outfile,",");
  indexi = expr_is_indexi(expr);
  if (indexi) {
    fprintf(outfile, "_Index[%d]", indexi-1);
  } else {
    brad_no_access++;
    gen_expr(outfile,expr);
    brad_no_access--;
  }
  fprintf(outfile,",");
  gen_read_write(outfile,io);
}


static void gen_ensemble_io_bin(FILE *outfile,io_t *io,expr_t *expr) {
  fprintf(outfile,"_StreamEnsembleBin(");
  gen_ensemble_io_gen(outfile,io,expr);
  fprintf(outfile,",");
  fprintf(outfile,"sizeof(");
  gen_pdt(outfile,T_TYPEINFO(expr),PDT_SIZE);
  fprintf(outfile,"));\n");
}


static void gen_ensemble_io_txt(FILE *outfile,io_t *io,expr_t *expr) {
  fprintf(outfile,"_StreamEnsembleTxt(");
  gen_ensemble_io_gen(outfile,io,expr);
  fprintf(outfile,",");
  gen_print_func(outfile,T_TYPEINFO(expr),io);
  fprintf(outfile,",");
  gen_control_string(outfile,T_TYPEINFO(expr),io);
  fprintf(outfile,");\n");
}


static void gen_ensemble_io(FILE *outfile,io_t *io,expr_t *expr) {
  if (IO_IS_BIN(io)) {
    gen_ensemble_io_bin(outfile,io,expr);
  } else {
    gen_ensemble_io_txt(outfile,io,expr);
  }
}


static void gen_region_io_gen(FILE *outfile,io_t *io,expr_t *expr) {
  gen_file_expr(outfile,io);
  fprintf(outfile,",");
  gen_region_base(outfile,expr);
  fprintf(outfile,",");
  gen_read_write(outfile,io);
}


static void gen_region_io_bin(FILE* outfile,io_t* io,expr_t* expr) {
  USR_FATAL(NULL,"Binary I/O not yet implemented for regions\n");
}


static void gen_region_io_txt(FILE* outfile,io_t* io,expr_t* expr) {
  fprintf(outfile,"_StreamRegionTxt(");
  gen_region_io_gen(outfile,io,expr);
  fprintf(outfile,",");
  gen_control_string(outfile,pdtREGION,io);
  fprintf(outfile,");\n");
}


static void gen_region_io(FILE* outfile,io_t* io,expr_t* expr) {
  if (IO_IS_BIN(io)) {
    gen_region_io_bin(outfile,io,expr);
  } else {
    gen_region_io_txt(outfile,io,expr);
  }
}


static void gen_scalar_io_bcast(FILE *outfile,io_t *io,expr_t *expr) {
  int save_codegen_imaginary = codegen_imaginary;

  codegen_imaginary = -1;

  fprintf(outfile,"_SCALAR_BCAST(");
  gen_io_expr_ref(outfile,expr);
  fprintf(outfile,",");
  gen_io_expr_size(outfile,expr);
  fprintf(outfile,");\n");

  codegen_imaginary = save_codegen_imaginary;
}


static void gen_scalar_io_bin(FILE *outfile,io_t *io,expr_t *expr) {
  int save_codegen_imaginary = codegen_imaginary;

  codegen_imaginary = -1;

  fprintf(outfile,"_SCALAR_F");
  if (IO_IS_READ(io)) {
    fprintf(outfile,"READ");
  } else {
    fprintf(outfile,"WRITE");
  }
  fprintf(outfile,"(");
  gen_io_expr_ref(outfile,expr);
  fprintf(outfile,",");
  gen_io_expr_size(outfile,expr);
  fprintf(outfile,",1,");
  gen_file_expr(outfile,io);
  fprintf(outfile,");\n");

  codegen_imaginary = save_codegen_imaginary;
}


static void gen_scalar_io_txt_readbyte(FILE *outfile,io_t *io,expr_t *expr) {
  fprintf(outfile,"_");
  gen_io_pdt(outfile,T_TYPEINFO(expr));
  fprintf(outfile,"(");
  gen_file_expr(outfile,io);
  fprintf(outfile,",");
  gen_scalar_io_expr(outfile,io,expr);
  fprintf(outfile,");\n");
}


static void gen_scalar_io_txt_writebyte(FILE *outfile,io_t *io,expr_t *expr) {
  fprintf(outfile,"(");
  gen_file_expr(outfile,io);
  fprintf(outfile,",");
  gen_control_string(outfile,T_TYPEINFO(expr),io);
  fprintf(outfile,",(");
  gen_pdt(outfile,T_TYPEINFO(expr),PDT_CAST);
  fprintf(outfile,")");
  gen_scalar_io_expr(outfile,io,expr);
  fprintf(outfile,");\n");
}


static void gen_scalar_io_txt_writebool(FILE *outfile,io_t *io,expr_t *expr) {
  fprintf(outfile,"_");
  gen_io_pdt(outfile,T_TYPEINFO(expr));
  fprintf(outfile,"(");
  gen_file_expr(outfile,io);
  fprintf(outfile,",");
  gen_scalar_io_expr(outfile,io,expr);
  fprintf(outfile,");\n");
}


static void gen_scalar_io_txt(FILE *outfile,io_t *io,expr_t *expr) {
  int numargs = 1;

  if (datatype_complex(T_TYPEINFO(expr))) {
    numargs = 2;
  }
  gen_printscan(outfile,io,numargs);
  if (EXPR_IS_BYTE(expr)) {
    if (IO_IS_READ(io)) {
      gen_scalar_io_txt_readbyte(outfile,io,expr);
    } else {
      gen_scalar_io_txt_writebyte(outfile,io,expr);
    }
  } else if (!IO_IS_READ(io) && EXPR_IS_BOOL(expr)) {
    gen_scalar_io_txt_writebool(outfile,io,expr);
  } else {
    fprintf(outfile,"(");
    gen_file_expr(outfile,io);
    fprintf(outfile,",");
    gen_control_string(outfile,T_TYPEINFO(expr),io);
    fprintf(outfile,",");
    gen_scalar_io_expr(outfile,io,expr);
    fprintf(outfile,");\n");
  }
}


static void gen_scalar_io(FILE *outfile,io_t *io,expr_t *expr) {
  if (IO_IS_BIN(io)) {
    gen_scalar_io_bin(outfile,io,expr);
  } else {
    gen_scalar_io_txt(outfile,io,expr);
  }
  if (IO_IS_READ(io)) {
    gen_scalar_io_bcast(outfile,io,expr);
  }
}


static void gen_const_io_bin(FILE *outfile,io_t *io,expr_t *expr) {
  fprintf(outfile,"_SCALAR_FWRITE(");
  gen_io_expr_ref(outfile,expr);
  fprintf(outfile,",");
  gen_io_expr_size(outfile,expr);
  fprintf(outfile,",1,");
  gen_file_expr(outfile,io);
  fprintf(outfile,");\n");
}


static void gen_const_io_txt(FILE *outfile,io_t *io,expr_t *expr) {
  int numargs = 0;

  if (IO_CONTROL(io)) {
    numargs = 1;
  }
  gen_printscan(outfile,io,numargs);
  fprintf(outfile,"(");
  gen_file_expr(outfile,io);
  fprintf(outfile,",");
  if (IO_CONTROL(io)) {
    gen_control_string(outfile, NULL, io);
    fprintf(outfile, ",");
  }
  gen_io_expr_str(outfile,expr);
  fprintf(outfile,");\n");
}


static void gen_const_io(FILE *outfile,io_t *io,expr_t *expr) {
  if (IO_IS_BIN(io)) {
    gen_const_io_bin(outfile,io,expr);
  } else {
    gen_const_io_txt(outfile,io,expr);
  }
}


void gen_io(FILE *outfile,statement_t *stmt) {
  io_t *io;
  expr_t *expr;

  io = T_IO(stmt);
  expr = IO_EXPR1(io);
  if (IO_TYPE(io) == IO_WRITELN || IO_TYPE(io) == IO_HALTLN) {
    gen_linefeed(outfile,io);
  } else {
    if (D_CLASS(T_TYPEINFO(expr)) == DT_REGION) {
      gen_region_io(outfile, io, expr);
    } else if (T_TYPEINFO_REG(expr) && IO_TYPE(io) != IO_HALT) { 
      gen_ensemble_io(outfile,io,expr);
    } else {
      if (T_TYPE(expr) == CONSTANT) {
	gen_const_io(outfile,io,expr);
      } else {
	gen_scalar_io(outfile,io,expr);
      }
    }
  }
}

