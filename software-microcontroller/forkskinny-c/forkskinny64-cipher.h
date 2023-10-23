#ifndef FORKSKINNY_C_FORKSKINNY64_CIPHER_H
#define FORKSKINNY_C_FORKSKINNY64_CIPHER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FORKSKINNY64_BLOCK_SIZE 8

#define FORKSKINNY_64_192_ROUNDS_BEFORE 17

#define FORKSKINNY_64_192_ROUNDS_AFTER 23

#define FORKSKINNY64_MAX_ROUNDS (FORKSKINNY_64_192_ROUNDS_BEFORE + 2*FORKSKINNY_64_192_ROUNDS_AFTER)

/**
 * Union that describes a 64-bit 4x4 array of cells.
 */
typedef union
{
    uint16_t row[4];        /**< Cell rows in 16-bit units */
    uint32_t lrow[2];       /**< Cell rows in 32-bit units */
    uint64_t llrow;         /**< Cell rows in 64-bit units */

} ForkSkinny64Cells_t;

/**
 * Union that describes a 32-bit 2x4 array of cells.
 */
typedef union
{
    uint16_t row[2];        /**< Cell rows in 16-bit units */
    uint32_t lrow;          /**< Cell rows in 32-bit units */

} ForkSkinny64HalfCells_t;

/**
 * Key schedule for Forkskinny-64-192
 */
typedef struct
{
    /** All words of the key schedule */
    ForkSkinny64HalfCells_t schedule[FORKSKINNY64_MAX_ROUNDS];

} ForkSkinny64Key_t;

/**
 * Pre-computes the key schedule for Forkskinny-64-192 for TK1
 * ks:
 * key:       pointer to key bytes; reads FORKSKINNY64_BLOCK_SIZE bytes from the key
 * nb_rounds: the number of rounds of the key schedule
 */
void forkskinny_c_64_192_init_tk1(ForkSkinny64Key_t *ks, const uint8_t *key, unsigned nb_rounds);
/**
 * Pre-computes the key schedule for Forkskinny-64-192 for TK2 AND TK3
 * ks:
 * key:       pointer to key bytes; reads 2*FORKSKINNY64_BLOCK_SIZE bytes from the key
 * nb_rounds: the number of rounds of the key schedule
 */
void forkskinny_c_64_192_init_tk2_tk3(ForkSkinny64Key_t *ks, const uint8_t *key, unsigned nb_rounds);

/**
 * Computes the forward direction of Forkskinny-64-192.
 * tks1:           key schedule for TK1 (see forkskinny_c_64_192_init_tk1)
 * tks2:           key schedule for TK2 and TK3 (see forkskinny_c_64_192_init_tk2_tk3)
 * output_left:   if NULL, the left leg is not computed, else pointer to FORKSKINNY64_BLOCK_SIZE byte; will contain the left output leg of the forkcipher
 * output_right:  pointer to FORKSKINNY64_BLOCK_SIZE byte; will contain the right output leg of the forkcipher
 * input:         pointer to FORKSKINNY64_BLOCK_SIZE byte; input to the forkcipher
 */
void forkskinny_c_64_192_encrypt(const ForkSkinny64Key_t *tks1, const ForkSkinny64Key_t *tks2,
  uint8_t *output_left, uint8_t *output_right, const uint8_t *input_right);

/**
 * Computes the inverse direction of Forkskinny-64-192.
 * tks1:           key schedule for TK1 (see forkskinny_c_64_192_init_tk1)
 * tks2:           key schedule for TK2 and TK3 (see forkskinny_c_64_192_init_tk2_tk3)
 * output_left:   if NULL, the left leg is not computed, else pointer to FORKSKINNY64_BLOCK_SIZE byte; will contain the left output leg of the forkcipher (i.e. mode 'o')
 * output_right:  pointer to FORKSKINNY64_BLOCK_SIZE byte; will contain the inverted input of the forkcipher (i.e. mode 'i')
 * input_right:   pointer to FORKSKINNY64_BLOCK_SIZE byte; input to the inverse forkcipher
 */
void forkskinny_c_64_192_decrypt(const ForkSkinny64Key_t *tks1, const ForkSkinny64Key_t *tks2,
  uint8_t *output_left, uint8_t *output_right, const uint8_t *input_right);

#ifdef __cplusplus
}
#endif

#endif // FORKSKINNY_C_FORKSKINNY64_CIPHER_H
