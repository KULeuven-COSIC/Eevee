
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "eevee-forkskinny/umbreon.h"
#include "skinny-modes/skinny_modes.h"
#include "eevee-forkskinny/jolteon.h"
#include "eevee-forkskinny/espeon.h"

#include "mimc-gmp/mimc-gmp.h"

#include "forkskinny-opt32/internal-forkskinny.h"
#include "forkskinny-c/forkskinny64-cipher.h"
#include "forkskinny-c/forkskinny128-cipher.h"

#include "forkskinny-opt32/skinny.h"
#include "skinny-c/include/skinny64-cipher.h"
#include "skinny-c/include/skinny128-cipher.h"

#include "aes/aes_gcm_siv.h"
#include "aes/aes_gcm.h"
#include "aes/jolteon_aes.h"
#include "aes/espeon_aes.h"

static const char *UMBREON_FORKSKINNY_64_192 = "umbreon_forkskinny_64_192";
static const char *UMBREON_FORKSKINNY_128_256 = "umbreon_forkskinny_128_256";
static const char *JOLTEON_FORKSKINNY_64_192 = "jolteon_forkskinny_64_192";
static const char *JOLTEON_FORKSKINNY_128_256 = "jolteon_forkskinny_128_256";
static const char *ESPEON_FORKSKINNY_128_256 = "espeon_forkskinny_128_256";
static const char *ESPEON_FORKSKINNY_128_384 = "espeon_forkskinny_128_384";
static const char *HTMAC_SKINNY_128_256 = "htmac_skinny_128_256";
static const char *PMAC_SKINNY_128_256 = "pmac_skinny_128_256";
static const char *HTMAC_MIMC_128 = "htmac_mimc_128";
static const char *PPMAC_MIMC_128 = "ppmac_mimc_128";

static const char *SKINNY_C_128_256 = "skinny_c_128_256";
static const char *SKINNY_C_64_192 = "skinny_c_64_192";
static const char *SKINNY_FK_128_256 = "skinny_fk_128_256";
static const char *SKINNY_FK_64_192 = "skinny_fk_64_192";

static const char *FORKSKINNY_64_192 = "forkskinny_64_192";
static const char *FORKSKINNY_128_256 = "forkskinny_128_256";
static const char *FORKSKINNY_C_64_192 = "forkskinny_c_64_192";
static const char *FORKSKINNY_C_128_256 = "forkskinny_c_128_256";

static const char *GCM_SIV_AES_128 = "aes_gcm_siv_128";
static const char *GCM_AES_128 = "aes_gcm_128";
static const char *JOLTEON_AES_128 = "jolteon_aes_128";
static const char *ESPEON_AES_128 = "espeon_aes_128";

typedef int (*aead_enc_f)(
  /** ciphertext destination buffer */
  unsigned char *c,
  /** tag destination buffer */
  unsigned char *tag,
  /** message and message length */
  const unsigned char *m,unsigned long long mlen,
  /** associated data and AD length */
  const unsigned char *ad,unsigned long long adlen,
  /** nonce **/
  const unsigned char *npub,
  /** key **/
  const unsigned char *k
);

typedef int (*aead_dec_f)(
  /** message destination buffer */
  unsigned char *m,
  /** ciphertext and length */
  const unsigned char *c,unsigned long long clen,
  /** tag **/
  const unsigned char *tag,
  /** associated data and AD length */
  const unsigned char *ad,unsigned long long adlen,
  /** nonce **/
  const unsigned char *npub,
  /** key **/
  const unsigned char *k
);

typedef int (*mimc_aead_enc_f)(
  /** ciphertext & tag destination buffer */
  unsigned char *c,unsigned long long clen,
  /** message and message length */
  const unsigned char *m,unsigned long long mlen,
  /** associated data and AD length */
  const unsigned char *ad,unsigned long long adlen,
  /** nonce **/
  const unsigned char *npub,
  /** key **/
  const unsigned char *k
);

typedef void (*skinny_enc_f)(
  /** ciphertext destination buffer */
  unsigned char *c,
  /** message */
  const unsigned char *m,
  /** key **/
  const unsigned char *k
);

typedef void (*forkskinny_enc_f)(
  /** ciphertext destination buffer */
  unsigned char *c0, unsigned char *c1,
  /** message */
  const unsigned char *m,
  /** key **/
  const unsigned char *k
);

