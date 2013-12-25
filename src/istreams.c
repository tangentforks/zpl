/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"

static const int _ie_buffsize[6][6] =
  {{  6,   9,  12,  15,  18,  21},
   { 11,  17,  23,  29,  35,  41},
   { 18,  28,  38,  48,  58,  68},
   { 27,  42,  57,  72,  87, 102},
   { 38,  59,  80, 101, 122, 143},
   { 51,  79, 107, 135, 163, 191}};

#define buffsize(levels, streams) (_ie_buffsize[(levels)-1][(streams)-1])

/*                                   {{ifelse({{$1}}, {{1}}, {{ 3 +  3 * $2}}, */
/*                                            {{$1}}, {{2}}, {{ 5 +  6 * $2}}, */
/*                                            {{$1}}, {{3}}, {{ 8 + 10 * $2}},  */
/*                                            {{$1}}, {{4}}, {{12 + 15 * $2}},  */
/*                                            {{$1}}, {{5}}, {{17 + 21 * $2}}, */
/*                                            {{$1}}, {{6}}, {{23 + 28 * $2}})}})dnl */

#define buffcurrent(stream) (2 + (stream) - 1)
#define buffjump(levels, streams, stream) (buffsize(levels, streams) - (levels) - (1 + (levels)) * (streams) + (stream) - 1)
#define buffstride(levels, streams, level, stream) (buffsize(levels, streams) - (1 + (levels) - (level)) - (1 + (levels) - (level)) * (streams) + (stream) - 1)
#define bufflength(levels, streams, level) (buffsize(levels, streams) - (1 + (levels) - (level)) - ((levels) - (level)) * (streams))

#include "istreams.h"

/***
 *** print
 ***/
void _is_print_f(int * const restrict data, int levels, int streams)
{
  int i;

  printf("Encoding:\n");
  if (!data) {
    printf("  istream data is NULL\n");
    return;
  }
  printf("  buffer size = %d, number of elts = %d, encoded = %d\n", data[0], data[1], data[2]);
  printf("  encoding level = %d, number of streams = %d\n", levels, streams);
  printf("  encode:  ");
  for (i = 3; i < data[0]; i++) {
    printf("%d", data[i]);
    if (i != data[0] - 1) {
      if ((i - 2) % (levels + streams + levels * streams) == 0) printf(",");
      printf(" ");
    }
  }
  printf("\n");
}

/***
 *** decode functions
 ***/
#define _IDF_MAXLEVELS 6
#define _IDF_MAXSTREAMS 6

static int _idf_buffer[1+2*_IDF_MAXLEVELS+_IDF_MAXLEVELS*_IDF_MAXSTREAMS];

#define _idf_pos (_idf_buffer[0])
#define _idf_len(l) (_idf_buffer[1+l])
#define _idf_slen(l) (_idf_buffer[7+l])
#define _idf_str(l, s) (_idf_buffer[13+6*l+s])

void _id_fstart(int * const restrict ind, int * const restrict data, int levels, int streams)
{
  int s, l;

  _idf_pos = 3;
  for (s = 0; s < streams; s++) {
    ind[s] = 0;
  }
  for (l = 0; l < levels; l++) {
    _idf_len(l) = 0;
  }
}

void _id_fread(int * const restrict ind, int * const restrict data, int levels, int streams)
{
  int s, l, g;

  if (!data[2]) {
    for (s = 0; s < streams; s++) {
      ind[s] = data[_idf_pos++];
    }
  }
  else {
    for (l = 0; l < levels; l++) {
      if (_idf_len(l) > (l > 0)) {
	for (s = 0; s < streams; s++) {
	  ind[s] += _idf_str(l, s);
	}
	_idf_len(l)--;
	for (g = 0; g < l; g++) {
	  _idf_len(g) = _idf_slen(g);
	}
	return;
      }
    }
    for (s = 0; s < streams; s++) {
      ind[s] += data[_idf_pos++];
    }
    for (l = 0; l < levels; l++) {
      for (s = 0; s < streams; s++) {
	_idf_str(l, s) = data[_idf_pos++];
      }
      _idf_len(l) = data[_idf_pos++] - (l == 0);
    }
    for (l = 0; l < levels - 1; l++) {
      _idf_slen(l) = _idf_len(l);
    }
  }
}

void _id_fstop(int * const restrict ind, int * const restrict data, int levels, int streams)
{
  return;
}

/***
 *** encode functions
 ***/
