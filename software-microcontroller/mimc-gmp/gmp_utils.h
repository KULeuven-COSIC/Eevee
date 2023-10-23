#ifndef MIMC_GMP_GMP_UTILS_H
#define MIMC_GMP_GMP_UTILS_H

#ifdef USE_MINI_GMP
#include "mini-gmp.h"
#else
#include <gmp.h>
#endif

void read(mpz_t dest, const unsigned char* src, unsigned int n);

void write(unsigned char* dest, const mpz_t src, size_t n);

#endif // MIMC_GMP_GMP_UTILS_H