void fill_rand(unsigned char *buf, unsigned int n) {
    for(unsigned int i=0; i<n; i++)
        buf[i] = (unsigned char)rand();
}

void print_buf_hex(unsigned char *buf, unsigned int len) {
  for(unsigned int i=0; i<len; i++) {
    printf("%02x", buf[i]);
  }
}

int generate_f(int mlen, int samples, int keylen, int noncelen, int taglen, aead_enc_f aead_enc, aead_dec_f aead_dec, int check) {
  unsigned char message[mlen];
  unsigned char key[keylen];
  unsigned char nonce[noncelen];
  unsigned char ciphertext[mlen];
  unsigned char tag[taglen];

  memset(ciphertext, 0, mlen);
  memset(tag, 0, taglen);

  unsigned char dec_message[mlen];
  srand(1);

  for(int i=0; i<samples; i++) {
    fill_rand(message, mlen);
    fill_rand(key, keylen);
    fill_rand(nonce, noncelen);
    int ret = aead_enc(ciphertext, tag, message, mlen, NULL, 0, nonce, key);
    assert(ret == 0);

    print_buf_hex(key, keylen);
    printf(" ");
    print_buf_hex(nonce, noncelen);
    printf(" ");
    print_buf_hex(message, mlen);
    printf(" ");

    if(check) {
      int tagcheck = aead_dec(dec_message, ciphertext, mlen, tag, NULL, 0, nonce, key);
      if(tagcheck != 0) {
        printf("Decryption failed!\n");
        return -1;
      }
      for(int i=0; i<mlen; i++) {
        if(message[i] != dec_message[i]) {
          printf("Decrypted message mismatch\n");
          return -1;
        }
      }
    }

    print_buf_hex(ciphertext, mlen);
    printf(" ");
    print_buf_hex(tag, taglen);
    printf("\n");
  }
  return 0;
}

void generate_mimc128(int mlen, int samples, mimc_aead_enc_f aead_enc) {
  unsigned char message[mlen];
  unsigned char key[16];
  unsigned char nonce[15];
  unsigned long long clen = clen_mimc128_ctr(mlen) + 16;
  unsigned char ciphertext[clen];

  mimc128 mimc;
  mimc128_init(&mimc);

  srand(1);
  for(int i=0; i<samples; i++) {
    fill_rand(message, mlen);
    int bad_key = 1;
    while(bad_key) {
      fill_rand(key, 16);
      mpz_t k;
      mpz_init(k);
      read(k, key, 16);
      bad_key = mpz_cmp(mimc.prime, k) <= 0;
    }

    fill_rand(nonce, 15);
    int ret = aead_enc(ciphertext, clen, message, mlen, NULL, 0, nonce, key);
    assert(ret == 0);
    print_buf_hex(key, 16);
    printf(" ");
    print_buf_hex(nonce, 15);
    printf(" ");
    print_buf_hex(message, mlen);
    printf(" ");
    print_buf_hex(ciphertext, clen-16);
    printf(" ");
    print_buf_hex(ciphertext+clen-16, 16);
    printf("\n");
  }
}

void generate_skinny(int samples, skinny_enc_f enc_f, unsigned blocksize, unsigned keysize) {
  unsigned char message[blocksize];
  unsigned char ciphertext[blocksize];
  unsigned char key[keysize];
  srand(2);
  for(int i=0; i < samples; i++) {
    fill_rand(message, blocksize);
    fill_rand(key, keysize);
    enc_f(ciphertext, message, key);
    print_buf_hex(key, keysize);
    printf(" ");
    print_buf_hex(NULL, 0); // no nonce
    printf(" ");
    print_buf_hex(message, blocksize);
    printf(" ");
    print_buf_hex(ciphertext, blocksize);
    printf(" ");
    print_buf_hex(NULL, 0); // no tag
    printf("\n");
  }
}

void skinny_c_64_192_enc(unsigned char *output, const unsigned char *input, const unsigned char *key) {
  Skinny64Key_t tks;
  skinny64_set_key(&tks, key, 24);
  skinny64_ecb_encrypt(output, input, &tks);
}

