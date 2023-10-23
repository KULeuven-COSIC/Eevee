#ifndef GHASH_H
#define GHASH_H

#include <stdint.h>

/**
 * Extracted from mbedtls/library/gcm.c
 */

typedef struct ghash_context
{
    uint64_t HL[16];                      /*!< Precalculated HTable low. */
    uint64_t HH[16];                      /*!< Precalculated HTable high. */
    unsigned char buf[16];                /*!< The buf working value. */
}
ghash_context;

void ghash_init(ghash_context *ctx, const unsigned char *key);
void ghash_process_block(ghash_context *ctx, const unsigned char *block);
void ghash_finalize(ghash_context *ctx, unsigned char *out);

#endif
