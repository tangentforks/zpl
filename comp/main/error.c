/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***
 *** file:          error.c
 *** description:   This file include functions for error checking
 *** author:        Sung-Eun Choi (with help from E)
 *** date:          7 January 1996
 *** modifications: 20 January 1996
 ***                Replaced simple check sum for internal error numbers
 ***                 with lower 16 bits of the Ethernet CRC (courtesy of E)
 ***                19 September 2002 (sungeun)
 ***                Removed all rcs id stuff
 ***
 *** IMPORTANT!
 ***    The script Maketable.pl generates a table of the internal
 ***     error numbers by employing the same algorithm used here.
 ***     Any changes made here must be also be made in Maketable.pl.
 ***
 ***/
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../include/error.h"
#include "../include/global.h"
#include "../include/scan.h"
#include "../include/parsetree.h"
#include "../include/treemac.h"
#include "../include/version.h"

/*
 * The polynomial.
 */
static char poly[] = {
    1, 1, 0, 1,
    1, 0, 1, 1,
    0, 1, 1, 1,
    0, 0, 0, 1,
    0, 0, 0, 0,
    0, 1, 1, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
};

/*
 * The shift register.
 */
static char reg[32];

/* 
 * Information about the location of the error macro.
 */
static char *file;
static int line;


static void CalcCRC(unsigned int value, int numbits) {
  int i, j, control;
  unsigned int input;
  unsigned int wrap, outbit, nextbit, in;

  control = 1;
  input = value;
  for (i = 0; i < numbits; i++) {    	       

    outbit = reg[31];
	
    in = input & 0x1;
    wrap = in ^ outbit;
    nextbit = control & wrap;

    input = input >> 1;
    /*
     * Do the shift.
     */
    for (j = 31; j > 0; j--) {
      if (poly[j - 1])
	reg[j] = (reg[j - 1]) ^ (nextbit);
      else
	reg[j] = reg[j - 1];
    }

    reg[0] = nextbit;

  }
}


static unsigned int crc(char * buffer, int length) {
  int i, loop;
  unsigned int byte, crc;

  /*
   * Initialize the register to all 0's.
   */
  for (i = 0; i < 32; i++) {
    reg[i] = 0;
  }

  /*
   * Read the input into the shift register.
   */
  for (loop = 0; loop < length; loop++) {
    byte = (unsigned int) buffer[loop];

    CalcCRC(byte, 8);
  }

  /*
   * Extract the CRC as if it were going to be transmitted
   * over the wire.
   */
  crc = 0x0;
  for (i = 31; i >= 0; i--) {
    crc = (crc << 1) | (reg[i] ^ 0x1);
  }

  return (crc);

}


void __ZPL_INT_FATAL(char *filename, int lineno) {
  char file_id[6];
  char error_string[1024];
  int len = strlen(filename) - 2;
  int front = (len < 3 ? len : 3);
  int back = front;

  if (len > 5) {
    /*** first 3 letters of the filename ***/
    strncpy(file_id, filename, front);
    /*** last 2 letters of the filename ***/
    if (len > front) {
      file_id[back++] = filename[len-2];
      file_id[back++] = filename[len-1];
    }
    file_id[back] = '\0';
  } else {
    strcpy(file_id, filename);
  }

  /*** contruct the string ***/
  /***  first 3 letters of the directory (capitalized) ***/
  /***  rcs version of the file ***/
  /***  first 3 letters and the last 2 letters of the file name ***/
  /***  line number ***/
  /***  the letter V ***/
  /***  compiler version ***/

  sprintf(error_string, "%s%dV%s", file_id, lineno, version_string());

  /*** print the internal error number and error string ***/
  /***  the error number is the bottom 16 bits of the ***/
  /***  CRC of the error string ***/
  fprintf(stderr, "\nINTERNAL ERROR %d (%s):\n",
	  crc(error_string, strlen(error_string)) & 0xFFFF,
	  error_string);
}


/* 
 * print_source_line() -- print source lines where error probably occurred.
 * Note that YACC is LR(1), which means we've looked ahead one token 
 * before we discovered an error, that is: if ntokens == 1 then the 
 * error was probably on the previous line, so print both.  Also, the error
 * diagnositic should say the error occurred 'before' this point.  
 */
void __ZPL_PRINT_SOURCE_LINE() {
    int  i;

    if (ntokens < 2)			/* < 3 if we have lookahead 2 */
    {
	fputs( prevline, stderr);
    }

    fputs( currline, stderr);

    for (i=0; i<column; i++)
	putc( '-', stderr);
    fputs( "^\n", stderr);
}

/* misc */

void setup_error(char *filename, int linenumber)
{
  line = linenumber;
  file = filename;
}

/* User errors */

/* FN: usr_warnx
 * echris - 4-19-97
 */
void usr_warnx(int lineno, char *filename, char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  display_usr("WARNING", lineno, filename, fmt, args);
  va_end(args);
}

/* FN: usr_fatalx
 * echris - 4-19-97
 */
void usr_fatalx(int lineno, char *filename, char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  display_usr("ERROR", lineno, filename, fmt, args);
  va_end(args);
  __ZPL_USR_EXIT();
}

/* FN: usr_fatal_contx
 * echris - 4-19-97
 */
void usr_fatal_contx(int lineno, char *filename, char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  display_usr("ERROR", lineno, filename, fmt, args);
  va_end(args);
  if (++fatal_count >= FATALMAX) __ZPL_USR_EXIT();
}

/****/

/* FN: usr_warn
 * echris - 4-19-97
 */
