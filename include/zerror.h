/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __ZERROR_H_
#define __ZERROR_H_

#include <stdio.h>
#include "file_io.h"
#include "md_zlib.h"

#define _RT_ANY_START       fflush(stdout),_PRIV_ACCESS(_io_in_ens) = 1
#define _RT_ANY_STOP        _PRIV_ACCESS(_io_in_ens) = 0

#define _RT_ALL_START       fflush(stdout),(_INDEX!=0) ? (_BarrierSynch()) : (
#define _RT_ALL_STOP        _BarrierSynch())

#define _RT_WARN_START      _SCALAR_FPRINTF0(zerr,"WARNING: ")
#define _RT_WARN_STOP       _SCALAR_LINEFEED(zerr)

#define _RT_FATAL_START     _SCALAR_FPRINTF0(zerr,"ERROR: ")
#define _RT_FATAL_STOP      _SCALAR_LINEFEED(zerr),_ZPL_halt(0)

#define _RT_ALL_WARN_START  _RT_ALL_START _RT_WARN_START
#define _RT_ALL_WARN_STOP   _RT_WARN_STOP,_RT_ALL_STOP

#define _RT_ANY_WARN_START  _RT_ANY_START,_RT_WARN_START
#define _RT_ANY_WARN_STOP   _RT_WARN_STOP,_RT_ANY_STOP

#define _RT_ALL_FATAL_START _RT_ALL_START _RT_FATAL_START
#define _RT_ALL_FATAL_STOP  _RT_FATAL_STOP,_RT_ALL_STOP

#define _RT_ANY_FATAL_START _RT_ANY_START,_RT_FATAL_START
#define _RT_ANY_FATAL_STOP  _RT_FATAL_STOP,_RT_ANY_STOP


#define _RT_ANY_WARN0(s) (_RT_ANY_WARN_START,\
			  _SCALAR_FPRINTF0(zerr,s),\
			  _RT_ANY_WARN_STOP)
#define _RT_ANY_WARN1(s,a) (_RT_ANY_WARN_START,\
			    _SCALAR_FPRINTF1(zerr,s,a),\
			    _RT_ANY_WARN_STOP)
#define _RT_ANY_WARN2(s,a,b) (_RT_ANY_WARN_START,\
			      _SCALAR_FPRINTF2(zerr,s,a,b),\
			      _RT_ANY_WARN_STOP)
#define _RT_ANY_WARN3(s,a,b,c) (_RT_ANY_WARN_START,\
				_SCALAR_FPRINTF3(zerr,s,a,b,c),\
				_RT_ANY_WARN_STOP)
#define _RT_ANY_WARN4(s,a,b,c,d) (_RT_ANY_WARN_START,\
				  _SCALAR_FPRINTF4(zerr,s,a,b,c,d),\
				  _RT_ANY_WARN_STOP)
#define _RT_ANY_WARN5(s,a,b,c,d,e) (_RT_ANY_WARN_START,\
				    _SCALAR_FPRINTF5(zerr,s,a,b,c,d,e),\
				    _RT_ANY_WARN_STOP)
#define _RT_ANY_WARN6(s,a,b,c,d,e,f) (_RT_ANY_WARN_START,\
				      _SCALAR_FPRINTF6(zerr,s,a,b,c,d,e,f),\
				      _RT_ANY_WARN_STOP)

#define _RT_ALL_WARN0(s) (_RT_ALL_WARN_START,\
			  _SCALAR_FPRINTF0(zerr,s),\
			  _RT_ALL_WARN_STOP)
#define _RT_ALL_WARN1(s,a) (_RT_ALL_WARN_START,\
			    _SCALAR_FPRINTF1(zerr,s,a),\
			    _RT_ALL_WARN_STOP)
#define _RT_ALL_WARN2(s,a,b) (_RT_ALL_WARN_START,\
			      _SCALAR_FPRINTF2(zerr,s,a,b),\
			      _RT_ALL_WARN_STOP)
#define _RT_ALL_WARN3(s,a,b,c) (_RT_ALL_WARN_START,\
				_SCALAR_FPRINTF3(zerr,s,a,b,c),\
				_RT_ALL_WARN_STOP)
