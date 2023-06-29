#include "dberror.h"

#include <stdio.h>

char *RC_message;

void printError(RC code) {
  if (RC_message == NULL) {
    printf("ERROR %d\n", code);
    return;
  }
  printf("ERROR %d: %s\n", code, RC_message);
}
