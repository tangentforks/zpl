/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __INDEX_LIST_H_
#define __INDEX_LIST_H_

typedef struct __index_list_info {
  int numdims;
  int capacity;
  int numelms;
  int* data;
  int minind[_MAXRANK];
  int maxind[_MAXRANK];
  _region_pnc reg;
  struct __index_list_info* headers;
} _index_list_info;

typedef _index_list_info* _index_list;

#endif

