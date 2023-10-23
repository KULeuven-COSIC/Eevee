#ifndef AES_XEX_FORK_H
#define AES_XEX_FORK_H

#include "aes.h"

#include <stdint.h>

typedef struct xex_key_t
{
  uint64_t HL[16];                      /*!< Precalculated HTable low. */
  uint64_t HH[16];                      /*!< Precalculated HTable high. */
} xex_key_t;

// XEX key for a long tweak of up to two blocks and a bit
typedef struct long_xex_key_t
{
  xex_key_t key_power[2];
  unsigned char key_cubed[16];

} long_xex_key_t;

void xex_init(xex_key_t *xex_key, const unsigned char *key);
void long_xex_init(long_xex_key_t *long_xex_key, const unsigned char *key);

void fork(unsigned char* c_left0, unsigned char* c_left1,
  const unsigned char* m0, const unsigned char* m1,
  const unsigned char* tweak0, const unsigned char* tweak1,
  const rkey_t *rkeys, const xex_key_t *xex_key);

void long_fork(unsigned char* c_left0, unsigned char* c_left1,
  const unsigned char* m0, const unsigned char* m1,
  const unsigned char* tweak0, const unsigned char* tweak1,
  const rkey_t *rkeys, const long_xex_key_t *long_xex_key);

#endif // AES_XEX_FORK_H
