#
# src/mpi/Makefile
#

#CFLAGS    = -g -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Werror -pedantic-errors
#START_CFLAGS = -g -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Werror -pedantic-errors


SHELL     = /bin/sh

ZLIBDIR   = $(ZPLHOME)/lib/$(ZPLETCDIR)/$(ZPLTARGET)

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(MD_LIB_INCLS)

MD_ZPL_LIB       = libzpl_$(ZPLCOMMLAYER).a
MD_START_LIB     = libstart_$(ZPLCOMMLAYER).a
MD_START_LIB_DBG = libstart_$(ZPLCOMMLAYER)_dbg.a

MD_ZPL_OBJS = \
	mpi_bcast.o \
	mpi_commid.o \
	mpi_halt.o \
	mpi_ironman.o \
	mpi_junk.o \
	mpi_permute.o \
	mpi_print.o \
	mpi_print_proc.o \
	mpi_scanred.o \
	mpi_sr_dbl.o \
	mpi_sr_flt.o \
	mpi_sr_int.o \
	mpi_sr_long.o \
	mpi_sr_uint.o \
	mpi_sr_ulong.o \
	mpi_sr_quad.o \
	mpi_sr_fcmplx.o \
	mpi_sr_dcmplx.o \
	mpi_sr_qcmplx.o \
	mpi_token.o \
	mpi_zlib.o

MD_START_OBJS = \
	mpi_start.o

MD_START_OBJS_DBG = \
	mpi_start_dbg.o

.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

all: noinstall install

remote: noinstall install clean

noinstall: $(MD_START_OBJS) $(MD_ZPL_LIB) $(MD_START_LIB) $(MD_START_LIB_DBG)

mpi_start.o: FORCE
	@if [ -r mpi_start.$(ZPLMPITYPE).c ] ; then \
		$(START_CC) -c $(START_CFLAGS) $(INCL) -o mpi_start.o mpi_start.$(ZPLMPITYPE).c; \
		$(START_CC) -c -D_MPI_DEBUG_ $(START_CFLAGS) $(INCL) -o mpi_start_dbg.o mpi_start.$(ZPLMPITYPE).c; \
	elif [ -d $(ZPLTARGET) ] ; then \
		cd $(ZPLTARGET) ; \
		./zmake ; \
		cd .. ; \
		cp $(ZPLTARGET)/*.o . ; \
	else \
		echo "ERROR: Unable to determine MPI type"; \
		echo "       Make sure environment variables are properly set."; \
		exit 1; \
	fi

$(MD_ZPL_LIB): $(MD_ZPL_OBJS)
	@echo "*** Building machine dependent library $(MD_ZPL_LIB) ***"
	$(ARC) $(MD_ZPL_LIB) $(MD_ZPL_OBJS)

$(MD_START_LIB): $(MD_START_OBJS)
	@echo "*** Building machine dependent library $(MD_START_LIB) ***"
	$(ARC) $(MD_START_LIB) $(MD_START_OBJS)

$(MD_START_LIB_DBG): $(MD_START_OBJS_DBG)
	@echo "*** Building machine dependent library $(MD_START_LIB_DBG) ***"
	$(ARC) $(MD_START_LIB_DBG) $(MD_START_OBJS_DBG)


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
	cp $(MD_START_LIB_DBG) $(ZLIBDIR)/$(MD_START_LIB_DBG)
	cp mpi_start.o $(ZLIBDIR)/$(ZPLCOMMLAYER)_start.o
	cp mpi_start_dbg.o $(ZLIBDIR)/$(ZPLCOMMLAYER)_start_dbg.o

mpi_sr_dbl.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_dbl.o -DTYPE=double -DFLOAT_TYPE mpi_scanred_tmpl.c

mpi_sr_flt.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_flt.o -DTYPE=float -DFLOAT_TYPE mpi_scanred_tmpl.c

mpi_sr_int.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_int.o -DTYPE=int mpi_scanred_tmpl.c

mpi_sr_long.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_long.o -DTYPE=long mpi_scanred_tmpl.c

mpi_sr_uint.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_uint.o -DTYPE=_uint mpi_scanred_tmpl.c

mpi_sr_ulong.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_ulong.o -DTYPE=_ulong mpi_scanred_tmpl.c

mpi_sr_quad.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_quad.o -DTYPE=_zquad -DFLOAT_TYPE mpi_scanred_tmpl.c

mpi_sr_fcmplx.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_fcmplx.o -DTYPE=fcomplex -DCOMPLEX_TYPE mpi_scanred_tmpl.c

mpi_sr_dcmplx.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_dcmplx.o -DTYPE=dcomplex -DCOMPLEX_TYPE mpi_scanred_tmpl.c

mpi_sr_qcmplx.o: mpi_scanred_tmpl.c
	$(CC) -c $(CFLAGS) $(INCL) -o mpi_sr_qcmplx.o -DTYPE=qcomplex -DCOMPLEX_TYPE mpi_scanred_tmpl.c

FORCE:
