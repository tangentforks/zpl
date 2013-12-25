/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/***************************************************************************
	Center for Supercomputing Research and Development
	at the University of Illinois at Urbana-Champaign
		305 Talbot Laboratory
		104 South Wright Street
		Urbana, Illinois  61801-2932 USA

Write to the above address for information on the CSRD Software Catalog,
or send e-mail to:  softcat@csrd.uiuc.edu

Copyright (c) 1991 by the Board of Trustees of the University of Illinois
*unpublished*.  All rights reserved.  This proprietary created material is
provided under a license agreement which restricts its use and protects its
confidentiality.  No publication or waiver of confidentiality is implied by
this transfer under the license.
***************************************************************************/




/*** SEE include/parsetree.h ***/
/*** The stmttypes should correspond directly to this ***/
/*** ANY CHANGES MADE HERE MUST BE MADE THERE ALSO ***/
char	*StmtName[] = {
	"Null",
	"ZPL exit",
	"ZPL Ironman comm",	/*** for IRONMAN comm ***/
	"Compound",
	"ZPL mighty scan",
	"Expression",
	"If",
	"Loop",
	"Return",
	"ZPL IO",
	"End",
	"ZPL REGION SCOPE",
	"ZPL MLOOP",
	"ZPL NLOOP",
	"ZPL WRAP",
	"ZPL REFLECT",
	"ZPL halt",
	"ZPL continue",
	"ZPL profiling information"
};

char *CommStmtName[] = {
  "Ironman Pre-Recv",
  "Ironman Send",
  "Ironman Recv",
  "Ironman Post-Send",
  "Ironman Wrap Pre-Recv",
  "Ironman Wrap Send",
  "Ironman Wrap Recv",
  "Ironman Wrap Post-Send"
};

char *OpGetsName[] = {
  "+=",
  "-=",
  "*=",
  "/=",
  "%=",
  "min=",
  "max=",
  "&=",
  "|=",
  "^=",
  "&&=",
  "||="
};


/*** SEE include/parsetree.h and util/expr.c ***/
/*** The exprtypes should correspond directly to this ***/
/*** The expr_priority should correspond directly to this ***/
/*** ANY CHANGES MADE HERE MUST BE MADE IN BOTH THOSE PLACES ALSO ***/
char	*OpName[] = {
	"Null expression",
	"Square Bracket Entity",
	"Variable",
	"Constant",
	"Size",
	"Grid",
	"Distribution",
	"-",
	"+",


	"", /* nudge */

	"?<<",

	"?||",

	">>",

	"?##",

	"!",

	"@",

	"+",
	"-",
	"*",
	"/",
	"%",

	"^",

	"?=",
	/*
	"+=",
	"-=",
	"*=",
	"/=",
	"%=",
	"min=",
	"max=",
	"&=",
	"|=",
	"^=",
	"&=",
	"|=",
	*/

	">",
	"<",
	">=",
	"<=",
	"==",
	"!=",

	"&&",
	"||",

	"=",

	".",

	"Binary Array Reference",
	"Function/Procedure Call"
};
