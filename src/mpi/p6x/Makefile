#
# src/mpi/p690/Makefile
#

SHELL     = /bin/sh

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(MD_LIB_INCLS)

TARG_START_OBJ = \
	mpi_start.o \
	mpi_start_dbg.o


.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

all: noinstall
	@echo "Finished making p690-specific files"

remote: noinstall clean

noinstall: $(TARG_START_OBJ)

clean:
	@echo "Cleaning..."
	@rm -f *.o

mpi_start.o: mpi_start.p690.c
	$(START_CC) -c -o mpi_start.o $(START_CFLAGS) $(INCL) mpi_start.p690.c

mpi_start_dbg.o: mpi_start.o
	@cp mpi_start.o mpi_start_dbg.o
