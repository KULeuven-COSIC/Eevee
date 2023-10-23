#include "skinny_modes.h"

#include <string.h>
#include <libblake2s/blake2s.h>

#if defined(EEVEE_FORKSKINNY_BACKEND_OPT32)
#include "../forkskinny-opt32/skinny.h"
#define SKINNY_MODES_TK_T skinny_128_256_tweakey_schedule_t
#define SKINNY_MODES_INIT_TK1(tks, key) skinny_128_256_init_tk1(tks, key, SKINNY_128_256_ROUNDS)
#define SKINNY_MODES_INIT_TK2(tks, key) skinny_128_256_init_tk2(tks, key, SKINNY_128_256_ROUNDS)
#define SKINNY_MODES_ENCRYPT(tk1, tk2, output, input) skinny_128_256_encrypt_with_tks(tk1, tk2, output, input)
#elif defined(EEVEE_FORKSKINNY_BACKEND_C)
#include "../skinny-c/include/skinny128-cipher.h"
#define SKINNY_MODES_TK_T Skinny128Key_t
#define SKINNY_MODES_INIT_TK1(tks, key) skinny128_init_tk1(tks, key)
#define SKINNY_MODES_INIT_TK2(tks, key) skinny128_init_tk2(tks, key)
#define SKINNY_MODES_ENCRYPT(tk1, tk2, output, input) skinny128_ecb_encrypt_with_tks(output, input, tk1, tk2)
#else
#error Unknown SKINNY backend
#endif

static void skinny_128_256_ctr(
  /** ciphertext destination buffer */
  unsigned char *c,
  /** message and message length */
  const unsigned char *m,unsigned long long mlen,
  /** nonce **/
  const unsigned char *npub,
  /** key **/
  const SKINNY_MODES_TK_T *tk1, // <- preloaded with key
  SKINNY_MODES_TK_T *tk2,
  unsigned char tweakey[2*SKINNY_MODES_128_256_KEY_LEN]
) {

  memset(tweakey + SKINNY_MODES_128_256_KEY_LEN, 0, SKINNY_MODES_128_256_KEY_LEN);
  unsigned long long ctr = 0;
  for(; ctr < mlen/SKINNY_MODES_128_256_BLOCK_LEN; ctr++, c += SKINNY_MODES_128_256_BLOCK_LEN, m += SKINNY_MODES_128_256_BLOCK_LEN) {
    // set ctr in tweak
    memcpy(tweakey + SKINNY_MODES_128_256_KEY_LEN, &ctr, sizeof(unsigned long long));
    SKINNY_MODES_INIT_TK2(tk2, tweakey + SKINNY_MODES_128_256_KEY_LEN);
    SKINNY_MODES_ENCRYPT(tk1, tk2, c, npub);
    for(int i=0; i<SKINNY_MODES_128_256_BLOCK_LEN; i++) {
      c[i] ^= m[i];
    }
  }

  if(mlen % SKINNY_MODES_128_256_BLOCK_LEN != 0) {
    unsigned char output[SKINNY_MODES_128_256_BLOCK_LEN];
    // set ctr in tweak
    memcpy(tweakey + SKINNY_MODES_128_256_KEY_LEN, &ctr, sizeof(unsigned long long));
    SKINNY_MODES_INIT_TK2(tk2, tweakey + SKINNY_MODES_128_256_KEY_LEN);
    SKINNY_MODES_ENCRYPT(tk1, tk2, output, npub);
    for(unsigned int i=0; i<(mlen % SKINNY_MODES_128_256_BLOCK_LEN); i++) {
      c[i] = output[i] ^ m[i];
    }
  }
}

int htmac_skinny_128_256_encrypt(
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
  unsigned char tweakey[2*SKINNY_MODES_128_256_KEY_LEN];
  SKINNY_MODES_TK_T tk1, tk2;
  memcpy(tweakey, k, SKINNY_MODES_128_256_KEY_LEN);
  SKINNY_MODES_INIT_TK1(&tk1, tweakey);
  skinny_128_256_ctr(c, m, mlen, npub, &tk1, &tk2, tweakey);

  // hash AD and ciphertext
  blake2s_state blake;
  blake2s_keyed_init(&blake, npub, SKINNY_MODES_128_256_NONCE_LEN);
  if(adlen > 0) {
    blake2s_update(&blake, ad, adlen);
  }
  if(mlen > 0) {
    blake2s_update(&blake, c, mlen);
  }
  unsigned char hash[32];
  blake2s_final(&blake, hash);
  memcpy(tag, hash, SKINNY_MODES_128_256_BLOCK_LEN);
  return 0;
}

