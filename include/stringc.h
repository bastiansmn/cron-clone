#ifndef STRING_H
#define STRING_H

#include <stdint.h>

typedef struct stringc {
  uint32_t L;
  char* val;
} stringc;
#endif