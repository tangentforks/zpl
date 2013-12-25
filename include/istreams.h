/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _ISTREAMS_H_
#define _ISTREAMS_H_

/***
 ***  Printing
 ***/
#define _IS_PRINT(data, levels, streams, uid) _is_print_f(data, levels, streams)

void _is_print_f(int * const restrict data, int levels, int streams);

/***
 ***  Encoding
 ***/
#define _IE_WRITE(nums, data, buff, levels, streams) \
  _ief_write(nums, &data, &buff, levels, streams)

#define _IE_STOP(data, buff, levels, streams) \
  _ief_stop(&data, &buff, levels, streams)

void _ief_write(int * restrict const nums, int * restrict * const restrict data, int * restrict * const restrict buff, int levels, int streams);
void _ief_stop(int * restrict * const restrict data, int * restrict * const restrict buff, int levels, int streams);

/***
 ***  Decoding
 ***/
#define _ID_START(ind, data, levels, streams, uid) \
  _ID_START##streams##_ENC##levels(ind, data, uid)

#define _ID_READ(ind, data, levels, streams, uid) \
  _ID_READ##streams##_ENC##levels(ind, data, uid)

#define _ID_STOP(ind, data, levels, streams, uid) \
  _ID_STOP##streams##_ENC##levels(ind, data, uid)

/* Decoding the unencoded */
#define _ID_START1_ENC0(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    int ind##1 = 0

#define _ID_READ1_ENC0(ind, data, uid) \
    ind##1 = data[_id_pos_##uid++]

#define _ID_STOP1_ENC0(ind, data, uid) \
  }

#define _ID_START2_ENC0(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    int ind##1 = 0; int ind##2 = 0

#define _ID_READ2_ENC0(ind, data, uid) \
    ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]

#define _ID_STOP2_ENC0(ind, data, uid) \
  }

#define _ID_START3_ENC0(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0

#define _ID_READ3_ENC0(ind, data, uid) \
    ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]

#define _ID_STOP3_ENC0(ind, data, uid) \
  }

#define _ID_START4_ENC0(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0

#define _ID_READ4_ENC0(ind, data, uid) \
    ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]

#define _ID_STOP4_ENC0(ind, data, uid) \
  }

#define _ID_START5_ENC0(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0

#define _ID_READ5_ENC0(ind, data, uid) \
    ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]

#define _ID_STOP5_ENC0(ind, data, uid) \
  }

#define _ID_START6_ENC0(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0

#define _ID_READ6_ENC0(ind, data, uid) \
    ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; ind##6 = data[_id_pos_##uid++]

#define _ID_STOP6_ENC0(ind, data, uid) \
  }

/* Decoding the encoded */
#define _ID_START1_ENC1(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; \
    int _id_str1_1_##uid; \
    int _id_len1_##uid = 0; \
    

#define _ID_READ1_ENC1(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        ; \
      } \
    }

#define _ID_STOP1_ENC1(ind, data, uid) \
  }

#define _ID_START2_ENC1(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; \
    int _id_len1_##uid = 0; \
    

#define _ID_READ2_ENC1(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        ; \
      } \
    }

#define _ID_STOP2_ENC1(ind, data, uid) \
  }

#define _ID_START3_ENC1(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; \
    int _id_len1_##uid = 0; \
    

#define _ID_READ3_ENC1(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        ; \
      } \
    }

#define _ID_STOP3_ENC1(ind, data, uid) \
  }

#define _ID_START4_ENC1(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; \
    int _id_len1_##uid = 0; \
    

#define _ID_READ4_ENC1(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        ; \
      } \
    }

#define _ID_STOP4_ENC1(ind, data, uid) \
  }

#define _ID_START5_ENC1(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; \
    int _id_len1_##uid = 0; \
    

#define _ID_READ5_ENC1(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        ; \
      } \
    }

#define _ID_STOP5_ENC1(ind, data, uid) \
  }

#define _ID_START6_ENC1(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; \
    int _id_len1_##uid = 0; \
    

#define _ID_READ6_ENC1(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; ind##6 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        ; \
      } \
    }

#define _ID_STOP6_ENC1(ind, data, uid) \
  }

#define _ID_START1_ENC2(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; \
    int _id_str1_1_##uid; int _id_str2_1_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; \
    int _id_slen1_##uid

#define _ID_READ1_ENC2(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; \
      } \
    }

#define _ID_STOP1_ENC2(ind, data, uid) \
  }

#define _ID_START2_ENC2(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; \
    int _id_slen1_##uid

#define _ID_READ2_ENC2(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; \
      } \
    }

#define _ID_STOP2_ENC2(ind, data, uid) \
  }

#define _ID_START3_ENC2(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; \
    int _id_slen1_##uid

#define _ID_READ3_ENC2(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; \
      } \
    }

#define _ID_STOP3_ENC2(ind, data, uid) \
  }

#define _ID_START4_ENC2(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; \
    int _id_slen1_##uid

#define _ID_READ4_ENC2(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; \
      } \
    }

#define _ID_STOP4_ENC2(ind, data, uid) \
  }

#define _ID_START5_ENC2(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; \
    int _id_slen1_##uid

#define _ID_READ5_ENC2(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; \
      } \
    }

#define _ID_STOP5_ENC2(ind, data, uid) \
  }

#define _ID_START6_ENC2(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; \
    int _id_slen1_##uid

#define _ID_READ6_ENC2(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; ind##6 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; \
      } \
    }

#define _ID_STOP6_ENC2(ind, data, uid) \
  }

#define _ID_START1_ENC3(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; \
    int _id_str1_1_##uid; int _id_str2_1_##uid; int _id_str3_1_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid

#define _ID_READ1_ENC3(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; \
      } \
    }

#define _ID_STOP1_ENC3(ind, data, uid) \
  }

#define _ID_START2_ENC3(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid

#define _ID_READ2_ENC3(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; \
      } \
    }

#define _ID_STOP2_ENC3(ind, data, uid) \
  }

#define _ID_START3_ENC3(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid

#define _ID_READ3_ENC3(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; \
      } \
    }

#define _ID_STOP3_ENC3(ind, data, uid) \
  }

