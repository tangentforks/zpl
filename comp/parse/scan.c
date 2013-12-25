/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/error.h"
#include "../include/const.h"
#include "../include/global.h"
#include "../include/cglobal.h"
#include "../include/macros.h"
#include "../include/struct.h"
#include "../include/parsetree.h"
#include "../include/symtab.h"
#include "../include/parser.h"
#include "../include/symmac.h"
#include "../include/scan.h"
#include "../include/db.h"

#define TOOPS	0

#define isodigit(x) ((x)>='0' && (x)<='7')

static int numkeys;
static int numCwords;

/* The following keywords must be sorted and their positions must
 * correspond to those in the keymap array.
 */

/* Again, to repeat: they *must* appear in alphabetical order, or
 * you'll get really confused...
 */

char	*keywords[] = {
	"array", 	/* zpl */
	"at", 		/* zpl */
	"band",
	"begin",	/* zpl */
	"block",
	"boolean",	/* zpl */
	"bor",
	"bread",
	"bwrite",
	"bxor",
	"by", 		/* zpl */
	"char",
	"complex",
	"config",	/* zpl */
	"const",	/* zpl */
	"constant",
	"continue",
	"cyclic",
	"dcomplex",
	"direction",	/* zpl */
	"distribution",       /* zpl */
	"do",
	"double",
	"downto",	/* zpl */
	"else",
	"elsif",	/* zpl */
	"end",		/* zpl */
	"exit", 	/* zpl */
	"extern",       /* zpl */
	"file",		/* zpl */
	"float",
	"for",
	"free",
	"generic",     /* zpl */
	"genericensemble", /* zpl */
	"grid",         /* zpl */
	"halt",		/* zpl */
	"if",
	"in",		/* zpl */
	"inout",
	"integer",	/* zpl: int -> integer */
        "interleave",   /* zpl */
	"irregular",
	"longint",	/* zpl: long -> longint */
	"max",		/* zpl */
	"min",		/* zpl */
	"multiblock",
	"nondist",
	"of",		/* zpl */
	"opaque",       /* zpl */
	"out",
	"procedure",    /* zpl */
	"program", 	/* zpl */
	"prototype", 	/* zpl */
	"qcomplex",
	"quad", 	/* zpl */
	"read",		/* zpl */
	"record",	/* zpl: struct -> record */
	"reflect",	/* zpl */
	"region",	/* zpl */
	"repeat", 	/* zpl */
	"return",
	"sbyte",        /* zpl */
	"scaledby",    /* zpl */
	"scan",         /* zpl */
	"shortint",	/* zpl: short -> shortint */
	"static",
	"string",	/* zpl */
	"swith",
	"swithout",
	"then",		/* zpl */
	"to",		/* zpl */
	"type",		/* zpl */
	"ubyte",	/* zpl */
	"uinteger",	/* zpl */
	"ulongint",	/* zpl */
	"union",
	"until",	/* zpl */
	"ushortint",	/* zpl */	
	"var",		/* zpl */
	"while",
	"with",
	"without",
	"wrap",		/* zpl */
	"write",	/* zpl */
	"writeln",	/* zpl */
	(char*) 0
};

/* these are tokens that are unknown to the parser as they are only used
   to grab out scans/reduces specified using strings. */

#define TMAX  -9999
#define TMIN  -9998
#define TBAND -9997
#define TBOR  -9996
#define TBXOR -9995

int	keymap[] = {
	TARRAY,		/* zpl */
	TREGIONAT,	/* zpl */
	TBAND,
	TBEGIN,		/* zpl */
	TBLOCK,
	TBOOLEAN,	/* zpl */
	TBOR,
	TBREAD,
	TBWRITE,
	TBXOR,
	TBY,		/* zpl */
	TCHAR,
	TCOMPLEX,
	TCONFIG,	/* zpl */
	TCONST, 
	TCONST,
	TCONTINUE,
	TCYCLIC,
	TDCOMPLEX,
	TDIRECTION,	/* zpl */
	TDISTRIBUTION,
	TDO,
	TDOUBLE,
	TDOWNTO,	/* zpl */
	TELSE,
	TELSIF,		/* zpl */
	TEND,		/* zpl */
	TEXIT,		/* zpl */
	TEXTERN,        /* zpl */
	TFILE,		/* zpl */
	TFLOAT,
	TFOR,
	TFREE,
	TGENERIC,       /* zpl */
	TGENERICENSEMBLE, /* zpl */
	TGRID,
	THALT,		/* zpl */
	TIF,
	TIN,		/* zpl */
	TINOUT,
	TINT,
        TMSCAN,
	TIRREGULAR,
	TLONG,
	TMAX,		/* zpl: Should never return this as a token */
	TMIN,		/* zpl: Should never return this as a token */
	TMULTIBLOCK,
	TNONDIST,
	TOF,		/* zpl */
	TOPAQUE,        /* zpl */
	TOUT,
	TPROCEDURE, 	/* zpl */
	TPROGRAM,	/* zpl */
	TPROTOTYPE,	/* zpl */
	TQCOMPLEX,
	TQUAD,		/* zpl */
	TREAD,		/* zpl */
	TRECORD,
	TREFLECT,	/* zpl */
	TREGION,	/* zpl */
	TREPEAT,	/* zpl */
	TRETURN,
	TSBYTE,         /* zpl */
	TSCALEDBY,
	TMSCAN,         /* zpl */
	TSHORT,
        TSTATIC,
	TSTRING,
	TSWITH,
	TSWITHOUT,
	TTHEN,		/* zpl */
	TTO,		/* zpl */
	TTYPE, 		/* zpl */
	TUNSBYTE, 	/* zpl */
	TUNSINT, 	/* zpl */
	TUNSLONG, 	/* zpl */
	TUNION,
	TUNTIL,		/* zpl */
	TUNSSHORT, 	/* zpl */
	TVAR,		/* zpl */
	TWHILE,
	TWITH,
	TWITHOUT,
	TWRAP,		/* zpl */
	TWRITE,		/* zpl */
	TWRITELN,	/* zpl */
	0
};

char	*Cwords[] = {
	"auto",
	"break",
	"case",
	"default",
	"enum",
	"extern",
	"goto",
	"int",
	"long",
	"main",
	"register",
	"short",
	"signed",
	"sizeof",
	"struct",
	"switch",
	"typedef",
	"union",
	"void",
	"volatile",
	0
};


static int inbrackets=0;


/* initialize_lexer()
 *    Initialize 'numkeys' from the keywords[] and keymap[] array.
 * 
 *    linc -- 4/22/93
 */
void initialize_lexer() {
  int count;

  /* Count number of keywords in keywords[] and keymap[].
   * Make sure these numbers agree.
   */

  numkeys = 0;
  while (keywords[numkeys] != 0) {
    if (numkeys > 0) {
      if (strcmp(keywords[numkeys-1],keywords[numkeys]) >= 0) {
	INT_FATAL(NULL,"Lexer error: keywords do not appear alphabetically");
      }
    }
    numkeys++;
  }

  count = 0;
  while (keymap[count] != 0)
    count++;

  if (count != numkeys) {
    INT_FATAL(NULL,"Lexer error: # of keywords does not match # of keymaps");
  }

  numCwords = 0;
  while (Cwords[numCwords] != 0)
    numCwords++;

  /* initialization for GETC() and UNGETC() */
  ntokens = 0;
  column = FirstTime;
  useBuffer = 0;
}


int eatwhite() {
  int ch;
  int ate=0;

  DB0(90, "eatwhite(scan.c):");

  while (isspace((ch = GETC(fname)))) {
    ate = 1;
    if (ch == '\n') {
      if (firstsemi != 1) yylineno++;
    }
  }
  UNGETC(ch, fname);

  DB2(90, "nextchar = %d(%c)\n", ch, ch);

  return ate;
}


