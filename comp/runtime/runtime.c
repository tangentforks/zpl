/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include "../include/Cgen.h"
#include "../include/Dgen.h"
#include "../include/Privgen.h"
#include "../include/Stackgen.h"
#include "../include/buildsym.h"
#include "../include/buildzplstmt.h"
#include "../include/expr.h"
#include "../include/glist.h"
#include "../include/global.h"
#include "../include/runtime.h"
#include "../include/symbol.h"
#include "../include/symboltable.h"
#include "../include/zutil.h"

void StartRuntime() {
  StartAccessors();
}

void EndRuntime() {
  EndAccessors();
}
