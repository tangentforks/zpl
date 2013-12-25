SHELL = /bin/sh

all: compiler libs

clean: cleanlibs FORCE
	cd comp && $(MAKE) clean

compiler: FORCE
	cd comp && $(MAKE)

libs: libmi libmd

cleanlibs: FORCE
	cd src && ./zmake clean
	cd src/$(ZPLCOMMLAYER) && ./zmake clean

libmi: FORCE
	cd src && ./zmake

libmd: FORCE
	cd src/$(ZPLCOMMLAYER) && ./zmake

FORCE:

