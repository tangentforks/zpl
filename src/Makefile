#
# src/Makefile
#

#CFLAGS    = -g -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -pedantic-errors -Werror
#START_CFLAGS    = -g -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -pedantic-errors -Werror

SHELL     = /bin/sh

ZLIBDIR   = $(ZPLHOME)/lib/$(ZPLETCDIR)/$(ZPLTARGET)

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL)

ZPL_LIB   = libzpl.a
START_LIB = libstart.a

SRCS = \
	at_comm.c \
	broadcast.c \
	checkpoint.c \
	config.c \
	distribution.c \
	dyn_reg.c \
	ensemble.c \
	file_io.c \
	file_io_elem.c \
	file_io_ens.c \
	flood.c \
	genaccess.c \
	grid.c \
	index.c \
	index_list.c \
	init.c \
	ironman_utils.c \
	istreams.c \
	memory.c \
	permute.c \
	promote.c \
	reflect.c \
	region.c \
	registry.c \
	scanred.c \
	sparse.c \
	sparse_dir.c \
	start.c \
	temp_ens.c \
	temp_param.c \
	timer.old.c \
	timer.c \
	token.c \
	wrap.c \
	zmath.c

ZPL_OBJS  = \
	$(SRCS:.c=.o) \
	scanred_quad.o \
	scanred_dbl.o \
	scanred_flt.o \
	scanred_int.o \
	scanred_long.o \
	scanred_uint.o \
	scanred_ulong.o \
	scanred_fcmplx.o \
	scanred_dcmplx.o \
	scanred_qcmplx.o

START_OBJS = \
	start_start.o

all: noinstall install

depend: $(SRCS)
	$(MAKEDEPEND_PRE) $(INCL) $(SRCS) $(MAKEDEPEND_POST)

remote: noinstall install clean

noinstall: $(ZPL_LIB) $(START_LIB)

$(ZPL_LIB): $(ZPL_OBJS)
	@echo "*** Building machine independent library $(ZPL_LIB) ***"
	$(ARC) $(ZPL_LIB) $(ZPL_OBJS)

$(START_LIB): $(START_OBJS)
	@echo "*** Building machine independent library $(START_LIB) ***"
	$(ARC) $(START_LIB) $(START_OBJS)

backup:
	cp *.c bkup
	cp Makefile bkup

clean:
	@echo "Cleaning..."
	@rm -f *.o
	@rm -f *.a

clobber:
	@echo "Clobbering..."
	@rm -f *.o
	@rm -f *.a
	@rm -f core
	@rm -f *~
	@rm -f #*
	@rm -f Makefile.depend

install:
	cp $(ZPL_LIB) $(ZLIBDIR)/$(ZPL_LIB)
	cp $(START_LIB) $(ZLIBDIR)/$(START_LIB)

.c.o:
	$(CC) -c $(CFLAGS) $(INCL) $<

start_start.o: start.c
	$(START_CC) -c $(START_CFLAGS) -D_PRINT_OK_ $(INCL) -o start_start.o start.c


#
# the dependences down here are somewhat fake... the question is how
# to automatically generate the dependences for an object file whose
# name differs from the source file.  makedepend and gcc -M don't
# provide an easy solution to this, so instead, we notice that the
# headers for scanred_tmpl.c are the same as scanred.c, so make the
# assumption that if scanred.o has been updated, scanred_tmpl.c needs
# to be recompield as well.
#

scanred_quad.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_quad.o -DTYPE=_zquad scanred_tmpl.c

scanred_dbl.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_dbl.o -DTYPE=double scanred_tmpl.c

scanred_flt.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_flt.o -DTYPE=float scanred_tmpl.c

scanred_int.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_int.o -DTYPE=int scanred_tmpl.c

scanred_long.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_long.o -DTYPE=long scanred_tmpl.c

scanred_uint.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_uint.o -DTYPE=_uint scanred_tmpl.c

scanred_ulong.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_ulong.o -DTYPE=_ulong scanred_tmpl.c

scanred_fcmplx.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_fcmplx.o -DTYPE=fcomplex scanred_tmpl.c

scanred_dcmplx.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_dcmplx.o -DTYPE=dcomplex scanred_tmpl.c

scanred_qcmplx.o: scanred_tmpl.c scanred.o
	$(CC) -c $(CFLAGS) $(INCL) -o scanred_qcmplx.o -DTYPE=qcomplex scanred_tmpl.c

include Makefile.depend
