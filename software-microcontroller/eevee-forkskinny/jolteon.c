#include "jolteon.h"
#include "eevee_common.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// #define EEVEE_VERBOSE 0

#ifdef EEVEE_VERBOSE
static void print_n(const unsigned char *buf, unsigned int n) {
    for(unsigned int i=0; i<n; i++)
        printf("%02x", buf[i]);
}
#endif

// compile JOLTEON_FORKSKINNY_64_192_N48_V1

// 15-bit counter => 2^15 * 8 byte
#define JOLTEON_MAX_MESSAGE_LENGTH 262144

#define JOLTEON_BLOCKSIZE 8
#define JOLTEON_TKSIZE 24
#define JOLTEON_KEYSIZE 16
#define JOLTEON_NONCESIZE 6

// forkskinny-opt32 has the forkcipher legs reversed
#define JOLTEON_FK_FORWARD(tks1,tks2,output_left,output_right,input) \
  eevee_common_forkskinny_64_192_encrypt(tks1, tks2, output_right, output_left, input)

#define JOLTEON_FK_INVERT(tks1, tks2,output_left,output_right,input) \
  eevee_common_forkskinny_64_192_decrypt(tks1, tks2, output_right, output_left, input)

#define JOLTEON_TKS_T eevee_forkskinny_64_192_tks_t
#define JOLTEON_INIT_FIXED_TKS(tk23, tweakey) \
  eevee_common_forkskinny_64_192_init_tk23(tk23, tweakey, FORKSKINNY_64_192_ROUNDS_BEFORE + 2*FORKSKINNY_64_192_ROUNDS_AFTER)
#define JOLTEON_INIT_VAR_TKS_ONE_LEG(tk1, tweakey) \
  eevee_common_forkskinny_64_192_init_tk1(tk1, tweakey + JOLTEON_KEYSIZE, FORKSKINNY_64_192_ROUNDS_BEFORE + FORKSKINNY_64_192_ROUNDS_AFTER)
#define JOLTEON_INIT_VAR_TKS_TWO_LEG(tk1, tweakey) \
  eevee_common_forkskinny_64_192_init_tk1(tk1, tweakey + JOLTEON_KEYSIZE, FORKSKINNY_64_192_ROUNDS_BEFORE + 2*FORKSKINNY_64_192_ROUNDS_AFTER)

#define JOLTEON_PREFIX(name) jolteon_forkskinny_64_192##name
#define JOLTEON_BLOCK_XOR block_xor64

#include "jolteon_core.c"

#undef JOLTEON_MAX_MESSAGE_LENGTH
#undef JOLTEON_BLOCKSIZE
#undef JOLTEON_TKSIZE
#undef JOLTEON_KEYSIZE
#undef JOLTEON_NONCESIZE
#undef JOLTEON_FK_FORWARD
#undef JOLTEON_FK_INVERT
#undef JOLTEON_TKS_T
#undef JOLTEON_INIT_FIXED_TKS
#undef JOLTEON_INIT_VAR_TKS_ONE_LEG
#undef JOLTEON_INIT_VAR_TKS_TWO_LEG
#undef JOLTEON_PREFIX
#undef JOLTEON_BLOCK_XOR

// compile JOLTEON_FORKSKINNY_128_256_N112_V1

// 15-bit counter => 2^15 * 16 byte
#define JOLTEON_MAX_MESSAGE_LENGTH 524288

#define JOLTEON_BLOCKSIZE 16
#define JOLTEON_TKSIZE 32
#define JOLTEON_KEYSIZE 16
#define JOLTEON_NONCESIZE 14

// forkskinny-opt32 has the forkcipher legs reversed
#define JOLTEON_FK_FORWARD(tks1,tks2,output_left,output_right,input) \
  eevee_common_forkskinny_128_256_encrypt(tks1, tks2, output_right, output_left, input)

#define JOLTEON_FK_INVERT(tks1,tks2,output_left,output_right,input) \
  eevee_common_forkskinny_128_256_decrypt(tks1, tks2, output_right, output_left, input)

#define JOLTEON_TKS_T eevee_forkskinny_128_256_tks_t
#define JOLTEON_INIT_FIXED_TKS(tk2, tweakey) \
  eevee_common_forkskinny_128_256_init_tk2(tk2, tweakey, FORKSKINNY_128_256_ROUNDS_BEFORE + 2*FORKSKINNY_128_256_ROUNDS_AFTER)
#define JOLTEON_INIT_VAR_TKS_ONE_LEG(tk1, tweakey) \
  eevee_common_forkskinny_128_256_init_tk1(tk1, tweakey + JOLTEON_KEYSIZE, FORKSKINNY_128_256_ROUNDS_BEFORE + FORKSKINNY_128_256_ROUNDS_AFTER)
#define JOLTEON_INIT_VAR_TKS_TWO_LEG(tk1, tweakey) \
  eevee_common_forkskinny_128_256_init_tk1(tk1, tweakey + JOLTEON_KEYSIZE, FORKSKINNY_128_256_ROUNDS_BEFORE + 2*FORKSKINNY_128_256_ROUNDS_AFTER)


#define JOLTEON_PREFIX(name) jolteon_forkskinny_128_256##name
#define JOLTEON_BLOCK_XOR block_xor128

#include "jolteon_core.c"

#undef JOLTEON_MAX_MESSAGE_LENGTH
#undef JOLTEON_BLOCKSIZE
#undef JOLTEON_TKSIZE
#undef JOLTEON_KEYSIZE
#undef JOLTEON_NONCESIZE
#undef JOLTEON_FK_FORWARD
#undef JOLTEON_FK_INVERT
#undef JOLTEON_TKS_T
#undef JOLTEON_INIT_FIXED_TKS
#undef JOLTEON_INIT_VAR_TKS_ONE_LEG
#undef JOLTEON_INIT_VAR_TKS_TWO_LEG
#undef JOLTEON_PREFIX
#undef JOLTEON_BLOCK_XOR
