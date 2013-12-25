/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _ISTREAMS_DRIVER_H_
#define _ISTREAMS_DRIVER_H_

/***
 ***  Testing
 ***/
#define _IS_TEST(levels, streams, uid, do_walkers, do_loop) \
  { \
    int seqs[1000][500]; \
    int failed[1000]; \
    int failures; \
    int verify; \
    int numseqs; \
    FILE* f; \
    int * restrict data = NULL; \
    int * restrict buff = NULL; \
    int i, j, k; \
    int num1, num2, num3, num4, num5, num6; \
    int nums[6]; \
    char filename[1000]; \
    sprintf(filename, "test_istreams%d_data.txt", streams); \
    f = fopen(filename, "r"); \
    if (!f) { printf("file %s not found!\n", filename); exit(1); } \
    i = 0; j = 0; \
    while (1) { \
      fscanf(f, "%d", &k); \
      seqs[i][j] = k; \
      j++; \
      if (k == -1) { \
        i++; j = 0; \
      } \
      if (k == -2) { \
        numseqs = i; \
        break; \
      } \
    } \
    fclose(f); \
    if (verbose) printf("Raw Sequence Data\n"); \
    for (i = 0; i < numseqs; i += streams) { \
      if (verbose) printf("%2d: ", i / streams); \
      for (j = 0; seqs[i][j] != -1; j++) { \
        if (streams > 1) if (verbose) printf("("); \
        for (k = 0; k < streams; k++) { \
          if (verbose) printf("%2d ", seqs[i+k][j]); \
        } \
        if (streams > 1) if (verbose) printf(") "); \
      } \
      if (verbose) printf("\n"); \
    } \
    for (i = 0; i < numseqs; i += streams) { \
      if (verbose) printf("sequence = "); \
      for (j = 0; seqs[i][j] != -1; j++) { \
        if (streams > 1) if (verbose) printf("("); \
        for (k = 0; k < streams; k++) { \
          if (verbose) printf("%2d ", seqs[i+k][j]); \
        } \
        if (streams > 1) if (verbose) printf(") "); \
      } \
      if (verbose) printf("\n"); \
      if (verbose) printf("enc: start "); \
      fflush(NULL); \
      data = 0; \
      for (j = 0; seqs[i][j] != -1; j++) { \
        if (streams > 1) if (verbose) printf("("); \
        if (streams > 0) { nums[0] = seqs[i-1+1][j]; if (verbose) printf("%2d ", nums[0]); } \
        if (streams > 1) { nums[1] = seqs[i-1+2][j]; if (verbose) printf("%2d ", nums[1]); } \
        if (streams > 2) { nums[2] = seqs[i-1+3][j]; if (verbose) printf("%2d ", nums[2]); } \
        if (streams > 3) { nums[3] = seqs[i-1+4][j]; if (verbose) printf("%2d ", nums[3]); } \
        if (streams > 4) { nums[4] = seqs[i-1+5][j]; if (verbose) printf("%2d ", nums[4]); } \
        if (streams > 5) { nums[5] = seqs[i-1+6][j]; if (verbose) printf("%2d ", nums[5]); } \
        if (streams > 1) if (verbose) printf(") "); \
        fflush(NULL); \
        _IE_WRITE(nums, data, buff, levels, streams); \
      } \
      _IE_STOP(data, buff, levels, streams); \
      if (verbose) printf("stop\n"); \
      fflush(NULL); \
      if (verbose) _IS_PRINT(data, levels, streams, uid); \
      failed[i / streams] = 0; \
      if (do_loop == 1) { \
        j = 0; \
        if (verbose) printf("dec: start "); \
        fflush(NULL); \
        _ID_LOOPSTART(num, data, levels, streams, uid); \
        _ID_LOOPREAD(num, data, levels, streams, uid); \
        if (streams > 1) if (verbose) printf("("); \
        if (streams > 0) { if (num1 != seqs[i-1+1][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num1); } \
        if (streams > 1) { if (num2 != seqs[i-1+2][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num2); } \
        if (streams > 2) { if (num3 != seqs[i-1+3][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num3); } \
        if (streams > 3) { if (num4 != seqs[i-1+4][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num4); } \
        if (streams > 4) { if (num5 != seqs[i-1+5][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num5); } \
        if (streams > 5) { if (num6 != seqs[i-1+6][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num6); } \
        j++; \
        if (streams > 1) if (verbose) printf(") "); \
        fflush(NULL); \
        _ID_BAILEDLOOPELSE(num, data, levels, streams, uid); \
        _ID_BAILEDLOOPREAD(num, data, levels, streams, uid); \
        if (streams > 1) if (verbose) printf("("); \
        if (streams > 0) { if (num1 != seqs[i-1+1][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num1); } \
        if (streams > 1) { if (num2 != seqs[i-1+2][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num2); } \
        if (streams > 2) { if (num3 != seqs[i-1+3][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num3); } \
        if (streams > 3) { if (num4 != seqs[i-1+4][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num4); } \
        if (streams > 4) { if (num5 != seqs[i-1+5][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num5); } \
        if (streams > 5) { if (num6 != seqs[i-1+6][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num6); } \
        j++; \
        if (streams > 1) if (verbose) printf(") "); \
        fflush(NULL); \
        _ID_LOOPSTOP(num, data, levels, streams, uid); \
        if (verbose) printf("stop\n"); \
        fflush(NULL); \
      } \
      else if (do_loop == 2) { \
        _ID_FUNCSTART(nums, data, levels, streams); \
        if (verbose) printf("dec: start "); \
        fflush(NULL); \
        for (j = 0; data && j < data[1]; j++) { \
          _ID_FUNCREAD(nums, data, levels, streams); \
          if (streams > 1) if (verbose) printf("("); \
          if (streams > 0) { if (nums[0] != seqs[i-1+1][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", nums[0]); } \
          if (streams > 1) { if (nums[1] != seqs[i-1+2][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", nums[1]); } \
          if (streams > 2) { if (nums[2] != seqs[i-1+3][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", nums[2]); } \
          if (streams > 3) { if (nums[3] != seqs[i-1+4][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", nums[3]); } \
          if (streams > 4) { if (nums[4] != seqs[i-1+5][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", nums[4]); } \
          if (streams > 5) { if (nums[5] != seqs[i-1+6][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", nums[5]); } \
          if (streams > 1) if (verbose) printf(") "); \
          fflush(NULL); \
        } \
        _ID_FUNCSTOP(nums, data, levels, streams); \
        if (verbose) printf("stop\n"); \
        fflush(NULL); \
      } \
      else { \
        _ID_START(num, data, levels, streams, uid); \
        if (verbose) printf("dec: start "); \
        fflush(NULL); \
        for (j = 0; data && j < data[1]; j++) { \
          _ID_READ(num, data, levels, streams, uid); \
          if (streams > 1) if (verbose) printf("("); \
          if (streams > 0) { if (num1 != seqs[i-1+1][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num1); } \
          if (streams > 1) { if (num2 != seqs[i-1+2][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num2); } \
          if (streams > 2) { if (num3 != seqs[i-1+3][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num3); } \
          if (streams > 3) { if (num4 != seqs[i-1+4][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num4); } \
          if (streams > 4) { if (num5 != seqs[i-1+5][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num5); } \
          if (streams > 5) { if (num6 != seqs[i-1+6][j]) failed[i / streams] = 1; if (verbose) printf("%2d ", num6); } \
          if (streams > 1) if (verbose) printf(") "); \
          fflush(NULL); \
        } \
        _ID_STOP(num, data, levels, streams, uid); \
        if (verbose) printf("stop\n"); \
        fflush(NULL); \
      } \
      if (seqs[i][j] != -1) failed[i / streams] = 1; \
      if (failed[i / streams]) if (verbose) printf("*** FAILED ***\n"); \
      if (verbose) printf("\n"); \
      _zfree(data, "kill stream in test"); \
    } \
    if (_INDEX == 0) { \
      printf("Summary:\n"); \
      failures = 0; \
      for (i = 0; i < numseqs; i += streams) { \
        if (failed[i / streams]) { \
          printf(" Encode/decode failure on test %d\n", i / streams); \
          failures++; \
        } \
      } \
      if (failures) { \
        printf("\n %d tests failed.\n", failures); \
      } \
      else { \
        printf(" All tests succeeded.\n"); \
      } \
    } \
  }

#define _ID_LOOPSTART1_ENC0(num, data, uid)
#define _ID_LOOPREAD1_ENC0(num, data, uid)
#define _ID_BAILEDLOOPELSE1_ENC0(num, data, uid)
#define _ID_BAILEDLOOPREAD1_ENC0(num, data, uid)
#define _ID_LOOPSTOP1_ENC0(num, data, uid)
#define _ID_LOOPSTART2_ENC0(num, data, uid)
#define _ID_LOOPREAD2_ENC0(num, data, uid)
#define _ID_BAILEDLOOPELSE2_ENC0(num, data, uid)
#define _ID_BAILEDLOOPREAD2_ENC0(num, data, uid)
#define _ID_LOOPSTOP2_ENC0(num, data, uid)
#define _ID_LOOPSTART3_ENC0(num, data, uid)
#define _ID_LOOPREAD3_ENC0(num, data, uid)
#define _ID_BAILEDLOOPELSE3_ENC0(num, data, uid)
#define _ID_BAILEDLOOPREAD3_ENC0(num, data, uid)
#define _ID_LOOPSTOP3_ENC0(num, data, uid)
#define _ID_LOOPSTART4_ENC0(num, data, uid)
#define _ID_LOOPREAD4_ENC0(num, data, uid)
#define _ID_BAILEDLOOPELSE4_ENC0(num, data, uid)
#define _ID_BAILEDLOOPREAD4_ENC0(num, data, uid)
#define _ID_LOOPSTOP4_ENC0(num, data, uid)
#define _ID_LOOPSTART5_ENC0(num, data, uid)
#define _ID_LOOPREAD5_ENC0(num, data, uid)
#define _ID_BAILEDLOOPELSE5_ENC0(num, data, uid)
#define _ID_BAILEDLOOPREAD5_ENC0(num, data, uid)
#define _ID_LOOPSTOP5_ENC0(num, data, uid)
#define _ID_LOOPSTART6_ENC0(num, data, uid)
#define _ID_LOOPREAD6_ENC0(num, data, uid)
#define _ID_BAILEDLOOPELSE6_ENC0(num, data, uid)
#define _ID_BAILEDLOOPREAD6_ENC0(num, data, uid)
#define _ID_LOOPSTOP6_ENC0(num, data, uid)

#endif