#define _ID_START4_ENC3(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid

#define _ID_READ4_ENC3(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; \
      } \
    }

#define _ID_STOP4_ENC3(ind, data, uid) \
  }

#define _ID_START5_ENC3(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid

#define _ID_READ5_ENC3(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; \
      } \
    }

#define _ID_STOP5_ENC3(ind, data, uid) \
  }

#define _ID_START6_ENC3(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str3_6_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid

#define _ID_READ6_ENC3(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; ind##6 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; ind##6 += _id_str3_6_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_str3_6_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; \
      } \
    }

#define _ID_STOP6_ENC3(ind, data, uid) \
  }

#define _ID_START1_ENC4(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; \
    int _id_str1_1_##uid; int _id_str2_1_##uid; int _id_str3_1_##uid; int _id_str4_1_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid

#define _ID_READ1_ENC4(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; \
      } \
    }

#define _ID_STOP1_ENC4(ind, data, uid) \
  }

#define _ID_START2_ENC4(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid

#define _ID_READ2_ENC4(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; \
      } \
    }

#define _ID_STOP2_ENC4(ind, data, uid) \
  }

#define _ID_START3_ENC4(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid

#define _ID_READ3_ENC4(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; \
      } \
    }

#define _ID_STOP3_ENC4(ind, data, uid) \
  }

#define _ID_START4_ENC4(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid

#define _ID_READ4_ENC4(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; \
      } \
    }

#define _ID_STOP4_ENC4(ind, data, uid) \
  }

#define _ID_START5_ENC4(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid

#define _ID_READ5_ENC4(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; \
      } \
    }

#define _ID_STOP5_ENC4(ind, data, uid) \
  }

#define _ID_START6_ENC4(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str3_6_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str4_6_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid

#define _ID_READ6_ENC4(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; ind##6 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; ind##6 += _id_str3_6_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; ind##6 += _id_str4_6_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_str3_6_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_str4_6_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; \
      } \
    }

#define _ID_STOP6_ENC4(ind, data, uid) \
  }

#define _ID_START1_ENC5(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; \
    int _id_str1_1_##uid; int _id_str2_1_##uid; int _id_str3_1_##uid; int _id_str4_1_##uid; int _id_str5_1_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid

#define _ID_READ1_ENC5(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; \
      } \
    }

#define _ID_STOP1_ENC5(ind, data, uid) \
  }

#define _ID_START2_ENC5(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid

#define _ID_READ2_ENC5(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; \
      } \
    }

#define _ID_STOP2_ENC5(ind, data, uid) \
  }

#define _ID_START3_ENC5(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid

#define _ID_READ3_ENC5(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; \
      } \
    }

#define _ID_STOP3_ENC5(ind, data, uid) \
  }

#define _ID_START4_ENC5(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid

#define _ID_READ4_ENC5(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; \
      } \
    }

#define _ID_STOP4_ENC5(ind, data, uid) \
  }

#define _ID_START5_ENC5(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str5_5_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid

#define _ID_READ5_ENC5(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; ind##5 += _id_str5_5_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_str5_5_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; \
      } \
    }

#define _ID_STOP5_ENC5(ind, data, uid) \
  }

#define _ID_START6_ENC5(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str3_6_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str4_6_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str5_5_##uid; int _id_str5_6_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid

#define _ID_READ6_ENC5(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; ind##6 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; ind##6 += _id_str3_6_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; ind##6 += _id_str4_6_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; ind##5 += _id_str5_5_##uid; ind##6 += _id_str5_6_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_str3_6_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_str4_6_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_str5_5_##uid = (data)[_id_pos_##uid++]; _id_str5_6_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; \
      } \
    }

#define _ID_STOP6_ENC5(ind, data, uid) \
  }

#define _ID_START1_ENC6(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; \
    int _id_str1_1_##uid; int _id_str2_1_##uid; int _id_str3_1_##uid; int _id_str4_1_##uid; int _id_str5_1_##uid; int _id_str6_1_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; int _id_len6_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid; int _id_slen5_##uid

#define _ID_READ1_ENC6(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else if (_id_len6_##uid > 1) { \
	ind##1 += _id_str6_1_##uid; \
        _id_len6_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; _id_len5_##uid = _id_slen5_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; _id_slen5_##uid = _id_len5_##uid; \
      } \
    }

#define _ID_STOP1_ENC6(ind, data, uid) \
  }

#define _ID_START2_ENC6(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; int _id_len6_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid; int _id_slen5_##uid

#define _ID_READ2_ENC6(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else if (_id_len6_##uid > 1) { \
	ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; \
        _id_len6_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; _id_len5_##uid = _id_slen5_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; _id_slen5_##uid = _id_len5_##uid; \
      } \
    }

#define _ID_STOP2_ENC6(ind, data, uid) \
  }

#define _ID_START3_ENC6(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; int _id_str6_3_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; int _id_len6_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid; int _id_slen5_##uid

#define _ID_READ3_ENC6(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else if (_id_len6_##uid > 1) { \
	ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; ind##3 += _id_str6_3_##uid; \
        _id_len6_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; _id_len5_##uid = _id_slen5_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_str6_3_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; _id_slen5_##uid = _id_len5_##uid; \
      } \
    }

#define _ID_STOP3_ENC6(ind, data, uid) \
  }

#define _ID_START4_ENC6(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; int _id_str6_3_##uid; int _id_str6_4_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; int _id_len6_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid; int _id_slen5_##uid

#define _ID_READ4_ENC6(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else if (_id_len6_##uid > 1) { \
	ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; ind##3 += _id_str6_3_##uid; ind##4 += _id_str6_4_##uid; \
        _id_len6_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; _id_len5_##uid = _id_slen5_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_str6_3_##uid = (data)[_id_pos_##uid++]; _id_str6_4_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; _id_slen5_##uid = _id_len5_##uid; \
      } \
    }

#define _ID_STOP4_ENC6(ind, data, uid) \
  }

#define _ID_START5_ENC6(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str5_5_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; int _id_str6_3_##uid; int _id_str6_4_##uid; int _id_str6_5_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; int _id_len6_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid; int _id_slen5_##uid