void eatline() {
  int	ch;

  DB0(90, "eatline(scan.c):");

  /* eat up remainder of this line */
  while ((ch = GETC(fname)) != '\n' && ch != EOF)
    ;

  DB2(90, "nextchar = %d(%c)\n", ch, ch);
}


void eatcomment() {
  int ch;

  DB0(90, "eatcomment(scan.c):");
  ch = GETC(fname);
  do {
    if (ch == '\n') {
      if (firstsemi != 1) yylineno++;
    } else if (ch == '*') {
      if ((ch = GETC(fname)) == '/') {
	return;
      } else {
	UNGETC(ch, fname);
      }
    }
  } while ((ch = GETC(fname)) != EOF);
  USR_FATALX(yylineno, in_file, "EOF reached searching for closing comment");
}

int is_reserved_C(char *s) {
  int i;

  for (i=0; i<numCwords; i++) {
    if (!strcmp(s, Cwords[i])) {
      return TRUE;
    }
  }
  return FALSE;
}

int eatident(int ch) {
  int j;
  int i = 1;
  int k;
  symboltable_t *pst;
  int scantok;
  int redtok;

  DB0(60, "eatident(scan.c):\n");
  buffer[0] = ch;
  ch = GETC(fname);
  while (isalnum(ch) || ch == '_') {
    buffer[i++] = ch;
    ch = GETC(fname);
  }
  buffer[i] = '\0';
  UNGETC(ch, fname);
  j = 0;
  while (j < numkeys) {
    k = strcmp(buffer, keywords[j]);
    if (k == 0) {
      /* 
       * Add switch statement to process reductions and scans.
       *	linc -- 4/22/93 
       */
      switch (keymap[j]) {
      case TBAND:
	scantok = TBANDSCAN;
	redtok = TBANDREDUCE;
	break;
      case TBOR:
	scantok = TBORSCAN;
	redtok = TBORREDUCE;
	break;
      case TBXOR:
	scantok = TBXORSCAN;
	redtok = TBXORREDUCE;
	break;
      case TMAX:
	scantok = TMAXSCAN;
	redtok = TMAXREDUCE;
	break;
      case TMIN:
	scantok = TMINSCAN;
	redtok = TMINREDUCE;
	break;
      default:
	break;
      }

      switch(keymap[j]) {
      case TBAND:
      case TBOR:
      case TBXOR:
      case TMAX:
      case TMIN:
	ch = GETC(fname);
	if (ch == '<') {
	  ch = GETC(fname);
	  if (ch == '<') {
	    buffer[i++] = ch;
	    buffer[i++] = ch;
	    buffer[i] = '\0';
	    return redtok;
	  } else {
	    UNGETC (ch, fname);
	    j++;
	    continue;
	  }
	} else if (ch == '|') {  
	  ch = GETC(fname);
	  if (ch == '|') {
	    buffer[i++] = ch;
	    buffer[i++] = ch;
	    buffer[i] = '\0';
	    return scantok;
	  } else {
	    UNGETC (ch, fname);
	    j++;
	    continue;
	  }
	} else {
	  UNGETC (ch, fname);
	  /* never return these tokens as tokens -- treat as idents */
	  j++;
	  continue;
	}
      default:
	break;
      }

      return (keymap[j]);
    }
    if (k < 0)
      break;
    j++;
  }
  /* Check that the user's identifier is not a reserved word in C.
   * If it is, prepend "_zpl_" to the name.
   *
   * If this code ever changes, the code in zpl.y that checks for
   * things like "struct" or "long" will need to as well...
   *                -brad 6/24/96
   */
  
  if (is_reserved_C(buffer)) {
    yylval.str = (char *)PMALLOC((strlen(buffer)+strlen(ZPLprefix)+1)
				 *sizeof(char));
    strcpy(yylval.str, ZPLprefix);
    strcat(yylval.str, buffer);
    pst = lu(yylval.str);
    last_ident = pst;
    if (pst == NULL) {
      DB0(60, "eatident(scan.c): return TIDENT\n");
      return TIDENT;
    }
    if (S_CLASS(pst) == S_TYPE) {
      DB0(60, "eatident(scan.c): return TTYPEIDENT\n");
      return TTYPEIDENT;
    }
    return TIDENT;
  }
  
  yylval.str = (char *)PMALLOC(strlen(buffer)+1);
  strcpy(yylval.str, buffer);
  DB1(60, "eatident(scan.c): calling lu with %s\n", yylval.str);
  pst = lu( yylval.str);
  last_ident = pst;
  if (pst == NULL) {
    DB0(60, "eatident(scan.c): return TIDENT\n");
    return TIDENT;
  }
  if (S_CLASS(pst) == S_TYPE) {
    DB0(60, "eatident(scan.c): return TTYPEIDENT\n");
    return TTYPEIDENT;
  }
  if (S_CLASS(pst) == S_SUBPROGRAM) {
    ch = GETC(fname);
    if (ch == '<') {
      ch = GETC(fname);
      if (ch == '<') {
	buffer[i++] = ch;
	buffer[i++] = ch;
	buffer[i] = '\0';
	return TUSRREDUCE;
      } else {
	UNGETC (ch, fname);
	UNGETC (ch, fname);
      }
    } else if (ch == '|') {  
      ch = GETC(fname);
      if (ch == '|') {
	buffer[i++] = ch;
	buffer[i++] = ch;
	buffer[i] = '\0';
	return TUSRSCAN;
      } else {
	UNGETC (ch, fname);
	UNGETC (ch, fname);
      }
    } else {
      UNGETC (ch, fname);
    }	
  }
  return TIDENT;
}