void _ief_write(int * restrict const nums, int * restrict * const restrict data, int * restrict * const restrict buff, int levels, int streams)
{
  int s, l;

  if (levels == 0) {
    int * const restrict pdata = *data;
    if (pdata) {
      const int size = pdata[0];
      const int pos = pdata[1];
      if (pos + streams <= size) {
	pdata[1] = pos + streams;
	for (s = 0; s < streams; s++) {
	  pdata[pos + s] = nums[s];
	}
      }
      else {
	int * const restrict newdata = (int *)_zrealloc(pdata, size * 2 * sizeof(int), "double encoding stream");
	newdata[0] = size * 2;
	newdata[1] = pos + streams;
	for (s = 0; s < streams; s++) {
	  newdata[pos + s] = nums[s];
	}
	*data = newdata;
      }
    }
    else {
      int * const restrict newdata = (int *)_zmalloc((3 + 4 * streams) * sizeof(int), "init encoding stream");
      newdata[0] = 3 + 4 * streams;
      newdata[1] = 3 + streams;
      newdata[2] = 0;
      for (s = 0; s < streams; s++) {
	newdata[3 + s] = nums[s];
      }
      *data = newdata;
    }
  }
  else {

    /*
     *  pointer to data
     */
    int * const restrict pdata = *data;

    /*
     *  if data is NULL, we have not yet encoded any integers; we must
     *  initialize data
     */
    if (!pdata) {

      /*
       *  allocate data and buff; buff is scratch for use during
       *  encoding
       */
      int * const restrict newdata = (int *)_zmalloc((3 + 2 * streams + levels + streams * levels) * sizeof(int), "init data encoding stream");
      int * const restrict newbuff = (int *)_zmalloc(buffsize(levels, streams) * sizeof(int), "init buff encoding stream");

      /*
       *  record the current size of data and set the number of elements
       *  encoded to one
       */
      newbuff[0] = 3 + 2 * streams + levels + streams * levels;
      newbuff[1] = 1;

      /*
       *  record the current elements and set the jumps to the current
       *  elements
       */
      for (s = 1; s <= streams; s++) {
	newbuff[buffcurrent(s)] = nums[s-1];
	newbuff[buffjump(1, streams, s)] = nums[s-1];
      }

      /*
       *  initialize the outer lengths, outer length of the first level
       *  is one, otherwise zero
       */
      for (l = 1; l <= levels; l++) {
	newbuff[bufflength(l, streams, l)] = (l == 1);
      }
      
      /*
       *  use the first data element to point to buff (no more), use
       *  the second to point to first position in data available, set
       *  the encoding flag to 1
       */
      newdata[1] = 3;
      newdata[2] = 1;

      /*
       *  save the new data as data
       */
      *data = newdata;
      *buff = newbuff;
      return;
    }

    /*
     *  if we have bailed on encoding, use level 0 encoding (no
     *  encoding)
     */
    if (!pdata[2]) {
      _IE_WRITE(nums, *data, *buff, 0, streams);
      return;
    }

    /*
     *  continue encoding which we have already started
     */
    {

      /*
       *  pointer to buff, scratch encoding space
       */
      int * const restrict pbuff = *buff;
      int last[7];
      int len[7];

      /*
       *  load last value encoded
       */
      for (s = 1; s <= streams; s++) {
	last[s] = pbuff[buffcurrent(s)];
      }

      /*
       *  encode level 1
       */
      {

	/*
	 *  load length
	 */
	len[1] = pbuff[bufflength(1, streams, 1)];

	/*
	 *  write first element; update current, set jump, set length
	 */
	if (len[1] == 0) {
	  for (s = 1; s <= streams; s++) {
	    pbuff[buffcurrent(s)] = nums[s-1];
	    pbuff[buffjump(1, streams, s)] = nums[s-1] - last[s];
	  }
	  pbuff[bufflength(1, streams, 1)] = 1;
	  pbuff[1]++;
	  return;
	}

	/*
	 *  write second element; update current, set stride, set length
	 */
	if (len[1] == 1) {
	  for (s = 1; s <= streams; s++) {
	    pbuff[buffcurrent(s)] = nums[s-1];
	    pbuff[buffstride(1, streams, 1, s)] = nums[s-1] - last[s];
	  }
	  pbuff[bufflength(1, streams, 1)] = 2;
	  pbuff[1]++;
	  return;
	}

	/*
	 *  write next element if last + stride = number for all streams
	 */
	{
	  int goodtest = 1;
	  for (s = 1; s <= streams; s++) {
	    if (last[s] + pbuff[buffstride(1, streams, 1, s)] != nums[s-1]) {
	      goodtest = 0;
	      break;
	    }
	  }
	  if (goodtest) {
	    for (s = 1; s <= streams; s++) {
	      pbuff[buffcurrent(s)] = nums[s-1];
	    }
	    pbuff[bufflength(1, streams, 1)] = len[1] + 1;
	    pbuff[1]++;
	    return;
	  }
	}
      }

      /*
       *  encode next levels
       */
      for (l = 2; l <= levels; l++) {

	/*
	 *  load outer length
	 */
	len[l] = pbuff[bufflength(l, streams, l)];

	/*
	 *  write first lower level encoding and recurse
	 */
	if (len[l] == 0) {
	  int i;
	  for (i = 0; i < streams + l - 1 + streams * (l - 1); i++) {
	    pbuff[buffjump(l, streams, 1) + i] = pbuff[buffjump((l - 1), streams, 1) + i];
	  }
	  pbuff[bufflength((l - 1), streams, (l - 1))] = 0;
	  pbuff[bufflength(l, streams, l)] = 1;
	  _IE_WRITE(nums, *data, *buff, levels, streams);
	  return;
	}

	/*
	 *  test to see if we can write more than just one at higher
	 *  encoding level; all strides and lengths must match up in
	 *  lower level encoding pieces
	 */
	{
	  int i, goodtest = 1;
	  for (i = 0; i < l - 1 + streams * (l - 1); i++) {
	    if (pbuff[buffstride(l, streams, 1, 1) + i] != pbuff[buffstride((l - 1), streams, 1, 1) + i]) {
	      goodtest = 0;
	      break;
	    }
	  }
	  if (goodtest) {

	    /*
	     *  write second lower level encoding and recurse
	     */
	    if (len[l] == 1) {
	      pbuff[bufflength((l - 1), streams, (l - 1))] = 0;
	      pbuff[bufflength(l, streams, l)] = 2;
	      for (s = 1; s <= streams; s++) {
		pbuff[buffstride(l, streams, l, s)] = pbuff[buffjump((l - 1), streams, s)];
	      }
	      _IE_WRITE(nums, *data, *buff, levels, streams);
	      return;
	    }

	    /*
	     *  write next lower level encoding and recurse
	     */
	    {
	      int goodtest2 = 1;
	      for (s = 1; s <= streams; s++) {
		if (pbuff[buffstride(l, streams, l, s)] != pbuff[buffjump((l - 1), streams, s)]) {
		  goodtest2 = 0;
		  break;
		}
	      }
	      if (goodtest2) {
		pbuff[bufflength((l - 1), streams, (l - 1))] = 0;
		pbuff[bufflength(l, streams, l)] = len[l] + 1;
		_IE_WRITE(nums, *data, *buff, levels, streams);
		return;
	      }
	    }
	  }
	}
      }

      /*
       *  bail out if we didn't encode enough in the first encoding
       *  piece
       */
      if (pdata[1] == 3 && pbuff[1] <= 1 + 2 * levels) {
	const int nelts = pbuff[1];
	int * restrict newdata = NULL;
	int * restrict newbuff = NULL;
	int * restrict olddata = NULL;
	int oldnums[6];
	int i;

	_IE_STOP(*data, *buff, levels, streams);
	olddata = *data;
	_ID_FUNCSTART(oldnums, olddata, levels, streams);
	for (i = 0; i < nelts; i++) {
	  _ID_FUNCREAD(oldnums, olddata, levels, streams);
	  _IE_WRITE(oldnums, newdata, newbuff, 0, streams);
	}
	_ID_FUNCSTOP(oldnums, olddata, levels, streams);
	*data = newdata;
	*buff = newbuff;
	_zfree(olddata, "bail data encoding stream");
	_IE_WRITE(nums, *data, *buff, 0, streams);
	return;
      }

      /*
       *  copy encoding piece to data
       */
      {
	const int size = pbuff[0];
	const int pos = pdata[1];
	int i;
	if (pos + streams + levels + streams * levels <= size) {
	  for (i = 0; i < streams + levels + streams * levels; i++) {
	    pdata[pos + i] = pbuff[buffjump(levels, streams, 1) + i];
	  }
	  pdata[1] = pos + streams + levels + streams * levels;
	}
	else {
	  int * const restrict newdata = (int *)_zrealloc(pdata, size * 2 * sizeof(int), "double encoding stream");
	  pbuff[0] = size * 2;
	  for (i = 0; i < streams + levels + streams * levels; i++) {
	    newdata[pos + i] = pbuff[buffjump(levels, streams, 1) + i];
	  }
	  newdata[1] = pos + streams + levels + streams * levels;
	  *data = newdata;
	}

	/*
	 *  set the outer length to zero and recurse
	 */
	pbuff[bufflength(levels, streams, levels)] = 0;
	_IE_WRITE(nums, *data, *buff, levels, streams);
      }
    }
  }
}

