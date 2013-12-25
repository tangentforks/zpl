#
# src/pvm/Makefile
#

#CFLAGS    = -g -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Werror -pedantic-errors
#START_CFLAGS    = -g -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Werror -pedantic-errors

SHELL     = /bin/sh

ZLIBDIR   = $(ZPLHOME)/lib/$(ZPLCOMMLAYER)/$(ZPLTARGET)

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(MD_LIB_INCLS)

TARG_OBJ         = goober
MD_ZPL_LIB       = libzpl_$(ZPLCOMMLAYER).a
MD_START_LIB     = libstart_$(ZPLCOMMLAYER).a
MD_START_LIB_DBG = libstart_$(ZPLCOMMLAYER)_dbg.a

SRCS = \
	pvm_bcast.c \
	pvm_commid.c \
	pvm_halt.c \
	pvm_ironman.c \
	pvm_permute.c \
	pvm_print.c \
	pvm_scanred.c \
	pvm_token.c \
	pvm_zlib.c

OBJS = \
	$(SRCS:.c=.o) \
	pvm_scanred.o \
	pvm_sr_dbl.o \
	pvm_sr_flt.o \
	pvm_sr_int.o \
	pvm_sr_long.o \
	pvm_sr_uint.o \
	pvm_sr_ulong.o \
	pvm_sr_fcmplx.o \
	pvm_sr_dcmplx.o \

MD_START_OBJS = pvm_start.o

MD_START_OBJS_DBG = pvm_start_dbg.o

.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

all: noinstall install

depend: $(SRCS)
	$(MAKEDEPEND_PRE) $(INCL) $(SRCS) $(MAKEDEPEND_POST)

remote: noinstall install clean

noinstall: $(TARG_OBJ) $(MD_ZPL_LIB) $(MD_START_LIB) $(MD_START_LIB_DBG)

$(TARG_OBJ):
	@if [ -d $(ZPLTARGET) ] ; then \
		cd $(ZPLTARGET) ; \
		./zmake ; \
		cd .. ; \
		cp $(ZPLTARGET)/*.o . ; \
	fi

$(MD_ZPL_LIB): $(OBJS)
	@echo "*** Building machine dependent library $(MD_ZPL_LIB) ***"
	$(ARC) $(MD_ZPL_LIB) $(OBJS)

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
	@rm -f t3d/*.o

clobber:
	@echo "Clobbering..."
	@rm -f *~
	@rm -f #*
	@rm -f *.a
	@rm -f *.o
	@rm -f core
	@rm -f Makefile.depend

install:
	cp $(MD_ZPL_LIB) $(ZLIBDIR)/$(MD_ZPL_LIB)
	cp $(MD_START_LIB) $(ZLIBDIR)/$(MD_START_LIB)
	cp $(MD_START_LIB_DBG) $(ZLIBDIR)/$(MD_START_LIB_DBG)
	cp pvm_start.o $(ZLIBDIR)/$(ZPLCOMMLAYER)_start.o
	cp pvm_start_dbg.o $(ZLIBDIR)/$(ZPLCOMMLAYER)_start_dbg.o

pvm_sr_dbl.o: pvm_scanred_tmpl.c pvm_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o pvm_sr_dbl.o -DTYPE=double -DFLOAT_TYPE pvm_scanred_tmpl.c

pvm_sr_flt.o: pvm_scanred_tmpl.c pvm_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o pvm_sr_flt.o -DTYPE=float -DFLOAT_TYPE pvm_scanred_tmpl.c

pvm_sr_int.o: pvm_scanred_tmpl.c pvm_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o pvm_sr_int.o -DTYPE=int pvm_scanred_tmpl.c

pvm_sr_long.o: pvm_scanred_tmpl.c pvm_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o pvm_sr_long.o -DTYPE=long pvm_scanred_tmpl.c

pvm_sr_uint.o: pvm_scanred_tmpl.c pvm_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o pvm_sr_uint.o -DTYPE=_uint pvm_scanred_tmpl.c

pvm_sr_ulong.o: pvm_scanred_tmpl.c pvm_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o pvm_sr_ulong.o -DTYPE=_ulong pvm_scanred_tmpl.c

pvm_sr_fcmplx.o: pvm_scanred_tmpl.c pvm_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o pvm_sr_fcmplx.o -DTYPE=fcomplex -DCOMPLEX_TYPE pvm_scanred_tmpl.c

pvm_sr_dcmplx.o: pvm_scanred_tmpl.c pvm_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o pvm_sr_dcmplx.o -DTYPE=dcomplex -DCOMPLEX_TYPE pvm_scanred_tmpl.c

pvm_start_dbg.o: pvm_start.c pvm_scanred.o
	$(START_CC) -c -DPVM_DEBUG $(START_CFLAGS) $(INCL) -o pvm_start_dbg.o pvm_start.c

pvm_start.o: pvm_start.c pvm_scanred.o
	$(START_CC) -c $(START_CFLAGS) $(INCL) $<

include Makefile.depend
