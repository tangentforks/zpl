/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "../include/Bgen.h"
#include "../include/Mgen.h"
#include "../include/global.h"

#define ADD 1
#define SUB 2


typedef enum {
  DISTANCE_ADVNC=0,
  DISTANCE_RESET,
  DISTANCE_TILED,
  DISTANCE_SKINY,
  DISTANCE_EOTIL,
  NUM_DISTANCES
} distancetype;


static char* distance_name[NUM_DISTANCES] = {"_ADVNC",
				     "_RESET",
				     "_TILED",
				     "_SKINY",
				     "_EOTIL"};


static void gen_bump_expr(FILE* outfile,expr_t* expr) {
  brad_no_access++;
  gen_expr(outfile, expr);
  brad_no_access--;
}


static void gen_distance(FILE* outfile,distancetype distance,expr_t* expr,int up,int dim,
		     int* first,int op,int val,expr_t *val_expr,int flat) {
  if (*first) {
    *first = 0;
    if (op == ADD) {
      fprintf(outfile,"   ");
    }
  } else {
    if (op == ADD) {
      fprintf(outfile," + ");
    }
  }
  if (op == SUB) {
    fprintf(outfile," - ");
  }
  fprintf(outfile,"_DISTANCE");
  if (current_mloop_type & MLOOP_STRIDED_BIT) {
    fprintf(outfile,"_STR");
  }
  fprintf(outfile,"%s",distance_name[distance]);
  if (up == -1) {
    fprintf(outfile,"_DN");
  } else {
    fprintf(outfile,"_UP");
  }
  if (flat) {
    fprintf(outfile,"_FLAT");
  }
  fprintf(outfile,"(");
  gen_bump_expr(outfile,expr);
  fprintf(outfile,",%d",dim);
  if (distance == DISTANCE_TILED) {
    fprintf(outfile,",");
    if (val_expr) {
      gen_expr(outfile, val_expr);
    } else {
      fprintf(outfile,"%d",val);
    }
  }
  fprintf(outfile,")");
  if (distance != DISTANCE_TILED) {
    fprintf(outfile,"  ");
  }
}


static void gen_advance(FILE* outfile,expr_t* expr,int up,int dim,int* first,
			int flat) {
  gen_distance(outfile,DISTANCE_ADVNC,expr,up,dim,first,ADD,0,NULL,flat);
}


static void gen_reset(FILE* outfile,expr_t* expr,int up,int dim,int* first,
		      int flat) {
  gen_distance(outfile,DISTANCE_RESET,expr,up,dim,first,ADD,0,NULL,flat);
}


static void gen_tile_advance(FILE* outfile,expr_t* expr,int up,int dim,
			     int* first,int tile,expr_t *tile_expr,int flat) {
  if (tile == 0) {
    gen_advance(outfile,expr,up,dim,first,flat);
  } else if (tile == 1) {
    gen_advance(outfile,expr,up,dim,first,flat);
  } else {
    gen_distance(outfile,DISTANCE_TILED,expr,up,dim,first,ADD,tile,tile_expr,flat);
  }
}

static void gen_tile_reset(FILE* outfile,expr_t* expr,int up,int dim,
			   int* first,int tile,expr_t *tile_expr,int flat) {
  if (tile == 0) {
    gen_reset(outfile,expr,up,dim,first,flat);
  } else if (tile == 1) {
  } else {
    gen_distance(outfile,DISTANCE_TILED,expr,up,dim,first,SUB,tile,tile_expr,flat);
  }
}


static void gen_tile_skinny_reset(FILE* outfile,expr_t* expr,int up,int dim,
				  int* first,int tile,int flat) {
  if (tile == 0) {
    gen_reset(outfile,expr,up,dim,first,flat);
  } else if (tile == 1) {
  } else {
    gen_distance(outfile,DISTANCE_SKINY,expr,up,dim,first,ADD,tile,NULL,flat);
  }
}


static void gen_tile_boundary_reset(FILE* outfile,expr_t* expr,int up,int dim,
				    int* first,int tile,int flat) {
  if (tile == 0) {
  } else if (tile == 1) {
  } else {
    gen_distance(outfile,DISTANCE_EOTIL,expr,up,dim,first,ADD,0,NULL,flat);
  }
}


void gen_access_distance(FILE *outfile,expr_t* expr,int numdims,int bumpnum,
			 int order[],int up[],int flat[]) {
  int first=1;
  int bumpdim;
  int prevnum;
  int prevdim;

  bumpdim = order[bumpnum];
  gen_advance(outfile,expr,up[bumpdim],bumpdim,&first,flat[bumpdim]);

  prevnum = bumpnum+1;
  if (prevnum != numdims) {
    prevdim = order[prevnum];
    gen_reset(outfile,expr,up[prevdim],prevdim,&first,flat[bumpdim]);
  }
}


