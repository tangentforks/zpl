#
# src/shmem/x1/Makefile
#

SHELL	= /bin/sh

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(MD_LIB_INCLS)

TARG_START_OBJ = \
	shmem_start.o

.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

all: noinstall
	@echo "Finished making x1-specific files"

remote: noinstall clean

noinstall: $(TARG_START_OBJ)

clean:
	@echo "Cleaning..."
	@rm -f *.o

shmem_start.o: shmem_start.x1.c
	$(START_CC) -c -o shmem_start.o $(START_CFLAGS) $(INCL) shmem_start.x1.c
