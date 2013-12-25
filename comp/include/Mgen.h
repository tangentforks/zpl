/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __MGEN_H_
#define __MGEN_H_

#include <stdio.h>
#include "Dgen.h"
#include "Cgen.h"
#include "glistmac.h"
#include "parsetree.h"
#include "symtab.h"

#ifndef MGEN_DECL
#define MGEN_DECL extern
#endif

void InitMgen(void);

int mgen_unroll_number(int);

void gen_mloop_rmo_start(FILE *,expr_t *,expr_t *,int,int,elist,int);
void gen_mloop_rmo_stop(FILE *,expr_t *,expr_t *,int,int,elist,int);

int gen_sub_mloop_start(FILE *,expr_t *,int,int *);
void gen_sub_mloop_stop(FILE *,int);

void gen_mloop(FILE*,mloop_t*);
void gen_fluffy_mloop(FILE*,mloop_t*,char*);

#define MLOOP_NONE        0x0
#define MLOOP_ON_BIT      0x1
#define MLOOP_STRIDED_BIT 0x2
#define MLOOP_SPARSE_BIT  0x4

typedef int mloop_type;

MGEN_DECL mloop_type current_mloop_type;
MGEN_DECL mloop_t* current_mloop;
MGEN_DECL int current_dimnum;
MGEN_DECL int in_mloop;

extern const int mloop_dim[];

#define MLOOP_DIM1 0x01
#define MLOOP_DIM2 0x02
#define MLOOP_DIM3 0x04
#define MLOOP_DIM4 0x08
#define MLOOP_DIM5 0x10
#define MLOOP_DIM6 0x20  /* MAXRANK dependent */
#define MLOOP_DIMX 0x40

#define START_6D_MLOOP (MLOOP_DIMX)
#define START_5D_MLOOP (START_6D_MLOOP|MLOOP_DIM6)
#define START_4D_MLOOP (START_5D_MLOOP|MLOOP_DIM5)
#define START_3D_MLOOP (START_4D_MLOOP|MLOOP_DIM4)
#define START_2D_MLOOP (START_3D_MLOOP|MLOOP_DIM3)
#define START_1D_MLOOP (START_2D_MLOOP|MLOOP_DIM2)
#define IN_MLOOP_BODY (START_1D_MLOOP|MLOOP_DIM1)

#endif