int eatnumber(int ch) {
  int fint = TRUE;
  int fhex = FALSE;
  int foct = FALSE;
  int fexp = FALSE;
  int keepgoing = TRUE;
  int i = 0;
  int nonoctalfound = FALSE;

  DB0(90, "eatnumber(scan.c):\n");
  buffer[i++] = ch;
  /* If the first character is a DOT we know we have a real number */
  if (ch == '.') {
    fint = FALSE;
  }
  if (ch == '0') {
    if ((ch = GETC(fname)) == 'x' || ch == 'X') {
      fhex = TRUE;
      buffer[i++] = 'x';
    } else {
      foct = TRUE;
      UNGETC(ch, fname);
    }
  }
  do {
    ch = GETC(fname);
    if (isdigit(ch) || (fhex && isxdigit(ch))) {
      if (foct && !isodigit(ch)) {
	/* can't flag error here, as we may be
	   parsing a float or double.  Check at
	   bottom */
	nonoctalfound = TRUE;
      }
      buffer[i++] = ch;
    } else if (fint && (ch == 'l' || ch == 'L')) {
      buffer[i++] = 'l';
      buffer[i] = '\0';
      yylval.i = atoi(buffer);
      return TINTEGER;
      /* The scanner simply returns TINTEGER for all
       * integer types.  The routine build_int() will 
       * differentiate longs and integers.
       */
    } else if (!fint && (ch == 'l' || ch == 'L')) {
      /* we have a double const */
      buffer[i++] = 'l';
      buffer[i] = '\0';
      yylval.r = atof(buffer);
      return TFLOAT;
      /* The scanner returns TFLOAT for all floating
       * point types.  The routine build_int() will 
       * differentiate reals and doubles.
       */
    } else if (fhex && (ch == '.')) {
      UNGETC(ch, fname);
      buffer[i] = '\0';
      yylval.i = atoi(buffer);
      return TINTEGER;
    } else if (fint && (ch == '.')) {
      ch = GETC(fname);
      if (ch == '.') {
	/* special case for ".." following a number */
	UNGETC(ch, fname);
	UNGETC(ch, fname);
	break;	/* exit do loop-- return the int */
      }
      UNGETC(ch, fname);
      buffer[i++] = '.';
      fint = FALSE;
      
    } else if (!fexp && (ch == 'e' || ch =='E')) {
      fint = FALSE;
      fexp = TRUE;
      buffer[i++] = ch;
      if ((ch = GETC(fname)) == '+' || ch == '-') {
	buffer[i++] = ch;
      } else
	UNGETC(ch, fname);
    } else {
      UNGETC(ch, fname);
      keepgoing = FALSE;
    }
  } while (keepgoing);
  if (foct && fint && nonoctalfound) {
    YY_FATAL_CONTX(yylineno, in_file, "Non-octal digit used in octal "
		   "const");
  }
  buffer[i] = '\0';
  
  if (fint || fhex) {
    yylval.i = atoi(buffer);
    return TINTEGER;
  } else {
    yylval.r = atof(buffer);
    return TFLOAT;
  }
}


