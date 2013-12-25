#
# src/pvm/t3e/Makefile
#

SHELL     = /bin/sh

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(MD_LIB_INCLS)

TARG_START_OBJ = \
	pvm_start.o \
	pvm_start_dbg.o


.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

all: noinstall
	@echo "Finished making t3e-specific files"

remote: noinstall clean

noinstall: $(TARG_START_OBJ)

clean:
	@echo "Cleaning..."
	@rm -f *.o

pvm_start.o: pvm_start.t3e.c
	$(START_CC) -c -o pvm_start.o $(START_CFLAGS) $(INCL) pvm_start.t3e.c

pvm_start_dbg.o: pvm_start.o
	@cp pvm_start.o pvm_start_dbg.o
