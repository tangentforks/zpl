#include "../include/version.h"
#include "../include/version_num.h"

char* version_string(void) {
  static char retval[16];
  sprintf(retval, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);

  return retval;
}


void version_fprint(FILE* outfile) {
  fprintf(outfile, "%s", version_string());
}