void gen_bump_distance(FILE* outfile,expr_t* expr,int numdims,int bumpnum,
		       int order[],int up[],int tile[],expr_t *tile_expr[],
		       int thin,int flat[]) {
  int bumpdim;
  int first = 1;
  int prevnum;
  int prevdim;

  bumpdim = order[bumpnum];

  gen_advance(outfile,expr,up[bumpdim],bumpdim,&first,flat[bumpdim]);
    
  prevnum = bumpnum+1;
  while (prevnum != numdims) {
    prevdim = order[prevnum];
    if (tile[prevdim] != 1) {
      break;
    }
    prevnum++;
  }
  if (prevnum != numdims) {
    if (thin == 0) {
      gen_tile_reset(outfile,expr,up[prevdim],prevdim,&first,tile[prevdim],
		     tile_expr[prevdim],flat[prevdim]);
    } else {
      gen_tile_skinny_reset(outfile,expr,up[prevdim],prevdim,&first,
			    tile[prevdim],flat[prevdim]);
    }
  }
  if (first) {
    fprintf(outfile,"   0");
  }
}


static void determine_tile_position(int atbegoftile[],int numdims,int order[],
				    int tileorder[],int tile[],int bumpnum) {
  int firsttile;
  int i;
  int j;
  int dim;
  int dim2;

  /* in this routine a nonzero value of atbegoftile[] indicates that
     the walker is at the beginning of a tile.  A zero value indicates
     that it's at the end of a tile (one position past the last
     element in the tile).

     In cases where it might matter, a value of 2 indicates that it's
     guaranteed to be at the beginning of a row, whereas a value of 1
     indicates that it might be internal to the array (typically on
     the boundary between full and skinny tiles.

     If the tile value is 1, the only thing that matters is zero or
     nonzero because there is no boundary between full and skinny.
  */

  firsttile = 1;
  for (i=0; i<numdims; i++) {
    dim = order[i];
    if (tile[dim] == 1) {
      atbegoftile[dim] = 2;
    } else {
      if (firsttile) {
	firsttile = 0;
	atbegoftile[dim] = 0;
      } else {
	atbegoftile[dim] = 1;
      }
    }
  }
  /* At this point, atbegoftile[] indicates where we'll be after we've
     iterated over the body of one tile. */

  /* Now, loop over all the tiles we've already bumped and set the position
     correctly as a result */
  firsttile = 1;
  for (i=bumpnum+1; i<numdims; i++) {
    dim = tileorder[i];
    if (tile[dim] == 1) {          /* this is a bumper we've bumped */
      for (j=0; j<numdims; j++) {  /* consider its effects on each dim */
	dim2 = tileorder[j];

	if (tile[dim2] == 1) {
	  if (i == j) { /* for tiled-by-1 dims, only the outermost will be
			   at the end */
	    if (firsttile) {
	      firsttile = 0;
	      atbegoftile[dim2] = 0;
	    }
	  }
	} else {
	  if (j > i) {
	    atbegoftile[dim2] = 2;
	  } else {
	    atbegoftile[dim2] = 1;
	  }
	}
      }
    }
  }
  /* At this point, atbegoftile[] indicates where we'll be after
     we've iterated over the tile and bumped all the tile bumpers
     that are inner as compared to us */

  /*
  if (numdims == 3) {
  printf("We're at [%c,%c,%c] for tile %d\n",
	 atbegoftile[0] ? ((atbegoftile[0] == 1) ? '|' : 'B') : 'E',
	 atbegoftile[1] ? ((atbegoftile[1] == 1) ? '|' : 'B') : 'E',
	 atbegoftile[2] ? ((atbegoftile[2] == 1) ? '|' : 'B') : 'E',
	 tileorder[bumpnum]);
  } else {
  printf("We're at [%c,%c] for tile %d\n",
	 atbegoftile[0] ? ((atbegoftile[0] == 1) ? '|' : 'B') : 'E',
	 atbegoftile[1] ? ((atbegoftile[1] == 1) ? '|' : 'B') : 'E',
	 tileorder[bumpnum]);
  }
  */
}


void gen_bump_tile_distance(FILE* outfile,expr_t* expr,int numdims,int bumpnum,
			    int order[],int up[],int tile[],
			    expr_t *tile_expr[],int tileorder[],
			    int tileup[],int thin,int flat[]) {
  int bumpdim;
  int i;
  int first = 1;
  int dim;
  int atbegoftile[MAXRANK];

  determine_tile_position(atbegoftile,numdims,order,tileorder,tile,bumpnum);

  bumpdim = tileorder[bumpnum];
  
  for (i = 0; i<bumpnum; i++) {
    dim = tileorder[i];
    if (atbegoftile[dim] == 0) {
      if (thin == 0) {
	gen_tile_reset(outfile,expr,up[dim],dim,&first,tile[dim],
		       tile_expr[dim],flat[dim]);
      } else {
	gen_tile_skinny_reset(outfile,expr,up[dim],dim,&first,tile[dim],
			      flat[dim]);
      }
    }
  }
  if (atbegoftile[bumpdim]) {
    gen_tile_advance(outfile,expr,up[bumpdim],bumpdim,&first,tile[bumpdim],
		     tile_expr[bumpdim],flat[bumpdim]);
  }
  for (i=bumpnum+1; i<numdims; i++) {
    dim = tileorder[i];
    if (atbegoftile[dim]) {
      if (atbegoftile[dim] == 1) {
	gen_tile_boundary_reset(outfile,expr,up[dim],dim,&first,tile[dim],
				flat[dim]);
      }
    } else {
      gen_reset(outfile,expr,up[dim],dim,&first,flat[dim]);
    }
  }
  if (first) {
    fprintf(outfile,"   0");
  }
}

