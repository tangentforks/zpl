/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

/* MDD */
extern void zprof_alarm_set();
extern void zprof_alarm_handler();

extern int _PCCNT_NUM;     /* number of call-edge counters  */
extern int *_PCCNT_ZPROF;
extern int _PRCNT_NUM;     /* number of basic bloc counters */
extern int *_PRCNT_ZPROF;
extern int _SRC_LOC_NUM;   /* number of distinct locations in the source code     */
                           /* variable hard-wired in the compiler-generated code  */

extern int _SRC_LOC;       /* global variable marking  source code locations      */
                           /* it is set at run-time byt the compiled code         */