void skinny_c_128_256_enc(unsigned char *output, const unsigned char *input, const unsigned char *key) {
  Skinny128Key_t tks;
  skinny128_set_key(&tks, key, 32);
  skinny128_ecb_encrypt(output, input, &tks);
}

void skinny_fk_64_192_enc(unsigned char *output, const unsigned char *input, const unsigned char *key) {
  skinny_64_192_encrypt(key, output, input);
}

void skinny_fk_128_256_enc(unsigned char *output, const unsigned char *input, const unsigned char *key) {
  skinny_128_256_encrypt(key, output, input);
}

void generate_forkskinny(int samples, forkskinny_enc_f enc_f, unsigned blocksize, unsigned keysize) {
  unsigned char message[blocksize];
  unsigned char ciphertext[2*blocksize];
  unsigned char key[keysize];
  srand(2);
  for(int i=0; i < samples; i++) {
    fill_rand(message, blocksize);
    fill_rand(key, keysize);
    enc_f(ciphertext, ciphertext + blocksize, message, key);
    print_buf_hex(key, keysize);
    printf(" ");
    print_buf_hex(NULL, 0); // no nonce
    printf(" ");
    print_buf_hex(message, blocksize);
    printf(" ");
    print_buf_hex(ciphertext, 2*blocksize);
    printf(" ");
    print_buf_hex(NULL, 0); // no tag
    printf("\n");
  }
}

void forkskinny_c_64_192_enc(unsigned char *output_left, unsigned char *output_right, const unsigned char *input, const unsigned char *key) {
  ForkSkinny64Key_t tks1, tks2;
  forkskinny_c_64_192_init_tk1(&tks1, key, FORKSKINNY64_MAX_ROUNDS);
  forkskinny_c_64_192_init_tk2_tk3(&tks2, key + 8, FORKSKINNY64_MAX_ROUNDS);
  forkskinny_c_64_192_encrypt(&tks1, &tks2, output_left, output_right, input);
}

void forkskinny_c_128_256_enc(unsigned char *output_left, unsigned char *output_right, const unsigned char *input, const unsigned char *key) {
  ForkSkinny128Key_t tks1, tks2;
  forkskinny_c_128_256_init_tk1(&tks1, key, FORKSKINNY128_MAX_ROUNDS);
  forkskinny_c_128_256_init_tk2(&tks2, key + 16, FORKSKINNY128_MAX_ROUNDS);
  forkskinny_c_128_256_encrypt(&tks1, &tks2, output_left, output_right, input);
}

void forkskinny_64_192_enc(unsigned char *output_left, unsigned char *output_right, const unsigned char *input, const unsigned char *key) {
  forkskinny_64_192_encrypt(key, output_left, output_right, input);
}

void forkskinny_128_256_enc(unsigned char *output_left, unsigned char *output_right, const unsigned char *input, const unsigned char *key) {
  forkskinny_128_256_encrypt(key, output_left, output_right, input);
}

