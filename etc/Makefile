#
# etc/Makefile - used to compile zc generated C source (machine independent)
#

SHELL     = /bin/sh

ZLIBDIR   = $(ZPLHOME)/lib/$(ZPLETCDIR)/$(ZPLTARGET)

MI_ZINCL  = $(ZPLHOME)/include
MD_ZINCL1 = $(MI_ZINCL)/$(ZPLCOMMLAYER)
INCL      = $(ZM_INCL) -I"$(MD_ZINCL1)" -I"$(MI_ZINCL)" $(MD_INCLS) $(ZC_INCLS)
START_INCL= $(ZM_INCL) -I$(MD_ZINCL1) -I$(MI_ZINCL) $(START_MD_INCLS) $(ZC_INCLS)

LIBPATH       = -L$(ZLIBDIR) $(MD_LIBPATHS) $(ZC_LIBPATHS)
START_LIBPATH = -L$(ZLIBDIR) $(START_MD_LIBPATHS) $(ZC_LIBPATHS)

MI_LIB    = zpl
MD_LIB    = zpl_$(ZPLCOMMLAYER)
LIB       = -l$(MD_LIB) -l$(MI_LIB) -l$(MD_LIB) -l$(MI_LIB) -l$(MD_LIB) $(MD_LIBS) $(ZC_LIBS) -lm

MI_STARTLIB = start
MD_STARTLIB = start_$(ZPLCOMMLAYER)
STARTLIB  = -l$(MI_STARTLIB) -l$(MD_STARTLIB) $(START_MD_LIBS) -lm

PROG_EXE = $(PROG)$(EXE_SUFFIX)
PROG_REAL = $(PROG)_real$(EXE_SUFFIX)
PROG_DBG = $(PROG)_dbg$(EXE_SUFFIX)

HEADERS = \

SRCS = \
	$(PROG).c \
	$(PROG)_cfg.c \
	$(PROG)_pcfg.c \
	$(PROG)_init.c \
	$(PROG)_sps.c \
	$(PROG)_acc.c

OBJS = $(PROG).o $(ZC_ADDITIONAL_OBJS)

START_OBJS = $(PROG)_start_cfg.o

all: $(PROG_EXE) $(PROG_REAL) $(PROG_DBG)

$(PROG).o: $(SRCS)
	@if [ "$(ZMAKEVERBOSE)" = "yes" ]; then \
		echo "$(CC) -c $(CFLAGS) $(INCL) $<"; \
	fi;
	@$(CC) -c $(CFLAGS) $(INCL) $<	

$(PROG)_start_cfg.o: $(PROG)_pcfg.c
	@if [ "$(ZMAKEVERBOSE)" = "yes" ]; then \
		echo "$(START_CC) -c $(START_CFLAGS) $(INCL) -o $(PROG)_start_cfg.o $(PROG)_pcfg.c"; \
	fi;
	@$(START_CC) -c $(START_CFLAGS) $(INCL) -o $(PROG)_start_cfg.o $(PROG)_pcfg.c

.c.o:
	@echo "Compiling $<...";
	@if [ "$(ZMAKEVERBOSE)" = "yes" ]; then \
		echo "$(CC) -c $(CFLAGS) $(INCL) $<"; \
	fi;
	@$(CC) -c $(CFLAGS) $(INCL) $<

$(PROG_EXE): $(OBJS) $(START_OBJS) $(ZLIBDIR)/lib$(MD_LIB).a $(ZLIBDIR)/lib$(MI_LIB).a
	@echo "Linking $(PROG_EXE)...";
	@if [ "$(FILE1)" = "yes" ]; then \
		if [ "$(ZMAKEVERBOSE)" = "yes" ]; then \
			echo "$(START_LINK) $(START_LFLAGS) -o $(PROG_EXE) $(START_LIBPATH) $(ZLIBDIR)/$(ZPLCOMMLAYER)_start.o $(START_OBJS) $(STARTLIB)"; \
		fi; \
		$(START_LINK) $(START_LFLAGS) -o $(PROG_EXE) $(START_LIBPATH) $(ZLIBDIR)/$(ZPLCOMMLAYER)_start.o $(START_OBJS) $(STARTLIB); \
	else \
		if [ "$(ZMAKEVERBOSE)" = "yes" ]; then \
			echo "$(LINK) $(LFLAGS) -o $(PROG_EXE) $(LIBPATH) $(OBJS) $(LIB)"; \
		fi; \
		$(START_LINK) $(LFLAGS) -o $(PROG_EXE) $(LIBPATH) $(OBJS) $(LIB); \
	fi;

$(PROG_REAL): $(OBJS) $(ZLIBDIR)/lib$(MD_LIB).a $(ZLIBDIR)/lib$(MI_LIB).a
	@if [ "$(FILE2)" = "yes" ] && [ "$(FILE1)" = "yes" ] ; then \
		echo "Linking $(PROG_REAL)..."; \
		if [ "$(ZMAKEVERBOSE)" = "yes" ]; then \
			echo "$(LINK) $(LFLAGS) -o $(PROG_REAL) $(LIBPATH) $(OBJS) $(LIB)"; \
		fi; \
		$(LINK) $(LFLAGS) -o $(PROG_REAL) $(LIBPATH) $(OBJS) $(LIB); \
	fi;

$(PROG_DBG): $(OBJS) $(START_OBJS)
	@if [ "$(FILE3)" = "yes" ]; then                                      \
		echo "Linking $(PROG_DBG)...";                                \
		if [ "$(ZMAKEVERBOSE)" = "yes" ]; then \
			echo "$(START_LINK) $(START_LFLAGS) -o $(PROG_DBG) $(START_LIBPATH) $(ZLIBDIR)/$(ZPLCOMMLAYER)_start_dbg.o $(START_OBJS) $(STARTLIB)"; \
		fi; \
		$(START_LINK) $(START_LFLAGS) -o $(PROG_DBG) $(START_LIBPATH) $(ZLIBDIR)/$(ZPLCOMMLAYER)_start_dbg.o $(START_OBJS) $(STARTLIB); \
	fi; 

backup:
	@echo "Backing up..."
	@cp *.c bkup
	@cp *.h bkup
	@cp Makefile bkup

clean:
	@echo "Cleaning..."
	@rm -f *.o
	@rm -f $(PROG_EXE)
	@rm -f $(PROG_REAL)
	@rm -f $(PROG_DBG)

clobber:
	@echo "Clobbering..."
	@rm -f *~ 
	@rm -f #* 
	@rm -f $(PROG_EXE) 
	@rm -f $(PROG_REAL) 
	@rm -f $(PROG_DBG) 
	@rm -f $(PROG)_cat.c
	@rm -f *.o 
	@rm -f core

depend: $(PROG)_cat.c
	@$(DEPEND) $(START_INCL) -- $(PROG)_cat.c

