#include "jolteon_aes.h"
#include "aes_xex_fork.h"
#include "aes.h"

#include <string.h>

#define JOLTEON_AES_BLOCKSIZE 16
#define JOLTEON_AES_NONCESIZE 14

int jolteon_aes_128_encrypt(
  /** ciphertext destination buffer */
  unsigned char *c,
  /** tag destination buffer */
  unsigned char *tag,
  /** message and message length */
  const unsigned char *m,unsigned long long mlen,
  /** associated data and AD length */
  __attribute__ ((unused)) const unsigned char *ad, __attribute__ ((unused)) unsigned long long adlen,
  /** nonce **/
  const unsigned char *npub,
  /** key **/
  const unsigned char *k
) {
  xex_key_t xex_key;
  xex_init(&xex_key, k+JOLTEON_AES_BLOCKSIZE);

  long long full_blocks = mlen/JOLTEON_AES_BLOCKSIZE;
  if ((mlen % JOLTEON_AES_BLOCKSIZE) != 0) {
    full_blocks++;
  }

  const unsigned char *m_in = m;
  unsigned char *c_out = c;
  unsigned char tweak0[JOLTEON_AES_BLOCKSIZE];
  unsigned char tweak1[JOLTEON_AES_BLOCKSIZE];
  memcpy(tweak0, npub, JOLTEON_AES_NONCESIZE);
  memcpy(tweak1, npub, JOLTEON_AES_NONCESIZE);

  rkey_t rkeys;
  aes128_keyschedule_ffs(&rkeys, k);

  unsigned char t_a[JOLTEON_AES_BLOCKSIZE];
  memset(t_a, 0, JOLTEON_AES_BLOCKSIZE);

  uint16_t cnt = 2;
  for(long long i=0; i<full_blocks-1; i += 2) {
    if(i != full_blocks-2) {
      // process two full message blocks
      tweak0[JOLTEON_AES_NONCESIZE] = 0x40 | ((cnt >> 8) & 0x3f);
      tweak0[JOLTEON_AES_NONCESIZE+1] = cnt & 0xff;
      cnt++;
      tweak1[JOLTEON_AES_NONCESIZE] = 0xc0 | ((cnt >> 8) & 0x3f);
      tweak1[JOLTEON_AES_NONCESIZE+1] = cnt & 0xff;
      fork(c_out, c_out+JOLTEON_AES_BLOCKSIZE, m_in, m_in+JOLTEON_AES_BLOCKSIZE, tweak0, tweak1, &rkeys, &xex_key);
      // add to t_a
      for(int j=0; j<JOLTEON_AES_BLOCKSIZE; j++) {
        t_a[j] ^= m_in[j] ^ m_in[JOLTEON_AES_BLOCKSIZE+j] ^ c_out[j] ^ c_out[JOLTEON_AES_BLOCKSIZE+j];
      }
      m_in += 2*JOLTEON_AES_BLOCKSIZE;
      c_out += 2*JOLTEON_AES_BLOCKSIZE;
    }else{
      // process one message block (before the last)
      tweak0[JOLTEON_AES_NONCESIZE] = 0x40 | ((cnt >> 8) & 0x3f);
      tweak0[JOLTEON_AES_NONCESIZE+1] = cnt & 0xff;
      fork(c_out, NULL, m_in, NULL, tweak0, NULL, &rkeys, &xex_key);
      // add to t_a
      for(int j=0; j<JOLTEON_AES_BLOCKSIZE; j++) { t_a[j] ^= m_in[j] ^ c_out[j]; }
      m_in += JOLTEON_AES_BLOCKSIZE;
      c_out += JOLTEON_AES_BLOCKSIZE;
    }
    cnt++;
  }

  int m_star_len = (mlen % JOLTEON_AES_BLOCKSIZE == 0) ? JOLTEON_AES_BLOCKSIZE : mlen % JOLTEON_AES_BLOCKSIZE;
  if(mlen > 0) {
    for(int j=0; j<m_star_len; j++) {
      t_a[j] ^= m_in[j];
    }
  }
  tweak0[JOLTEON_AES_NONCESIZE] = 0x40;
  tweak0[JOLTEON_AES_NONCESIZE+1] = 0;
  tweak1[JOLTEON_AES_NONCESIZE] = 0xc0;
  tweak1[JOLTEON_AES_NONCESIZE+1] = 0;
  unsigned char c_star[JOLTEON_AES_BLOCKSIZE];
  fork(tag, c_star, t_a, t_a, tweak0, tweak1, &rkeys, &xex_key);
  if (mlen == 0) { return 0; }
  // write out c_star
  for(int j=0; j<m_star_len; j++) {
    c_out[j] = c_star[j] ^ t_a[j];
  }
  return 0;
}
