#ifndef FORKSKINNY_C_FORKSKINNY128_CIPHER_H
#define FORKSKINNY_C_FORKSKINNY128_CIPHER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


#define FORKSKINNY128_BLOCK_SIZE 16

#define FORKSKINNY_128_256_ROUNDS_BEFORE 21

#define FORKSKINNY_128_256_ROUNDS_AFTER 27

#define FORKSKINNY_128_384_ROUNDS_BEFORE 25

#define FORKSKINNY_128_384_ROUNDS_AFTER 31

#define FORKSKINNY128_MAX_ROUNDS (FORKSKINNY_128_384_ROUNDS_BEFORE + 2*FORKSKINNY_128_384_ROUNDS_AFTER)

/**
 * Union that describes a 128-bit 4x4 array of cells.
 */
typedef union
{
    uint32_t row[4];        /**< Cell rows in 32-bit units */
    uint64_t lrow[2];       /**< Cell rows in 64-bit units */

} ForkSkinny128Cells_t;

/**
 * Union that describes a 64-bit 2x4 array of cells.
 */
typedef union
{
    uint32_t row[2];        /**< Cell rows in 32-bit units */
    uint64_t lrow;          /**< Cell rows in 64-bit units */

} ForkSkinny128HalfCells_t;

/**
 * Key schedule for Forkskinny-128-256 and Forkskinny-128-384
 */
typedef struct
{
    /** All words of the key schedule */
    ForkSkinny128HalfCells_t schedule[FORKSKINNY128_MAX_ROUNDS];

} ForkSkinny128Key_t;

/**
 * Pre-computes the key schedule for Forkskinny-128-256 for TK1
 * ks:
 * key:       pointer to key bytes; reads FORKSKINNY128_BLOCK_SIZE bytes from the key
 * nb_rounds: the number of rounds of the key schedule
 */
void forkskinny_c_128_256_init_tk1(ForkSkinny128Key_t *ks, const uint8_t *key, unsigned nb_rounds);
/**
 * Pre-computes the key schedule for Forkskinny-128-256 for TK2
 * ks:
 * key:       pointer to key bytes; reads FORKSKINNY128_BLOCK_SIZE bytes from the key
 * nb_rounds: the number of rounds of the key schedule
 */
void forkskinny_c_128_256_init_tk2(ForkSkinny128Key_t *ks, const uint8_t *key, unsigned nb_rounds);

/**
 * Pre-computes the key schedule for Forkskinny-128-384 for TK1
 * ks:
 * key:       pointer to key bytes; reads FORKSKINNY128_BLOCK_SIZE bytes from the key
 * nb_rounds: the number of rounds of the key schedule
 */
void forkskinny_c_128_384_init_tk1(ForkSkinny128Key_t *ks, const uint8_t *key, unsigned nb_rounds);
/**
 * Pre-computes the key schedule for Forkskinny-128-384 for TK2
 * ks:
 * key:       pointer to key bytes; reads FORKSKINNY128_BLOCK_SIZE bytes from the key
 * nb_rounds: the number of rounds of the key schedule
 */
void forkskinny_c_128_384_init_tk2(ForkSkinny128Key_t *ks, const uint8_t *key, unsigned nb_rounds);
/**
 * Pre-computes the key schedule for Forkskinny-128-384 for TK3
 * ks:
 * key:       pointer to key bytes; reads FORKSKINNY128_BLOCK_SIZE bytes from the key
 * nb_rounds: the number of rounds of the key schedule
 */
void forkskinny_c_128_384_init_tk3(ForkSkinny128Key_t *ks, const uint8_t *key, unsigned nb_rounds);

/**
 * Computes the forward direction of Forkskinny-128-256.
 * ks1:           key schedule for TK1 (see forkskinny_c_128_256_init_tk1)
 * ks2:           key schedule for TK2 (see forkskinny_c_128_256_init_tk2)
 * output_left:   if NULL, the left leg is not computed, else pointer to FORKSKINNY128_BLOCK_SIZE byte; will contain the left output leg of the forkcipher
 * output_right:  pointer to FORKSKINNY128_BLOCK_SIZE byte; will contain the right output leg of the forkcipher
 * input:         pointer to FORKSKINNY128_BLOCK_SIZE byte; input to the forkcipher
 */
void forkskinny_c_128_256_encrypt(const ForkSkinny128Key_t *ks1, const ForkSkinny128Key_t *ks2, uint8_t *output_left, uint8_t *output_right, const uint8_t *input);

/**
 * Computes the inverse direction of Forkskinny-128-256.
 * ks1:           key schedule for TK1 (see forkskinny_c_128_256_init_tk1)
 * ks2:           key schedule for TK2 (see forkskinny_c_128_256_init_tk2)
 * output_left:   if NULL, the left leg is not computed, else pointer to FORKSKINNY128_BLOCK_SIZE byte; will contain the left output leg of the forkcipher (i.e. mode 'o')
 * output_right:  pointer to FORKSKINNY128_BLOCK_SIZE byte; will contain the inverted input of the forkcipher (i.e. mode 'i')
 * input_right:   pointer to FORKSKINNY128_BLOCK_SIZE byte; input to the inverse forkcipher
 */
void forkskinny_c_128_256_decrypt(const ForkSkinny128Key_t *ks1, const ForkSkinny128Key_t *ks2, uint8_t *output_left, uint8_t *output_right, const uint8_t *input_right);

/**
 * Computes the forward direction of Forkskinny-128-384.
 * ks1:           key schedule for TK1 (see forkskinny_c_128_384_init_tk1)
 * ks2:           key schedule for TK2 (see forkskinny_c_128_384_init_tk2)
 * ks3:           key schedule for TK3 (see forkskinny_c_128_384_init_tk3)
 * output_left:   if NULL, the left leg is not computed, else pointer to FORKSKINNY128_BLOCK_SIZE byte; will contain the left output leg of the forkcipher
 * output_right:  pointer to FORKSKINNY128_BLOCK_SIZE byte; will contain the right output leg of the forkcipher
 * input:         pointer to FORKSKINNY128_BLOCK_SIZE byte; input to the forkcipher
 */
void forkskinny_c_128_384_encrypt(const ForkSkinny128Key_t *ks1, const ForkSkinny128Key_t *ks2, const ForkSkinny128Key_t *ks3, uint8_t *output_left, uint8_t *output_right, const uint8_t *input);

/**
 * Computes the inverse direction of Forkskinny-128-384.
 * ks1:           key schedule for TK1 (see forkskinny_c_128_384_init_tk1)
 * ks2:           key schedule for TK2 (see forkskinny_c_128_384_init_tk2)
 * ks3:           key schedule for TK3 (see forkskinny_c_128_384_init_tk3)
 * output_left:   if NULL, the left leg is not computed, else pointer to FORKSKINNY128_BLOCK_SIZE byte; will contain the left output leg of the forkcipher (i.e. mode 'o')
 * output_right:  pointer to FORKSKINNY128_BLOCK_SIZE byte; will contain the inverted input of the forkcipher (i.e. mode 'i')
 * input_right:   pointer to FORKSKINNY128_BLOCK_SIZE byte; input to the inverse forkcipher
 */
void forkskinny_c_128_384_decrypt(const ForkSkinny128Key_t *ks1, const ForkSkinny128Key_t *ks2, const ForkSkinny128Key_t *ks3, uint8_t *output_left, uint8_t *output_right, const uint8_t *input_right);
#ifdef __cplusplus
}
#endif

#endif // FORKSKINNY_C_FORKSKINNY128_CIPHER_H
