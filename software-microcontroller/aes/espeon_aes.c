#include "espeon_aes.h"

#include "aes_xex_fork.h"
#include "aes.h"
#include <string.h>

#define ESPEON_AES_BLOCKSIZE 16
#define ESPEON_AES_NONCESIZE 16

int espeon_aes_128_encrypt(
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
  long_xex_key_t xex_key;
  long_xex_init(&xex_key, k+ESPEON_AES_BLOCKSIZE);

  unsigned char initial_tweak[32];
  memcpy(initial_tweak + ESPEON_AES_NONCESIZE, npub, ESPEON_AES_NONCESIZE);
  memset(initial_tweak, 0, 15);
  initial_tweak[15] = 0x4 << 2;

  unsigned char t_a[ESPEON_AES_NONCESIZE];
  memset(t_a, 0, ESPEON_AES_NONCESIZE);

  rkey_t rkeys;
  aes128_keyschedule_ffs(&rkeys, k);

  // message
  if(mlen == 0) {
      // output T_a as tag
      memcpy(tag, t_a, ESPEON_AES_NONCESIZE);
      return 0;
  }

  if(mlen <= ESPEON_AES_BLOCKSIZE) {
    int remaining_blocks = (mlen % ESPEON_AES_BLOCKSIZE == 0) ? ESPEON_AES_BLOCKSIZE : (mlen % ESPEON_AES_BLOCKSIZE);
    for(int i=0; i<remaining_blocks; i++) { t_a[i] ^= m[i]; }
    // fork
    initial_tweak[15] = 0;
    unsigned char c_star[ESPEON_AES_BLOCKSIZE];
    long_fork(tag, c_star, t_a, t_a, initial_tweak, initial_tweak, &rkeys, &xex_key);
    for(int i=0; i<remaining_blocks; i++) { c[i] = c_star[i] ^ m[i]; }
    return 0;
  }

  const unsigned char *input = t_a;
  for(int i=0; i < ESPEON_AES_BLOCKSIZE; i++) { t_a[i] ^= m[i]; }
  const unsigned char *tweak = initial_tweak;
  unsigned char *output = c;
  unsigned long long n_full_blocks = mlen/ESPEON_AES_BLOCKSIZE;
  if(mlen % ESPEON_AES_BLOCKSIZE == 0) {
    n_full_blocks -=1 ;
  }

  for(unsigned long long i=0; i < n_full_blocks; i++) {
    if(i == 1) {
      memcpy(initial_tweak + ESPEON_AES_BLOCKSIZE, output, ESPEON_AES_BLOCKSIZE);
    }else{
      tweak = output - 2*ESPEON_AES_BLOCKSIZE;
    }
    long_fork(output, NULL, input, NULL, tweak, NULL, &rkeys, &xex_key);
    if(i != 0) {
      for(int j=0; j<ESPEON_AES_BLOCKSIZE; j++) { t_a[j] ^= input[j]; }
    }else{
      input = m;
    }
    output += ESPEON_AES_BLOCKSIZE;
    input += ESPEON_AES_BLOCKSIZE;
  }

  int remaining_blocks = (mlen % ESPEON_AES_BLOCKSIZE == 0) ? ESPEON_AES_BLOCKSIZE : (mlen % ESPEON_AES_BLOCKSIZE);
  for(int i=0; i<remaining_blocks; i++) { t_a[i] ^= input[i]; }
  // fork
  if(n_full_blocks == 1) {
    memcpy(initial_tweak + ESPEON_AES_BLOCKSIZE, output, ESPEON_AES_BLOCKSIZE);
  }else if(n_full_blocks == 2) {
    tweak = output - 2*ESPEON_AES_BLOCKSIZE;
  }
  unsigned char c_star[ESPEON_AES_BLOCKSIZE];
  long_fork(tag, c_star, t_a, t_a, tweak, tweak, &rkeys, &xex_key);
  for(int i=0; i<remaining_blocks; i++) { output[i] = c_star[i] ^ input[i]; }
  return 0;
}
