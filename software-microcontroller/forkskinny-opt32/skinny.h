#ifndef SKINNY_H
#define SKINNY_H

#include "internal-util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SKINNY_128_256_ROUNDS 48
#define SKINNY_64_192_ROUNDS 40


typedef struct
{
    uint32_t S[4];          /**< Current block state */

} skinny_128_256_state_t;

typedef struct
{
    /** Words of the full key schedule */
    uint32_t row0[SKINNY_128_256_ROUNDS];
    uint32_t row1[SKINNY_128_256_ROUNDS];

} skinny_128_256_tweakey_schedule_t;

void skinny_128_256_init_tks(skinny_128_256_tweakey_schedule_t *tks1, skinny_128_256_tweakey_schedule_t *tks2, const unsigned char key[32], uint8_t nb_rounds);

void skinny_128_256_init_tk1(skinny_128_256_tweakey_schedule_t *tks, const unsigned char key[16], uint8_t nb_rounds);
void skinny_128_256_init_tk2(skinny_128_256_tweakey_schedule_t *tks, const unsigned char key[16], uint8_t nb_rounds);

void skinny_128_256_rounds(skinny_128_256_state_t *state, const skinny_128_256_tweakey_schedule_t *tks1, const skinny_128_256_tweakey_schedule_t *tks2, unsigned first, unsigned last);

void skinny_128_256_encrypt(const unsigned char key[32], unsigned char *output, const unsigned char *input);
void skinny_128_256_encrypt_with_tks(const skinny_128_256_tweakey_schedule_t *tks1, const skinny_128_256_tweakey_schedule_t *tks2, unsigned char *output, const unsigned char *input);

typedef struct
{
    uint16_t S[4];      /**< Current block state */

} skinny_64_192_state_t;

typedef struct
{
    /** Words of the full key schedule */
    uint16_t row0[SKINNY_64_192_ROUNDS];
    uint16_t row1[SKINNY_64_192_ROUNDS];


} skinny_64_192_tweakey_schedule_t;

void skinny_64_192_init_tks(skinny_64_192_tweakey_schedule_t *tks, const unsigned char key[24], uint8_t nb_rounds);
void skinny_64_192_rounds(skinny_64_192_state_t *state, const skinny_64_192_tweakey_schedule_t *tks, unsigned first, unsigned last);
void skinny_64_192_encrypt(const unsigned char key[24], unsigned char *output, const unsigned char *input);

#ifdef __cplusplus
}
#endif

#endif // SKINNY_H
