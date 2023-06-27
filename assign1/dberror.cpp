#include "dberror.h"
#include <iostream>

std::string RC_message;

void printError(RC code) {
  if (RC_message.empty()) {
    throw std::invalid_argument("Error message not set.");
  }
  std::cout << "ERROR " << code << ": " << RC_message << std::endl;
}
