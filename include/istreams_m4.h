/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

changequote({{, }})dnl
changecom({{/*}}, {{*/}})dnl
define({{m4fordo}}, {{ifelse(eval({{$2}} <= {{$3}}), {{1}}, {{pushdef({{$1}}, {{$2}})_m4fordo({{$1}}, {{$2}}, {{$3}}, {{$4}}, {{$5}})popdef({{$1}})}})}})dnl
define({{_m4fordo}}, {{$4{{}}ifelse($1, {{$3}}, , {{$5{{}}define({{$1}}, incr($1))_m4fordo({{$1}}, {{$2}}, {{$3}}, {{$4}}, {{$5}})}})}})dnl
define({{m4downdo}}, {{ifelse(eval({{$2}} >= {{$3}}), {{1}}, {{pushdef({{$1}}, {{$2}})_m4downdo({{$1}}, {{$2}}, {{$3}}, {{$4}}, {{$5}})popdef({{$1}})}})}})dnl
define({{_m4downdo}}, {{$4{{}}ifelse($1, {{$3}}, , {{$5{{}}define({{$1}}, decr($1))_m4downdo({{$1}}, {{$2}}, {{$3}}, {{$4}}, {{$5}})}})}})dnl
dnl
define({{formalnumargs}}, {{m4fordo({{stream}}, 1, streams, {{int num{{}}stream}}, {{, }})}})dnl
define({{actualnumargs}}, {{m4fordo({{stream}}, 1, streams, {{num{{}}stream}}, {{, }})}})dnl
dnl /* foreachstream(streams, statement) */
dnl /* foreachlevel(levels, statement) */
dnl /* foreachgrade(level, statement) */
define({{foreachstream}}, {{m4fordo({{stream}}, 1, $1, {{$2}}, {{; }})}})dnl
define({{foreachlevel}}, {{m4fordo({{level}}, 1, $1, {{$2}}, {{; }})}})dnl
define({{foreacharray}}, {{m4fordo({{array}}, 1, $1, {{$2}}, {{; }})}})dnl
define({{foreachgrade}}, {{m4fordo({{grade}}, 1, $1, {{$2}}, {{; }})}})dnl
dnl
dnl /* foreachstreamcond(streams, statement) */
dnl /* foreachlevelcond(levels, statement) */
dnl /* foreachgradecond(level, statement) */
define({{foreachstreamcond}}, {{m4fordo({{stream}}, 1, $1, {{$2}}, {{ && }})}})dnl
define({{foreachlevelcond}}, {{m4fordo({{level}}, 1, $1, {{$2}}, {{ && }})}})dnl
define({{foreachgradecond}}, {{m4fordo({{grade}}, 1, $1, {{$2}}, {{ && }})}})dnl
dnl
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
m4fordo({{streams}}, 1, 6, {{dnl
#define _ID_START{{}}streams{{}}_ENC0(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    foreachstream({{streams}}, {{int ind{{}}##{{}}stream = 0}})

#define _ID_READ{{}}streams{{}}_ENC0(ind, data, uid) \
    foreachstream({{streams}}, {{ind{{}}##{{}}stream = data[_id_pos_##uid++]}})

#define _ID_STOP{{}}streams{{}}_ENC0(ind, data, uid) \
  }
}}, {{
}})dnl

/* Decoding the encoded */
m4fordo({{levels}}, 1, 6, {{dnl
m4fordo({{streams}}, 1, 6, {{dnl
#define _ID_START{{}}streams{{}}_ENC{{}}levels{{}}(ind, data, uid) \
  if (data) { \
    int _id_pos_##uid = 3; \
    const int _id_enc_##uid = data[2]; \
    foreachstream({{streams}}, {{int ind{{}}##{{}}stream = 0}}); \
    foreachlevel({{levels}}, {{foreachstream({{streams}}, {{int _id_str{{}}level{{}}_{{}}stream{{}}_##uid}})}}); \
    foreachlevel({{levels}}, {{int _id_len{{}}level{{}}_##uid = 0}}); \
    foreachlevel(eval(levels - 1), {{int _id_slen{{}}level{{}}_##uid}})

#define _ID_READ{{}}streams{{}}_ENC{{}}levels{{}}(ind, data, uid) \
    if (!_id_enc_##uid) { \
      foreachstream({{streams}}, {{ind{{}}##{{}}stream = data[_id_pos_##uid++]}}); \
    } \
    else { \
      m4fordo({{level}}, 1, levels, {{if (_id_len{{}}level{{}}_##uid > eval(level > 1)) { \
	foreachstream({{streams}}, {{ind{{}}##{{}}stream += _id_str{{}}level{{}}_{{}}stream{{}}_##uid}}); \
        _id_len{{}}level{{}}_##uid--; \
        foreachgrade(eval(level - 1), {{_id_len{{}}grade{{}}_##uid = _id_slen{{}}grade{{}}_##uid}}); \
      } \
}}, {{      else }})dnl
      else { \
        foreachstream({{streams}}, {{ind{{}}##{{}}stream += (data)[_id_pos_##uid++]}}); \
        foreachlevel({{levels}}, {{foreachstream({{streams}}, {{_id_str{{}}level{{}}_{{}}stream{{}}_##uid = (data)[_id_pos_##uid++]}}); _id_len{{}}level{{}}_##uid = (data)[_id_pos_##uid++] - eval(level = 1)}}); \
        foreachlevel(eval(levels - 1), {{_id_slen{{}}level{{}}_##uid = _id_len{{}}level{{}}_##uid}}); \
      } \
    }

#define _ID_STOP{{}}streams{{}}_ENC{{}}levels{{}}(ind, data, uid) \
  }
}}, {{
}})dnl
}}, {{
}})dnl

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

m4fordo({{levels}}, 1, 6, {{dnl
m4fordo({{streams}}, 1, 6, {{dnl
#define _ID_LOOPSTART{{}}streams{{}}_ENC{{}}levels{{}}(ind, data, uid) \
  if (data) { \
    if (data[2]) { \
      int _id_pos_##uid = 3; \
      foreachlevel({{levels}}, {{foreachstream({{streams}}, {{int _id_str{{}}level{{}}_{{}}stream{{}}_##uid}})}}); \
      foreachlevel({{levels}}, {{int _id_len{{}}level{{}}_##uid}}); \
      foreachlevel({{levels}}, {{int _id_cnt{{}}level{{}}_##uid}}); \
      foreachstream({{streams}}, {{int ind{{}}##{{}}stream = 0}}); \
      while (_id_pos_##uid + streams < data[0]) { \
        foreachstream({{streams}}, {{ind{{}}##{{}}stream += (data)[_id_pos_##uid++]}}); \
        foreachlevel({{levels}}, {{foreachstream({{streams}}, {{_id_str{{}}level{{}}_{{}}stream{{}}_##uid = (data)[_id_pos_##uid++]}}); _id_len{{}}level{{}}_##uid = (data)[_id_pos_##uid++] - eval(level = 1)}}); \
        m4downdo({{level}}, levels, 1, {{dnl
for (_id_cnt{{}}level{{}}_##uid = 0; _id_cnt{{}}level{{}}_##uid ifelse(level, {{1}}, {{<=}}, {{<}}) _id_len{{}}level{{}}_##uid; _id_cnt{{}}level{{}}_##uid++) { \
}}, {{        }})dnl

#define _ID_LOOPREAD{{}}streams{{}}_ENC{{}}levels{{}}(ind, data, uid)

#define _ID_BAILEDLOOPELSE{{}}streams{{}}_ENC{{}}levels{{}}(ind, data, uid) \
        m4fordo({{level}}, 1, levels, {{dnl
foreachstream({{streams}}, {{ind{{}}##{{}}stream += _id_str{{}}level{{}}_{{}}stream{{}}_##uid}}); \
        } \
        foreachstream({{streams}}, {{ind{{}}##{{}}stream -= _id_str{{}}level{{}}_{{}}stream{{}}_##uid}}); \
}}, {{        }})dnl
      } \
    } \
    else { \
      _ID_START(ind, data, 0, streams, uid); \
      { \
        int _id_i; \
        for (_id_i = 0; _id_i < data[1]; _id_i++) {

#define _ID_BAILEDLOOPREAD{{}}streams{{}}_ENC{{}}levels{{}}(ind, data, uid) \
          _ID_READ(ind, data, 0, streams, uid)

#define _ID_LOOPSTOP{{}}streams{{}}_ENC{{}}levels{{}}(ind, data, uid) \
        } \
      } \
      _ID_STOP(ind, data, 0, streams, uid); \
    } \
  }
}}, {{
}})dnl
}}, {{
}})dnl

#endif
