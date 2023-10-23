#include "aes_gcm_siv.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>

#include <mbedtls/aes.h>
#include <mbedtls/../../library/common.h>

// uncomment to use AES-GCM-SIV polynomial (defined in RFC 8452)
#define USE_GHASH_POLY

#ifdef USE_GHASH_POLY
#include "ghash.h"
#else
#include "polyval.h"
#endif

#define AES_BLOCK_LEN 16
#define AES_GCM_SIV_NONCE_LEN 12

static int derive_keys(mbedtls_aes_context *aes, unsigned char *auth_key, unsigned char *enc_key, const unsigned char *key, const unsigned char *nonce) {
  unsigned char block[AES_BLOCK_LEN] = {0};
  unsigned char out[AES_BLOCK_LEN] = {0};
  memcpy(block+4, nonce, AES_GCM_SIV_NONCE_LEN);

  int success = mbedtls_aes_setkey_enc(aes, key, 128);
  if(success != 0) return success;
  success = mbedtls_aes_crypt_ecb(aes, MBEDTLS_AES_ENCRYPT, block, out);
  if(success != 0) return success;
  memcpy(auth_key, out, 8);

  block[0] = 1;
  success = mbedtls_aes_crypt_ecb(aes, MBEDTLS_AES_ENCRYPT, block, out);
  if(success != 0) return success;
  memcpy(auth_key+8, out, 8);

  block[0] = 2;
  success = mbedtls_aes_crypt_ecb(aes, MBEDTLS_AES_ENCRYPT, block, out);
  if(success != 0) return success;
  memcpy(enc_key, out, 8);

  block[0] = 3;
  success = mbedtls_aes_crypt_ecb(aes, MBEDTLS_AES_ENCRYPT, block, out);
  if(success != 0) return success;
  memcpy(enc_key+8, out, 8);
  return 0;
}

int aes_gcm_siv_128_encrypt(
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
) {

  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);

  // derive keys
  unsigned char auth_key[AES_BLOCK_LEN];
  unsigned char enc_key[AES_BLOCK_LEN];
  int success = derive_keys(&aes_ctx, auth_key, enc_key, k, npub);
  if(success != 0) return success;

  unsigned char length_block[AES_BLOCK_LEN];


  #ifdef USE_GHASH_POLY
  ghash_context ghash_ctx;
  ghash_init(&ghash_ctx, auth_key);
  #else
  polyval_context poly_ctx;
  polyval_init(&poly_ctx, auth_key);
  #endif

  // process padded AD
  unsigned long long adblocks = adlen/16;
  const unsigned char *ad_ptr = ad;
  int ad_rem = adlen % 16;
  for(unsigned long long i=0; i<adblocks; i++, ad_ptr += 16) {
    #ifdef USE_GHASH_POLY
    ghash_process_block(&ghash_ctx, ad_ptr);
    #else
    polyval_process_block(&poly_ctx, ad_ptr);
    #endif
  }
  if(ad_rem != 0) {
    memset(length_block, 0, AES_BLOCK_LEN);
    memcpy(length_block, ad_ptr, ad_rem);
    #ifdef USE_GHASH_POLY
    ghash_process_block(&ghash_ctx, length_block);
    #else
    polyval_process_block(&poly_ctx, length_block);
    #endif
  }

  // process padded msg
  unsigned long long mblocks = mlen/16;
  const unsigned char *m_ptr = m;
  int m_rem = mlen % 16;
  for(unsigned long long i=0; i<mblocks; i++, m_ptr += 16) {
    #ifdef USE_GHASH_POLY
    ghash_process_block(&ghash_ctx, m_ptr);
    #else
    polyval_process_block(&poly_ctx, m_ptr);
    #endif
  }
  if(m_rem != 0) {
    memset(length_block, 0, AES_BLOCK_LEN);
    memcpy(length_block, m_ptr, m_rem);
    #ifdef USE_GHASH_POLY
    ghash_process_block(&ghash_ctx, length_block);
    #else
    polyval_process_block(&poly_ctx, length_block);
    #endif
  }

  // length block
  uint64_t ad_len_block = adlen * 8;
  uint64_t m_len_block = mlen * 8;
  MBEDTLS_PUT_UINT64_LE(ad_len_block, length_block, 0);
  MBEDTLS_PUT_UINT64_LE(m_len_block, length_block, 8);
  #ifdef USE_GHASH_POLY
  ghash_process_block(&ghash_ctx, length_block);
  #else
  polyval_process_block(&poly_ctx, length_block);
  #endif

  // finalize polyval
  #ifdef USE_GHASH_POLY
  ghash_finalize(&ghash_ctx, length_block);
  #else
  polyval_finalize(&poly_ctx, length_block);
  #endif

  // XOR nonce
  for(int i=0; i<12; i++) {
    length_block[i] ^= npub[i];
  }
  // clear MSbit
  length_block[15] &= 0x7f;

  success = mbedtls_aes_setkey_enc(&aes_ctx, enc_key, 128);
  if(success != 0) return success;
  success = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, length_block, tag);
  if(success != 0) return success;


  unsigned char counter[AES_BLOCK_LEN];
  memcpy(counter, tag, AES_BLOCK_LEN);
  // set MSbit
  counter[15] |= 0x80;
  // AES-CTR
  m_ptr = m;
  unsigned char* c_ptr = c;
  uint32_t cnt = MBEDTLS_GET_UINT32_LE(counter, 0);
  for(unsigned long long i=0; i<mblocks; i++, m_ptr += 16, c_ptr += 16) {
    success = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, counter, c_ptr);
    if(success != 0) return success;
    // xor message
    for(int j=0; j<16; j++) {
      c_ptr[j] ^= m_ptr[j];
    }
    // increment counter
    cnt++;
    MBEDTLS_PUT_UINT32_LE(cnt, counter, 0);
  }
  if(m_rem != 0) {
    success = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, counter, length_block);
    if(success != 0) return success;
    // xor message
    for(int j=0; j<m_rem; j++) {
      c_ptr[j] = length_block[j] ^ m_ptr[j];
    }
  }

  return 0;
}