int generate(const char *primitive, int mlen, int samples, int check) {
  if(strncmp(HTMAC_MIMC_128, primitive, strlen(HTMAC_MIMC_128)) == 0) {
    generate_mimc128(mlen, samples, htmac_mimc_128_crypto_aead_encrypt);
    return 0;
  }else if(strncmp(PPMAC_MIMC_128, primitive, strlen(PPMAC_MIMC_128)) == 0) {
    generate_mimc128(mlen, samples, ppmac_mimc_128_crypto_aead_encrypt);
    return 0;
  }

  if(strncmp(SKINNY_C_64_192, primitive, strlen(SKINNY_C_64_192)) == 0) {
    if(check) {
      printf("--check not supported for primitive %s\n", SKINNY_C_64_192);
      return -2;
    }
    generate_skinny(samples, skinny_c_64_192_enc, 8, 24);
    return 0;
  }
  if(strncmp(SKINNY_C_128_256, primitive, strlen(SKINNY_C_128_256)) == 0) {
    if(check) {
      printf("--check not supported for primitive %s\n", SKINNY_C_128_256);
      return -2;
    }
    generate_skinny(samples, skinny_c_128_256_enc, 16, 32);
    return 0;
  }
  if(strncmp(SKINNY_FK_64_192, primitive, strlen(SKINNY_FK_64_192)) == 0) {
    if(check) {
      printf("--check not supported for primitive %s\n", SKINNY_FK_64_192);
      return -2;
    }
    generate_skinny(samples, skinny_fk_64_192_enc, 8, 24);
    return 0;
  }
  if(strncmp(SKINNY_FK_128_256, primitive, strlen(SKINNY_FK_128_256)) == 0) {
    if(check) {
      printf("--check not supported for primitive %s\n", SKINNY_FK_128_256);
      return -2;
    }
    generate_skinny(samples, skinny_fk_128_256_enc, 16, 32);
    return 0;
  }

  if(strncmp(FORKSKINNY_C_64_192, primitive, strlen(FORKSKINNY_C_64_192)) == 0) {
    generate_forkskinny(samples, forkskinny_c_64_192_enc, 8, 24);
    return 0;
  }
  if(strncmp(FORKSKINNY_C_128_256, primitive, strlen(FORKSKINNY_C_128_256)) == 0) {
    generate_forkskinny(samples, forkskinny_c_128_256_enc, 16, 32);
    return 0;
  }
  if(strncmp(FORKSKINNY_64_192, primitive, strlen(FORKSKINNY_64_192)) == 0) {
    generate_forkskinny(samples, forkskinny_64_192_enc, 8, 24);
    return 0;
  }
  if(strncmp(FORKSKINNY_128_256, primitive, strlen(FORKSKINNY_128_256)) == 0) {
    generate_forkskinny(samples, forkskinny_128_256_enc, 16, 32);
    return 0;
  }


  int keylen;
  int noncelen;
  int taglen;
  aead_enc_f aead_enc = NULL;
  aead_dec_f aead_dec = NULL;
  if(strncmp(UMBREON_FORKSKINNY_64_192, primitive, strlen(UMBREON_FORKSKINNY_64_192)) == 0) {
    keylen = 128/8;
    noncelen = 48/8;
    taglen = 64/8;
    aead_enc = umbreon_forkskinny_64_192_encrypt;
    aead_dec = umbreon_forkskinny_64_192_decrypt;
  }else if(strncmp(UMBREON_FORKSKINNY_128_256, primitive, strlen(UMBREON_FORKSKINNY_128_256)) == 0) {
    keylen = 128/8;
    noncelen = 112/8;
    taglen = 128/8;
    aead_enc = umbreon_forkskinny_128_256_encrypt;
    aead_dec = umbreon_forkskinny_128_256_decrypt;
  }else if(strncmp(HTMAC_SKINNY_128_256, primitive, strlen(HTMAC_SKINNY_128_256)) == 0) {
    keylen = 128/8;
    noncelen = 128/8;
    taglen = 128/8;
    aead_enc = htmac_skinny_128_256_encrypt;
    if(check) {
      printf("--check not supported for primitive %s\n", HTMAC_SKINNY_128_256);
      return -2;
    }
  } else if(strncmp(PMAC_SKINNY_128_256, primitive, strlen(PMAC_SKINNY_128_256)) == 0) {
    keylen = 128/8;
    noncelen = 128/8;
    taglen = 128/8;
    aead_enc = pmac_skinny_128_256_encrypt;
    if(check) {
      printf("--check not supported for primitive %s\n", HTMAC_SKINNY_128_256);
      return -2;
    }
  } else if(strncmp(JOLTEON_FORKSKINNY_64_192, primitive, strlen(JOLTEON_FORKSKINNY_64_192)) == 0) {
    keylen = 128/8;
    noncelen = 48/8;
    taglen = 64/8;
    aead_enc = jolteon_forkskinny_64_192_encrypt;
    aead_dec = jolteon_forkskinny_64_192_decrypt;
  }else if(strncmp(JOLTEON_FORKSKINNY_128_256, primitive, strlen(JOLTEON_FORKSKINNY_128_256)) == 0) {
    keylen = 128/8;
    noncelen = 112/8;
    taglen = 128/8;
    aead_enc = jolteon_forkskinny_128_256_encrypt;
    aead_dec = jolteon_forkskinny_128_256_decrypt;
  }else if(strncmp(ESPEON_FORKSKINNY_128_384, primitive, strlen(ESPEON_FORKSKINNY_128_384)) == 0) {
    keylen = 128/8;
    noncelen = 128/8;
    taglen = 128/8;
    aead_enc = espeon_forkskinny_128_384_encrypt;
    aead_dec = espeon_forkskinny_128_384_decrypt;
  } else if(strncmp(ESPEON_FORKSKINNY_128_256, primitive, strlen(ESPEON_FORKSKINNY_128_256)) == 0) {
    keylen = 128/8;
    noncelen = 13;
    taglen = 128/8;
    aead_enc = espeon_forkskinny_128_256_encrypt;
    aead_dec = espeon_forkskinny_128_256_decrypt;
  } else if(strncmp(GCM_SIV_AES_128, primitive, strlen(GCM_SIV_AES_128)) == 0) {
    keylen = 128/8;
    noncelen = 96/8;
    taglen = 128/8;
    aead_enc = aes_gcm_siv_128_encrypt;
    if(check) {
      printf("--check not supported for primitive %s\n", GCM_SIV_AES_128);
      return -2;
    }
  } else if(strncmp(GCM_AES_128, primitive, strlen(GCM_AES_128)) == 0) {
    keylen = 128/8;
    noncelen = 96/8;
    taglen = 128/8;
    aead_enc = aes_gcm_128_encrypt;
    if(check) {
      printf("--check not supported for primitive %s\n", GCM_AES_128);
      return -2;
    }
  } else if(strncmp(JOLTEON_AES_128, primitive, strlen(JOLTEON_AES_128)) == 0) {
    keylen = 2*(128/8);
    noncelen = 112/8;
    taglen = 128/8;
    aead_enc = jolteon_aes_128_encrypt;
    if(check) {
      printf("--check not supported for primitive %s\n", JOLTEON_AES_128);
      return -2;
    }
  } else if(strncmp(ESPEON_AES_128, primitive, strlen(ESPEON_AES_128)) == 0) {
    keylen = 2*(128/8);
    noncelen = 128/8;
    taglen = 128/8;
    aead_enc = espeon_aes_128_encrypt;
    if(check) {
      printf("--check not supported for primitive %s\n", ESPEON_AES_128);
      return -2;
    }
  } else {
    assert(0);
  }

  return generate_f(mlen, samples, keylen, noncelen, taglen, aead_enc, aead_dec, check);
}

