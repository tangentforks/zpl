/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __STENCIL_H_
#define __STENCIL_H_

/*
 * Stencil Debugging
 */

#define STENCIL_DEFDBG FALSE
#define STENCIL_DEBUG(s) if (stencil_debug) s

/*
 * Stencil Benefit Function
 *
 *  benefit = ADDS * (number of adds eliminated)              +
 *            MEMS * (number of memory references eliminated) +
 *            MULS * (number of multiplies eliminated)        +
 *            REGS * (number of registers used)               -
 *            BBAR
 */

#define STENCIL_DEFADDS    1.5
#define STENCIL_DEFMEMS    1.5
#define STENCIL_DEFMULS    2.0
#define STENCIL_DEFREGS   -1.0
#define STENCIL_DEFBBAR    1.5

#endif
