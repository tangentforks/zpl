/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* MDD */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "zlib.h"

int _PRCNT_NUM;     /* number of basic bloc counters */
int *_PRCNT_ZPROF;

int _PCCNT_NUM;     /* number of call-edge counters  */
int *_PCCNT_ZPROF;

int _SRCSAMPL_NUM;   /* number of distinct locations in the source code     */
                    /* variable hard-wired in the compiler-generated code  */

int _SRC_LOC;       /* global variable marking  source code locations      */
                    /* it is set at run-time byt the compiled code         */

int *_SRCSAMPL_ZPROF;  /* statistical data on execution-time spending          */

static int _ZPROF_ALARM_TIMING;

/*
 * zprof_ds_out: library routine to produce raw profile data
 */
void zprof_ds_out(void) {

  int i;
  int tot_time = 0;
  FILE *outfp;

  outfp = fopen(".profiles","w");  
  fprintf(outfp, "__profiles_\n bbcnt %d\n", _PRCNT_NUM);
  for (i=0; i<_PRCNT_NUM; i++)
    fprintf(outfp, "%d %d\n", i, _PRCNT_ZPROF[i]);
  
  fprintf(outfp, "call_edge_cnt %d\n", _PCCNT_NUM);
  for (i=0; i<_PCCNT_NUM; i++)
    fprintf(outfp, "%d %d\n", i, _PCCNT_ZPROF[i]);

  for (i=0; i<_SRCSAMPL_NUM; i++)
    tot_time += _SRCSAMPL_ZPROF[i];

  fprintf(outfp, "timings %d %d\n", _SRCSAMPL_NUM, tot_time);
  for (i=0; i<_SRCSAMPL_NUM; i++)
    fprintf(outfp, "%d %d\n", i, _SRCSAMPL_ZPROF[i]);

  fclose(outfp);

}


static void zprof_alarm_handler(int s) {

  _SRCSAMPL_ZPROF[_SRC_LOC] += 1;
  ualarm(_ZPROF_ALARM_TIMING, 0);

}



void zprof_alarm_set(void) {

    int i;

    _SRCSAMPL_ZPROF = (int *) _zmalloc(_SRCSAMPL_NUM*sizeof(int),
					 "zprof samples");
    for (i=0; i<_SRCSAMPL_NUM; i++)
      _SRCSAMPL_ZPROF[i] = 0;

    _ZPROF_ALARM_TIMING = 10;
    signal(SIGALRM, zprof_alarm_handler);
    ualarm(_ZPROF_ALARM_TIMING, 0);
}