#define _ID_READ5_ENC6(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; ind##5 += _id_str5_5_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else if (_id_len6_##uid > 1) { \
	ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; ind##3 += _id_str6_3_##uid; ind##4 += _id_str6_4_##uid; ind##5 += _id_str6_5_##uid; \
        _id_len6_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; _id_len5_##uid = _id_slen5_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_str5_5_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_str6_3_##uid = (data)[_id_pos_##uid++]; _id_str6_4_##uid = (data)[_id_pos_##uid++]; _id_str6_5_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; _id_slen5_##uid = _id_len5_##uid; \
      } \
    }

#define _ID_STOP5_ENC6(ind, data, uid) \
  }

#define _ID_START6_ENC6(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
    int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str3_6_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str4_6_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str5_5_##uid; int _id_str5_6_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; int _id_str6_3_##uid; int _id_str6_4_##uid; int _id_str6_5_##uid; int _id_str6_6_##uid; \
    int _id_len1_##uid = 0; int _id_len2_##uid = 0; int _id_len3_##uid = 0; int _id_len4_##uid = 0; int _id_len5_##uid = 0; int _id_len6_##uid = 0; \
    int _id_slen1_##uid; int _id_slen2_##uid; int _id_slen3_##uid; int _id_slen4_##uid; int _id_slen5_##uid

#define _ID_READ6_ENC6(ind, data, uid) \
    if (!_id_enc_##uid) { \
      ind##1 = data[_id_pos_##uid++]; ind##2 = data[_id_pos_##uid++]; ind##3 = data[_id_pos_##uid++]; ind##4 = data[_id_pos_##uid++]; ind##5 = data[_id_pos_##uid++]; ind##6 = data[_id_pos_##uid++]; \
    } \
    else { \
      if (_id_len1_##uid > 0) { \
	ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        _id_len1_##uid--; \
        ; \
      } \
      else if (_id_len2_##uid > 1) { \
	ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        _id_len2_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; \
      } \
      else if (_id_len3_##uid > 1) { \
	ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; ind##6 += _id_str3_6_##uid; \
        _id_len3_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; \
      } \
      else if (_id_len4_##uid > 1) { \
	ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; ind##6 += _id_str4_6_##uid; \
        _id_len4_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; \
      } \
      else if (_id_len5_##uid > 1) { \
	ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; ind##5 += _id_str5_5_##uid; ind##6 += _id_str5_6_##uid; \
        _id_len5_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; \
      } \
      else if (_id_len6_##uid > 1) { \
	ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; ind##3 += _id_str6_3_##uid; ind##4 += _id_str6_4_##uid; ind##5 += _id_str6_5_##uid; ind##6 += _id_str6_6_##uid; \
        _id_len6_##uid--; \
        _id_len1_##uid = _id_slen1_##uid; _id_len2_##uid = _id_slen2_##uid; _id_len3_##uid = _id_slen3_##uid; _id_len4_##uid = _id_slen4_##uid; _id_len5_##uid = _id_slen5_##uid; \
      } \
      else { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_str3_6_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_str4_6_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_str5_5_##uid = (data)[_id_pos_##uid++]; _id_str5_6_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_str6_3_##uid = (data)[_id_pos_##uid++]; _id_str6_4_##uid = (data)[_id_pos_##uid++]; _id_str6_5_##uid = (data)[_id_pos_##uid++]; _id_str6_6_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        _id_slen1_##uid = _id_len1_##uid; _id_slen2_##uid = _id_len2_##uid; _id_slen3_##uid = _id_len3_##uid; _id_slen4_##uid = _id_len4_##uid; _id_slen5_##uid = _id_len5_##uid; \
      } \
    }

#define _ID_STOP6_ENC6(ind, data, uid) \
  }

/***
 ***  Decoding (Functions)
 ***/
#define _ID_FUNCSTART(ind, data, levels, streams) \
  _id_fstart(ind, data, levels, streams)

#define _ID_FUNCREAD(ind, data, levels, streams) \
  _id_fread(ind, data, levels, streams)

#define _ID_FUNCSTOP(ind, data, levels, streams) \
  _id_fstop(ind, data, levels, streams)

void _id_fstart(int * const restrict ind, int * const restrict data, int levels, int streams);
void _id_fread(int * const restrict ind, int * const restrict data, int levels, int streams);
void _id_fstop(int * const restrict ind, int * const restrict data, int levels, int streams);

/***
 ***  Decoding (Loop Optimization)
 ***/
#define _ID_LOOPSTART(ind, data, levels, streams, uid) \
  _ID_LOOPSTART##streams##_ENC##levels(ind, data, uid)

#define _ID_LOOPREAD(ind, data, levels, streams, uid) \
  _ID_LOOPREAD##streams##_ENC##levels(ind, data, uid)

#define _ID_BAILEDLOOPELSE(ind, data, levels, streams, uid) \
  _ID_BAILEDLOOPELSE##streams##_ENC##levels(ind, data, uid)

#define _ID_BAILEDLOOPREAD(ind, data, levels, streams, uid) \
  _ID_BAILEDLOOPREAD##streams##_ENC##levels(ind, data, uid)

#define _ID_LOOPSTOP(ind, data, levels, streams, uid) \
  _ID_LOOPSTOP##streams##_ENC##levels(ind, data, uid)

#define _ID_LOOPSTART1_ENC1(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; \
      int _id_len1_##uid; \
      int _id_cnt1_##uid; \
      int ind##1 = 0; \
      while (_id_pos_##uid + 1 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD1_ENC1(ind, data, uid)

#define _ID_BAILEDLOOPELSE1_ENC1(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 1, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD1_ENC1(ind, data, uid) \
          _ID_READ(ind, data, 0, 1, uid)

#define _ID_LOOPSTOP1_ENC1(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 1, uid); \
    } \
  }

#define _ID_LOOPSTART2_ENC1(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; \
      int _id_len1_##uid; \
      int _id_cnt1_##uid; \
      int ind##1 = 0; int ind##2 = 0; \
      while (_id_pos_##uid + 2 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD2_ENC1(ind, data, uid)

#define _ID_BAILEDLOOPELSE2_ENC1(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 2, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD2_ENC1(ind, data, uid) \
          _ID_READ(ind, data, 0, 2, uid)

#define _ID_LOOPSTOP2_ENC1(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 2, uid); \
    } \
  }

