#ifndef MIMC_GMP_MIMC_H
#define MIMC_GMP_MIMC_H

#ifdef USE_MINI_GMP

#include "mini-gmp.h"

#ifndef MINI_GMP_LIMB_TYPE
#define MINI_GMP_LIMB_TYPE int
#endif // MINI_GMP_LIMB_TYPE

#else

#include <gmp.h>

#endif

#ifndef NDEBUG
#define CHECK_RET(x) (x)
#else
#define CHECK_RET(x) assert((x) == 0)
#endif

#define mod_add(rop,a,b,modulus) mpz_add(rop, a, b); mpz_mod(rop, rop, modulus)
#define mod_sub(rop,a,b,modulus) mpz_sub(rop, a, b); mpz_mod(rop, rop, modulus)
#define mod_mul(rop,a,b,modulus) mpz_mul(rop, a, b); mpz_mod(rop, rop, modulus)

#endif // MIMC_H
