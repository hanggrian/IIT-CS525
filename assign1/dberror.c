#include "dberror.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *RC_message;

// Print a message to standard out describing the code.
void printError(RC code) {
  if (RC_message != NULL) {
    printf("EC (%i), \"%s\"\n", code, RC_message);
  } else {
    printf("EC (%i)\n", code);
  }
}

char *errorMessage(RC code) {
  char *message;
  if (RC_message != NULL) {
    message = (char *) malloc(strlen(RC_message) + 30);
    sprintf(message, "EC (%i), \"%s\"\n", code, RC_message);
  } else {
    message = (char *) malloc(30);
    sprintf(message, "EC (%i)\n", code);
  }
  return message;
}