#define _ID_LOOPSTART3_ENC1(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; \
      int _id_len1_##uid; \
      int _id_cnt1_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
      while (_id_pos_##uid + 3 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD3_ENC1(ind, data, uid)

#define _ID_BAILEDLOOPELSE3_ENC1(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 3, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD3_ENC1(ind, data, uid) \
          _ID_READ(ind, data, 0, 3, uid)

#define _ID_LOOPSTOP3_ENC1(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 3, uid); \
    } \
  }

#define _ID_LOOPSTART4_ENC1(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; \
      int _id_len1_##uid; \
      int _id_cnt1_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
      while (_id_pos_##uid + 4 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD4_ENC1(ind, data, uid)

#define _ID_BAILEDLOOPELSE4_ENC1(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 4, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD4_ENC1(ind, data, uid) \
          _ID_READ(ind, data, 0, 4, uid)

#define _ID_LOOPSTOP4_ENC1(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 4, uid); \
    } \
  }

#define _ID_LOOPSTART5_ENC1(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; \
      int _id_len1_##uid; \
      int _id_cnt1_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
      while (_id_pos_##uid + 5 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD5_ENC1(ind, data, uid)

#define _ID_BAILEDLOOPELSE5_ENC1(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 5, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD5_ENC1(ind, data, uid) \
          _ID_READ(ind, data, 0, 5, uid)

#define _ID_LOOPSTOP5_ENC1(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 5, uid); \
    } \
  }

#define _ID_LOOPSTART6_ENC1(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; \
      int _id_len1_##uid; \
      int _id_cnt1_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
      while (_id_pos_##uid + 6 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD6_ENC1(ind, data, uid)

#define _ID_BAILEDLOOPELSE6_ENC1(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; ind##6 -= _id_str1_6_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 6, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD6_ENC1(ind, data, uid) \
          _ID_READ(ind, data, 0, 6, uid)

#define _ID_LOOPSTOP6_ENC1(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 6, uid); \
    } \
  }

#define _ID_LOOPSTART1_ENC2(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str2_1_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; \
      int ind##1 = 0; \
      while (_id_pos_##uid + 1 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD1_ENC2(ind, data, uid)

#define _ID_BAILEDLOOPELSE1_ENC2(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; \
        ind##1 += _id_str2_1_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 1, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD1_ENC2(ind, data, uid) \
          _ID_READ(ind, data, 0, 1, uid)

#define _ID_LOOPSTOP1_ENC2(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 1, uid); \
    } \
  }

#define _ID_LOOPSTART2_ENC2(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; \
      int ind##1 = 0; int ind##2 = 0; \
      while (_id_pos_##uid + 2 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD2_ENC2(ind, data, uid)

#define _ID_BAILEDLOOPELSE2_ENC2(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 2, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD2_ENC2(ind, data, uid) \
          _ID_READ(ind, data, 0, 2, uid)

#define _ID_LOOPSTOP2_ENC2(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 2, uid); \
    } \
  }

#define _ID_LOOPSTART3_ENC2(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
      while (_id_pos_##uid + 3 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD3_ENC2(ind, data, uid)

#define _ID_BAILEDLOOPELSE3_ENC2(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 3, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD3_ENC2(ind, data, uid) \
          _ID_READ(ind, data, 0, 3, uid)

#define _ID_LOOPSTOP3_ENC2(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 3, uid); \
    } \
  }

#define _ID_LOOPSTART4_ENC2(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
      while (_id_pos_##uid + 4 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD4_ENC2(ind, data, uid)

#define _ID_BAILEDLOOPELSE4_ENC2(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 4, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD4_ENC2(ind, data, uid) \
          _ID_READ(ind, data, 0, 4, uid)

#define _ID_LOOPSTOP4_ENC2(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 4, uid); \
    } \
  }

#define _ID_LOOPSTART5_ENC2(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
      while (_id_pos_##uid + 5 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD5_ENC2(ind, data, uid)

#define _ID_BAILEDLOOPELSE5_ENC2(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 5, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD5_ENC2(ind, data, uid) \
          _ID_READ(ind, data, 0, 5, uid)

#define _ID_LOOPSTOP5_ENC2(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 5, uid); \
    } \
  }

#define _ID_LOOPSTART6_ENC2(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
      while (_id_pos_##uid + 6 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD6_ENC2(ind, data, uid)

#define _ID_BAILEDLOOPELSE6_ENC2(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; ind##6 -= _id_str1_6_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; ind##6 -= _id_str2_6_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 6, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD6_ENC2(ind, data, uid) \
          _ID_READ(ind, data, 0, 6, uid)

#define _ID_LOOPSTOP6_ENC2(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 6, uid); \
    } \
  }

#define _ID_LOOPSTART1_ENC3(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str2_1_##uid; int _id_str3_1_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; \
      int ind##1 = 0; \
      while (_id_pos_##uid + 1 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD1_ENC3(ind, data, uid)

#define _ID_BAILEDLOOPELSE1_ENC3(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; \
        ind##1 += _id_str2_1_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; \
        ind##1 += _id_str3_1_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 1, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD1_ENC3(ind, data, uid) \
          _ID_READ(ind, data, 0, 1, uid)

#define _ID_LOOPSTOP1_ENC3(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 1, uid); \
    } \
  }

#define _ID_LOOPSTART2_ENC3(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; \
      int ind##1 = 0; int ind##2 = 0; \
      while (_id_pos_##uid + 2 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD2_ENC3(ind, data, uid)

#define _ID_BAILEDLOOPELSE2_ENC3(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 2, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD2_ENC3(ind, data, uid) \
          _ID_READ(ind, data, 0, 2, uid)

#define _ID_LOOPSTOP2_ENC3(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 2, uid); \
    } \
  }

