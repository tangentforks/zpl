/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "../include/const.h"
#include "../include/error.h"
#include "../include/global.h"
#include "../include/macros.h"
#include "../include/parser.h"
#include "../include/parsetree.h"
#include "../include/scan.h"
#include "../include/struct.h"

#define TENDOFFILE 0

/* variables defined in global.h */

char* base_in_file;

int firstsemi = 2; /* when we see the first semicolon, presumably
		      after program, we switch over to reading the
		      standard context, stdzpl.z;
		      firstsemi is 1 while we are reading the standard
		      context and 0 after we are done; 2 before we
		      start but that shouldn't be for long */
FILE	*fname;			
FILE    *fnamesave;
char	buffer[MAXBUFF];	

/* 
 * Variables used by GETC() and UNGETC() for error reporting.
 */
#define FirstTime (MAXBUFF+1)
char  prevline[MAXBUFF]; /* Previous 'significant' line (white space skipped)*/
char  currline[MAXBUFF]; /* Current line for our implementation of GETC() */
int   prevlineno;        /* line number of prevline[] */
int   column;            /* column is the index into currline of the character 
                          * just returned by getc(). */
int   ntokens;           /* number of tokens currently found on currline */
int   useBuffer;         /* ungetc() uses a six character buffer */
static char cbuffer[6];  /*   useBuffer is initially 0 */

int GETC(FILE *f) {
  char *eof; /* NULL => EOF has been reached */

  if (useBuffer > 0) {
    column++;
    return cbuffer[--useBuffer];
  }

  if (column == FirstTime || currline[column] == '\n') {
    /* 
     * Read a new line
     */

    /* clobber prevline only if tokens were found on the current line */
    if (ntokens) {
      strcpy( prevline, currline);
      prevlineno = yylineno;
    }
    eof = fgets( currline, MAXBUFF, f);
    if (strlen(currline) == MAXBUFF-1) {
      YY_FATALX(yylineno, in_file,
                "Input line too long (> %d characters)",MAXBUFF-1);
    }
    if (eof == NULL) {
      return EOF;
    }
    ntokens = 0;
    column = 0;
  } else {
    column++;
  }

  if (currline[column] == '\0') {
    return (EOF);
  }
  return currline[column];
}

void UNGETC(char c, FILE *f)
{
  cbuffer[useBuffer++] = c;
  column--;
}