void _ief_stop(int * restrict * const restrict data, int * restrict * const restrict buff, int levels, int streams)
{
  int s, l;

  if (levels == 0) {
    int * const restrict pdata = *data;
    if (pdata) {
      const int newsize = pdata[1];
      int * const restrict newdata = (int *)_zrealloc(pdata, newsize * sizeof(int), "final encoding stream");
      newdata[0] = newsize;
      newdata[1] = (newsize - 3) / streams;
      *data = newdata;
    }
  }
  else {
    int * const restrict pdata = *data;

    if (pdata) {
      if (pdata[2]) {
	int * const restrict pbuff = *buff;
	int len[7];

	/*
	 *  encode next levels
	 */
	for (l = 2; l <= levels; l++) {

	  /*
	   *  load outer length
	   */
	  len[l] = pbuff[bufflength(l, streams, l)];

	  /*
	   *  write first lower level encoding and recurse
	   */
	  if (len[l] == 0) {
	    int i;
	    for (i = 0; i < streams + l - 1 + streams * (l - 1); i++) {
	      pbuff[buffjump(l, streams, 1) + i] = pbuff[buffjump((l - 1), streams, 1) + i];
	    }
	    pbuff[bufflength((l - 1), streams, (l - 1))] = 0;
	    pbuff[bufflength(l, streams, l)] = 1;
	  }
	  else {

	    /*
	     *  test to see if we can write more than just one at higher
	     *  encoding level; all strides and lengths must match up in
	     *  lower level encoding pieces
	     */
	    int i, goodtest = 1;
	    for (i = 0; i < l - 1 + streams * (l - 1); i++) {
	      if (pbuff[buffstride(l, streams, 1, 1) + i] != pbuff[buffstride((l - 1), streams, 1, 1) + i]) {
		goodtest = 0;
		break;
	      }
	    }
	    if (goodtest) {
	      
	      /*
	       *  write second lower level encoding and recurse
	       */
	      if (len[l] == 1) {
		pbuff[bufflength((l - 1), streams, (l - 1))] = 0;
		pbuff[bufflength(l, streams, l)] = 2;
		for (s = 1; s <= streams; s++) {
		  pbuff[buffstride(l, streams, l, s)] = pbuff[buffjump((l - 1), streams, s)];
		}
	      }
	      
	      /*
	       *  write next lower level encoding and recurse
	       */
	      else {
		int goodtest2 = 1;
		for (s = 1; s <= streams; s++) {
		  if (pbuff[buffstride(l, streams, l, s)] != pbuff[buffjump((l - 1), streams, s)]) {
		    goodtest2 = 0;
		    break;
		  }
		}
		if (goodtest2) {
		  pbuff[bufflength((l - 1), streams, (l - 1))] = 0;
		  pbuff[bufflength(l, streams, l)] = len[l] + 1;
		}
	      }
	    }
	  }
	}

	/*
	 *  copy encoding piece to data
	 */
	for (l = levels; l >= 1; l--) {
	  int * const restrict pd = *data;
	  int * const restrict pb = *buff;
	  const int size = pb[0];
	  const int pos = pd[1];
	  int i;

	  if (pb[bufflength(l, streams, l)]) {
	    if (pos + streams + levels + streams * levels <= size) {
	      for (i = 0; i < streams + l + streams * l; i++) {
		pd[pos + i] = pb[buffjump(l, streams, 1) + i];
	      }
	      for (i = streams + l + streams * l; i < streams + levels + streams * levels; i++) {
		pd[pos + i] = 1;
	      }
	      pd[1] = pos + streams + levels + streams * levels;
	    }
	    else {
	      int * const restrict newdata = (int *)_zrealloc(pd, size * 2 * sizeof(int), "double encoding stream");
	      pb[0] = size * 2;
	      for (i = 0; i < streams + l + streams * l; i++) {
		newdata[pos + i] = pb[buffjump(l, streams, 1) + i];
	      }
	      for (i = streams + l + streams * l; i < streams + levels + streams * levels; i++) {
		newdata[pos + i] = 1;
	      }
	      newdata[1] = pos + streams + levels + streams * levels;
	      *data = newdata;
	    }
	  }
	}
	{
	  int * const restrict pd = *data;
	  int * const restrict pb = *buff;
	  const int newsize = pd[1] + streams;
	  int * const restrict newdata = (int *)_zrealloc(pd, newsize * sizeof(int), "final encoding stream");
	  newdata[0] = newsize;
	  newdata[1] = pb[1];
	  for (s = 1; s <= streams; s++) {
	    newdata[newsize - (streams - s + 1)] = pb[buffcurrent(s)];
	  }
	  *data = newdata;
	  _zfree(pb, "destroy buffer encoding stream");
	}
      }
      else {
	_IE_STOP(*data, *buff, 0, streams);
      }
    }
  }
}
