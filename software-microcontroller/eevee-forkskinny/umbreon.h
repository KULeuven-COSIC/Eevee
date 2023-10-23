

#ifndef UMBREON_H
#define UMBREON_H

int umbreon_forkskinny_64_192_encrypt(
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

int umbreon_forkskinny_64_192_decrypt(
    /** message destination buffer */
    unsigned char *m,
    /** ciphertext and length */
    const unsigned char *c,unsigned long long clen,
    /** tag **/
    const unsigned char *tag,
    /** associated data and AD length */
    const unsigned char *ad,unsigned long long adlen,
    /** nonce **/
    const unsigned char *npub,
    /** key **/
    const unsigned char *k
    );

int umbreon_forkskinny_128_256_encrypt(
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

int umbreon_forkskinny_128_256_decrypt(
    /** message destination buffer */
    unsigned char *m,
    /** ciphertext and length */
    const unsigned char *c,unsigned long long clen,
    /** tag **/
    const unsigned char *tag,
    /** associated data and AD length */
    const unsigned char *ad,unsigned long long adlen,
    /** nonce **/
    const unsigned char *npub,
    /** key **/
    const unsigned char *k
    );

#endif // UMBREON_H
