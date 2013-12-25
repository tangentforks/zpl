/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "type.h"
#include "zlib.h"

fcomplex _fcomplex_temp1;
fcomplex _fcomplex_temp2;
dcomplex _dcomplex_temp1;
dcomplex _dcomplex_temp2;
qcomplex _qcomplex_temp1;
qcomplex _qcomplex_temp2;

double _ZPL_SQR(double x) {
  return (x*x);
}


double _ZPL_CUBE(double x) {
  return (x*x*x);
}


int _bpop(_zulonglong ull) {
  static _zulonglong _bpop1_ull = 0; /* to become OCTAL 111...1 */
  static _zulonglong _bpop3_ull = 0; /* to become OCTAL 333...3 */
  static _zulonglong _bpop7_ull = 0; /* to become OCTAL 007007...007 */
  _zulonglong tmp;

  if (_bpop1_ull == 0) {
    _bpop1_ull = 01;
    do {
      tmp = _bpop1_ull;
      _bpop1_ull = 01 + (_bpop1_ull << 3);
    } while (_bpop1_ull != tmp);
    _bpop3_ull = 03;
    do {
      tmp = _bpop3_ull;
      _bpop3_ull = 03 + (_bpop3_ull << 3);
    } while (_bpop3_ull != tmp);
    _bpop7_ull = 07;
    do {
      tmp = _bpop7_ull;
      _bpop7_ull = 07 + (_bpop7_ull << 9);
    } while (_bpop7_ull != tmp);
  }
  tmp = ull - ((ull >> 1) & _bpop3_ull) - ((ull >> 2) & _bpop1_ull);
  return (((tmp + (tmp >> 3)) & _bpop7_ull) + ((tmp >> 6) & _bpop7_ull)) % 511;
}
