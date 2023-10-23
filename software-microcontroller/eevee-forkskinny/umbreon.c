#include "umbreon.h"
#include "eevee_common.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#ifdef EEVEE_VERBOSE
static void print_n(const unsigned char *buf, unsigned int n) {
    for(unsigned int i=0; i<n; i++)
        printf("%02x", buf[i]);
}
#endif

// compile UMBREON_FORKSKINNY_64_192_N48_V1

// 15-bit counter => 2^15 * 8 byte
#define EEVEE_MAX_MESSAGE_LENGTH 262144

#define EEVEE_BLOCKSIZE 8
#define EEVEE_TKSIZE 24
#define EEVEE_KEYSIZE 16
#define EEVEE_NONCESIZE 6

// forkskinny-opt32 has the forkcipher legs reversed
#define EEVEE_FK_FORWARD(tks1, tks2, output_left,output_right,input) \
  eevee_common_forkskinny_64_192_encrypt(tks1, tks2, output_right, output_left, input)

#define EEVEE_FK_INVERT(tks1, tks2, output_left,output_right,input) \
  eevee_common_forkskinny_64_192_decrypt(tks1, tks2, output_right, output_left, input)

#define EEVEE_TKS_T eevee_forkskinny_64_192_tks_t
#define EEVEE_INIT_FIXED_TKS(tk23, tweakey) \
  eevee_common_forkskinny_64_192_init_tk23(tk23, tweakey, FORKSKINNY_64_192_ROUNDS_BEFORE + 2*FORKSKINNY_64_192_ROUNDS_AFTER)
#define EEVEE_INIT_VAR_TKS_ONE_LEG(tk1, tweakey) \
  eevee_common_forkskinny_64_192_init_tk1(tk1, tweakey + EEVEE_KEYSIZE, FORKSKINNY_64_192_ROUNDS_BEFORE + FORKSKINNY_64_192_ROUNDS_AFTER)
#define EEVEE_INIT_VAR_TKS_TWO_LEG(tk1, tweakey) \
  eevee_common_forkskinny_64_192_init_tk1(tk1, tweakey + EEVEE_KEYSIZE, FORKSKINNY_64_192_ROUNDS_BEFORE + 2*FORKSKINNY_64_192_ROUNDS_AFTER)

#define EEVEE_BLOCK_XOR block_xor64

#define EEVEE_PREFIX(name) umbreon_forkskinny_64_192##name

#include "umbreon_core.c"

#undef EEVEE_MAX_MESSAGE_LENGTH
#undef EEVEE_BLOCKSIZE
#undef EEVEE_TKSIZE
#undef EEVEE_KEYSIZE
#undef EEVEE_NONCESIZE
#undef EEVEE_FK_FORWARD
#undef EEVEE_FK_INVERT
#undef EEVEE_TKS_T
#undef EEVEE_INIT_FIXED_TKS
#undef EEVEE_INIT_VAR_TKS_ONE_LEG
#undef EEVEE_INIT_VAR_TKS_TWO_LEG
#undef EEVEE_PREFIX
#undef EEVEE_BLOCK_XOR

// compile UMBREON_FORKSKINNY_128_256_N112_V1

// 15-bit counter => 2^15 * 16 byte
#define EEVEE_MAX_MESSAGE_LENGTH 524288

#define EEVEE_BLOCKSIZE 16
#define EEVEE_TKSIZE 32
#define EEVEE_KEYSIZE 16
#define EEVEE_NONCESIZE 14

// forkskinny-opt32 has the forkcipher legs reversed
#define EEVEE_FK_FORWARD(tks1, tks2,output_left,output_right,input) \
  eevee_common_forkskinny_128_256_encrypt(tks1, tks2, output_right, output_left, input)

#define EEVEE_FK_INVERT(tks1, tks2, output_left,output_right,input) \
  eevee_common_forkskinny_128_256_decrypt(tks1, tks2, output_right, output_left, input)

#define EEVEE_TKS_T eevee_forkskinny_128_256_tks_t
#define EEVEE_INIT_FIXED_TKS(tk2, tweakey) \
  eevee_common_forkskinny_128_256_init_tk2(tk2, tweakey, FORKSKINNY_128_256_ROUNDS_BEFORE + 2*FORKSKINNY_128_256_ROUNDS_AFTER)
#define EEVEE_INIT_VAR_TKS_ONE_LEG(tk1, tweakey) \
  eevee_common_forkskinny_128_256_init_tk1(tk1, tweakey + EEVEE_KEYSIZE, FORKSKINNY_128_256_ROUNDS_BEFORE + FORKSKINNY_128_256_ROUNDS_AFTER)
#define EEVEE_INIT_VAR_TKS_TWO_LEG(tk1, tweakey) \
  eevee_common_forkskinny_128_256_init_tk1(tk1, tweakey + EEVEE_KEYSIZE, FORKSKINNY_128_256_ROUNDS_BEFORE + 2*FORKSKINNY_128_256_ROUNDS_AFTER)

#define EEVEE_BLOCK_XOR block_xor128

#define EEVEE_PREFIX(name) umbreon_forkskinny_128_256##name

#include "umbreon_core.c"

#undef EEVEE_MAX_MESSAGE_LENGTH
#undef EEVEE_BLOCKSIZE
#undef EEVEE_TKSIZE
#undef EEVEE_KEYSIZE
#undef EEVEE_NONCESIZE
#undef EEVEE_FK_FORWARD
#undef EEVEE_FK_INVERT
#undef EEVEE_TKS_T
#undef EEVEE_INIT_FIXED_TKS
#undef EEVEE_INIT_VAR_TKS_ONE_LEG
#undef EEVEE_INIT_VAR_TKS_TWO_LEG
#undef EEVEE_PREFIX
#undef EEVEE_BLOCK_XOR
