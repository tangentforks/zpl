/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/beautify.h"
#include "../include/error.h"
#include "../include/macros.h"
#include "../include/generic_stack.h"
#include "../include/global.h"

#define ZLINEFORMAT "# %d \"%s\"\n"
#define ZLINEINPUT "/* ZLINE: "
#define ZLINEINPUTFORMAT "/* ZLINE: %d %s"
#define ZLINEINPUTLEN strlen(ZLINEINPUT)
#define ZCMAXMAPSIZE 4096

static stack *justification;
static int depth = 0;
static int parens = 0;
static int justify = 0;
static int inquote = FALSE;
static int intick = FALSE;
static int escaped = FALSE;

static void update_state(char *line) {
  int * oldstuff;	/* characters up to the last open paren */
  int stuff;		/* characters since last open paren */
  char *cp;
  int oldoldstuff;

  oldstuff = (int *)malloc(sizeof(int));
  *oldstuff = justify;
  stuff = 0;
  cp = line;
  escaped = FALSE;

  while (cp[0] != '\0') {
    switch (*cp) {
    case '\\': 
      escaped = !escaped;
      stuff++;
      break;
    case '\'':
      stuff++;
      if (!escaped && !inquote) {
	intick = !intick;
      }
      escaped = FALSE;
      break;
    case '\"':
      stuff++;
      if (!escaped) {
	inquote = !inquote;
      }
      escaped = FALSE;
      break;
    case '{':
      if (!inquote && !intick) {
	if (oldstuff == NULL) {
	  INT_FATAL(NULL,"Unbalanced curly braces:\n\t%s",line);
	}
	*oldstuff = 0;	/* assume all parens have been closed */
	stuff = 0;
	depth++;
      } else {
	stuff++;
      }
      escaped = FALSE;
      break;
    case '}':
      if (!inquote && !intick) {
	if (oldstuff == NULL) {
	  INT_FATAL(NULL,"Unbalanced curly braces:\n\t%s",line);
	}
	*oldstuff = 0;	/* assume all parens have been closed */
	stuff = 0;
	depth--;
      } else {
	stuff++;
      }
      escaped = FALSE;
      break;
    case '(':
      if (!inquote && !intick) {
	g_push(justification, oldstuff);
	if (oldstuff == NULL) {
	  INT_FATAL(NULL,"Unbalanced parentheses:\n\t%s",line);
	}
	oldoldstuff = *oldstuff;
	oldstuff = (int *)malloc(sizeof(int));
	*oldstuff = oldoldstuff + stuff + 1;
	stuff = 0;
	parens++;
      } else {
	stuff++;
      }
      escaped = FALSE;
      break;
    case ')':
      if (!inquote && !intick) {
	free(oldstuff);
	oldstuff = g_pop(justification);
	stuff = 0;
	parens--;
      } else {
	stuff++;
      }
      escaped = FALSE;
      break;
    default:
      stuff++;
      escaped = FALSE;
      break;
    }
    cp++;
  }

  if ((parens == 0) || (oldstuff == NULL)) {
    justify = 0;
  } else {
    justify = *oldstuff;
  }

  free(oldstuff);

}

void beautify(char *infilename,
	      char *outfilename)
{
  char line[1024];
  char *cp;
  FILE *inputfile;
  FILE *outputfile;
  char command[256];
  int i;
  int new_line, indent;
  int zline;
  char zname[1024];
  int current_line;
  int total_lines;
  int old_depth;
  char* znptr;

  zline = -1;

  justification = g_new_stack();

  /* remove the old outfile */
  /* sprintf(command, "rm -f %s", outfilename); */
  /* system(command); */

  inputfile = fopen(infilename, "r");
  if (inputfile == NULL) {
    INT_FATAL(NULL, "Could not open file '%s' in beautify().", infilename);
  }

  outputfile = fopen(outfilename, "w");
  if (outputfile == NULL) {
    USR_FATAL(NULL, "Could not open output file '%s'.", outfilename);
  }

  new_line = TRUE;
  indent = TRUE;
  total_lines = 0;
  current_line = 0;
  while (fgets(line, sizeof(line), inputfile)) {
    cp = line;
    if (new_line == TRUE) {
      indent = TRUE;

      while (isspace((int)(*cp)))		/* remove leading spaces */
	cp++;

      /* record zpl/c source line map info */
      if (!strncmp(cp, ZLINEINPUT, ZLINEINPUTLEN)) {
	sscanf(cp, ZLINEINPUTFORMAT, &zline, zname);
	znptr = strrchr(zname,'/');
	if (znptr != NULL) {
	  strcpy(zname,znptr+1);
	}
	
	total_lines = max(total_lines, zline);
      }
    }
    else {
      indent = FALSE;
    }

    if (cp[strlen(cp)-1] == '\n')
      new_line = TRUE;
    else
      new_line = FALSE;

    switch (cp[0]) {
    case '\0':
      fprintf(outputfile, "\n");	/* output blank line */
	break;
    case '}':
      /*** assumes there is no open curly braces follow on the line ***/
      old_depth = depth;
      update_state(cp);		/* update state first */
      if (print_cpp_lines && (zline >= 0)) {
	fprintf(outputfile, ZLINEFORMAT, zline, zname);
	current_line++;
      }
      for (i = 0; i < 2*(old_depth-1)+justify; i++) {
	fprintf(outputfile, " ");
      }
      fprintf(outputfile, "%s", cp);	/* output line */
      break;
    default:
      if (strncmp(cp, "case", 4) == 0)
	i = 1;
      else
	i = 0;

      if (strncmp(cp, ZLINEINPUT, ZLINEINPUTLEN)) { /* don't output ZLINE comments */
	if (print_cpp_lines && (zline >= 0)) {
	  fprintf(outputfile, ZLINEFORMAT, zline, zname);
	  current_line++;
	}
	if ((indent == TRUE) && (cp[0] != '#'))
	  for (i = 0; i < 2*depth+justify; i++)
	    fprintf(outputfile, " ");
	else {}				/* don't indent pre-processor stmts*/
	fprintf(outputfile, "%s", cp);	/* output line */
      } else {				/* ignore ZLINE lines in line count */
	current_line--;
      }

      update_state(cp);		/* update state */
    }
    current_line++;
  }

  fclose(inputfile);

  /* remove the ugly infile */
  sprintf(command, "rm -f %s", infilename);
  system(command);

  fclose(outputfile);

  if (g_free_stack(justification) == G_ERROR) {
    INT_FATAL(NULL, "Parentheses or curly braces are not balanced in "
	      "codegen.  Ugly code may reside in '%s'", infilename);
  }

}
   

     
