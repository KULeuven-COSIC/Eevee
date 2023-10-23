#include "polyval.h"

#include <string.h>
#include <stdio.h>

/**
 * Get the unsigned 32 bits integer corresponding to four bytes in
 * little-endian order (LSB first).
 *
 * \param   data    Base address of the memory to get the four bytes from.
 * \param   offset  Offset from \p data of the first and least significant
 *                  byte of the four bytes to build the 32 bits unsigned
 *                  integer from.
 */
#define GET_UINT32_LE( data, offset )                   \
    (                                                           \
          ( (uint32_t) ( data )[( offset )    ]       )         \
        | ( (uint32_t) ( data )[( offset ) + 1] <<  8 )         \
        | ( (uint32_t) ( data )[( offset ) + 2] << 16 )         \
        | ( (uint32_t) ( data )[( offset ) + 3] << 24 )         \
    )

#define BYTE_0( x ) ( (uint8_t) (   ( x )         & 0xff ) )
#define BYTE_1( x ) ( (uint8_t) ( ( ( x ) >> 8  ) & 0xff ) )
#define BYTE_2( x ) ( (uint8_t) ( ( ( x ) >> 16 ) & 0xff ) )
#define BYTE_3( x ) ( (uint8_t) ( ( ( x ) >> 24 ) & 0xff ) )

/**
 * Put in memory a 32 bits unsigned integer in little-endian order.
 *
 * \param   n       32 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 32
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the least significant
 *                  byte of the 32 bits unsigned integer \p n.
 */
#define PUT_UINT32_LE( n, data, offset )                  \
  {                                                       \
      ( data )[( offset )    ] = BYTE_0( n );             \
      ( data )[( offset ) + 1] = BYTE_1( n );             \
      ( data )[( offset ) + 2] = BYTE_2( n );             \
      ( data )[( offset ) + 3] = BYTE_3( n );             \
  }

void polyval_init(polyval_context *ctx, const unsigned char *key) {
  memset((unsigned char*)ctx, 0, sizeof(polyval_context));

  ctx->h[0] = GET_UINT32_LE(key, 0);
  ctx->h[1] = GET_UINT32_LE(key, 4);
  ctx->h[2] = GET_UINT32_LE(key, 8);
  ctx->h[3] = GET_UINT32_LE(key, 12);
}

/// Bit-reverse a 32-bit word in constant time.
static uint32_t rev32(uint32_t x) {
    x = ((x & 0x55555555) << 1) | (x >> 1 & 0x55555555);
    x = ((x & 0x33333333) << 2) | (x >> 2 & 0x33333333);
    x = ((x & 0x0f0f0f0f) << 4) | (x >> 4 & 0x0f0f0f0f);
    x = ((x & 0x00ff00ff) << 8) | (x >> 8 & 0x00ff00ff);
    return (x << 16) | (x >> 16);
}

/// Multiplication in GF(2)[X], truncated to the low 32-bits, with “holes”
/// (sequences of zeroes) to avoid carry spilling.
///
/// When carries do occur, they wind up in a "hole" and are subsequently masked
/// out of the result.
static uint32_t bmul32(uint32_t x, uint32_t y) {
  uint32_t x0 = (x & 0x11111111);
  uint32_t x1 = (x & 0x22222222);
  uint32_t x2 = (x & 0x44444444);
  uint32_t x3 = (x & 0x88888888);
  uint32_t y0 = (y & 0x11111111);
  uint32_t y1 = (y & 0x22222222);
  uint32_t y2 = (y & 0x44444444);
  uint32_t y3 = (y & 0x88888888);

  uint32_t z0 = (x0 * y0) ^ (x1 * y3) ^ (x2 * y2) ^ (x3 * y1);
  uint32_t z1 = (x0 * y1) ^ (x1 * y0) ^ (x2 * y3) ^ (x3 * y2);
  uint32_t z2 = (x0 * y2) ^ (x1 * y1) ^ (x2 * y0) ^ (x3 * y3);
  uint32_t z3 = (x0 * y3) ^ (x1 * y2) ^ (x2 * y1) ^ (x3 * y0);

  z0 &= 0x11111111;
  z1 &= 0x22222222;
  z2 &= 0x44444444;
  z3 &= 0x88888888;

  return (z0 | z1 | z2 | z3);
}

/**
 * a *= b
 */
