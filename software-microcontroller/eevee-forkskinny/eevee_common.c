#include "eevee_common.h"

// #include <stdio.h>

void block_xor64(unsigned char *dst, const unsigned char *src) {
  // printf("block_xor64(%p, %p)\n", (void*)dst, (void*)src);
  for(unsigned i=0; i<64/8; i++)
    dst[i] ^= src[i];
}

void block_xor128(unsigned char *dst, const unsigned char *src) {
  for(unsigned i=0; i<128/8; i++)
    dst[i] ^= src[i];
}