int eatstring() {
  int ch;
  int i = 0;

  DB0(90, "eatstring(scan.c):\n");

  if (inbrackets) {
    return TQUOTE;
  }

  do {
    while ((ch = GETC(fname)) != EOF && ch != '\\' 
	   && ch != '\"' && ch != '\n') {
      if (i < MAXBUFF) {
	buffer[i++] = ch;
      }
    }
    if (ch == EOF) {
      USR_FATALX(yylineno, in_file, "EOF reached searching for closing quote");
    } else if (ch == '\n') {
      if (firstsemi != 1) yylineno++;
      USR_FATALX(yylineno, in_file, "EOL reached before end of string");
      return TOOPS;
    } else if (ch == '\\') {
      if ((ch = GETC(fname)) != '\n' && i < MAXBUFF) {
	buffer[i++] = '\\';
	buffer[i++] = ch;
	ch = '\0';
      } else if (ch == '\n') {
	if (firstsemi != 1) yylineno++;
      }
    }
  } while (ch != '"');
  buffer[i] = '\0';
  if (i == MAXBUFF){
    INT_FATALX(yylineno, in_file, "maximum length exceeded, string truncated");
  }
  yylval.str = (char *)PMALLOC(strlen(buffer)+1);
  strcpy(yylval.str, buffer);
  return TSTR;
}


int eatcharacter() {
  int ch;
  int i = 0;

  DB0(90, "eatcharacter(scan.c):\n");
  buffer[i++] = '\'';
  ch = GETC(fname);
  buffer[i++] = ch;
  if (ch == '\'') {
    if (inbrackets) {
      return TQUOTE;
    } else {
      USR_WARNX(yylineno, in_file, "empty character const");
      buffer[i] = '\0';
      return TOOPS;
    }
  }
  if (ch == '\\') {
    if ((ch = GETC(fname)) == '\'' || ch == '\\' || ch == 'f' ||
	ch == 'r' || ch == 'b' || ch == 'v' ||
	ch == 't' || ch == 'n') {
      buffer[i++] = ch;
    } else if (isdigit(ch)) {
      buffer[i++] = ch;
      while (isdigit((ch = GETC(fname))) && i < 4)
	buffer[i++] = ch;
      UNGETC(ch, fname);
    } else {
      buffer[i-1] = ch;
    }
  }
  buffer[i++] = '\'';
  buffer[i] = '\0';
  ch = GETC(fname);
  if (ch != '\'') {
    UNGETC(ch,fname);
    i-=2;
    while (i > 0) {
      UNGETC(buffer[i--],fname);
    }
    return TPRIME;
  }
  yylval.ch = buffer[0];

  return TCHARACTER;
}


