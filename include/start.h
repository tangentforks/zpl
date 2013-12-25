/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _START_H_
#define _START_H_

/* start.c */

void _ParseArgs(int,char* [],int);
void _QueryProcInfo(int*,int []);
void _QueryQuietMode(int *);
int _QueryMemTrace(void);
int _QueryMPIPrintHelp(void);
int _QueryMPIPrintNode(void);
int _QueryMPICollective(void);
int _QueryCheckpointInterval(void);
char *_QueryCheckpointLocation(void);
int _QueryVerboseHierarchy(void);
int _QueryVerboseRemap(void);
int _QueryValgrind(void);

#endif