static void mul(uint32_t *x, const uint32_t *y) {
  uint32_t hwr[4] = { rev32(x[0]), rev32(x[1]), rev32(x[2]), rev32(x[3]) };

  // We are using Karatsuba: the 128x128 multiplication is
  // reduced to three 64x64 multiplications, hence nine
  // 32x32 multiplications. With the bit-reversal trick,
  // we have to perform 18 32x32 multiplications.

  uint32_t a[18] = { 0 };

  a[0] = y[0];
  a[1] = y[1];
  a[2] = y[2];
  a[3] = y[3];
  a[4] = a[0] ^ a[1];
  a[5] = a[2] ^ a[3];
  a[6] = a[0] ^ a[2];
  a[7] = a[1] ^ a[3];
  a[8] = a[6] ^ a[7];
  a[9] = rev32(y[0]);
  a[10] = rev32(y[1]);
  a[11] = rev32(y[2]);
  a[12] = rev32(y[3]);
  a[13] = a[9] ^ a[10];
  a[14] = a[11] ^ a[12];
  a[15] = a[9] ^ a[11];
  a[16] = a[10] ^ a[12];
  a[17] = a[15] ^ a[16];

  uint32_t b[18] = { 0 };

  b[0] = x[0];
  b[1] = x[1];
  b[2] = x[2];
  b[3] = x[3];
  b[4] = b[0] ^ b[1];
  b[5] = b[2] ^ b[3];
  b[6] = b[0] ^ b[2];
  b[7] = b[1] ^ b[3];
  b[8] = b[6] ^ b[7];
  b[9] = hwr[0];
  b[10] = hwr[1];
  b[11] = hwr[2];
  b[12] = hwr[3];
  b[13] = b[9] ^ b[10];
  b[14] = b[11] ^ b[12];
  b[15] = b[9] ^ b[11];
  b[16] = b[10] ^ b[12];
  b[17] = b[15] ^ b[16];

  for (int i=0; i<18; i++) {
    a[i] = bmul32(a[i], b[i]);
  }

  a[4] ^= a[0] ^ a[1];
  a[5] ^= a[2] ^ a[3];
  a[8] ^= a[6] ^ a[7];

  a[13] ^= a[9] ^ a[10];
  a[14] ^= a[11] ^ a[12];
  a[17] ^= a[15] ^ a[16];

  b[0] = a[0];
  b[1] = a[4] ^ rev32(a[9]) >> 1;
  b[2] = a[1] ^ a[0] ^ a[2] ^ a[6] ^ rev32(a[13]) >> 1;
  b[3] = a[4] ^ a[5] ^ a[8] ^ rev32(a[10] ^ a[9] ^ a[11] ^ a[15]) >> 1;
  b[4] = a[2] ^ a[1] ^ a[3] ^ a[7] ^ rev32(a[13] ^ a[14] ^ a[17]) >> 1;
  b[5] = a[5] ^ rev32(a[11] ^ a[10] ^ a[12] ^ a[16]) >> 1;
  b[6] = a[3] ^ rev32(a[14]) >> 1;
  b[7] = rev32(a[12]) >> 1;

  for (int i=0; i<4; i++) {
    uint32_t lw = b[i];
    b[i + 4] ^= lw ^ (lw >> 1) ^ (lw >> 2) ^ (lw >> 7);
    b[i + 3] ^= (lw << 31) ^ (lw << 30) ^ (lw << 25);
  }

  x[0] = b[4];
  x[1] = b[5];
  x[2] = b[6];
  x[3] = b[7];
}

void polyval_process_block(polyval_context *ctx, const unsigned char *block) {
  // ctx->s = (ctx->s + bloc) * ctx->h
  uint32_t x[4] = {
    GET_UINT32_LE(block, 0), GET_UINT32_LE(block, 4),
    GET_UINT32_LE(block, 8), GET_UINT32_LE(block, 12)
  };
  // add
  ctx->s[0] ^= x[0];
  ctx->s[1] ^= x[1];
  ctx->s[2] ^= x[2];
  ctx->s[3] ^= x[3];
  mul(ctx->s, ctx->h);
}

void polyval_finalize(polyval_context *ctx, unsigned char *output) {
  PUT_UINT32_LE(ctx->s[0], output, 0);
  PUT_UINT32_LE(ctx->s[1], output, 4);
  PUT_UINT32_LE(ctx->s[2], output, 8);
  PUT_UINT32_LE(ctx->s[3], output, 12);
  memset((unsigned char*) ctx, 0, sizeof(polyval_context));
}

// int main() {
//   unsigned char h[16] = {0x25, 0x62, 0x93, 0x47, 0x58, 0x92, 0x42, 0x76, 0x1d, 0x31, 0xf8, 0x26, 0xba, 0x4b, 0x75, 0x7b};
//   unsigned char x1[16] = {0x4f, 0x4f, 0x95, 0x66, 0x8c, 0x83, 0xdf, 0xb6, 0x40, 0x17, 0x62, 0xbb, 0x2d, 0x01, 0xa2, 0x62};
//   unsigned char x2[16] = {0xd1, 0xa2, 0x4d, 0xdd, 0x27, 0x21, 0xd0, 0x06, 0xbb, 0xe4, 0x5f, 0x20, 0xd3, 0xc9, 0xf3, 0x62};
//
//   unsigned char result[16];
//   polyval_context ctx;
//   polyval_init(&ctx, h);
//   polyval_process_block(&ctx, x1);
//   polyval_process_block(&ctx, x2);
//   polyval_finalize(&ctx, result);
//
//   for(unsigned int i=0; i<16; i++) {
//     printf("%02x", result[i]);
//   }
//   printf("\n");
// }
