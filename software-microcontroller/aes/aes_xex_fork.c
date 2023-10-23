#include "aes_xex_fork.h"
#include "aes.h"
#include <string.h>

/***
 * This file implements Jolteon with a trivial forkcipher construction based on
 * AES-128. The forkcipher is constructed by two parallel tweakable block ciphers,
 * i.e., for AES-128, we use the LRW-XEX construction to make it tweakable. The tweak
 * is multiplied with the XEX-key. This implementation uses the GHASH multiplication
 * by interpreting tweak and XEX-key as GHASH field elements.
 * The gmul code is taken from ghash.c.
 */


#define GET_UINT32_BE( data , offset )                  \
    (                                                           \
          ( (uint32_t) ( data )[( offset )    ] << 24 )         \
        | ( (uint32_t) ( data )[( offset ) + 1] << 16 )         \
        | ( (uint32_t) ( data )[( offset ) + 2] <<  8 )         \
        | ( (uint32_t) ( data )[( offset ) + 3]       )         \
    )

#define BYTE_0( x ) ( (uint8_t) (   ( x )         & 0xff ) )
#define BYTE_1( x ) ( (uint8_t) ( ( ( x ) >> 8  ) & 0xff ) )
#define BYTE_2( x ) ( (uint8_t) ( ( ( x ) >> 16 ) & 0xff ) )
#define BYTE_3( x ) ( (uint8_t) ( ( ( x ) >> 24 ) & 0xff ) )

#define PUT_UINT32_BE( n, data, offset )                \
{                                                       \
    ( data )[( offset )    ] = BYTE_3( n );             \
    ( data )[( offset ) + 1] = BYTE_2( n );             \
    ( data )[( offset ) + 2] = BYTE_1( n );             \
    ( data )[( offset ) + 3] = BYTE_0( n );             \
}

void xex_init(xex_key_t *ctx, const unsigned char *h) {
  memset(ctx, 0, sizeof(xex_key_t));
  /* pack h as two 64-bits ints, big-endian */
  int i, j;
  uint64_t hi, lo;
  uint64_t vl, vh;
  hi = GET_UINT32_BE( h,  0  );
  lo = GET_UINT32_BE( h,  4  );
  vh = (uint64_t) hi << 32 | lo;

  hi = GET_UINT32_BE( h,  8  );
  lo = GET_UINT32_BE( h,  12 );
  vl = (uint64_t) hi << 32 | lo;

  /* 8 = 1000 corresponds to 1 in GF(2^128) */
  ctx->HL[8] = vl;
  ctx->HH[8] = vh;
  /* 0 corresponds to 0 in GF(2^128) */
  ctx->HH[0] = 0;
  ctx->HL[0] = 0;

  for( i = 4; i > 0; i >>= 1 )
  {
      uint32_t T = ( vl & 1 ) * 0xe1000000U;
      vl  = ( vh << 63 ) | ( vl >> 1 );
      vh  = ( vh >> 1 ) ^ ( (uint64_t) T << 32);

      ctx->HL[i] = vl;
      ctx->HH[i] = vh;
  }

  for( i = 2; i <= 8; i *= 2 )
  {
      uint64_t *HiL = ctx->HL + i, *HiH = ctx->HH + i;
      vh = *HiH;
      vl = *HiL;
      for( j = 1; j < i; j++ )
      {
          HiH[j] = vh ^ ctx->HH[j];
          HiL[j] = vl ^ ctx->HL[j];
      }
  }
}

/*
 * Shoup's method for multiplication use this table with
 *      last4[x] = x times P^128
 * where x and last4[x] are seen as elements of GF(2^128) as in [MGV]
 */
static const uint64_t last4[16] =
{
    0x0000, 0x1c20, 0x3840, 0x2460,
    0x7080, 0x6ca0, 0x48c0, 0x54e0,
    0xe100, 0xfd20, 0xd940, 0xc560,
    0x9180, 0x8da0, 0xa9c0, 0xb5e0
};

/*
 * Sets output to x times the XEX key using the precomputed tables.
 * x and output are seen as elements of GF(2^128) as in [MGV].
 */
