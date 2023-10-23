#include "ppmac128_gmp.h"
#include "mimc128_gmp.h"

#include "gmp_utils.h"

#include <assert.h>
#include <stdio.h>

static void aead_setup(mimc128 *instance, mpz_t nonce, mpz_t key, mpz_t tag_input, mpz_t e_k_0, mpz_t e_k_1, const unsigned char *npub, const unsigned char *k) {
  mpz_init(nonce);
  read(nonce, npub, 15);
  mpz_init(key);
  read(key, k, 16);

  mpz_init_set_ui(e_k_1, 1);
  mpz_init(e_k_0);

  mimc128_init(instance);

  mimc128_enc(instance, e_k_0, key, e_k_0);
  mimc128_enc(instance, e_k_1, key, e_k_1);

  mpz_init_set(tag_input, nonce);
}

static void ppmac_ad(mpz_t tag_input, mpz_t element, mpz_t ctr_ad, const mimc128 *instance, const mpz_t e_k_0, const mpz_t key, const unsigned char *ad,unsigned long long adlen) {
  mpz_init(element);
  mpz_init_set(ctr_ad, e_k_0);
  for(unsigned long long ad_counter = 0; ad_counter < adlen/15; ad_counter++) {
    read(element, &ad[15*ad_counter], 15);
    // ctr_ad = i * E_k(0)
    mod_add(element, element, ctr_ad, instance->prime);
    mimc128_enc(instance, element, key, element);
    mod_add(tag_input, tag_input, element, instance->prime);

    //ctr_ad += E_k(0)
    mod_add(ctr_ad, ctr_ad, e_k_0, instance->prime);
  }
  if(adlen % 15 != 0) {
    read(element, &ad[adlen - adlen%15], adlen%15);
    // ctr_ad = i * E_k(0)
    mod_add(element, element, ctr_ad, instance->prime);
    mimc128_enc(instance, element, key, element);
    mod_add(tag_input, tag_input, element, instance->prime);

    //ctr_ad += E_k(0)
    mod_add(ctr_ad, ctr_ad, e_k_0, instance->prime);
  }
}

int ppmac_mimc_128_crypto_aead_encrypt(
  unsigned char *c, unsigned long long clen,
  const unsigned char *m,unsigned long long mlen,
  const unsigned char *ad,unsigned long long adlen,
  const unsigned char *npub,
  const unsigned char *k) {
    // ignore unused nsec
    (void)clen;
    // if(*clen < (mlen%15 == 0) ? 16*mlen/15+16 : 16*mlen/15+32) {
    //   printf("Insufficiently sized ciphertext buffer\n");
    //   return -1;
    // }

    mpz_t nonce, key, e_k_1, e_k_0, tag_input;
    mimc128 instance;
    aead_setup(&instance, nonce, key, tag_input, e_k_0, e_k_1, npub, k);

    mpz_t element, ctr_ad;
    ppmac_ad(tag_input, element, ctr_ad, &instance, e_k_0, key, ad, adlen);

    mpz_t ctr_m;
    mpz_init_set(ctr_m, e_k_1);
    mpz_t keystream;
    mpz_init(keystream);
    for(unsigned long long m_counter = 0; m_counter < mlen/15; m_counter++) {
      read(element, &m[15*m_counter], 15);
      // ctr_m = i * E_k(1)
      mod_add(keystream, ctr_m, nonce, instance.prime);
      mimc128_enc(&instance, keystream, key, keystream);

      mod_add(element, element, keystream, instance.prime);
      write(&c[16*m_counter], element, 16);

      mod_add(element, element, ctr_ad, instance.prime);
      mimc128_enc(&instance, element, key, element);
      mod_add(tag_input, tag_input, element, instance.prime);

      //ctr_m += E_k(1)
      mod_add(ctr_m, ctr_m, e_k_1, instance.prime);
      //ctr_ad += E_k(0)
      mod_add(ctr_ad, ctr_ad, e_k_0, instance.prime);
    }
    mpz_clear(e_k_1);

    unsigned long long tag_start = mlen/15 * 16;
    if(mlen % 15 != 0) {
      read(element, &m[mlen - mlen%15], mlen%15);
      // ctr_m = i * E_k(1)
      mod_add(keystream, ctr_m, nonce, instance.prime);
      mimc128_enc(&instance, keystream, key, keystream);

      mod_add(element, element, keystream, instance.prime);
      write(&c[tag_start], element, 16);
      tag_start += 16;

      mod_add(element, element, ctr_ad, instance.prime);
      mimc128_enc(&instance, element, key, element);
      mod_add(tag_input, tag_input, element, instance.prime);
    }
    mpz_clear(nonce);
    mpz_clear(element);
    mpz_clear(ctr_m);
    mpz_clear(keystream);

    // finalize tag
    mpz_set_str(ctr_ad, "800000000000000000000000001b8000", 16);
    mod_mul(ctr_ad, ctr_ad, e_k_0, instance.prime);
    mod_add(tag_input, tag_input, ctr_ad, instance.prime);
    mimc128_enc(&instance, tag_input, key, tag_input);
    write(&c[tag_start], tag_input, 16);

    mpz_clear(key);
    mpz_clear(e_k_0);
    mpz_clear(tag_input);
    mpz_clear(ctr_ad);

    mimc128_clear(&instance);
    return 0;
}

