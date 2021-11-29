#ifndef STRING_H
#define STRING_H

#include <stdint.h>

struct stringc {
  uint32_t L;
  char* val;
};
#endif