int eatleftovers(int ch) {
  int i = 0;		
  int token;
  int ch2;

  DB0(90, "eatleftovers(scan.c):\n");
  buffer[i++] = ch;
  switch (ch) {

  case '=':
    token = TEQUAL;
    if ((ch = GETC(fname)) == '=') {
      USR_FATALX(yylineno, in_file, "Found illegal token: '=='");
    }
    UNGETC(ch, fname);
    break;

  case ':':
    token = TCOLON;
    if ((ch = GETC(fname)) == '=') {
      token = TASSIGN;
      buffer[i++] = ch;
    } else if (ch == ':') { 
      token = TRGRID;
      buffer[i++] = ch;
    } else {
      UNGETC(ch, fname);
    }
    break;

  case ',':
    token = TCOMMA;
    break;

  case '.':
    token = TDOT;
    if ((ch = GETC(fname)) == '.') {
      token = TDOTDOT;
      buffer[i++] = ch;
    } else {
      UNGETC (ch, fname);
    }
    break;

  case ';':	
    if (firstsemi == 2 && tokenize == FALSE) {
      char *name;

      name = malloc(256);
      sprintf(name, "%s/include/stdzpl.z", getenv("ZPLHOME"));
      fnamesave = fname;
      if ((fname = fopen(name, "r")) == NULL) {
	USR_FATAL(NULL, "can't retrieve standard context: %s", name);
      }
      firstsemi = 1;
      yylineno++;
    }
    return TSEMI;

  case '(':	
    return TLP;

  case ')':	
    return TRP;

  case '{':	
    return TLCBR;

  case '}':	
    return TRCBR;

  case '[':
    inbrackets++;
    return TLSBR;

  case ']':
    inbrackets--;
    return TRSBR;

  case '@':
    token = TAT;		/* linc -- 4/22/93 */
    ch = GETC(fname);
    if (ch == '^') {
      token = TWRAPAT;
      buffer[i++] = ch;
    } else {
      UNGETC(ch,fname);
    }
    break;
    
  case '*':
    token = TSTAR;
    /* 
     * check for *<< and *||
     *	linc -- 4/22/93
     *      brad -- 7/03/96
     */
    ch = GETC(fname);
    if (ch == '=') {
      token = TASSMULT;
      buffer[i++] = ch;
    } else if (ch == '<') {
      if ((ch2 = GETC(fname)) == '<') {
	token = TTIMESREDUCE;
	buffer[i++] = ch;
	buffer[i++] = ch2;
      } else {
	UNGETC(ch2, fname); 
	UNGETC(ch, fname);
      }
    } else if (ch == '|') {
      if ((ch2 = GETC(fname)) == '|') {
	token = TTIMESSCAN;
	buffer[i++] = ch;
	buffer[i++] = ch2;
      } else {
	UNGETC(ch2, fname);
	UNGETC(ch, fname);
      }
    } else {
      UNGETC(ch, fname);
    }
    break;

  case '&':
    token = TAND;
    
    /* 
     * check for &= &<< &||
     *	linc -- 4/22/93 
     *    brad -- 7/03/96
     */
    ch = GETC(fname);
    if (ch == '=') {				/* &= */
      token = TASSAND;
      buffer[i++] = ch;
    } else if (ch == '<') {
      if ((ch2 = GETC(fname)) == '<') {	/* &<< */
	token = TANDREDUCE;
	buffer[i++] = ch;
	buffer[i++] = ch2;
      } else {				/* &\ */
	UNGETC(ch2, fname);
	UNGETC(ch, fname);
      }
    } else if (ch == '|') {
      if ((ch2 = GETC(fname)) == '|') {
	token = TANDSCAN;
	buffer[i++] = ch;
	buffer[i++] = ch2;
      } else {
	UNGETC(ch2, fname);
	UNGETC(ch, fname);
      }
    } else {
      UNGETC(ch, fname);
    }
    break;
    
  case '+':
    token = TPLUS;
    /* 
     * check for +<< and +||
     *	linc -- 4/22/93
     *      brad -- 7/03/96
     */
    ch = GETC(fname);
    if (ch == '=') {
      token = TASSPLUS;
      buffer[i++] = ch;
    } else if (ch == '<') {
      if ((ch2 = GETC(fname)) == '<') {
	token = TPLUSREDUCE;
	buffer[i++] = ch;
	buffer[i++] = ch2;
      } else {
	UNGETC(ch2, fname);
	UNGETC(ch, fname);
      }
    } else if (ch == '|') {
      if ((ch2 = GETC(fname)) == '|') {
	token = TPLUSSCAN;
	buffer[i++] = ch;
	buffer[i++] = ch2;
      } else {
	UNGETC(ch2, fname);
	UNGETC(ch, fname);
      }
    } else {
      UNGETC(ch, fname);
    }
    break;
    
  case '-':
    token = TMINUS;
    
    if ((ch = GETC(fname)) == '=') {
      token = TASSMINUS;
      buffer[i++] = ch;
    } else  {
      UNGETC(ch, fname);
    }
    break;

  case '!':
    token = TNOT;
    if ((ch = GETC(fname)) == '=') {
      token = TNOTEQUAL;
      buffer[i++] = ch;
    } else {
      UNGETC(ch, fname);
    }
    break;

  case '/':
    token = TDIVIDE;
    if ((ch = GETC(fname)) == '=') {
      token = TASSDIVIDE;
      buffer[i++] = ch;
    } else {
      UNGETC(ch, fname);
    }
    break;

  case '%':
    token = TMODULUS;
    if ((ch = GETC(fname)) == '=') {
      token = TASSMODULUS;
      buffer[i++] = ch;
    } else {
      UNGETC(ch, fname);
    }
    break;

  case '^':
    token = TEXP;
    break;
    
  case '|':
    token = TOR;
    
    /* 
     * check for |= |<< |||
     *	linc -- 4/22/93 
     */
    ch = GETC(fname);
    if (ch == '=') {				/* |= */
      token = TASSOR;
      buffer[i++] = ch;
    } else if (ch == '<') {
      if ((ch2 = GETC(fname)) == '<') {	/* |<< */
	token = TORREDUCE;
	buffer[i++] = ch;
	buffer[i++] = ch2;
      } else {
	UNGETC(ch2, fname);
	UNGETC(ch, fname);
      }
    } else if (ch == '|') {
      if ((ch2 = GETC(fname)) == '|') {	/* ||| */
	token = TORSCAN;
	buffer[i++] = ch;
	buffer[i++] = ch2;
      } else {
	UNGETC(ch2, fname);
	UNGETC(ch, fname);
      }
    } else {
      UNGETC(ch,fname);
    }
    break;

  case '#':
    token = TPERMUTE;
    break;
    
  case '>':
    token = TGREATER;
    if ((ch = GETC(fname)) == '=') {
      token = TGREATEREQUAL;
      buffer[i++] = ch;
    } else if (ch == '>') {
      token = TFLOOD;
      buffer[i++] = ch;
    } else {
      UNGETC(ch, fname);
    }
    break;

  case '<':
      token = TLESS;
      if ((ch = GETC(fname)) == '=') {
	  if ((ch2 = GETC(fname)) == '=') {
	      token = TDESTROY;
	      buffer[i++] = ch;
	      buffer[i++] = ch2;
	  } else if (ch2 == '#') {
	      token = TPRESERVE;
	      buffer[i++] = ch;
	      buffer[i++] = ch2;
	  } else {
	      UNGETC(ch2, fname);
	      token = TLESSEQUAL;
	      buffer[i++] = ch;
	  }
      } else {
	  UNGETC(ch, fname);
      }
      break;

  case '?':
    token = TQUEST;
    break;

  default :
    YY_FATALX(yylineno, in_file, "unrecognized character %c", ch);
    token = -1;
  }
  buffer[i] = '\0';

  return token;
}
