#ifndef EEVEE_COMMON_H
#define EEVEE_COMMON_H

void block_xor64(unsigned char *dst, const unsigned char *src);
void block_xor128(unsigned char *dst, const unsigned char *src);

// define bindings to a Forkskinny implementation
#if defined EEVEE_FORKSKINNY_BACKEND_OPT32
#include "../forkskinny-opt32/internal-forkskinny.h"
typedef forkskinny_64_192_tweakey_schedule_t eevee_forkskinny_64_192_tks_t;
typedef forkskinny_128_256_tweakey_schedule_t eevee_forkskinny_128_256_tks_t;
typedef forkskinny_128_384_tweakey_schedule_t eevee_forkskinny_128_384_tks_t;

#define eevee_common_forkskinny_64_192_init_tk1(/*eevee_forkskinny_64_192_tks_t* */tk1, /*const uint8_t* */ key, /*unsigned */nb_rounds) \
  forkskinny_64_192_init_tks_tweakpart(tk1, key, nb_rounds);

#define eevee_common_forkskinny_64_192_init_tk23(/*eevee_forkskinny_64_192_tks_t* */tk23, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_64_192_init_tks_keypart(tk23, key, nb_rounds);

#define eevee_common_forkskinny_128_256_init_tk1(/*eevee_forkskinny_128_256_tks_t* */tk1, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_128_256_init_tks_part(tk1, key, nb_rounds, 0);

#define eevee_common_forkskinny_128_256_init_tk2(/*eevee_forkskinny_128_256_tks_t* */tk2, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_128_256_init_tks_part(tk2, key, nb_rounds, 1);

#define eevee_common_forkskinny_128_384_init_tk1(/*eevee_forkskinny_128_384_tks_t* */tk1, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_128_384_init_tks_part(tk1, key, nb_rounds, 0);

#define eevee_common_forkskinny_128_384_init_tk2(/*eevee_forkskinny_128_384_tks_t* */tk2, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_128_384_init_tks_part(tk2, key, nb_rounds, 1);

#define eevee_common_forkskinny_128_384_init_tk3(/*eevee_forkskinny_128_384_tks_t* */tk3, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_128_384_init_tks_part(tk3, key, nb_rounds, 2);

#define eevee_common_forkskinny_64_192_encrypt(/*const eevee_forkskinny_64_192_tks_t* */ tk1, /*const eevee_forkskinny_64_192_tks_t* */ tk23, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_64_192_encrypt_with_tks(tk1, tk23, output_left, output_right, input);

#define eevee_common_forkskinny_64_192_decrypt(/*const eevee_forkskinny_64_192_tks_t* */ tk1, /*const eevee_forkskinny_64_192_tks_t* */ tk23, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_64_192_decrypt_with_tks(tk1, tk23, output_left, output_right, input);

#define eevee_common_forkskinny_128_256_encrypt(/*const eevee_forkskinny_128_256_tks_t* */ tk1, /*const eevee_forkskinny_128_256_tks_t* */ tk2, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_128_256_encrypt_with_tks(tk1, tk2, output_left, output_right, input);

#define eevee_common_forkskinny_128_256_decrypt(/*const eevee_forkskinny_128_256_tks_t* */ tk1, /*const eevee_forkskinny_128_256_tks_t* */ tk2, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_128_256_decrypt_with_tks(tk1, tk2, output_left, output_right, input);

#define eevee_common_forkskinny_128_384_encrypt(/*const eevee_forkskinny_128_384_tks_t* */ tk1, /*const eevee_forkskinny_128_384_tks_t* */ tk2, /*const eevee_forkskinny_128_384_tks_t* */ tk3, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_128_384_encrypt_with_tks(tk1, tk2, tk3, output_left, output_right, input);

#define eevee_common_forkskinny_128_384_decrypt(/*const eevee_forkskinny_128_384_tks_t* */ tk1, /*const eevee_forkskinny_128_384_tks_t* */ tk2, /*const eevee_forkskinny_128_384_tks_t* */ tk3, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_128_384_decrypt_with_tks(tk1, tk2, tk3, output_left, output_right, input);

