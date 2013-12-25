#
# src/mpi/x1/Makefile
#

SHELL     = /bin/sh

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(MD_LIB_INCLS)

TARG_START_OBJ = \
	mpi_start.o


.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

all: noinstall
	@echo "Finished making x1-specific files"

remote: noinstall clean

noinstall: $(TARG_START_OBJ)

clean:
	@echo "Cleaning..."
	@rm -f *.o

mpi_start.o: mpi_start.x1.c
	$(START_CC) -D_PRINT_OK_ -c -o mpi_start.o $(START_CFLAGS) $(INCL) mpi_start.x1.c
