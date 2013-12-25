/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "md_zlib.h"
#include "zc_proto.h"
#include "zlib.h"


void _InitLibraries(const int numcomms,const int enspercommid[],
		    const int maxdirs,const int maxwrapdirs,
		    const int numremapmaps) {
  _InitIndexi();
  _InitAtComm(numcomms,enspercommid,maxdirs,maxwrapdirs);
  _InitWrap(numcomms,enspercommid);
  _InitScanRed();
  _PERM_Init(numremapmaps);
  _InitNodeStateVector();
  _MemLogMark("Start of user data");
}


void _InitRuntime(int argc,char* argv[]) {
  _InitMem();
  _InitIO();
  _ParseConfigArgs(argc,argv);
}


void _UnInitRuntime(void) {
  _UnInitMem();
}