static int yylex2(void) {
  int ch;
  int chkcomment = FALSE;

yuk_a_goto:
	eatwhite();
	if ((ch = GETC(fname)) == EOF) {
	  if (firstsemi == 1) {
	    firstsemi = 0;
	    fclose(fname);
	    fname = fnamesave;
	    goto yuk_a_goto;
	  }
	  else {
	    return(TENDOFFILE);
	  }
	}
	do {
		if (ch == '/')
			if ((ch = GETC(fname)) == '*') {
				eatcomment();
				eatwhite();
				chkcomment = TRUE;
				if ((ch = GETC(fname)) == EOF)
					return(TENDOFFILE);
			} else {
				UNGETC(ch, fname);
				ch = '/';
				chkcomment = FALSE;
			}
		else if (ch == '-')	/* -- is a comment in our language */
		{
			if ((ch = GETC(fname)) == '-')
			{
				eatline();	 /* ignore rest of line */
				if (firstsemi != 1) yylineno++;
				eatwhite();
				chkcomment = TRUE;
				if ((ch = GETC(fname)) == EOF)
					return(TENDOFFILE);
			}
			else
			{
				UNGETC(ch, fname);
				ch = '-';
				chkcomment = FALSE;
			}
		}
		else
			chkcomment = FALSE;
	} while (chkcomment);
	if (isalpha(ch) || ch == '_')
		return(eatident(ch));
	if (isdigit(ch))
		return(eatnumber(ch));
	if (ch == '"')
		return(eatstring());
	if (ch == '\'') {
	  char ch2;
	  char ch3;
	  char chat;
	  int ate;
	  
	  ch2 = GETC(fname);
	  ch3 = GETC(fname);
	  UNGETC(ch3,fname);
	  UNGETC(ch2,fname);
	  if (ch2 != ' ' || ch3 != '\'') {
	    /* can't eat whitespace if char contains whitespace */
	    ate = eatwhite();
	    chat = GETC(fname);
	    UNGETC(chat,fname);
	    if (chat == '@') {
	      if (ate || (ch2 != '@' && ch3 != '\'')) {
		return TPRIME;
	      }
	    }
	  }
	  return eatcharacter();
	}
	if (ch == '.') {
		if (isdigit(ch = GETC(fname))) {
			UNGETC(ch, fname);
			ch = '.';
			return(eatnumber(ch));
		} else {
			UNGETC(ch, fname);
			ch = '.';
		}
	}
	if (ch == '#') {
	  char ch2;
		eatwhite();
		/* remove "line" if it exists - some vers of cpp have this */
		if ((ch2 = GETC(fname)) == '[') {
		  UNGETC(ch2, fname);
		  return eatleftovers(ch);
		}
		else {
		  UNGETC(ch2, fname);
		}
		if (!isdigit(ch = GETC(fname))) {
			UNGETC(ch, fname);
			yylex2();
			if (!strcmp(buffer,"pragma")) {
			  for (ch = GETC(fname); ch != '\n'; ch = GETC(fname)) ;
			  goto yuk_a_goto;
			}
			if (strcmp(buffer,"line")) {
			  USR_FATALX(yylineno, in_file,
				     "syntax error (appears to be a cpp directive: try -cpp)");
			}
			eatwhite();
		} else {
			UNGETC(ch, fname);
		}

		if (!isdigit(ch = GETC(fname))) {
			UNGETC(ch, fname);
			USR_FATALX(yylineno, in_file,
				   "syntax error (appears to be a cpp directive: use -cpp)");
		} else
			UNGETC(ch, fname);
		yylex2();
		yylineno = atoi(buffer);

		for (ch = GETC(fname); ch != '"' && ch != '\n'; ch = GETC(fname))
			;
		if (ch != '"') {
			goto yuk_a_goto;
		} else
			UNGETC(ch, fname);
		yylex2();

		
		in_file = (char *)PMALLOC((strlen(buffer) + 1)*sizeof(char));
		strcpy(in_file, buffer);
		base_in_file = strrchr(in_file,'/');
		if (base_in_file == NULL) {
		  base_in_file = in_file;
		} else {
		  base_in_file++;
		}
		for (ch = GETC(fname); ch != '\n'; ch = GETC(fname))
			;

		goto yuk_a_goto;
	}
	if (ispunct(ch))
		return(eatleftovers(ch));

	
	buffer[0] = ch; buffer[1] = '\0';

	  if (firstsemi == 1) {
	    firstsemi = 0;
	    fclose(fname);
	    fname = fnamesave;
	    goto yuk_a_goto;
	  }

	return(-1);
}


static void yylextokenizemode(void) {
  int lastlineno=1;
  int token;
  int tottokens=0;
  int numtokens=0;
  int numlines=0;

  printf("1: ");
  while (1) {
    token = yylex2();

    if (token <= 0) {
      printf("(%d)\n", numtokens);
      numlines++;
      tottokens += numtokens;
      printf("# lines: %d\n", numlines);
      printf("# tokens: %d\n", tottokens);
      printf("tokens/line: %f\n", (double)tottokens/numlines);
      exit(0);
    }
    if (yylineno != lastlineno) {
      printf("(%d)\n%d: ", numtokens, yylineno);
      numlines++;
      tottokens += numtokens;
      numtokens = 0;
      lastlineno = yylineno;
    }
    numtokens++;

    /*  These lines can be uncommented to print out the token string -Brad*/
    switch (token) {
    case TIDENT:
      printf("<ident>");
      break;
    case TTYPEIDENT:
      printf("<type>");
      break;
    case TSTRING:
      printf("<string>");
      break;
    case TSTR:
      printf("<str>");
      break;
    case TUSRREDUCE:
      printf("usr<<");
      break;
    case TUSRSCAN:
      printf("usr||");
      break;
    case TARRAY:
      printf("array");
      break;
    case TBEGIN:
      printf("begin");
      break;
    case TMSCAN:
      printf("scan");
      break;
    case TBREAD:
      printf("bread");
      break;
    case TBWRITE:
      printf("bwrite");
      break;
    case TBY:
      printf("by");
      break;
    case TCONFIG:
      printf("config");
      break;
    case TCONST:
      printf("const");
      break;
    case TDIRECTION:
      printf("direction");
      break;
    case TDOWNTO:
      printf("downto");
      break;
    case TEND:
      printf("end");
      break;
    case TEXIT:
      printf("exit");
      break;
    case TFILE:
      printf("file");
      break;
    case THALT:
      printf("halt");
      break;
    case TIN:
      printf("in");
      break;
    case TOF:
      printf("of");
      break;
    case TPROCEDURE:
      printf("procedure");
      break;
    case TPROGRAM:
      printf("program");
      break;
    case TPROTOTYPE:
      printf("prototype");
      break;
    case TREAD:
      printf("read");
      break;
    case TRECORD:
      printf("record");
      break;
    case TREFLECT:
      printf("reflect");
      break;
    case TREGION:
      printf("region");
      break;
    case TREPEAT:
      printf("repeat");
      break;
    case TTHEN:
      printf("then");
      break;
    case TTO:
      printf("to");
      break;
    case TTYPE:
      printf("type");
      break;
    case TUNTIL:
      printf("until");
      break;
    case TVAR:
      printf("var");
      break;
    case TWITH:
      printf("with");
      break;
    case TWITHOUT:
      printf("without");
      break;
    case TSCALEDBY:
      printf("scaledby");
      break;
    case TSWITH:
      printf("swith");
      break;
    case TSWITHOUT:
      printf("swithout");
      break;
    case TWRAP:
      printf("wrap");
      break;
    case TWRITE:
      printf("write");
      break;
    case TWRITELN:
      printf("writeln");
      break;
    case TFREE:
      printf("free");
      break;
    case TBOOLEAN:
      printf("boolean");
      break;
    case TCHAR:
      printf("char");
      break;
    case TCOMPLEX:
      printf("complex");
      break;
    case TCONTINUE:
      printf("continue");
      break;
    case TDCOMPLEX:
      printf("dcomplex");
      break;
    case TDO:
      printf("do");
      break;
    case TDOUBLE:
      printf("double");
      break;
    case TQUAD:
      printf("quad");
      break;
    case TGENERIC:
      printf("generic");
      break;
    case TGENERICENSEMBLE:
      printf("generic ensemble");
      break;
    case TELSE:
      printf("else");
      break;
    case TELSIF:
      printf("elsif");
      break;
    case TFOR:
      printf("for");
      break;
    case TIF:
      printf("if");
      break;
    case TINT:
      printf("integer");
      break;
    case TLONG:
      printf("longint");
      break;
    case TOPAQUE:
      printf("opaque");
      break;
    case TQCOMPLEX:
      printf("qcomplex");
      break;
    case TRETURN:
      printf("return");
      break;
    case TSBYTE:
      printf("sbyte");
      break;
    case TSHORT:
      printf("short");
      break;
    case TUNION:
      printf("union");
      break;
    case TUNSBYTE:
      printf("ubyte");
      break;
    case TUNSINT:
      printf("uinteger");
      break;
    case TUNSSHORT:
      printf("ushortint");
      break;
    case TUNSLONG:
      printf("ulongint");
      break;
    case TWHILE:
      printf("while");
      break;
    case TQUEST:
      printf("?");
      break;
    case TDISTRIBUTION:
      printf("distribution");
      break;
    case TGRID:
      printf("grid");
      break;
    case TQUOTE:
      printf("\"");
      break;
    case TSEMI:
      printf(";");
      break;
    case TLP:
      printf("(");
      break;
    case TRP:
      printf(")");
      break;
    case TLCBR:
      printf("{");
      break;
    case TRCBR:
      printf("}");
      break;
    case TLSBR:
      printf("[");
      break;
    case TRSBR:
      printf("]");
      break;
    case TNOT:
      printf("!");
      break;
    case TCOMMA:
      printf(",");
      break;
    case TINTEGER:
      printf("integer");
      break;
    case TFLOAT:
      printf("float");
      break;
    case TCHARACTER:
      printf("character");
      break;
    case TEXTERN:
      printf("extern");
      break;
    case TASSIGN:
      printf(":=");
      break;
    case TASSPLUS:
      printf("+=");
      break;
    case TASSMINUS:
      printf("-=");
      break;
    case TASSMULT:
      printf("*=");
      break;
    case TASSDIVIDE:
      printf("/=");
      break;
    case TASSMODULUS:
      printf("%%=");
      break;
    case TASSAND:
      printf("&=");
      break;
    case TASSOR:
      printf("|=");
      break;
    case TCOLON:
      printf(":");
      break;
    case TDOTDOT:
      printf("..");
      break;
    case TAND:
      printf("&");
      break;
    case TOR:
      printf("|");
      break;
    case TLESS:
      printf("<");
      break;
    case TGREATER:
      printf(">");
      break;
    case TLESSEQUAL:
      printf("<=");
      break;
    case TGREATEREQUAL:
      printf(">=");
      break;
    case TEQUAL:
      printf("=");
      break;
    case TNOTEQUAL:
      printf("!=");
      break;
    case TPLUS:
      printf("+");
      break;
    case TMINUS:
      printf("-");
      break;
    case TSTAR:
      printf("*");
      break;
    case TDIVIDE:
      printf("/");
      break;
    case TMODULUS:
      printf("%%");
      break;
    case TEXP:
      printf("^");
      break;
    case TANDREDUCE:
      printf("&<<");
      break;
    case TMAXREDUCE:
      printf("max<<");
      break;
    case TMINREDUCE:
      printf("min<<");
      break;
    case TORREDUCE:
      printf("|<<");
      break;
    case TPLUSREDUCE:
      printf("+<<");
      break;
    case TTIMESREDUCE:
      printf("*<<");
      break;
    case TANDSCAN:
      printf("&||");
      break;
    case TMAXSCAN:
      printf("max||");
      break;
    case TMINSCAN:
      printf("min||");
      break;
    case TORSCAN:
      printf("or||");
      break;
    case TPLUSSCAN:
      printf("+||");
      break;
    case TTIMESSCAN:
      printf("*||");
      break;
    case TBORREDUCE:
      printf("bor<<");
      break;
    case TBANDREDUCE:
      printf("band<<");
      break;
    case TBXORREDUCE:
      printf("bxor<<");
      break;
    case TBORSCAN:
      printf("bor||");
      break;
    case TBANDSCAN:
      printf("band||");
      break;
    case TBXORSCAN:
      printf("bxor||");
      break;
    case TFLOOD:
      printf(">>");
      break;
    case TPERMUTE:
      printf("#");
      break;
    case TREDUCE:
      printf("<<");
      break;
    case TUNARY:
      printf("<\?\?\?>");
      break;
    case TAT:
      printf("at");
      break;
    case TWRAPAT:
      printf("wrapat");
      break;
    case TPRIME:
      printf("'");
      break;
    case TDOT:
      printf(".");
      break;
    case TREGIONAT:
      printf("at");
      break;
    case TRGRID:
      printf("::");
      break;
    default:
      printf("<\?\?\?: %d>", token);
      break;
    }
    printf(" ");
  }
}


int yylex()
{
  int token;

  if (tokenize) {
    yylextokenizemode();
  }

  token = yylex2();

  return token;
}
