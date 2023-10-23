#ifndef POLYVAL_H
#define POLYVAL_H

#include <stdint.h>

// adapted from https://github.com/RustCrypto/universal-hashes/blob/master/polyval/src/backend/soft32.rs

typedef struct polyval_context
{
    uint32_t h[4];
    uint32_t s[4];
}
polyval_context;

void polyval_init(polyval_context *ctx, const unsigned char *key);
void polyval_process_block(polyval_context *ctx, const unsigned char *block);
void polyval_finalize(polyval_context *ctx, unsigned char *output);

#endif