#define _ID_LOOPSTART3_ENC3(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
      while (_id_pos_##uid + 3 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD3_ENC3(ind, data, uid)

#define _ID_BAILEDLOOPELSE3_ENC3(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 3, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD3_ENC3(ind, data, uid) \
          _ID_READ(ind, data, 0, 3, uid)

#define _ID_LOOPSTOP3_ENC3(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 3, uid); \
    } \
  }

#define _ID_LOOPSTART4_ENC3(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
      while (_id_pos_##uid + 4 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD4_ENC3(ind, data, uid)

#define _ID_BAILEDLOOPELSE4_ENC3(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 4, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD4_ENC3(ind, data, uid) \
          _ID_READ(ind, data, 0, 4, uid)

#define _ID_LOOPSTOP4_ENC3(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 4, uid); \
    } \
  }

#define _ID_LOOPSTART5_ENC3(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
      while (_id_pos_##uid + 5 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD5_ENC3(ind, data, uid)

#define _ID_BAILEDLOOPELSE5_ENC3(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; ind##5 -= _id_str3_5_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 5, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD5_ENC3(ind, data, uid) \
          _ID_READ(ind, data, 0, 5, uid)

#define _ID_LOOPSTOP5_ENC3(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 5, uid); \
    } \
  }

#define _ID_LOOPSTART6_ENC3(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str3_6_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
      while (_id_pos_##uid + 6 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_str3_6_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD6_ENC3(ind, data, uid)

#define _ID_BAILEDLOOPELSE6_ENC3(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; ind##6 -= _id_str1_6_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; ind##6 -= _id_str2_6_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; ind##6 += _id_str3_6_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; ind##5 -= _id_str3_5_##uid; ind##6 -= _id_str3_6_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 6, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD6_ENC3(ind, data, uid) \
          _ID_READ(ind, data, 0, 6, uid)

#define _ID_LOOPSTOP6_ENC3(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 6, uid); \
    } \
  }

#define _ID_LOOPSTART1_ENC4(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str2_1_##uid; int _id_str3_1_##uid; int _id_str4_1_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; \
      int ind##1 = 0; \
      while (_id_pos_##uid + 1 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD1_ENC4(ind, data, uid)

#define _ID_BAILEDLOOPELSE1_ENC4(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; \
        ind##1 += _id_str2_1_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; \
        ind##1 += _id_str3_1_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; \
        ind##1 += _id_str4_1_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 1, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD1_ENC4(ind, data, uid) \
          _ID_READ(ind, data, 0, 1, uid)

#define _ID_LOOPSTOP1_ENC4(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 1, uid); \
    } \
  }

#define _ID_LOOPSTART2_ENC4(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; \
      int ind##1 = 0; int ind##2 = 0; \
      while (_id_pos_##uid + 2 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD2_ENC4(ind, data, uid)

#define _ID_BAILEDLOOPELSE2_ENC4(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 2, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD2_ENC4(ind, data, uid) \
          _ID_READ(ind, data, 0, 2, uid)

#define _ID_LOOPSTOP2_ENC4(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 2, uid); \
    } \
  }

#define _ID_LOOPSTART3_ENC4(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
      while (_id_pos_##uid + 3 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD3_ENC4(ind, data, uid)

#define _ID_BAILEDLOOPELSE3_ENC4(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 3, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD3_ENC4(ind, data, uid) \
          _ID_READ(ind, data, 0, 3, uid)

#define _ID_LOOPSTOP3_ENC4(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 3, uid); \
    } \
  }

#define _ID_LOOPSTART4_ENC4(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
      while (_id_pos_##uid + 4 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD4_ENC4(ind, data, uid)

#define _ID_BAILEDLOOPELSE4_ENC4(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 4, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD4_ENC4(ind, data, uid) \
          _ID_READ(ind, data, 0, 4, uid)

#define _ID_LOOPSTOP4_ENC4(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 4, uid); \
    } \
  }

#define _ID_LOOPSTART5_ENC4(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
      while (_id_pos_##uid + 5 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD5_ENC4(ind, data, uid)

#define _ID_BAILEDLOOPELSE5_ENC4(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; ind##5 -= _id_str3_5_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; ind##5 -= _id_str4_5_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 5, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD5_ENC4(ind, data, uid) \
          _ID_READ(ind, data, 0, 5, uid)

#define _ID_LOOPSTOP5_ENC4(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 5, uid); \
    } \
  }

#define _ID_LOOPSTART6_ENC4(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str3_6_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str4_6_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
      while (_id_pos_##uid + 6 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_str3_6_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_str4_6_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD6_ENC4(ind, data, uid)

#define _ID_BAILEDLOOPELSE6_ENC4(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; ind##6 -= _id_str1_6_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; ind##6 -= _id_str2_6_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; ind##6 += _id_str3_6_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; ind##5 -= _id_str3_5_##uid; ind##6 -= _id_str3_6_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; ind##6 += _id_str4_6_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; ind##5 -= _id_str4_5_##uid; ind##6 -= _id_str4_6_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 6, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD6_ENC4(ind, data, uid) \
          _ID_READ(ind, data, 0, 6, uid)

#define _ID_LOOPSTOP6_ENC4(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 6, uid); \
    } \
  }

#define _ID_LOOPSTART1_ENC5(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str2_1_##uid; int _id_str3_1_##uid; int _id_str4_1_##uid; int _id_str5_1_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; \
      int ind##1 = 0; \
      while (_id_pos_##uid + 1 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD1_ENC5(ind, data, uid)

#define _ID_BAILEDLOOPELSE1_ENC5(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; \
        ind##1 += _id_str2_1_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; \
        ind##1 += _id_str3_1_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; \
        ind##1 += _id_str4_1_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; \
        ind##1 += _id_str5_1_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 1, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD1_ENC5(ind, data, uid) \
          _ID_READ(ind, data, 0, 1, uid)

#define _ID_LOOPSTOP1_ENC5(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 1, uid); \
    } \
  }

#define _ID_LOOPSTART2_ENC5(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; \
      int ind##1 = 0; int ind##2 = 0; \
      while (_id_pos_##uid + 2 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD2_ENC5(ind, data, uid)

#define _ID_BAILEDLOOPELSE2_ENC5(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 2, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD2_ENC5(ind, data, uid) \
          _ID_READ(ind, data, 0, 2, uid)

#define _ID_LOOPSTOP2_ENC5(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 2, uid); \
    } \
  }