void usr_warn(statement_t *s, char *fmt, ...)
{
  va_list args;
  int lineno = -1;
  char *filename = NULL;

  if (s) {
    lineno = T_LINENO(s);
    filename = T_FILENAME(s);
  }
  va_start(args, fmt);
  display_usr("WARNING", lineno, filename, fmt, args);
  va_end(args);
}

/* FN: usr_fatal
 * echris - 4-19-97
 */
void usr_fatal(statement_t *s, char *fmt, ...)
{
  va_list args;
  int lineno = -1;
  char *filename = NULL;

  if (s) {
    lineno = T_LINENO(s);
    filename = T_FILENAME(s);
  }
  va_start(args, fmt);
  display_usr("ERROR", lineno, filename, fmt, args);
  va_end(args);
  __ZPL_USR_EXIT();
}

/* FN: usr_fatal_cont
 * echris - 4-19-97
 */
void usr_fatal_cont(statement_t *s, char *fmt, ...)
{
  va_list args;
  int lineno = -1;
  char *filename = NULL;

  if (s) {
    lineno = T_LINENO(s);
    filename = T_FILENAME(s);
  }
  va_start(args, fmt);
  display_usr("ERROR", lineno, filename, fmt, args);
  va_end(args);
  if (++fatal_count >= FATALMAX) __ZPL_USR_EXIT();
}


/* FN: usr_cond_fatal
 * echris - 4-19-97
 */
void usr_cond_fatal(int c, statement_t *s, char *fmt, ...)
{
  if (!c) {
    va_list args;
    int lineno = -1;
    char *filename = NULL;

    if (s) {
      lineno = T_LINENO(s);
      filename = T_FILENAME(s);
    }
    va_start(args, fmt);
    display_usr("ERROR", lineno, filename, fmt, args);
    va_end(args);
    __ZPL_USR_EXIT();
  }
}


/* Internal errors */

/* FN: int_fatalx
 * echris - 4-19-97
 */
void int_fatalx(int lineno, char *filename, char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  display_int_fatal(lineno, filename, fmt, args);
  va_end(args);
  __ZPL_INT_EXIT();
}

/* FN: int_fatal
 * echris - 4-19-97
 */
void int_fatal(statement_t *s, char *fmt, ...)
{
  va_list args;
  int lineno = -1;
  char *filename = NULL;
  
  if (s) {
    lineno = T_LINENO(s);
    filename = T_FILENAME(s);
  }
  va_start(args, fmt);
  display_int_fatal(lineno, filename, fmt, args);
  va_end(args);
  __ZPL_INT_EXIT();
}

/* FN: int_cond_fatal
 * echris - 4-19-97
 */
void int_cond_fatal(int c, statement_t *s, char *fmt, ...)
{
  if (!c) {
    va_list args;
    int lineno = -1;
    char *filename = NULL;
  
    if (s) {
      lineno = T_LINENO(s);
      filename = T_FILENAME(s);
    }
    va_start(args, fmt);
    display_int_fatal(lineno, filename, fmt, args);
    va_end(args);
    __ZPL_INT_EXIT();
  }
}


/* helper functions */

/* FN: display_int_fatal
 * echris - 4-19-97
 */
void display_int_fatal(int sourceline, char *sourcefile, char *fmt, 
		       va_list args)
{
  __ZPL_INT_FATAL(file, line);
  if (RELEASE_VER)
    fprintf(stderr, "  Please contact zpl-info@cs.washington.edu\n\n");
  else {
    fprintf(stderr, "  In \"%s\" (%d): ", file, line);
    vfprintf(stderr, fmt, args);

    if ((sourcefile != NULL) && (sourceline>=0)) {
      fprintf(stderr, " (%s:%d)", sourcefile, sourceline);
    } else if (sourcefile != NULL) {
      fprintf(stderr, " (%s)", sourcefile);
    } else if (sourceline>=0) {
      fprintf(stderr, " (line %d)", sourceline);
    }
    fprintf(stderr, "\n\n");
  }
}

/* FN: display_usr
 * echris - 4-19-97
 */
void display_usr(char *kind, int sourceline, char *sourcefile, char *fmt, 
		 va_list args)
{
  char str[200];
  char *str1, *str2;
  char tmp[200];
  int i, j;

  vsprintf(str, fmt, args);

  /* remove OVERLOAD mangled names, replace with original name */
  str1 = strstr(str, "_ZPL");
  str2 = strstr(str, "OVERLOAD_");
  while (str1 != NULL && str2 != NULL) {
    str2 += 9 * sizeof(char);
    strcpy(tmp, str2);
    strcpy(str1, tmp);
    str1 = strstr(str, "_ZPL");
    str2 = strstr(str, "OVERLOAD_");
  }

  /* replace % with %%, the %% changed to % when we printed to a string */
  for (i = 0; i < 200; i++) {
    if (str[i] == '\0') {
      break;
    }
    if (str[i] == '%') {
      for (j = 200 - 1; j > i; j--) {
	str[j] = str[j - 1];
      }
      i++;
    }
  }

  fprintf(stderr, "%s: ", kind);
  fprintf(stderr, str);
  if ((sourcefile != NULL) && (sourceline>=0)) {
    fprintf(stderr, " (%s:%d)", sourcefile, sourceline);
  } else if (sourcefile != NULL) {
    fprintf(stderr, " (%s)", sourcefile);
  } else if (sourceline>=0) {
    fprintf(stderr, " (line %d)", sourceline);
  }
  fprintf(stderr,"\n");
}
