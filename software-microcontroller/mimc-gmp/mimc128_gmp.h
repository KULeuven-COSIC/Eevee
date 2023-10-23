#ifndef MIMC_GMP_MIMC128_GMP_H
#define MIMC_GMP_MIMC128_GMP_H

#include "mimc.h"

#define MIMC128_GMP_CONSTANTS 73

typedef struct {
  mpz_t prime;
  mpz_t constants[MIMC128_GMP_CONSTANTS];
} mimc128;

void mimc128_init(mimc128 *instance);

void mimc128_enc(const mimc128 *instance, mpz_t ciphertext, const mpz_t key, const mpz_t message);

void mimc128_clear(mimc128 *instance);

#endif // MIMC_GMP_MIMC128_GMP_H