#elif defined EEVEE_FORKSKINNY_BACKEND_C
#include "../forkskinny-c/forkskinny64-cipher.h"
#include "../forkskinny-c/forkskinny128-cipher.h"

typedef ForkSkinny64Key_t eevee_forkskinny_64_192_tks_t;
typedef ForkSkinny128Key_t eevee_forkskinny_128_256_tks_t;
typedef ForkSkinny128Key_t eevee_forkskinny_128_384_tks_t;

#define eevee_common_forkskinny_64_192_init_tk1(/*eevee_forkskinny_64_192_tks_t* */tk1, /*const uint8_t* */ key, /*unsigned */nb_rounds) \
  forkskinny_c_64_192_init_tk1(tk1, key, nb_rounds);

#define eevee_common_forkskinny_64_192_init_tk23(/*eevee_forkskinny_64_192_tks_t* */tk23, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_c_64_192_init_tk2_tk3(tk23, key, nb_rounds);

#define eevee_common_forkskinny_128_256_init_tk1(/*eevee_forkskinny_128_256_tks_t* */tk1, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_c_128_256_init_tk1(tk1, key, nb_rounds);

#define eevee_common_forkskinny_128_256_init_tk2(/*eevee_forkskinny_128_256_tks_t* */tk2, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_c_128_256_init_tk2(tk2, key, nb_rounds);

#define eevee_common_forkskinny_128_384_init_tk1(/*eevee_forkskinny_128_384_tks_t* */tk1, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_c_128_384_init_tk1(tk1, key, nb_rounds);

#define eevee_common_forkskinny_128_384_init_tk2(/*eevee_forkskinny_128_384_tks_t* */tk2, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_c_128_384_init_tk2(tk2, key, nb_rounds);

#define eevee_common_forkskinny_128_384_init_tk3(/*eevee_forkskinny_128_384_tks_t* */tk3, /*const uint8_t* */ key, /*unsigned*/ nb_rounds) \
  forkskinny_c_128_384_init_tk3(tk3, key, nb_rounds);


#define eevee_common_forkskinny_64_192_encrypt(/*const eevee_forkskinny_64_192_tks_t* */ tk1, /*const eevee_forkskinny_64_192_tks_t* */ tk23, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_c_64_192_encrypt(tk1, tk23, output_left, output_right, input);

#define eevee_common_forkskinny_64_192_decrypt(/*const eevee_forkskinny_64_192_tks_t* */ tk1, /*const eevee_forkskinny_64_192_tks_t* */ tk23, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_c_64_192_decrypt(tk1, tk23, output_left, output_right, input);

#define eevee_common_forkskinny_128_256_encrypt(/*const eevee_forkskinny_128_256_tks_t* */ tk1, /*const eevee_forkskinny_128_256_tks_t* */ tk2, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_c_128_256_encrypt(tk1, tk2, output_left, output_right, input);

#define eevee_common_forkskinny_128_256_decrypt(/*const eevee_forkskinny_128_256_tks_t* */ tk1, /*const eevee_forkskinny_128_256_tks_t* */ tk2, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_c_128_256_decrypt(tk1, tk2, output_left, output_right, input);

#define eevee_common_forkskinny_128_384_encrypt(/*const eevee_forkskinny_128_384_tks_t* */ tk1, /*const eevee_forkskinny_128_384_tks_t* */ tk2, /*const eevee_forkskinny_128_384_tks_t* */ tk3, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_c_128_384_encrypt(tk1, tk2, tk3, output_left, output_right, input);

#define eevee_common_forkskinny_128_384_decrypt(/*const eevee_forkskinny_128_384_tks_t* */ tk1, /*const eevee_forkskinny_128_384_tks_t* */ tk2, /*const eevee_forkskinny_128_384_tks_t* */ tk3, /*uint8_t* */ output_left, /*uint8_t* */ output_right, /*const uint8_t* */ input) \
  forkskinny_c_128_384_decrypt(tk1, tk2, tk3, output_left, output_right, input);

#else
#error Unsupported Forkskinny backend
#endif


#endif // EEVEE_COMMON_H