int check_supported_primitive(const char *input) {
  if (strncmp(UMBREON_FORKSKINNY_64_192, input, strlen(UMBREON_FORKSKINNY_64_192)) == 0)
    return 0;
  if (strncmp(UMBREON_FORKSKINNY_128_256, input, strlen(UMBREON_FORKSKINNY_128_256)) == 0)
    return 0;
  if (strncmp(JOLTEON_FORKSKINNY_64_192, input, strlen(JOLTEON_FORKSKINNY_64_192)) == 0)
    return 0;
  if (strncmp(JOLTEON_FORKSKINNY_128_256, input, strlen(JOLTEON_FORKSKINNY_128_256)) == 0)
    return 0;
  if (strncmp(HTMAC_SKINNY_128_256, input, strlen(HTMAC_SKINNY_128_256)) == 0)
    return 0;
  if (strncmp(PMAC_SKINNY_128_256, input, strlen(PMAC_SKINNY_128_256)) == 0)
    return 0;
  if (strncmp(HTMAC_MIMC_128, input, strlen(HTMAC_MIMC_128)) == 0)
    return 0;
  if (strncmp(PPMAC_MIMC_128, input, strlen(PPMAC_MIMC_128)) == 0)
    return 0;
  if (strncmp(ESPEON_FORKSKINNY_128_256, input, strlen(ESPEON_FORKSKINNY_128_256)) == 0)
    return 0;
  if (strncmp(ESPEON_FORKSKINNY_128_384, input, strlen(ESPEON_FORKSKINNY_128_384)) == 0)
    return 0;
  if (strncmp(SKINNY_C_64_192, input, strlen(SKINNY_C_64_192)) == 0)
    return 0;
  if (strncmp(SKINNY_C_128_256, input, strlen(SKINNY_C_128_256)) == 0)
    return 0;
  if (strncmp(SKINNY_FK_64_192, input, strlen(SKINNY_FK_64_192)) == 0)
    return 0;
  if (strncmp(SKINNY_FK_128_256, input, strlen(SKINNY_FK_128_256)) == 0)
    return 0;
  if (strncmp(FORKSKINNY_64_192, input, strlen(FORKSKINNY_64_192)) == 0)
    return 0;
  if (strncmp(FORKSKINNY_128_256, input, strlen(FORKSKINNY_128_256)) == 0)
    return 0;
  if (strncmp(FORKSKINNY_C_64_192, input, strlen(FORKSKINNY_C_64_192)) == 0)
    return 0;
  if (strncmp(FORKSKINNY_C_128_256, input, strlen(FORKSKINNY_C_128_256)) == 0)
    return 0;
  if (strncmp(GCM_SIV_AES_128, input, strlen(GCM_SIV_AES_128)) == 0)
    return 0;
  if (strncmp(GCM_AES_128, input, strlen(GCM_AES_128)) == 0)
    return 0;
  if (strncmp(JOLTEON_AES_128, input, strlen(JOLTEON_AES_128)) == 0)
    return 0;
  if(strncmp(ESPEON_AES_128, input, strlen(ESPEON_AES_128)) == 0)
    return 0;
  return 1;
}

