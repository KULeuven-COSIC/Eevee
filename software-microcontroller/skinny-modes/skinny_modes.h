#ifndef SKINNY_MODES_H
#define SKINNY_MODES_H

#define SKINNY_MODES_128_256_KEY_LEN 16
#define SKINNY_MODES_128_256_NONCE_LEN 16
#define SKINNY_MODES_128_256_BLOCK_LEN 16


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
);

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
);

#endif // SKINNY_MODES_H