static void gcm_mult(const xex_key_t *ctx, const unsigned char x[16],
                      unsigned char output[16] )
{
    int i = 0;
    unsigned char lo, hi, rem;
    uint64_t zh, zl;

    lo = x[15] & 0xf;

    zh = ctx->HH[lo];
    zl = ctx->HL[lo];

    for( i = 15; i >= 0; i-- )
    {
        lo = x[i] & 0xf;
        hi = ( x[i] >> 4 ) & 0xf;

        if( i != 15 )
        {
            rem = (unsigned char) zl & 0xf;
            zl = ( zh << 60 ) | ( zl >> 4 );
            zh = ( zh >> 4 );
            zh ^= (uint64_t) last4[rem] << 48;
            zh ^= ctx->HH[lo];
            zl ^= ctx->HL[lo];

        }

        rem = (unsigned char) zl & 0xf;
        zl = ( zh << 60 ) | ( zl >> 4 );
        zh = ( zh >> 4 );
        zh ^= (uint64_t) last4[rem] << 48;
        zh ^= ctx->HH[hi];
        zl ^= ctx->HL[hi];
    }

    PUT_UINT32_BE( zh >> 32, output, 0 );
    PUT_UINT32_BE( zh, output, 4 );
    PUT_UINT32_BE( zl >> 32, output, 8 );
    PUT_UINT32_BE( zl, output, 12 );
}

void long_xex_init(long_xex_key_t *long_xex_key, const unsigned char *key) {
  // load normal key into first key_power
  xex_init(long_xex_key->key_power, key);
  // square key using key_cubed as temporary buffer
  gcm_mult(long_xex_key->key_power, key, long_xex_key->key_cubed);
  // load squared key into second power
  xex_init(long_xex_key->key_power+1, long_xex_key->key_cubed);
  // cube key
  gcm_mult(long_xex_key->key_power+1, key, long_xex_key->key_cubed);
}

void fork(unsigned char* c_left0, unsigned char* c_left1,
  const unsigned char* m0, const unsigned char* m1,
  const unsigned char* tweak0, const unsigned char* tweak1,
  const rkey_t *rkeys, const xex_key_t *xex_key) {
    unsigned char xex_tweak0[16];
    unsigned char xex_tweak1[16];
    unsigned char m0_xex[16];
    gcm_mult(xex_key, tweak0, xex_tweak0);
    for(int i=0; i<16; i++) { m0_xex[i] = m0[i] ^ xex_tweak0[i]; }
    if(m1 == NULL) {
      aes128_encrypt_ffs(c_left0, xex_tweak1, m0_xex, NULL, rkeys);
      for(int i=0; i<16; i++) { c_left0[i] ^= xex_tweak0[i]; }
    }else{
      unsigned char m1_xex[16];
      gcm_mult(xex_key, tweak1, xex_tweak1);
      for(int i=0; i<16; i++) { m1_xex[i] = m1[i] ^ xex_tweak1[i]; }
      aes128_encrypt_ffs(c_left0, c_left1, m0_xex, m1_xex, rkeys);
      for(int i=0; i<16; i++) { c_left0[i] ^= xex_tweak0[i]; }
      for(int i=0; i<16; i++) { c_left1[i] ^= xex_tweak1[i]; }
    }
}

// tweak blocks are swapped
// i.e. tweak[0..15] contain second tweak block
//      tweak[16..31] contain first tweak block
void long_fork(unsigned char* c_left0, unsigned char* c_left1,
  const unsigned char* m0, const unsigned char* m1,
  const unsigned char* tweak0, const unsigned char* tweak1,
  const rkey_t *rkeys, const long_xex_key_t *long_xex_key) {
    unsigned char xex_tweak0[16];
    unsigned char xex_tweak1[16];
    gcm_mult(long_xex_key->key_power, tweak0 + 16, xex_tweak0);
    // second block of tweak using c_left0 as temporary buffer
    gcm_mult(long_xex_key->key_power+1, tweak0, c_left0);
    for(int i=0; i<16; i++) { xex_tweak0[i] ^= c_left0[i]; }

    if (tweak1 != NULL) {
      gcm_mult(long_xex_key->key_power, tweak1 + 16, xex_tweak1);
      // second block of tweak using c_left0 as temporary buffer
      gcm_mult(long_xex_key->key_power+1, tweak1, c_left0);
      // add in third block (=1 bit)
      for(int i=0; i<16; i++) {
        xex_tweak1[i] ^= c_left0[i] ^ long_xex_key->key_cubed[i];
      }
    }

    unsigned char m0_xex[16];
    unsigned char m1_xex[16];
    for(int i=0; i<16; i++) { m0_xex[i] = m0[i] ^ xex_tweak0[i]; }
    if (tweak1 == NULL) {
      aes128_encrypt_ffs(c_left0, m1_xex, m0_xex, NULL, rkeys);
      for(int i=0; i<16; i++) { c_left0[i] ^= xex_tweak0[i]; }
    }else{
      for(int i=0; i<16; i++) { m1_xex[i] = m1[i] ^ xex_tweak1[i]; }
      aes128_encrypt_ffs(c_left0, c_left1, m0_xex, m1_xex, rkeys);
      for(int i=0; i<16; i++) { c_left0[i] ^= xex_tweak0[i]; }
      for(int i=0; i<16; i++) { c_left1[i] ^= xex_tweak1[i]; }
    }
}
