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




#ifndef	__DB_H_
#define	__DB_H_

#include "../include/global.h"
#if (!RELEASE_VER)

#ifndef DEBUG
#define DEBUG
#endif

extern int debug_flag;

#define DBINFO	fprintf(zstdout,"\"%s\"(%d): ", __FILE__, __LINE__)

#define IFDB(level)\
if (debug_flag >= level)


#define DB(level)\
if (debug_flag >= level)

#define DB0(level, mesg)\
if (debug_flag >= level)\
{DBINFO;fprintf(zstdout,mesg);fflush(zstdout);}

#define DBS0(level, mesg)\
if (debug_flag >= level) {fprintf(zstdout,mesg);fflush(zstdout);}

#define DB1(level, mesg, arg)\
if (debug_flag >= level)\
{DBINFO;fprintf(zstdout,mesg,arg);fflush(zstdout);}

#define DBS1(level, mesg, arg)\
if (debug_flag >= level) {fprintf(zstdout,mesg,arg);fflush(zstdout);}

#define DB2(level, mesg, arg1, arg2)\
if (debug_flag >= level)\
{DBINFO; fprintf(zstdout,mesg,arg1,arg2); fflush(zstdout); }

#define DBS2(level, mesg, arg1, arg2)\
if (debug_flag >= level) {fprintf(zstdout,mesg,arg1,arg2); fflush(zstdout); }

#define DB3(level, mesg, arg1, arg2, arg3)\
if (debug_flag >= level)\
{DBINFO;fprintf(zstdout,mesg,arg1,arg2,arg3);fflush(zstdout);}

#define DBS3(level, mesg, arg1, arg2, arg3)\
if (debug_flag >= level)\
{fprintf(zstdout,mesg,arg1,arg2,arg3); fflush(zstdout);}

#define DB4(level, mesg, arg1, arg2, arg3, arg4)\
if (debug_flag >= level)\
{DBINFO;\
fprintf(zstdout,mesg,arg1,arg2,arg3,arg4); fflush(zstdout); }

#define DBS4(level, mesg, arg1, arg2, arg3, arg4)\
if (debug_flag >= level)\
{fprintf(zstdout,mesg,arg1,arg2,arg3,arg4); fflush(zstdout); }

#define DB5(level, mesg, arg1, arg2, arg3, arg4, arg5)\
if (debug_flag >= level)\
{DBINFO; \
fprintf(zstdout,mesg,arg1,arg2,arg3,arg4,arg5); fflush(zstdout); }

#define DBS5(level, mesg, arg1, arg2, arg3, arg4, arg5)\
if (debug_flag >= level)\
{ fprintf(zstdout,mesg,arg1,arg2,arg3,arg4,arg5); fflush(zstdout); }

#define DB6(level, mesg, arg1, arg2, arg3, arg4, arg5, arg6)\
if (debug_flag >= level)\
{DBINFO; \
fprintf(zstdout,mesg,arg1,arg2,arg3,arg4,arg5,arg6); fflush(zstdout); }

#define DBS6(level, mesg, arg1, arg2, arg3, arg4, arg5, arg6)\
if (debug_flag >= level)\
{ fprintf(zstdout,mesg,arg1,arg2,arg3,arg4,arg5,arg6); fflush(zstdout); }

#define DB7(level, mesg, arg1, arg2, arg3, arg4, arg5, arg6, arg7)\
if (debug_flag >= level)\
{DBINFO; \
fprintf(zstdout,mesg,arg1,arg2,arg3,arg4,arg5,arg6,arg7); fflush(zstdout); }

#define DBS7(level, mesg, arg1, arg2, arg3, arg4, arg5, arg6, arg7)\
if (debug_flag >= level)\
{ fprintf(zstdout,mesg,arg1,arg2,arg3,arg4,arg5,arg6,arg7); fflush(zstdout); }

#else

#define IFDB(level)	if (0)
#define DB(level)	if (0)
#define DB0(level, mesg)
#define DB1(level, mesg, arg)
#define DB2(level, mesg, arg1, arg2)
#define DB3(level, mesg, arg1, arg2, arg3)
#define DB4(level, mesg, arg1, arg2, arg3, arg4)
#define DB5(level, mesg, arg1, arg2, arg3, arg4, arg5)
#define DB6(level, mesg, arg1, arg2, arg3, arg4, arg5, arg6)
#define DB7(level, mesg, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define DBS0(level, mesg)
#define DBS1(level, mesg, arg)
#define DBS2(level, mesg, arg1, arg2)
#define DBS3(level, mesg, arg1, arg2, arg3)
#define DBS4(level, mesg, arg1, arg2, arg3, arg4)
#define DBS5(level, mesg, arg1, arg2, arg3, arg4, arg5)
#define DBS6(level, mesg, arg1, arg2, arg3, arg4, arg5, arg6)
#define DBS7(level, mesg, arg1, arg2, arg3, arg4, arg5, arg6, arg7)

#ifdef DEBUG
#undef DEBUG
#endif

#endif
#endif