int main(int argc, char* argv[]) {

  if(argc <= 2) {
    printf("%s [--check] primitive (mlen samples)+\n", argv[0]);
    printf("\t--check:\t check if encryption produces a valid ciphertext\n");
    printf("\tsupported primitives:\n");
    printf("\t\t%s\n", UMBREON_FORKSKINNY_64_192);
    printf("\t\t%s\n", UMBREON_FORKSKINNY_128_256);
    printf("\t\t%s\n", JOLTEON_FORKSKINNY_64_192);
    printf("\t\t%s\n", JOLTEON_FORKSKINNY_128_256);
    printf("\t\t%s\n", ESPEON_FORKSKINNY_128_256);
    printf("\t\t%s\n", ESPEON_FORKSKINNY_128_384);
    printf("\t\t%s\n", HTMAC_SKINNY_128_256);
    printf("\t\t%s\n", PMAC_SKINNY_128_256);
    printf("\t\t%s\n", HTMAC_MIMC_128);
    printf("\t\t%s\n", PPMAC_MIMC_128);
    printf("\t\t%s\n", SKINNY_C_64_192);
    printf("\t\t%s\n", SKINNY_C_128_256);
    printf("\t\t%s\n", SKINNY_FK_64_192);
    printf("\t\t%s\n", SKINNY_FK_128_256);
    printf("\t\t%s\n", FORKSKINNY_C_64_192);
    printf("\t\t%s\n", FORKSKINNY_C_128_256);
    printf("\t\t%s\n", FORKSKINNY_64_192);
    printf("\t\t%s\n", FORKSKINNY_128_256);
    printf("\t\t%s\n", GCM_SIV_AES_128);
    printf("\t\t%s\n", GCM_AES_128);
    printf("\t\t%s\n", JOLTEON_AES_128);
    printf("\t\t%s\n", ESPEON_AES_128);
    printf("\n\tmlen samples: the number of testvectors to generate for each message length mlen\n");
    return 1;
  }

  unsigned offset = 0;
  int check = 0;
  if(strcmp("--check", argv[1]) == 0) {
    offset += 1;
    check = 1;
  }

  char *primitive = argv[offset+1];
  if(check_supported_primitive(primitive)) {
    printf("Unsupported primitive %s\n", primitive);
    return 2;
  }

  int pairs= (argc-2-offset)/2;

  int mlen[pairs];
  int samples[pairs];
  for(int i=0; i<pairs; i++) {
    mlen[i] = strtol(argv[offset + 2 + 2*i], NULL, 10);
    if(mlen[i] == 0) {
      printf("Cannot parse %s as decimal number\n", argv[offset + 2 + 2*i]);
      return 3;
    }
    samples[i] = strtol(argv[offset + 2 + 2*i+1], NULL, 10);
    if(samples[i] == 0) {
      printf("Cannot parse %s as decimal number\n", argv[offset + 2 + 2*i + 1]);
      return 3;
    }
  }

  for(int i=0; i<pairs; i++) {
    // printf("Generate %d %d (%d)", mlen[i], samples[i], pairs);
    int success = generate(primitive, mlen[i], samples[i], check);
    if(success != 0) {
      return success;
    }
  }
  return 0;
}
