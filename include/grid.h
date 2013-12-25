/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "ironman.h"

#ifndef _GRID_H
#define _GRID_H

#include "struct.h"
#include "ironman.h"
#include "zplglobal.h"
#include "type.h"
#include "direction.h"
#include "registry.h"
#include "priv_access.h"

#define _UNIPROC(numdims) ((1 << (numdims)) - 1)

#define PROCESSORS (_PROCESSORS)
#define GRIDPROCS(grid, dim) (_GRID_SIZE(grid, dim - 1))
#define GRIDPROC(grid, dim) (_GRID_LOC(grid, dim - 1))

#define numLocales() (_PROCESSORS)
#define localeID() (_INDEX)

#define _NEXTPROC ((_INDEX+1)%_PROCESSORS)
#define _LEGALPROC (_INDEX < _PROCESSORS)

#define _GRID_SIZE_V(grid) (grid->size)
#define _GRID_SIZE(grid, dim) (grid->size[(dim)])
#define _GRID_LOC_V(grid) (grid->loc)
#define _GRID_LOC(grid, dim) (grid->loc[(dim)])
#define _GRID_BLK_V(grid) (grid->blocksize)
#define _GRID_BLK(grid, dim) (grid->blocksize[(dim)])
#define _GRID_NUMSLICES(grid) (grid->numslices)
#define _GRID_DISTRIBUTED_V(grid) (grid->distributed)
#define _GRID_DISTRIBUTED(grid, slc) (grid->distributed[slc])

#define _GRID_LABEL(grid) (grid->grid_label)

#define _GRID_BEG(grid, dim) (_GRID_LOC(grid, dim) == 0)
#define _GRID_END(grid, dim) (_GRID_LOC(grid, dim) == _GRID_SIZE(grid, dim) - 1)

#define _GRID_NUM_USEFUL_COMM_DIRS(grid) (grid->num_useful_comm_dirs)
#define _GRID_NUM_USEFUL_AT_DIRS(grid) (grid->num_useful_at_dirs)
#define _GRID_DIRPTR(grid) (grid->dirptr)

#define _GRID_REGISTER(grid, dist) _GRID_DISTLIST(grid) = _REGISTER(_GRID_DISTLIST(grid), (void*)dist)
#define _GRID_UNREGISTER(grid, dist) _GRID_DISTLIST(grid) = _UNREGISTER(_GRID_DISTLIST(grid), (void*)dist) 
#define _GRID_DISTLIST(grid) (grid->distributions)
#define _GRID_DISTLIST_DIST(gdlist) ((_distribution_fnc)_REGISTRY_PTR(gdlist))
#define _GRID_DISTLIST_NEXT(gdlist) (_REGISTRY_NEXT(gdlist))

#define _GRID_INIT_BIT 01
#define _GRID_SETUP_BIT 02

#define _GRID_CLEAR_FLAG(grid)      (grid->gridflag = 0)

#define _GRID_GET_INIT(grid)        (grid->gridflag & _GRID_INIT_BIT)
#define _GRID_SET_INIT_ON(grid)     (grid->gridflag |= _GRID_INIT_BIT)
#define _GRID_SET_INIT_OFF(grid)    (grid->gridflag &= ~_GRID_INIT_BIT)

#define _GRID_GET_SETUP(grid)        (grid->gridflag & _GRID_SETUP_BIT)
#define _GRID_SET_SETUP_ON(grid)     (grid->gridflag |= _GRID_SETUP_BIT)
#define _GRID_SET_SETUP_OFF(grid)    (grid->gridflag &= ~_GRID_SETUP_BIT)

typedef struct {
  _region_pnc reg;
  _array_pnc arr;
  _vector lo;
  _vector hi;
  unsigned int elemsize;
  char* buffer;
} spsinfo;

typedef struct {
  int numsps;
  spsinfo* vect;
} spsinfovect;

typedef struct {
  spsinfovect recvsps;
  spsinfovect sendsps;
} spscomminfo;

typedef struct {
  int recvready;
  IM_comminfo* recvinfo;
  int sendready;
  IM_comminfo* sendinfo;
} comminfo;

typedef struct {
  unsigned int elemsize;
  _array_pnc arr;
  int lhscomm;
  int numwrapdirs;
  int numdirs;
  _direction_pnc* dirlist;
} _arrcomminfo;

typedef struct {
  int numarr;
  _arrcomminfo* arrlist;
} _atcomminfo;

typedef struct {
  int remcomms;
  int lclcomms;
} commcount;

struct __grid_info {
  int gridflag;
  int processors;          /* number of processors involved            */
  int index;               /* my index number                          */
  int size[_MAXRANK];      /* number of processors per dimension       */
  int loc[_MAXRANK];       /* my location in the processor grid        */
  int blocksize[_MAXRANK]; /* multiplicative factors for conversion of
	 		      logical indices to/from processor index  */
  int numslices;           /* number of slices = 2^rank                */
  int* distributed;        /* 1 if slice is distributed; 0 otherwise   */

  int num_useful_comm_dirs;     /* at_comm stuff */
  int num_useful_at_dirs;       /* at_comm stuff */
  int* dirptr;                  /* at_comm stuff */

  _registry* distributions;
  char* grid_label;
  _MD_GRID_INFO            /* machine dependent info, e.g. proc groups */
};

extern _grid_fnc _DefaultGrid;

extern int _PROCESSORS;
extern int _INDEX;

#define _UPDATE_GRID(G1, G2, preserve) _UpdateGrid(G1, G2, preserve)
#define _COPY_GRID(G1, G2, process_arrays) _CopyGrid(G1, G2, process_arrays)

#endif