#define _RT_ALL_WARN4(s,a,b,c,d) (_RT_ALL_WARN_START,\
				  _SCALAR_FPRINTF4(zerr,s,a,b,c,d),\
				  _RT_ALL_WARN_STOP)
#define _RT_ALL_WARN5(s,a,b,c,d,e) (_RT_ALL_WARN_START,\
				    _SCALAR_FPRINTF5(zerr,s,a,b,c,d,e),\
				    _RT_ALL_WARN_STOP)
#define _RT_ALL_WARN6(s,a,b,c,d,e,f) (_RT_ALL_WARN_START,\
				      _SCALAR_FPRINTF6(zerr,s,a,b,c,d,e,f),\
				      _RT_ALL_WARN_STOP)


#define _RT_ANY_FATAL0(s) (_RT_ANY_FATAL_START,\
			   _SCALAR_FPRINTF0(zerr,s),\
			   _RT_ANY_FATAL_STOP)
#define _RT_ANY_FATAL1(s,a) (_RT_ANY_FATAL_START,\
			     _SCALAR_FPRINTF1(zerr,s,a),\
			     _RT_ANY_FATAL_STOP)
#define _RT_ANY_FATAL2(s,a,b) (_RT_ANY_FATAL_START,\
			       _SCALAR_FPRINTF2(zerr,s,a,b),\
			       _RT_ANY_FATAL_STOP)
#define _RT_ANY_FATAL3(s,a,b,c) (_RT_ANY_FATAL_START,\
				 _SCALAR_FPRINTF3(zerr,s,a,b,c),\
				 _RT_ANY_FATAL_STOP)
#define _RT_ANY_FATAL4(s,a,b,c,d) (_RT_ANY_FATAL_START,\
				   _SCALAR_FPRINTF4(zerr,s,a,b,c,d),\
				   _RT_ANY_FATAL_STOP)
#define _RT_ANY_FATAL5(s,a,b,c,d,e) (_RT_ANY_FATAL_START,\
				     _SCALAR_FPRINTF5(zerr,s,a,b,c,d,e),\
				     _RT_ANY_FATAL_STOP)
#define _RT_ANY_FATAL6(s,a,b,c,d,e,f) (_RT_ANY_FATAL_START,\
				       _SCALAR_FPRINTF6(zerr,s,a,b,c,d,e,f),\
				       _RT_ANY_FATAL_STOP)


#define _RT_ALL_FATAL0(s) (_RT_ALL_FATAL_START,\
			   _SCALAR_FPRINTF0(zerr,s),\
			   _RT_ALL_FATAL_STOP)
#define _RT_ALL_FATAL1(s,a) (_RT_ALL_FATAL_START,\
			     _SCALAR_FPRINTF1(zerr,s,a),\
			     _RT_ALL_FATAL_STOP)
#define _RT_ALL_FATAL2(s,a,b) (_RT_ALL_FATAL_START,\
			       _SCALAR_FPRINTF2(zerr,s,a,b),\
			       _RT_ALL_FATAL_STOP)
#define _RT_ALL_FATAL3(s,a,b,c) (_RT_ALL_FATAL_START,\
				 _SCALAR_FPRINTF3(zerr,s,a,b,c),\
				 _RT_ALL_FATAL_STOP)
#define _RT_ALL_FATAL4(s,a,b,c,d) (_RT_ALL_FATAL_START,\
				   _SCALAR_FPRINTF4(zerr,s,a,b,c,d),\
				   _RT_ALL_FATAL_STOP)
#define _RT_ALL_FATAL5(s,a,b,c,d,e) (_RT_ALL_FATAL_START,\
				     _SCALAR_FPRINTF5(zerr,s,a,b,c,d,e),\
				     _RT_ALL_FATAL_STOP)
#define _RT_ALL_FATAL6(s,a,b,c,d,e,f) (_RT_ALL_FATAL_START,\
				       _SCALAR_FPRINTF6(zerr,s,a,b,c,d,e,f),\
				       _RT_ALL_FATAL_STOP)


#endif
