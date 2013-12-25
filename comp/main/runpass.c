/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/times.h>
#include <unistd.h>
#include "../include/error.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/main.h"
#include "../include/passlist.h"
#define __PASSES_C_
#include "../include/passes.h"

/*** get a line from f and put it in *buffer ***/
/*** buffer may get resized, so return the size ***/
static int
zgetline(FILE *f, char **buffer, int size)
{
  char *b;
  char c;
  int new_size;
  int done = FALSE;

  b = *buffer;
  new_size = size;

  while (!done) {
    fgets(b, new_size, f);
    if (strlen(*buffer) < new_size-1) {
      /*** line is shorter that buffer size ***/
      done = TRUE;
    }
    else {
      /*** check if the buffer was exactly the right size ***/

      /*** backup one byte ***/
      fseek(f, -1, SEEK_CUR);

      /*** read that byte ***/
      c = fgetc(f);
      if (c == '\n') {
	done = TRUE;
      }
      else {
	/*** resize the buffer and read in the next new_size+size bytes ***/
	b = &(*buffer)[new_size-1];
	new_size += size;
	if (*buffer != NULL) {
	  *buffer = (char *)realloc(*buffer, new_size);
	} else {
	  *buffer = (char *)PMALLOC(new_size);
	}
	INT_COND_FATAL((*buffer!=NULL), NULL, "Realloc failed in zgetline()");
      }
    }
  }

  return new_size;
}


/*** run a single pass ***/
static void runonepass(module_t *mod, char *call, FILE *passfile)
{
  int i, j;
  char opts[80] = "";
  char name[80];
  char *p, *q;
  struct tms time1,time2;
  int t1, t2;

  sscanf(call, " %s", name);
  for (i = 0;
       (strncmp(name, passes[i].name, strlen(name)) &&
	strcmp(passes[i].name, NO_MORE_PASSES));
       i++) /*** find the pass ***/ ;

  if (!strcmp(passes[i].name, NO_MORE_PASSES)) {
    if (passfile == NULL) {
      INT_FATAL(NULL, "Invalid pass '%s'", name);
    }
    else {
      USR_FATAL(NULL, "Invalid pass '%s'", name);
    }
  }

#ifdef DEBUG
  debug_flag = 0;
#endif

  if (strlen(call) != strlen(passes[i].name)) {
    /*** parse the flags ***/
    p = &(call[strlen(name)]);
    q = opts;
    while ((*p != '\0') && (*p != '-')) p++;
    if (*p != '\0') p++;
    while (*p != '\0') {
      switch (*p) {
      case 'd':
#ifdef DEBUG
	sscanf(p, "d %d", &debug_flag);
#endif
	break;
      default:
	q = strcat(q, " -");
	q = strcat(q, p);
	for (j = 2;
	     (*p != '\0') && !((*p == ' ') && (*(p+1) == '-'));
	     j++)
	  p++;
	q[j] = '\0';
	q = q + j;
	break;
      }
      while ((*p != '\0') && (*p != '-')) p++;
      if (*p != '\0') p++;
    }
  }

  t1 = times(&time1);
  if (quiet == FALSE) {
    printf("%17s  ", call);
    fflush(stdout);
  }

  if (passes[i].fun != NULL)
    (*passes[i].fun)(mod, opts);

  if (quiet == FALSE) {
    t2 = times(&time2);
    printf("[ %8.5f  %8.5f ]",
	   ((float) (time2.tms_utime - time1.tms_utime)/6 ) / 10,
	   ((float) (t2 - t1)/6 ) / 10);
    printf("\n");
    fflush(stdout);
  }

  return;
}


int runpasses(module_t *mod, FILE *passfile)
{
  char *line;
  int size = 80, numpasses;
  char *p, *q;
  int i;

  zstdout = stdout;
  zstdin = stdin;

  if (passfile != NULL) {
    /*** use an external pass file ***/

    line = (char *) PMALLOC(size*sizeof(char));
    strcpy(line, "\n");
    p = line;
    while ((p == NULL) || (strncmp(FIRST, p, strlen(FIRST)))) {
      size = zgetline(passfile, &line, size);
      p = strchr(line, '"');
      if (p == NULL)
	p = line;
      else
	p++;
    }

    numpasses = 1;
    while (strncmp(LAST, p, strlen(LAST))) {
      size = zgetline(passfile, &line, size);
      p = strchr(line, '"');
      if (p == NULL) {
	p = line;
      }
      else {
	p++;
	q = strchr(p, '"');
	if (q == NULL) {
	  USR_FATAL(NULL, "Incorrect pass file format");
	}
	*q = '\0';
	strcpy(passlist[numpasses++], p);
      }
    }
    numpasses--;
  }
  else {
    /*** use the internal pass file ***/
    for (numpasses = 1; strcmp(LAST, passlist[numpasses]); numpasses++)
      /*** count the number of passes ***/
      ;
  }

  for (i = 1; i < numpasses; i++) {
    /*** run each pass ***/
    runonepass(mod, passlist[i], passfile);
  }

  return 0;
}


int get_flag_arg (char *s,char c) {
  if (s == NULL) return (-1);
  while (*s != '\0') {
    while ((*s != '-') && (*s != '\0')) {
      s++;
    }
    if (*s == '\0') {
      return (-1);
    }
    if (s[1] == c) {
      return (0);
    } else if (s[1] != '\0') {
      s+= 2;
    }
  }
  return (-1);
}
