/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ZLIB_H_
#define __ZLIB_H_

#include "md_zinc.h"
#include "direction.h"
#include "ensemble.h"
#include "file_io_base.h"
#include "index_list.h"
#include "ironman.h"
#include "region.h"
#include "timer.h"
#include "grid.h"

/* at_comm.c */

int _SetupDenseMemInfo(IM_memblock* const,_region,_array,const int,
		       const _vector,const _vector,const unsigned int);
void _InitAtComm(const int,const int [],const int,const int);
void _InitAtCommPerGrid(_grid_fnc);
void _LocalComm(IM_comminfo* const,IM_comminfo* const);
void _AtComm_New(_region,const int,const _arrcomminfo [],const int);
void _AtComm_DR(_region, const int);
void _AtComm_SR(_region, const int);

#ifdef IM_DN
#define _AtComm_DN(r, x)
#else
void _AtComm_DN(_region, const int);
#endif
#ifdef IM_SV
#define _AtComm_SV(r, x)
#else
void _AtComm_SV(_region, const int);
#endif
void _AtComm_Old(_region, const int);


/* broadcast.c */

void _Broadcast_dflt(_grid, IM_comminfo*,IM_comminfo*,int,int);
void _BroadcastToSlice_dflt(_grid, IM_comminfo*,IM_comminfo*,int,int,int,int);
#ifndef _MD_BCAST_NO_DEFAULT
#define _Broadcast        _Broadcast_dflt
#define _BroadcastToSlice _BroadcastToSlice_dflt
#endif


/* config.c */

void _ParseCfgFile(char *);
void _ParseFileArgs(int,char* []);
void _ParseSetArgs(int,char* []);
void _ParseConfigArgs(int,char* []);


/* distribution.c */

void _SetupDistribution(_distribution_fnc, _grid_fnc, char*);
void _InitDistribution(_distribution_fnc dist, int process_arrays);
void _InitDistributionCore(_grid grid, _distribution_fnc dist, int process_arrays, int deep);
void _print_distribution_hierarchy(_distribution);
void _UpdateDistribution(_distribution D1, _distribution_nc D2, int preserve);
void _CopyDistribution(_distribution_fnc D1, _distribution D2, int process_arrays);

/* dyn_reg.c */

void _SetupWrapReflect(_region,_region,_direction);
void _AdjustRegionWithOf(_region_fnc,_region,_direction);
void _AdjustRegionWithAt(_region_fnc,_region,_direction);
void _AdjustRegionWithIn(_region_fnc,_region,_direction);
void _AdjustRegionWithBy(_region_fnc,_region,_direction);
void _AdjustRegionWithPlus(_region_fnc,_region,_direction);
void _AdjustRegionWithMinus(_region_fnc,_region,_direction);
void _InitDynamicRegion(_region_fnc);


/* ensemble.c */

char* _ComputeEnsembleOrigin(_array);
void _SetupEnsemble(_array_fnc, _region_fnc, int, char*(*)(_array, _vector),
		    void(*)(_region,_array,_array), char*, int, int);
void _InitEnsemble(_array_fnc arr);
void _InitEnsembleCore(_grid grid, _distribution dist, _region reg, _array_fnc arr);
void _DestroyEnsemble(_array_fnc, int);
void _ProcessEnsembles();


/* file_io.c */

void _InitIO(void);
_zfile _OpenFile(char *,char *);
int _EOFile(_zfile);
int _CloseFile(_zfile *);
void bind_write_func(_array_fnc,_filepointfn);
void bind_read_func(_array_fnc,_filepointfn);
void unbind_write_func(_array_fnc);
void unbind_read_func(_array_fnc);


/* file_io_elem.c */

void _StreamOut_bool(_zfile,void *,char *);
void _StreamOut_char(_zfile,void *,char *);
void _StreamOut_uchar(_zfile,void *,char *);
void _StreamOut_short(_zfile,void *,char *);
void _StreamOut_int(_zfile,void *,char *);
void _StreamOut_long(_zfile,void *,char *);
void _StreamOut_float(_zfile,void *,char *);
void _StreamOut_double(_zfile,void *,char *);
void _StreamOut_quad(_zfile,void *,char *);
void _StreamOut_string(_zfile,void *,char *);
void _StreamOut_fcomplex(_zfile,void *,char *);
void _StreamOut_dcomplex(_zfile,void *,char *);
void _StreamOut_qcomplex(_zfile,void *,char *);
void _StreamIn_bool(_zfile,void *,char *);
void _StreamIn_char(_zfile,void *,char *);
void _StreamIn_uchar(_zfile,void *,char *);
void _StreamIn_short(_zfile,void *,char *);
void _StreamIn_int(_zfile,void *,char *);
void _StreamIn_long(_zfile,void *,char *);
void _StreamIn_float(_zfile,void *,char *);
void _StreamIn_double(_zfile,void *,char *);
void _StreamIn_quad(_zfile,void *,char *);
void _StreamIn_string(_zfile,void *,char *);
void _StreamIn_fcomplex(_zfile,void *,char *);
void _StreamIn_dcomplex(_zfile,void *,char *);
void _StreamIn_qcomplex(_zfile,void *,char *);


/* file_io_ens.c */

void _InitEnsIO(void);
void _StreamEnsembleTxt(_zfile,_region,_array,int,_filepointfn,char *);
void _StreamEnsembleBin(_zfile,_region,_array,int,unsigned int);
void _StreamRegionTxt(_zfile,_region_fnc,int,char*);
void _StreamRegionBin(_zfile,_region_fnc,int,char*);
void _DrawRegionSparsity(_zfile,_region_fnc);


/* flood.c */

void _FloodGuts(_region,_region,_array,_array,unsigned int,int);
void _SetupTempFloodEnsemble(_array_fnc,_region_fnc,unsigned int);
void _DestroyTempFloodEnsemble(_array);
void _CreateFloodRegion(_region_fnc,_region,_region,int);
int _Flood(_region,_region,_array,_array,unsigned int,int);


/* genaccess.c */

void *_GenAccess(_array,int *);
void *_GenFastAccess(_array,int *);
int _GenAccessDistance(_array,int *,int *);


/* grid.c */

void _SetupGrid(_grid_fnc, char*, int[], int);
void _InitGrid(_grid_fnc grid, int process_arrays);
void _InitGridCore(_grid_fnc grid, int process_arrays, int deep);
void _print_grid_hierarchy(_grid);
int _GRID_TO_PROC(_grid, const int,const int []);
int _GetSliceColor(_grid, int);
int _BegOfSlice(_grid, int);
int _EndOfSlice(_grid, int);
int _NextProcSameSlice(_grid, int);
int _PosToProcInSlice(_grid, const int,const int[],const int);
int _MaxProcInSlice(_grid, const int);
int _MyLocInSlice(_grid, int);
void _InitNodeStateVector(void);
void _PrintGridConfiguration(_grid);
void _UpdateGrid(_grid G1, _grid_nc G2, int preserve);
void _CopyGrid(_grid_fnc G1, _grid G2, int process_arrays);

/* index.c */
void _InitIndexi(void);


/* index_list.c */
void _print_index_list(_index_list);
void _add_sparse_index(_index_list,int []);
void _sort_ind_list(_index_list);
_index_list _new_index_list(int,_region);
_index_list _new_index_list_withcap(int,_region,int);
void _old_index_list(_index_list);
void _ReallocSparseArray(_array_fnc);


/* init.c */

void _InitLibraries(const int,const int [],const int,const int,const int);
void _InitRuntime(int,char* []);
void _UnInitRuntime(void);


/* memory.c */

void _InitMem(void);
void _MemLogMark(char*);
void _UnInitMem(void);
void * restrict _zmalloc(unsigned long, char*);
void * restrict _zmalloc_reg(unsigned long, char*, _region);
void * restrict _zcalloc(unsigned long, unsigned long, char*);
void * restrict _zcalloc_reg(unsigned long, unsigned long, char*, _region);
void * restrict _zrealloc(void*, unsigned long, char*);
void * restrict _zrealloc_reg(void*, unsigned long, char*,_region);
void _zfree(void *, char*);
void _zfree_reg(void*, char*, _region);

#ifndef _MD_SYM_ALLOC
#define _ZPL_SYM_ALLOC _zmalloc
#define _ZPL_SYM_FREE _zfree
#else
void *_ZPL_SYM_ALLOC(unsigned long,char*);
void _ZPL_SYM_FREE(void *,char*);
#endif


/* promote.c */

_array_nc _PromoteScalarToEnsemble(void*,_array_fnc,unsigned long,_region_fnc);


/* reflect.c */

void _ReflectI(_region,_array *,unsigned int *,int);


/* region.c */

void _InitRegion(_region_fnc reg, int process_arrays);
void _InitRegionCore(_grid grid, _distribution dist, _region_fnc reg, int process_arrays, int deep);
void _print_region_hierarchy(_region);
void _SetMyBlock(int, _region_fnc, int, int, _grid);
int _REG_TOTELMS(_region);
void _UpdateRegion(_region R1, _region_nc R2, int preserve);
void _CopyRegion(_region_fnc R1, _region R2, int process_arrays);

/* scanred.c */

int _ClassifyUnknownRed(_region);
void _DfltRedBcastUnknown(_region,void*,int,int,int,int);
void _CalcRedReg(_region_fnc,_region,_region);


/* scanred_tmpl.c */

#ifndef _MD_STUBS_REDS
void _Reduce_int(int*,int,_region,_region,int);
void _Reduce__uint(_uint*,int,_region,_region,int);
void _Reduce_long(long*,int,_region,_region,int);
void _Reduce__ulong(_ulong*,int,_region,_region,int);
void _Reduce_float(float*,int,_region,_region,int);
void _Reduce_double(double*,int,_region,_region,int);
void _Reduce__zquad(_zquad*,int,_region,_region,int);
void _Reduce_fcomplex(fcomplex*,int,_region,_region,int);
void _Reduce_dcomplex(dcomplex*,int,_region,_region,int);
void _Reduce_qcomplex(qcomplex*,int,_region,_region,int);
void _Reduce_user(void*,int,_region,_region,void (*)(void*,void*),size_t);
#endif


/* sparse.c */

void _SpsCopyShiftStructure(_region_fnc reg,_region basereg,_direction dir);
void _PrintSparse2DRegion(_region reg,int);
void _PrintSparseNodes(_region);


/* sparse_dir.c */

void _SparseBuildDirectory(_region_fnc,_index_list);


/* temp_ens.c */

void _ConstructTempEnsemble(_region_fnc,_array_fnc,unsigned int);
void _DestroyTempEnsemble(_array);


/* temp_param.c */

void _CreateOffsetEnsemble(_array_fnc,_array,int,_direction,int);


/* timer.old.c */

void _InitOldTimer(void);
double ResetTimer(void);
double CheckTimer(void);
double ResetCheckpointTimer(void);
double CheckCheckpointTimer(void);


/* token.c */

void _SendToken(int);
void _RecvToken(void);
void _PassToken(void);
void _PassTokenSameSlice(_grid, int);


/* wrap.c */

void _InitWrap(const int,const int []);
void _WrapI(_region,int,_array *,unsigned int *,int);
void _WrapII(_region,int);


/* zmath.c */

double _ZPL_SQR(double);
double _ZPL_CUBE(double);
int _bpop(_zulonglong);

#endif
