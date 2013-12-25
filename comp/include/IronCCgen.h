/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __IRONCC_H_
#define __IRONCC_H_

#include "parsetree.h"

/* collective operators */
/* new ones */
#define _ALL_TO_ONE 1
#define _ONE_TO_ALL 3
#define _ONE_TO_MANY 5
#define _MANY_TO_MANY 7
#define _ALL_TO_ALL 9

/* names */
#define _REDUCE		1
#define _BROADCAST	2
#define _FLOOD		3
#define _SCAN		4
#define _CCNAME_UNKNOWN	100

/* subnodes */
#define _GLOBAL		1
#define _SETUP		2
#define _CLEANUP	3
#define _LOCAL		4
#define _LOCALOUT	5
#define _UNKNOWN_SUB	100

#define IRON_CC_NOT_CCOM 0
#define IRON_CC_REDUCE 1
#define IRON_CC_SCAN 2
#define IRON_CC_BCAST 3

/* binary operators */
/* new ones */
#define _BIN_COPY 1
#define _BIN_MIN  2
#define _BIN_MAX  3
#define _BIN_ADD  4
#define _BIN_MULT 5
#define _BIN_AND  6
#define _BIN_OR   7
#define _BIN_BAND 8
#define _BIN_BOR  9
#define _BIN_BXOR 10
#define _BIN_USER 11
#define _BIN_UNKNOWN 100

#define IRON_BIN_NOOP 0
#define IRON_BIN_COPY 1
#define IRON_BIN_MIN  2
#define IRON_BIN_MAX  3
#define IRON_BIN_PLUS 4

/* types */
/* new ones */
#define _CCUNKNOWN_TYPE 0
#define _CCCHAR   1
#define _CCINT    2
#define _CCDOUBLE 3
#define _CCGENERIC 100

#define IRON_NOTYPE 0
#define IRON_CHAR   1
#define IRON_INT    2
#define IRON_DOUBLE 3

#define MAXIDS 10
struct ccinfo {
   int			 CCOP; /* e.g. _ALL_TO_ONE communication */
   int			 CCNAME; /* e.g. _REDUCE */
   int			 SUBNODE; /* e.g. _CLEANUP, _GLOBAL, _SETUP */
   exprtype		 originalCCOP[MAXIDS]; /* e.g. REDUCEMIN */
   statement_t 		*originalStmt[MAXIDS];
   int			 numNodes;
   symboltable_t 	*SrcRegion,
   		 	*DstRegion;
   expr_t 		*lhs[MAXIDS],
	  		*rhs[MAXIDS];
   int    		 binaryop[MAXIDS],
	  		 ID[MAXIDS],
	  		 TYPES[MAXIDS];
   int			 myID;
   int 	  		 rank;
   statement_t 		*globalnodes[3];
   symboltable_t	*red_in_sym[MAXIDS],
			*red_out_sym[MAXIDS];
};

/* optimization levels */
/*
   These are optimization levels for collective communication
BB 			0		black box (original) code
BB_MLOOP 		1		split out mloops
ECC 			2		ecclippse code
ECC_COMBINE		3 		combining
ECC_OVERLAP 		4		overlapping (best)
ECC_OVERLAP1 		41		overlap incoming
ECC_OVERLAP2 		42		overlap outgoing
ECC_OVERLAP3 		43		overlap both
ECC_COMBINE_OVERLAP 	5		combine (best) + overlap (best)
ECC_COMBINE_OVERLAP1 	51				 overlap incoming
ECC_COMBINE_OVERLAP2 	52				 overlap outgoing
ECC_COMBINE_OVERLAP3 	53				 overlap both
ECC_OVERLAP_COMBINE	6		overlap (best) + combine (best)
ECC_OVERLAP1_COMBINE	61		overlap incoming
ECC_OVERLAP2_COMBINE	62		overlap outgoing
ECC_OVERLAP3_COMBINE	63		overlap both
ECC_OC			7		overlap/combine 
ECC_OC1			71		overlap (incoming)/combine
ECC_OC2			72		overlap (outgoing)/combine
ECC_OC3			73		overlap (both)/combine
*/
#define BB 			0
#define BB_MLOOP 		1
#define ECC 			2
#define ECC_COMBINE		3 
#define ECC_OVERLAP 		4
#define ECC_OVERLAP1 		41
#define ECC_OVERLAP2 		42
#define ECC_OVERLAP3 		43
#define ECC_COMBINE_OVERLAP 	5
#define ECC_COMBINE_OVERLAP1 	51
#define ECC_COMBINE_OVERLAP2 	52
#define ECC_COMBINE_OVERLAP3 	53
#define ECC_OVERLAP_COMBINE	6
#define ECC_OVERLAP1_COMBINE	61
#define ECC_OVERLAP2_COMBINE	62
#define ECC_OVERLAP3_COMBINE	63
#define ECC_OC			7
#define ECC_OC1			71
#define ECC_OC2			72
#define ECC_OC3			73
   

#endif
