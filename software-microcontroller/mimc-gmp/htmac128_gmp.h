#ifndef MIMC_GMP_HTMAC128_GMP_H
#define MIMC_GMP_HTMAC128_GMP_H

int htmac_mimc_128_crypto_aead_encrypt(
       unsigned char *c,unsigned long long clen,
       const unsigned char *m,unsigned long long mlen,
       const unsigned char *ad,unsigned long long adlen,
       const unsigned char *npub,
       const unsigned char *k
);

int htmac_mimc_128_crypto_aead_decrypt(
       unsigned char *m,unsigned long long *mlen,
       const unsigned char *c,unsigned long long clen,
       const unsigned char *ad,unsigned long long adlen,
       const unsigned char *npub,
       const unsigned char *k
);

#endif // MIMC_GMP_HTMAC128_GMP_H