#define _ID_LOOPSTART3_ENC5(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
      while (_id_pos_##uid + 3 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD3_ENC5(ind, data, uid)

#define _ID_BAILEDLOOPELSE3_ENC5(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; ind##3 -= _id_str5_3_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 3, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD3_ENC5(ind, data, uid) \
          _ID_READ(ind, data, 0, 3, uid)

#define _ID_LOOPSTOP3_ENC5(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 3, uid); \
    } \
  }

#define _ID_LOOPSTART4_ENC5(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
      while (_id_pos_##uid + 4 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD4_ENC5(ind, data, uid)

#define _ID_BAILEDLOOPELSE4_ENC5(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; ind##3 -= _id_str5_3_##uid; ind##4 -= _id_str5_4_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 4, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD4_ENC5(ind, data, uid) \
          _ID_READ(ind, data, 0, 4, uid)

#define _ID_LOOPSTOP4_ENC5(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 4, uid); \
    } \
  }

#define _ID_LOOPSTART5_ENC5(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str5_5_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
      while (_id_pos_##uid + 5 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_str5_5_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD5_ENC5(ind, data, uid)

#define _ID_BAILEDLOOPELSE5_ENC5(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; ind##5 -= _id_str3_5_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; ind##5 -= _id_str4_5_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; ind##5 += _id_str5_5_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; ind##3 -= _id_str5_3_##uid; ind##4 -= _id_str5_4_##uid; ind##5 -= _id_str5_5_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 5, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD5_ENC5(ind, data, uid) \
          _ID_READ(ind, data, 0, 5, uid)

#define _ID_LOOPSTOP5_ENC5(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 5, uid); \
    } \
  }

#define _ID_LOOPSTART6_ENC5(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str3_6_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str4_6_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str5_5_##uid; int _id_str5_6_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
      while (_id_pos_##uid + 6 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_str3_6_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_str4_6_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_str5_5_##uid = (data)[_id_pos_##uid++]; _id_str5_6_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD6_ENC5(ind, data, uid)

#define _ID_BAILEDLOOPELSE6_ENC5(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; ind##6 -= _id_str1_6_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; ind##6 -= _id_str2_6_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; ind##6 += _id_str3_6_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; ind##5 -= _id_str3_5_##uid; ind##6 -= _id_str3_6_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; ind##6 += _id_str4_6_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; ind##5 -= _id_str4_5_##uid; ind##6 -= _id_str4_6_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; ind##5 += _id_str5_5_##uid; ind##6 += _id_str5_6_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; ind##3 -= _id_str5_3_##uid; ind##4 -= _id_str5_4_##uid; ind##5 -= _id_str5_5_##uid; ind##6 -= _id_str5_6_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 6, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD6_ENC5(ind, data, uid) \
          _ID_READ(ind, data, 0, 6, uid)

#define _ID_LOOPSTOP6_ENC5(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 6, uid); \
    } \
  }

#define _ID_LOOPSTART1_ENC6(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str2_1_##uid; int _id_str3_1_##uid; int _id_str4_1_##uid; int _id_str5_1_##uid; int _id_str6_1_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; int _id_len6_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; int _id_cnt6_##uid; \
      int ind##1 = 0; \
      while (_id_pos_##uid + 1 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt6_##uid = 0; _id_cnt6_##uid < _id_len6_##uid; _id_cnt6_##uid++) { \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD1_ENC6(ind, data, uid)

#define _ID_BAILEDLOOPELSE1_ENC6(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; \
        ind##1 += _id_str2_1_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; \
        ind##1 += _id_str3_1_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; \
        ind##1 += _id_str4_1_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; \
        ind##1 += _id_str5_1_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; \
        ind##1 += _id_str6_1_##uid; \
        } \
        ind##1 -= _id_str6_1_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 1, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD1_ENC6(ind, data, uid) \
          _ID_READ(ind, data, 0, 1, uid)

#define _ID_LOOPSTOP1_ENC6(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 1, uid); \
    } \
  }

#define _ID_LOOPSTART2_ENC6(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; int _id_len6_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; int _id_cnt6_##uid; \
      int ind##1 = 0; int ind##2 = 0; \
      while (_id_pos_##uid + 2 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt6_##uid = 0; _id_cnt6_##uid < _id_len6_##uid; _id_cnt6_##uid++) { \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD2_ENC6(ind, data, uid)

#define _ID_BAILEDLOOPELSE2_ENC6(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; \
        ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; \
        } \
        ind##1 -= _id_str6_1_##uid; ind##2 -= _id_str6_2_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 2, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD2_ENC6(ind, data, uid) \
          _ID_READ(ind, data, 0, 2, uid)

#define _ID_LOOPSTOP2_ENC6(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 2, uid); \
    } \
  }

#define _ID_LOOPSTART3_ENC6(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; int _id_str6_3_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; int _id_len6_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; int _id_cnt6_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; \
      while (_id_pos_##uid + 3 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_str6_3_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt6_##uid = 0; _id_cnt6_##uid < _id_len6_##uid; _id_cnt6_##uid++) { \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD3_ENC6(ind, data, uid)

#define _ID_BAILEDLOOPELSE3_ENC6(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; ind##3 -= _id_str5_3_##uid; \
        ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; ind##3 += _id_str6_3_##uid; \
        } \
        ind##1 -= _id_str6_1_##uid; ind##2 -= _id_str6_2_##uid; ind##3 -= _id_str6_3_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 3, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD3_ENC6(ind, data, uid) \
          _ID_READ(ind, data, 0, 3, uid)

#define _ID_LOOPSTOP3_ENC6(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 3, uid); \
    } \
  }

