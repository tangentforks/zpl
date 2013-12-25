#
# src/alpha/Makefile
#

#CFLAGS    = -g -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -pedantic-errors -Werror
#START_CFLAGS    = -g -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -pedantic-errors -Werror

SHELL     = /bin/sh

ZLIBDIR   = $(ZPLHOME)/lib/$(ZPLCOMMLAYER)/$(ZPLTARGET)

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(MD_LIB_INCLS)

MD_ZPL_LIB = libzpl_$(ZPLCOMMLAYER).a

SRCS = \
	seq_bcast.c \
	seq_halt.c \
	seq_permute.c \
	seq_scanred.c \
	seq_zlib.c \
	seq_zprof.c

OBJS = \
	$(SRCS:.c=.o) \
	seq_sr_dbl.o \
	seq_sr_flt.o \
	seq_sr_int.o \
	seq_sr_long.o \
	seq_sr_uint.o \
	seq_sr_ulong.o \
	seq_sr_quad.o \
	seq_sr_fcmplx.o \
	seq_sr_dcmplx.o \
	seq_sr_qcmplx.o

.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

all: noinstall install

depend: $(SRCS)
	$(MAKEDEPEND_PRE) $(INCL) $(SRCS) $(MAKEDEPEND_POST)

remote: noinstall install clean

noinstall: $(MD_ZPL_LIB)

$(MD_ZPL_LIB): $(OBJS)
	@echo "*** Building machine dependent library $(MD_ZPL_LIB) ***"
	$(ARC) $(MD_ZPL_LIB) $(OBJS)

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
	@rm -f Makefile.depend

install:
	cp $(MD_ZPL_LIB) $(ZLIBDIR)/$(MD_ZPL_LIB)

# See note about hacked dependences in ../Makefile

seq_sr_dbl.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_dbl.o -DTYPE=double -DFLOAT_TYPE seq_scanred_tmpl.c

seq_sr_flt.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_flt.o -DTYPE=float -DFLOAT_TYPE seq_scanred_tmpl.c

seq_sr_int.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_int.o -DTYPE=int seq_scanred_tmpl.c

seq_sr_long.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_long.o -DTYPE=long seq_scanred_tmpl.c

seq_sr_uint.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_uint.o -DTYPE=_uint seq_scanred_tmpl.c

seq_sr_ulong.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_ulong.o -DTYPE=_ulong seq_scanred_tmpl.c

seq_sr_quad.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_quad.o -DTYPE=_zquad seq_scanred_tmpl.c

seq_sr_fcmplx.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_fcmplx.o -DTYPE=fcomplex -DCOMPLEX_TYPE seq_scanred_tmpl.c

seq_sr_dcmplx.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_dcmplx.o -DTYPE=dcomplex -DCOMPLEX_TYPE seq_scanred_tmpl.c

seq_sr_qcmplx.o: seq_scanred_tmpl.c seq_scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o seq_sr_qcmplx.o -DTYPE=qcomplex -DCOMPLEX_TYPE seq_scanred_tmpl.c

include Makefile.depend
