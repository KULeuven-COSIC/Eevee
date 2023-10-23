#include "htmac128_gmp.h"

#include <assert.h>
#include <stdio.h>

#include <libblake2s/blake2s.h>

#include "mimc128_gmp.h"
#include "gmp_utils.h"

static void ctr(unsigned char *dest,
  const unsigned char *src,
  unsigned long long src_len,
  const unsigned char *npub,
  mimc128 *instance,
  mpz_t e_k_1,
  mpz_t key,
  int do_enc
) {
  unsigned read_len;
  unsigned write_len;
  if(do_enc) {
    read_len = 15;
    write_len = 16;
  }else{
    read_len = 16;
    write_len = 15;
  }

  mpz_t nonce, element;

  mpz_init(nonce);
  read(nonce, npub, 15);

  mpz_t ctr_m;
  mpz_init_set(ctr_m, e_k_1);
  mpz_t keystream;
  mpz_init(keystream);
  mpz_init(element);
  for(unsigned long long m_counter = 0; m_counter < src_len/read_len; m_counter++) {
    // printf("Reading from %lld (%d)\n", read_len*m_counter, read_len);
    read(element, &src[read_len*m_counter], read_len);
    // ctr_m = i * E_k(1)
    mod_add(keystream, ctr_m, nonce, instance->prime);
    mimc128_enc(instance, keystream, key, keystream);

    if(do_enc) {
      mod_add(element, element, keystream, instance->prime);
    }else{
      mod_sub(element, element, keystream, instance->prime);
    }

    write(&dest[write_len*m_counter], element, write_len);

    //ctr_m += E_k(1)
    mod_add(ctr_m, ctr_m, e_k_1, instance->prime);
  }

  if(do_enc && src_len % read_len != 0) {
    // printf("Reading from %lld (%lld)\n", src_len - src_len%read_len, src_len%read_len);
    read(element, &src[src_len - src_len%read_len], src_len%read_len);
    // ctr_m = i * E_k(1)
    mod_add(keystream, ctr_m, nonce, instance->prime);
    mimc128_enc(instance, keystream, key, keystream);

    mod_add(element, element, keystream, instance->prime);

    write(&dest[write_len * (src_len/read_len)], element, write_len);
  }
  mpz_clear(nonce);
  mpz_clear(keystream);
  mpz_clear(element);
  mpz_clear(ctr_m);
}

int htmac_mimc_128_crypto_aead_encrypt(
       unsigned char *c,unsigned long long clen,
       const unsigned char *m,unsigned long long mlen,
       const unsigned char *ad,unsigned long long adlen,
       const unsigned char *npub,
       const unsigned char *k
) {
  (void)clen;

  mpz_t key, e_k_1;
  mimc128 instance;


  mpz_init(key);
  read(key, k, 16);
  mpz_init_set_ui(e_k_1, 1);

  mimc128_init(&instance);

  mimc128_enc(&instance, e_k_1, key, e_k_1);

  ctr(c, m, mlen, npub, &instance, e_k_1, key, 1);

  unsigned int tag_start = mlen/15 * 16;
  if(mlen % 15 != 0) {
    tag_start += 16;
  }

  // compute hash over AD and ciphertext
  blake2s_state blake;
  blake2s_keyed_init(&blake, npub, 15);
  if(adlen > 0) {
    blake2s_update(&blake, ad, adlen);
  }
  if(mlen > 0) {
    blake2s_update(&blake, c, tag_start);
  }
  unsigned char hash[32];
  blake2s_final(&blake, hash);
  mpz_t element;
  mpz_init(element);
  read(element, hash, 16);
  mpz_mod(element, element, instance.prime);



  // finalize tag
  mpz_t ctr_m;
  mpz_init_set_str(ctr_m, "800000000000000000000000001b8000", 16);
  mod_mul(ctr_m, ctr_m, e_k_1, instance.prime);
  mod_add(element, element, ctr_m, instance.prime);
  mimc128_enc(&instance, element, key, element);

  write(&c[tag_start], element, 16);

  mpz_clear(key);
  mpz_clear(element);
  mpz_clear(ctr_m);
  mpz_clear(e_k_1);

  mimc128_clear(&instance);
  return 0;
}

int htmac_mimc_128_crypto_aead_decrypt(
       unsigned char *m,unsigned long long *mlen,
       const unsigned char *c,unsigned long long clen,
       const unsigned char *ad,unsigned long long adlen,
       const unsigned char *npub,
       const unsigned char *k
) {
  (void)mlen;
  assert(clen % 16 == 0);
  mpz_t key, e_k_1;
  mimc128 instance;


  mpz_init(key);
  read(key, k, 16);
  mpz_init_set_ui(e_k_1, 1);

  mimc128_init(&instance);

  mimc128_enc(&instance, e_k_1, key, e_k_1);

  ctr(m, c, clen-16, npub, &instance, e_k_1, key, 0);

  // compute hash over AD and ciphertext
  blake2s_state blake;
  blake2s_keyed_init(&blake, npub, 15);
  if(adlen > 0) {
    blake2s_update(&blake, ad, adlen);
  }
  if(clen-16 > 0) {
    blake2s_update(&blake, c, clen-16);
  }
  unsigned char hash[32];
  blake2s_final(&blake, hash);
  mpz_t element;
  mpz_init(element);
  read(element, hash, 16);
  mpz_mod(element, element, instance.prime);

  // finalize tag
  mpz_t ctr_m;
  mpz_init_set_str(ctr_m, "800000000000000000000000001b8000", 16);
  mod_mul(ctr_m, ctr_m, e_k_1, instance.prime);
  mod_add(element, element, ctr_m, instance.prime);
  mimc128_enc(&instance, element, key, element);

  mpz_clear(key);
  mpz_clear(e_k_1);
  mimc128_clear(&instance);

  // compare tag
  read(ctr_m, &c[clen-16], 16);
  int res = mpz_cmp(ctr_m, element);

  mpz_clear(ctr_m);
  mpz_clear(element);
  return res == 0 ? 0 : -1;
}