#define _ID_LOOPSTART4_ENC6(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; int _id_str6_3_##uid; int _id_str6_4_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; int _id_len6_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; int _id_cnt6_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; \
      while (_id_pos_##uid + 4 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_str6_3_##uid = (data)[_id_pos_##uid++]; _id_str6_4_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt6_##uid = 0; _id_cnt6_##uid < _id_len6_##uid; _id_cnt6_##uid++) { \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD4_ENC6(ind, data, uid)

#define _ID_BAILEDLOOPELSE4_ENC6(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; ind##3 -= _id_str5_3_##uid; ind##4 -= _id_str5_4_##uid; \
        ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; ind##3 += _id_str6_3_##uid; ind##4 += _id_str6_4_##uid; \
        } \
        ind##1 -= _id_str6_1_##uid; ind##2 -= _id_str6_2_##uid; ind##3 -= _id_str6_3_##uid; ind##4 -= _id_str6_4_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 4, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD4_ENC6(ind, data, uid) \
          _ID_READ(ind, data, 0, 4, uid)

#define _ID_LOOPSTOP4_ENC6(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 4, uid); \
    } \
  }

#define _ID_LOOPSTART5_ENC6(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str5_5_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; int _id_str6_3_##uid; int _id_str6_4_##uid; int _id_str6_5_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; int _id_len6_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; int _id_cnt6_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; \
      while (_id_pos_##uid + 5 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_str5_5_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_str6_3_##uid = (data)[_id_pos_##uid++]; _id_str6_4_##uid = (data)[_id_pos_##uid++]; _id_str6_5_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt6_##uid = 0; _id_cnt6_##uid < _id_len6_##uid; _id_cnt6_##uid++) { \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD5_ENC6(ind, data, uid)

#define _ID_BAILEDLOOPELSE5_ENC6(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; ind##5 -= _id_str3_5_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; ind##5 -= _id_str4_5_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; ind##5 += _id_str5_5_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; ind##3 -= _id_str5_3_##uid; ind##4 -= _id_str5_4_##uid; ind##5 -= _id_str5_5_##uid; \
        ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; ind##3 += _id_str6_3_##uid; ind##4 += _id_str6_4_##uid; ind##5 += _id_str6_5_##uid; \
        } \
        ind##1 -= _id_str6_1_##uid; ind##2 -= _id_str6_2_##uid; ind##3 -= _id_str6_3_##uid; ind##4 -= _id_str6_4_##uid; ind##5 -= _id_str6_5_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 5, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD5_ENC6(ind, data, uid) \
          _ID_READ(ind, data, 0, 5, uid)

#define _ID_LOOPSTOP5_ENC6(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 5, uid); \
    } \
  }

