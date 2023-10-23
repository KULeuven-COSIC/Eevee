#ifndef AES_H
#define AES_H

// adapted from https://github.com/aadomn/aes/

#include <stdint.h>

#ifndef AES_ASSEMBLY
typedef struct rkey_t
{
  uint32_t rkeys[88];
} rkey_t;
#else
typedef struct rkey_t
{
	uint8_t rkeys[11*16];
} rkey_t;
#endif

/* Fully-fixsliced encryption functions */
/**
 * ctext0: 16 byte buffer
 * ctext1: 16 byte buffer
 * ptext0: 16 byte buffer
 * ptext1: 16 byte buffer
 * rkeys: buffer of 88 uint32_t
 */
void aes128_encrypt_ffs(unsigned char *ctext0, unsigned char *ctext1,
				const unsigned char *ptext0, const unsigned char *ptext1,
				const rkey_t *rkeys);
/* Fully-fixsliced key schedule functions */
/**
 * rkeys: buffer of 88 uint32_t
 * key0: 16 byte buffer
 * key1: 16 byte buffer
 */
void aes128_keyschedule_ffs(rkey_t *rkeys, const unsigned char *key);

#ifdef AES_ASSEMBLY

extern void AES_128_keyschedule(const uint8_t *, uint8_t *);
extern void AES_128_encrypt(const uint8_t *, const uint8_t *, uint8_t *);
#endif
#endif
