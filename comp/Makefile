#
# The new and improved Makefile for zpl compiler
# Targets:
#   default: builds the compiler (release unless .make.default used)
#   all:     builds the compiler (same as default)
#   debug:   builds a debug version of the compiler
#   devel:   builds a developer version of the compiler
#   release: builds a release verison of the compiler
#   clean:   cleans the compiler directories
#   clobber: really cleans the compiler directories
#   depend:  builds dependences for the compiler directory Makefiles
#
# Typically, a user should just be able to type make.
#
# A developer will probably want to make depend initially, and again
# when the #include files change in the source code.  To build, the
# developer will tend to do "make devel" which turns on a number of
# flags that will enhance warning messages and prevent problems from
# occurring (this is required before checking code into the CVS source
# tree.
#
# To make development easy, a file named .make.default may be placed
# in this directory that will contain the name of the default target
# to build in each subdirectory.  For example, developers will
# typically put "devel" in this file, after which they can simply type
# "make" to get a development build.
#
# gmake-specific concepts were avoided in the writing of this Make
# system to enable portability.
#

SHELL=/bin/sh

default: all

all: FORCE
	@if [ -r .make.default ]; then \
		target=`cat .make.default`; \
	else \
		target=all; \
	fi && ./Makezc $$target 2>&1 | grep -v "make\["

debug: FORCE
	@./Makezc debug 2>&1 | grep -v "make\["

release: FORCE
	@./Makezc release 2>&1 | grep -v "make\["

devel: FORCE
	@./Makezc devel 2>&1 | grep -v "make\["

clean: FORCE
	@./Makezc clean 2>&1 | grep -v "make\["

clobber: FORCE
	@./Makezc clobber 2>&1 | grep -v "make\["

depend: FORCE
	@./Makezc depend 2>&1 | grep -v "make\["

FORCE:
