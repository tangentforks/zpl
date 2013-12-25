/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MD_ZLIB_H_
#define __MD_ZLIB_H_

#include "md_zinc.h"
#include "region.h"
#include "ensemble.h"
#include "ironman.h"
#include "quad.h"
#include "permute.h"

/* from _barrier.c -- moved into its own file to allow barriers to
   stand alone. */

#include "barrier.h"


/* from _bcast.c */

#ifndef _BroadcastSimple
void _BroadcastSimple(_grid, void *,unsigned int,int,int);
#endif
#ifndef _BroadcastSimpleToSlice
void _BroadcastSimpleToSlice(_grid, void *,unsigned int,int,int,int,int);
#endif
#ifndef _Broadcast
void _Broadcast(_grid, IM_comminfo*,IM_comminfo*,int,int);
#endif
#ifndef _BroadcastToSlice
void _BroadcastToSlice(_grid, IM_comminfo*,IM_comminfo*,int,int,int,int);
#endif


/* from _permute.c */

/*
 * _Init - sets up machine specific global entities if necessary
 * _MS   - map setup (compute region, permute rank) returns permute map
 * _IR   - indices ready (permute map)
 * _IN   - indices needed (permute map) returns processor
 * _DS   - data setup (permute map, permute data, data element size, is scatter)
 * _DR   - data ready (permute map, permute data, is scatter)
 * _DN   - data needed (permute map, permute data, is scatter) returns processor
 * _DD   - data destroy (permute map, permute data)
 * _MD   - map destroy (permute map)
 * _Done - destroys machine specific global entities if necessary and all permute maps saved
 */
void _PERM_Init(const int);
_permmap* _PERM_MS(_region, const int);
void _PERM_IR(_permmap* const);
void _PERM_IN(_permmap* const, const int, const int, _array, _array, _region, int, int, int, int, int, int);
void _PERM_DS(const _permmap* const, _permdata* const, const int, const int, _array, _array, _region, int, int, int, int, int, int);
void _PERM_DR(const _permmap* const, _permdata* const, const int, _array_fnc, _array_fnc);
int _PERM_DN(const _permmap* const, _permdata* const, const int);
void _PERM_DD(const _permmap* const, _permdata* const, const int);
void _PERM_MD(_permmap*);
void _PERM_Done(void);

/* from _scanred*.c */

#ifndef _InitScanRed
void _InitScanRed(void);
#endif

void _Scanint(int *,int *,_region,int,int,int,int,int);
void _Scan_uint(_uint *,_uint *,_region,int,int,int,int,_uint);
void _Scanlong(long *,long *,_region,int,int,int,int,long);
void _Scan_ulong(_ulong *,_ulong *,_region,int,int,int,int,_ulong);
void _Scanfloat(float *,float *,_region,int,int,int,int,float);
void _Scandouble(double *,double *,_region,int,int,int,int,double);
void _Scan_zquad(_zquad *,_zquad *,_region,int,int,int,int,_zquad);
void _Scanqcomplex(qcomplex *,qcomplex*,_region,int,int,int,int,qcomplex);
void _Scandcomplex(dcomplex *,dcomplex*,_region,int,int,int,int,dcomplex);
void _Scanfcomplex(fcomplex *,fcomplex*,_region,int,int,int,int,fcomplex);
 
#ifndef _MD_STUBS_REDS
void _Reduceint(_grid, int*,int,int,int,int,int);
void _Reduce_uint(_grid, _uint*,int,int,int,int,int);
void _Reducelong(_grid, long*,int,int,int,int,int);
void _Reduce_ulong(_grid, _ulong*,int,int,int,int,int);
void _Reducefloat(_grid, float*,int,int,int,int,int);
void _Reducedouble(_grid, double*,int,int,int,int,int);
void _Reduce_zquad(_grid, _zquad*,int,int,int,int,int);
void _Reducefcomplex(_grid, fcomplex*,int,int,int,int,int);
void _Reducedcomplex(_grid, dcomplex*,int,int,int,int,int);
void _Reduceqcomplex(_grid, qcomplex*,int,int,int,int,int);
void _Reduceuser(_grid, void*,int,int,int,int,void (*)(void*,void*),size_t);
#endif

void _MDInitGrid(_grid_fnc);
void _MDSetupGrid(_grid_fnc);

/* from _token.c */

#ifndef _ResetToken
void _ResetToken(void);
#endif
#ifndef _SendTokenWithValue
void _SendTokenWithValue(int,long *);
#endif
#ifndef _RecvTokenWithValue
void _RecvTokenWithValue(long *);
#endif


void _ReadPort(int,int,int,void *,int);
void _WritePort(int,int,int,void *,int);
void _ZPL_halt(int);

#endif