int ppmac_mimc_128_crypto_aead_decrypt(
       unsigned char *m,unsigned long long mlen,
       const unsigned char *c,unsigned long long clen,
       const unsigned char *ad,unsigned long long adlen,
       const unsigned char *npub,
       const unsigned char *k
     ) {
    // ignore unused nsec
    (void)mlen;
    mpz_t nonce, key, e_k_1, e_k_0, tag_input;
    mimc128 instance;
    aead_setup(&instance, nonce, key, tag_input, e_k_0, e_k_1, npub, k);

    mpz_t element, ctr_ad;
    ppmac_ad(tag_input, element, ctr_ad, &instance, e_k_0, key, ad, adlen);

    mpz_t ctr_m;
    mpz_init_set(ctr_m, e_k_1);
    mpz_t keystream;
    mpz_init(keystream);
    for(unsigned long long m_counter = 0; m_counter < (clen-16)/16; m_counter++) {
      read(element, &c[16*m_counter], 16);
      // ctr_m = i * E_k(1)
      mod_add(keystream, ctr_m, nonce, instance.prime);
      mimc128_enc(&instance, keystream, key, keystream);

      mod_sub(keystream, element, keystream, instance.prime);
      write(&m[15*m_counter], keystream, 15);

      mod_add(element, element, ctr_ad, instance.prime);
      mimc128_enc(&instance, element, key, element);
      mod_add(tag_input, tag_input, element, instance.prime);

      //ctr_m += E_k(1)
      mod_add(ctr_m, ctr_m, e_k_1, instance.prime);
      //ctr_ad += E_k(0)
      mod_add(ctr_ad, ctr_ad, e_k_0, instance.prime);
    }
    mpz_clear(nonce);
    mpz_clear(e_k_1);
    mpz_clear(ctr_m);
    mpz_clear(keystream);

    unsigned long long tag_start = clen-16;
    // finalize tag
    mpz_set_str(ctr_ad, "800000000000000000000000001b8000", 16);
    mod_mul(ctr_ad, ctr_ad, e_k_0, instance.prime);
    mod_add(tag_input, tag_input, ctr_ad, instance.prime);
    mimc128_enc(&instance, tag_input, key, tag_input);

    mpz_clear(key);
    mpz_clear(e_k_0);
    mpz_clear(ctr_ad);
    mimc128_clear(&instance);

    // compare tag
    read(element, &c[tag_start], 16);
    int res = mpz_cmp(tag_input, element);

    mpz_clear(tag_input);
    mpz_clear(element);
    return res == 0 ? 0 : -1;
}
