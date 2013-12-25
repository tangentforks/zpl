/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef SCAN_H_
#define SCAN_H_

extern char  prevline[MAXBUFF];
extern char  currline[MAXBUFF];
extern int   prevlineno;
extern int   column;
extern int   ntokens;
extern int   useBuffer;

int GETC(FILE *f);
void UNGETC(char c, FILE *f);

#define FirstTime (MAXBUFF+1)


#define ZPLprefix    "_zpl_"


/* from scan.c */

int is_reserved_C(char*);
void initialize_lexer(void);
int eatleftovers(int);
int yylex(void);
void eatline(void);
int eatwhite(void);
void eatcomment(void);
int eatident(int);
int eatstring(void);
int eatcharacter(void);
int eatnumber(int);


/* from parser.c */

int yyparse(void);
extern int yychar;

#endif