#define _ID_LOOPSTART6_ENC6(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      int _id_str1_1_##uid; int _id_str1_2_##uid; int _id_str1_3_##uid; int _id_str1_4_##uid; int _id_str1_5_##uid; int _id_str1_6_##uid; int _id_str2_1_##uid; int _id_str2_2_##uid; int _id_str2_3_##uid; int _id_str2_4_##uid; int _id_str2_5_##uid; int _id_str2_6_##uid; int _id_str3_1_##uid; int _id_str3_2_##uid; int _id_str3_3_##uid; int _id_str3_4_##uid; int _id_str3_5_##uid; int _id_str3_6_##uid; int _id_str4_1_##uid; int _id_str4_2_##uid; int _id_str4_3_##uid; int _id_str4_4_##uid; int _id_str4_5_##uid; int _id_str4_6_##uid; int _id_str5_1_##uid; int _id_str5_2_##uid; int _id_str5_3_##uid; int _id_str5_4_##uid; int _id_str5_5_##uid; int _id_str5_6_##uid; int _id_str6_1_##uid; int _id_str6_2_##uid; int _id_str6_3_##uid; int _id_str6_4_##uid; int _id_str6_5_##uid; int _id_str6_6_##uid; \
      int _id_len1_##uid; int _id_len2_##uid; int _id_len3_##uid; int _id_len4_##uid; int _id_len5_##uid; int _id_len6_##uid; \
      int _id_cnt1_##uid; int _id_cnt2_##uid; int _id_cnt3_##uid; int _id_cnt4_##uid; int _id_cnt5_##uid; int _id_cnt6_##uid; \
      int ind##1 = 0; int ind##2 = 0; int ind##3 = 0; int ind##4 = 0; int ind##5 = 0; int ind##6 = 0; \
      while (_id_pos_##uid + 6 < data[0]) { \
        ind##1 += (data)[_id_pos_##uid++]; ind##2 += (data)[_id_pos_##uid++]; ind##3 += (data)[_id_pos_##uid++]; ind##4 += (data)[_id_pos_##uid++]; ind##5 += (data)[_id_pos_##uid++]; ind##6 += (data)[_id_pos_##uid++]; \
        _id_str1_1_##uid = (data)[_id_pos_##uid++]; _id_str1_2_##uid = (data)[_id_pos_##uid++]; _id_str1_3_##uid = (data)[_id_pos_##uid++]; _id_str1_4_##uid = (data)[_id_pos_##uid++]; _id_str1_5_##uid = (data)[_id_pos_##uid++]; _id_str1_6_##uid = (data)[_id_pos_##uid++]; _id_len1_##uid = (data)[_id_pos_##uid++] - 1; _id_str2_1_##uid = (data)[_id_pos_##uid++]; _id_str2_2_##uid = (data)[_id_pos_##uid++]; _id_str2_3_##uid = (data)[_id_pos_##uid++]; _id_str2_4_##uid = (data)[_id_pos_##uid++]; _id_str2_5_##uid = (data)[_id_pos_##uid++]; _id_str2_6_##uid = (data)[_id_pos_##uid++]; _id_len2_##uid = (data)[_id_pos_##uid++] - 0; _id_str3_1_##uid = (data)[_id_pos_##uid++]; _id_str3_2_##uid = (data)[_id_pos_##uid++]; _id_str3_3_##uid = (data)[_id_pos_##uid++]; _id_str3_4_##uid = (data)[_id_pos_##uid++]; _id_str3_5_##uid = (data)[_id_pos_##uid++]; _id_str3_6_##uid = (data)[_id_pos_##uid++]; _id_len3_##uid = (data)[_id_pos_##uid++] - 0; _id_str4_1_##uid = (data)[_id_pos_##uid++]; _id_str4_2_##uid = (data)[_id_pos_##uid++]; _id_str4_3_##uid = (data)[_id_pos_##uid++]; _id_str4_4_##uid = (data)[_id_pos_##uid++]; _id_str4_5_##uid = (data)[_id_pos_##uid++]; _id_str4_6_##uid = (data)[_id_pos_##uid++]; _id_len4_##uid = (data)[_id_pos_##uid++] - 0; _id_str5_1_##uid = (data)[_id_pos_##uid++]; _id_str5_2_##uid = (data)[_id_pos_##uid++]; _id_str5_3_##uid = (data)[_id_pos_##uid++]; _id_str5_4_##uid = (data)[_id_pos_##uid++]; _id_str5_5_##uid = (data)[_id_pos_##uid++]; _id_str5_6_##uid = (data)[_id_pos_##uid++]; _id_len5_##uid = (data)[_id_pos_##uid++] - 0; _id_str6_1_##uid = (data)[_id_pos_##uid++]; _id_str6_2_##uid = (data)[_id_pos_##uid++]; _id_str6_3_##uid = (data)[_id_pos_##uid++]; _id_str6_4_##uid = (data)[_id_pos_##uid++]; _id_str6_5_##uid = (data)[_id_pos_##uid++]; _id_str6_6_##uid = (data)[_id_pos_##uid++]; _id_len6_##uid = (data)[_id_pos_##uid++] - 0; \
        for (_id_cnt6_##uid = 0; _id_cnt6_##uid < _id_len6_##uid; _id_cnt6_##uid++) { \
        for (_id_cnt5_##uid = 0; _id_cnt5_##uid < _id_len5_##uid; _id_cnt5_##uid++) { \
        for (_id_cnt4_##uid = 0; _id_cnt4_##uid < _id_len4_##uid; _id_cnt4_##uid++) { \
        for (_id_cnt3_##uid = 0; _id_cnt3_##uid < _id_len3_##uid; _id_cnt3_##uid++) { \
        for (_id_cnt2_##uid = 0; _id_cnt2_##uid < _id_len2_##uid; _id_cnt2_##uid++) { \
        for (_id_cnt1_##uid = 0; _id_cnt1_##uid <= _id_len1_##uid; _id_cnt1_##uid++) { \

#define _ID_LOOPREAD6_ENC6(ind, data, uid)

#define _ID_BAILEDLOOPELSE6_ENC6(ind, data, uid) \
        ind##1 += _id_str1_1_##uid; ind##2 += _id_str1_2_##uid; ind##3 += _id_str1_3_##uid; ind##4 += _id_str1_4_##uid; ind##5 += _id_str1_5_##uid; ind##6 += _id_str1_6_##uid; \
        } \
        ind##1 -= _id_str1_1_##uid; ind##2 -= _id_str1_2_##uid; ind##3 -= _id_str1_3_##uid; ind##4 -= _id_str1_4_##uid; ind##5 -= _id_str1_5_##uid; ind##6 -= _id_str1_6_##uid; \
        ind##1 += _id_str2_1_##uid; ind##2 += _id_str2_2_##uid; ind##3 += _id_str2_3_##uid; ind##4 += _id_str2_4_##uid; ind##5 += _id_str2_5_##uid; ind##6 += _id_str2_6_##uid; \
        } \
        ind##1 -= _id_str2_1_##uid; ind##2 -= _id_str2_2_##uid; ind##3 -= _id_str2_3_##uid; ind##4 -= _id_str2_4_##uid; ind##5 -= _id_str2_5_##uid; ind##6 -= _id_str2_6_##uid; \
        ind##1 += _id_str3_1_##uid; ind##2 += _id_str3_2_##uid; ind##3 += _id_str3_3_##uid; ind##4 += _id_str3_4_##uid; ind##5 += _id_str3_5_##uid; ind##6 += _id_str3_6_##uid; \
        } \
        ind##1 -= _id_str3_1_##uid; ind##2 -= _id_str3_2_##uid; ind##3 -= _id_str3_3_##uid; ind##4 -= _id_str3_4_##uid; ind##5 -= _id_str3_5_##uid; ind##6 -= _id_str3_6_##uid; \
        ind##1 += _id_str4_1_##uid; ind##2 += _id_str4_2_##uid; ind##3 += _id_str4_3_##uid; ind##4 += _id_str4_4_##uid; ind##5 += _id_str4_5_##uid; ind##6 += _id_str4_6_##uid; \
        } \
        ind##1 -= _id_str4_1_##uid; ind##2 -= _id_str4_2_##uid; ind##3 -= _id_str4_3_##uid; ind##4 -= _id_str4_4_##uid; ind##5 -= _id_str4_5_##uid; ind##6 -= _id_str4_6_##uid; \
        ind##1 += _id_str5_1_##uid; ind##2 += _id_str5_2_##uid; ind##3 += _id_str5_3_##uid; ind##4 += _id_str5_4_##uid; ind##5 += _id_str5_5_##uid; ind##6 += _id_str5_6_##uid; \
        } \
        ind##1 -= _id_str5_1_##uid; ind##2 -= _id_str5_2_##uid; ind##3 -= _id_str5_3_##uid; ind##4 -= _id_str5_4_##uid; ind##5 -= _id_str5_5_##uid; ind##6 -= _id_str5_6_##uid; \
        ind##1 += _id_str6_1_##uid; ind##2 += _id_str6_2_##uid; ind##3 += _id_str6_3_##uid; ind##4 += _id_str6_4_##uid; ind##5 += _id_str6_5_##uid; ind##6 += _id_str6_6_##uid; \
        } \
        ind##1 -= _id_str6_1_##uid; ind##2 -= _id_str6_2_##uid; ind##3 -= _id_str6_3_##uid; ind##4 -= _id_str6_4_##uid; ind##5 -= _id_str6_5_##uid; ind##6 -= _id_str6_6_##uid; \
      } \
    } \
    else { \
      _ID_START(ind, data, 0, 6, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD6_ENC6(ind, data, uid) \
          _ID_READ(ind, data, 0, 6, uid)

#define _ID_LOOPSTOP6_ENC6(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, 6, uid); \
    } \
  }

#endif
