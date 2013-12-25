/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "stand_inc.h"

_PRIV_DECL(, _vector, _zindex);

/*** This is a bad attempt to keep the code easier to maintain.      ***/
/***                                                                 ***/
/*** If we need to privatize _Index, set up the _IndexInfo any way   ***/
/***  but set the "origin" and "data" to something fake (_MYzindex). ***/
/***  In _InitIndexi(), copy the appropriate _IndexInfo, and reset   ***/
/***  the "origin" and "data".  Yuck.  On the bright side, versions  ***/
/***  that don't need to privatize still have the advantage of using ***/
/***  statically assigned stuff that is const (if there is an        ***/
/***  advantage at all).                                             ***/
#ifndef _PRIV_SPECIAL
#define _MYzindex _zindex
#else
static _vector _MYzindex;
#endif

static void *_AccessIndex1(_array arr,_vector i) {
  return &(i[0]);
}

static void *_AccessIndex2(_array arr,_vector i) {
  return &(i[1]);
}

static void *_AccessIndex3(_array arr,_vector i) {
  return &(i[2]);
}

static void *_AccessIndex4(_array arr,_vector i) {
  return &(i[3]);
}

static void *_AccessIndex5(_array arr,_vector i) {
  return &(i[4]);
}

static void *_AccessIndex6(_array arr,_vector i) {
  return &(i[5]);
}

static void _PrintIndex(_zfile outfile, void* elemptr, char* ctrl) {
  fprintf(outfile->fptr, ctrl, *(int*)(elemptr));
}

static _arr_info _IndexInfo[_MAXRANK] = {
  {(char *)&(_MYzindex[0]),0,{0,0,0,0,0,0},_ACCESSOR_CAST(_AccessIndex1),
   (char *)&(_MYzindex[0]),{{0,1},{0,1},{0,1},{0,1},{0,1},{0,1}},1,NULL,
   sizeof(int),{{0,0,0,0,0,0},{0,0,0,0,0,0},0},"int","I0",_PrintIndex,NULL},
  {(char *)&(_MYzindex[1]),0,{0,0,0,0,0,0},_ACCESSOR_CAST(_AccessIndex2),
   (char *)&(_MYzindex[1]),{{0,1},{0,1},{0,1},{0,1},{0,1},{0,1}},2,NULL,
   sizeof(int),{{0,0,0,0,0,0},{0,0,0,0,0,0},0},"int","I1",_PrintIndex,NULL},
  {(char *)&(_MYzindex[2]),0,{0,0,0,0,0,0},_ACCESSOR_CAST(_AccessIndex3),
   (char *)&(_MYzindex[2]),{{0,1},{0,1},{0,1},{0,1},{0,1},{0,1}},3,NULL,
   sizeof(int),{{0,0,0,0,0,0},{0,0,0,0,0,0},0},"int","I2",_PrintIndex,NULL},
  {(char *)&(_MYzindex[3]),0,{0,0,0,0,0,0},_ACCESSOR_CAST(_AccessIndex4),
   (char *)&(_MYzindex[3]),{{0,1},{0,1},{0,1},{0,1},{0,1},{0,1}},4,NULL,
   sizeof(int),{{0,0,0,0,0,0},{0,0,0,0,0,0},0},"int","I3",_PrintIndex,NULL},
  {(char *)&(_MYzindex[4]),0,{0,0,0,0,0,0},_ACCESSOR_CAST(_AccessIndex5),
   (char *)&(_MYzindex[4]),{{0,1},{0,1},{0,1},{0,1},{0,1},{0,1}},5,NULL,
   sizeof(int),{{0,0,0,0,0,0},{0,0,0,0,0,0},0},"int","I4",_PrintIndex,NULL},
  {(char *)&(_MYzindex[5]),0,{0,0,0,0,0,0},_ACCESSOR_CAST(_AccessIndex6),
   (char *)&(_MYzindex[5]),{{0,1},{0,1},{0,1},{0,1},{0,1},{0,1}},6,NULL,
   sizeof(int),{{0,0,0,0,0,0},{0,0,0,0,0,0},0},"int","I5",_PrintIndex,NULL},
};

#ifndef _PRIV_SPECIAL
_IndexType _Index = {&_IndexInfo[0],
		       &_IndexInfo[1],
		       &_IndexInfo[2],
		       &_IndexInfo[3],
		       &_IndexInfo[4],
		       &_IndexInfo[5]
		   };
#else
_PRIV_DECL(, _IndexType, _Index);
#endif

void
_InitIndexi()
{
#ifdef _PRIV_SPECIAL
  int i;
  _PRIV_TID_DECL;

  _PRIV_ALLOC(_vector, _zindex);
  _PRIV_ALLOC(_IndexType, _Index);

  for (i = 0; i < _MAXRANK; i++) {
    _PRIV_ACCESS(_Index)[i] = _zmalloc(sizeof(_ens_info));
    memcpy(_PRIV_ACCESS(_Index)[i], &(_IndexInfo[i]), sizeof(_ens_info));

    _PRIV_ACCESS(_Index)[i]->origin = &(_PRIV_ACCESS(_zindex)[i]);
    _PRIV_ACCESS(_Index)[i]->data = &(_PRIV_ACCESS(_zindex)[i]);
  }

#endif
}
