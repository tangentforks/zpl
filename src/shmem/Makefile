#
# src/shmem/Makefile
#

ZLIBDIR   = $(ZPLHOME)/lib/$(ZPLCOMMLAYER)/$(ZPLTARGET)

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(MD_LIB_INCLS)

TARG_OBJ     = force
MD_ZPL_LIB   = libzpl_$(ZPLCOMMLAYER).a
MD_START_LIB = libstart_$(ZPLCOMMLAYER).a

MD_ZPL_OBJS = \
	shmem_barrier.o \
	shmem_bcast.o \
	shmem_halt.o \
	shmem_ironman.o \
	shmem_mem.o \
	shmem_permute.o \
	shmem_scanred.o \
	shmem_sr_dbl.o \
	shmem_sr_flt.o \
	shmem_sr_int.o \
	shmem_sr_long.o \
	shmem_sr_uint.o \
	shmem_sr_ulong.o \
	shmem_sr_dcmplx.o \
	shmem_synch.o \
	shmem_token.o \
	shmem_zlib.o

MD_START_OBJS = \
	shmem_start.o

all: noinstall install

remote: noinstall install clean

noinstall: $(TARG_OBJ) $(MD_ZPL_LIB) $(MD_START_LIB)

$(TARG_OBJ):
	@if [ -d $(ZPLTARGET) ] ; then \
		cd $(ZPLTARGET) ; \
		./zmake ; \
		cd .. ; \
		cp $(ZPLTARGET)/*.o . ; \
	fi

$(MD_ZPL_LIB): $(MD_ZPL_OBJS)
	@echo "*** Building machine dependent library $(MD_ZPL_LIB) ***"
	$(ARC) $(MD_ZPL_LIB) $(MD_ZPL_OBJS)

$(MD_START_LIB): $(MD_START_OBJS)
	@echo "*** Building machine dependent library $(MD_START_LIB) ***"
	$(ARC) $(MD_START_LIB) $(MD_START_OBJS)

backup:
	cp *.c bkup
	cp Makefile bkup

clean:
	@echo "Cleaning..."
	@rm -f *.o
	@rm -f *.a

clobber:
	@echo "Clobbering..."
	@rm -f *~
	@rm -f #*
	@rm -f *.a
	@rm -f *.o
	@rm -f core

install:
	cp $(MD_ZPL_LIB) $(ZLIBDIR)/$(MD_ZPL_LIB)
	cp $(MD_START_LIB) $(ZLIBDIR)/$(MD_START_LIB)
	cp $(ZPLCOMMLAYER)_start.o $(ZLIBDIR)/$(ZPLCOMMLAYER)_start.o


.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

shmem_sr_dbl.o: shmem_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o shmem_sr_dbl.o -DTYPE=double -DFLOAT_TYPE shmem_scanred_tmpl.c

shmem_sr_flt.o: shmem_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o shmem_sr_flt.o -DTYPE=float -DFLOAT_TYPE shmem_scanred_tmpl.c

shmem_sr_int.o: shmem_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o shmem_sr_int.o -DTYPE=int shmem_scanred_tmpl.c

shmem_sr_long.o: shmem_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o shmem_sr_long.o -DTYPE=long shmem_scanred_tmpl.c

shmem_sr_uint.o: shmem_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o shmem_sr_uint.o -DTYPE=_uint shmem_scanred_tmpl.c

shmem_sr_ulong.o: shmem_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o shmem_sr_ulong.o -DTYPE=_ulong shmem_scanred_tmpl.c

shmem_sr_dcmplx.o: shmem_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o shmem_sr_dcmplx.o -DTYPE=dcomplex -DCOMPLEX_TYPE shmem_scanred_tmpl.c


shmem_start.o: shmem_start.c
	$(START_CC) -c $(START_CFLAGS) $(INCL) $<
