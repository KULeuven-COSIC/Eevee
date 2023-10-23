#include "aes_gcm.h"
#include <string.h>

#define IMPL 0

#ifdef IMPL
#include "aes.h"
#include "ghash.h"
#else
#include <mbedtls/gcm.h>
#include <mbedtls/aes.h>
#endif

#define BYTE_0( x ) ( (uint8_t) (   ( x )         & 0xff ) )
#define BYTE_1( x ) ( (uint8_t) ( ( ( x ) >> 8  ) & 0xff ) )
#define BYTE_2( x ) ( (uint8_t) ( ( ( x ) >> 16 ) & 0xff ) )
#define BYTE_3( x ) ( (uint8_t) ( ( ( x ) >> 24 ) & 0xff ) )

#define PUT_UINT32_BE( n, data, offset )                \
{                                                               \
    ( data )[( offset )    ] = BYTE_3( n );             \
    ( data )[( offset ) + 1] = BYTE_2( n );             \
    ( data )[( offset ) + 2] = BYTE_1( n );             \
    ( data )[( offset ) + 3] = BYTE_0( n );             \
}

#ifdef IMPL

/* Increment the counter. */
static void gcm_incr( unsigned char y[16] )
{
    size_t i;
    for( i = 16; i > 12; i-- )
        if( ++y[i - 1] != 0 )
            break;
}

#endif

int aes_gcm_128_encrypt(
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
  #ifdef IMPL
  // derive hash key and tag mask
  unsigned char hash_key[16];
  unsigned char tag_mask[16];

  unsigned char buf[16];
  memset(buf, 0, 16);
  unsigned char cnt0[16];
  memset(cnt0, 0, 16);
  memcpy(cnt0, npub, 12);
  cnt0[15] = 1;

  rkey_t aes_schedule;
  aes128_keyschedule_ffs(&aes_schedule, k);
  aes128_encrypt_ffs(hash_key, tag_mask, buf, cnt0, &aes_schedule);

  gcm_incr(cnt0);
  unsigned char cnt1[16];
  memcpy(cnt1, cnt0, 16);
  gcm_incr(cnt1);
  unsigned char out[32];

  unsigned long long n_blocks = mlen/16;
  unsigned char *c_ptr = c;
  for(unsigned long long i=0; i<n_blocks/2; i++) {
    aes128_encrypt_ffs(out, out+16, cnt0, cnt1, &aes_schedule);
    // create ciphertext
    for(int j=0; j<32; j++) {
      c_ptr[j] = m[j] ^ out[j];
    }
    c_ptr += 32;
    m += 32;
    // increment both counters by 2
    gcm_incr(cnt0);
    gcm_incr(cnt0);
    gcm_incr(cnt1);
    gcm_incr(cnt1);
  }
  // remaining blocks
  aes128_encrypt_ffs(out, out+16, cnt0, cnt1, &aes_schedule);
  for(unsigned long long j=0; j<(mlen - (n_blocks/2)*32); j++) {
    c_ptr[j] = m[j] ^ out[j];
  }

  ghash_context ghash;
  ghash_init(&ghash, hash_key);
  if(adlen > 0) {
    unsigned long long ad_blocks = adlen/16;
    for(unsigned long long i=0; i<ad_blocks; i++) {
      ghash_process_block(&ghash, ad);
      ad += 16;
    }
    if(adlen % 16 != 0) {
      memset(buf, 0, 16);
      memcpy(buf, ad, adlen % 16);
      ghash_process_block(&ghash, buf);
    }
  }
  if(mlen > 0) {
    unsigned long long c_blocks = mlen/16;
    for(unsigned long long i=0; i<c_blocks; i++) {
      ghash_process_block(&ghash, c);
      c += 16;
    }
    if(mlen % 16 != 0) {
      memset(buf, 0, 16);
      memcpy(buf, c, mlen % 16);
      ghash_process_block(&ghash, buf);
    }
  }
  // length block
  memset(buf, 0, 16);
  PUT_UINT32_BE( ( (8*adlen) >> 32 ), buf, 0  );
  PUT_UINT32_BE( ( 8*adlen       ), buf, 4  );
  PUT_UINT32_BE( ( (8*mlen)  >> 32 ), buf, 8  );
  PUT_UINT32_BE( ( 8*mlen        ), buf, 12 );

  ghash_process_block(&ghash, buf);

  ghash_finalize(&ghash, buf);

  // move tag out
  for(int i=0; i<16; i++) {
    tag[i] = buf[i] ^ tag_mask[i];
  }
  return 0;
  #else
  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);
  int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, k, 128);
  if (ret != 0) {
    mbedtls_gcm_free(&ctx);
    return ret;
  }
  ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, mlen, npub, 12, ad, adlen, m, c, 16, tag);
  mbedtls_gcm_free(&ctx);
  return ret;
  #endif
}
