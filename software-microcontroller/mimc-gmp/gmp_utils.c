#include "gmp_utils.h"

#include <assert.h>

void read(mpz_t dest, const unsigned char* src, unsigned int n) {
  // assert(n <= 16);
  mpz_import(dest, n, -1, sizeof(unsigned char), 0, 0, src);
}

void write(unsigned char* dest, const mpz_t src, size_t n) {
  // assert(n <= 16);
  size_t count;
  mpz_export(dest, &count, -1, sizeof(unsigned char), 0, 0, src);
  // leading zero limbs are not written by mpz_export
  for(size_t i=count; i<n; i++) {
    dest[i] = 0;
  }
}