static void pmac_skinny_128_256_consume(
  unsigned char *state,
  const SKINNY_MODES_TK_T *tk1,
  SKINNY_MODES_TK_T *tk2,
  unsigned char tweakey[2*SKINNY_MODES_128_256_KEY_LEN],
  const unsigned char *buf, unsigned long long len,
  unsigned char domain1,
  unsigned char domain2
) {
  memset(tweakey + SKINNY_MODES_128_256_KEY_LEN, 0x0, SKINNY_MODES_128_256_KEY_LEN);
  tweakey[SKINNY_MODES_128_256_KEY_LEN] = domain1;
  unsigned long long ctr = 1;
  unsigned long long last_block = (len % SKINNY_MODES_128_256_BLOCK_LEN == 0) ? len/SKINNY_MODES_128_256_BLOCK_LEN-1 : len/SKINNY_MODES_128_256_BLOCK_LEN;

  unsigned char output[SKINNY_MODES_128_256_BLOCK_LEN];
  for(; ctr <= last_block; ctr++) {
    // set ctr in tweak
    memcpy(tweakey + SKINNY_MODES_128_256_KEY_LEN + 1, &ctr, sizeof(unsigned long long));
    unsigned long long index = (ctr-1) * SKINNY_MODES_128_256_BLOCK_LEN;
    SKINNY_MODES_INIT_TK2(tk2, tweakey + SKINNY_MODES_128_256_KEY_LEN);
    SKINNY_MODES_ENCRYPT(tk1, tk2, output, buf + index);
    for(int i=0; i<SKINNY_MODES_128_256_BLOCK_LEN; i++) {
      state[i] ^= output[i];
    }
  }
  // last block
  unsigned char input[SKINNY_MODES_128_256_BLOCK_LEN];
  unsigned long long last_index = (len % SKINNY_MODES_128_256_BLOCK_LEN == 0) ? SKINNY_MODES_128_256_BLOCK_LEN : len % SKINNY_MODES_128_256_BLOCK_LEN;
  memcpy(input, buf + (ctr-1) * SKINNY_MODES_128_256_BLOCK_LEN, last_index);
  // padding
  if(last_index < SKINNY_MODES_128_256_BLOCK_LEN) {
    input[last_index] = 0x80;
    last_index++;
    for(; last_index < SKINNY_MODES_128_256_BLOCK_LEN; last_index++)
      input[last_index] = 0;
  }
  tweakey[SKINNY_MODES_128_256_KEY_LEN] = domain2;
  // set ctr in tweak
  memcpy(tweakey + SKINNY_MODES_128_256_KEY_LEN + 1, &ctr, sizeof(unsigned long long));
  SKINNY_MODES_INIT_TK2(tk2, tweakey + SKINNY_MODES_128_256_KEY_LEN);
  SKINNY_MODES_ENCRYPT(tk1, tk2, output, input);
  // move tag out
  for(int i=0; i<SKINNY_MODES_128_256_BLOCK_LEN; i++) {
    state[i] ^= output[i];
  }
}

static void pmac_skinny_128_256(
  /** input buffer and input length **/
  const unsigned char *c, unsigned long long clen,
  /** associated data and AD length */
  const unsigned char *ad, unsigned long long adlen,
  /** nonce **/
  const unsigned char *npub,
  /** tag destination buffer **/
  unsigned char *tag,
  /** tweakey with the first 16 byte key set **/
  unsigned char *tweakey,
  /** TK states **/
  SKINNY_MODES_TK_T *tk1, SKINNY_MODES_TK_T *tk2
) {
  memset(tweakey + SKINNY_MODES_128_256_KEY_LEN, 0x0, SKINNY_MODES_128_256_KEY_LEN);
  unsigned char state[SKINNY_MODES_128_256_BLOCK_LEN];
  memset(state, 0, SKINNY_MODES_128_256_BLOCK_LEN);
  if(adlen > 0) {
    pmac_skinny_128_256_consume(state, tk1, tk2, tweakey, ad, adlen, 0x1, 0x2);
  }
  if(clen > 0) {
    pmac_skinny_128_256_consume(state, tk1, tk2, tweakey, c, clen, 0x3, 0x4);
  }

  // XOR nonce
  for(int i=0; i<SKINNY_MODES_128_256_BLOCK_LEN; i++)
    state[i] ^= npub[i];
  tweakey[SKINNY_MODES_128_256_KEY_LEN] = 0x5;
  memset(tweakey + 1 + SKINNY_MODES_128_256_KEY_LEN, 0xff, SKINNY_MODES_128_256_KEY_LEN-1);
  SKINNY_MODES_INIT_TK2(tk2, tweakey + SKINNY_MODES_128_256_KEY_LEN);
  SKINNY_MODES_ENCRYPT(tk1, tk2, tag, state);
}

int pmac_skinny_128_256_encrypt(
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
  unsigned char tweakey[2*SKINNY_MODES_128_256_KEY_LEN];
  SKINNY_MODES_TK_T tk1, tk2;
  memcpy(tweakey, k, SKINNY_MODES_128_256_KEY_LEN);
  SKINNY_MODES_INIT_TK1(&tk1, tweakey);
  skinny_128_256_ctr(c, m, mlen, npub, &tk1, &tk2, tweakey);

  pmac_skinny_128_256(c, mlen, ad, adlen, npub, tag, tweakey, &tk1, &tk2);

  // memset(tweakey + SKINNY_MODES_128_256_KEY_LEN, 0x0, SKINNY_MODES_128_256_KEY_LEN);
  // unsigned char state[SKINNY_MODES_128_256_BLOCK_LEN];
  // memset(state, 0, SKINNY_MODES_128_256_BLOCK_LEN);
  // if(adlen > 0) {
  //   pmac_skinny_128_256_consume(state, &tk1, &tk2, tweakey, ad, adlen, 0x1, 0x2);
  // }
  // if(mlen > 0) {
  //   pmac_skinny_128_256_consume(state, &tk1, &tk2, tweakey, c, mlen, 0x3, 0x4);
  // }
  //
  // // XOR nonce
  // for(int i=0; i<SKINNY_MODES_128_256_BLOCK_LEN; i++)
  //   state[i] ^= npub[i];
  // tweakey[SKINNY_MODES_128_256_KEY_LEN] = 0x5;
  // memset(tweakey + 1 + SKINNY_MODES_128_256_KEY_LEN, 0xff, SKINNY_MODES_128_256_KEY_LEN-1);
  // SKINNY_MODES_INIT_TK2(&tk2, tweakey + SKINNY_MODES_128_256_KEY_LEN);
  // SKINNY_MODES_ENCRYPT(&tk1, &tk2, tag, state);
  return 0;
}
