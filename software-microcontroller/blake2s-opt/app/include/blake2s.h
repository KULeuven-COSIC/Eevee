#ifndef BLAKE2S_H
#define BLAKE2S_H

#include <stddef.h>

#if !defined(LIB_PUBLIC)
#define LIB_PUBLIC
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct blake2s_state_t {
	unsigned char opaque[128];
} blake2s_state;

/* incremental */
LIB_PUBLIC void blake2s_init(blake2s_state *S);
LIB_PUBLIC void blake2s_keyed_init(blake2s_state *S, const unsigned char *key, size_t keylen);
LIB_PUBLIC void blake2s_update(blake2s_state *S, const unsigned char *in, size_t inlen);
LIB_PUBLIC void blake2s_final(blake2s_state *S, unsigned char *hash);

/* one-shot */
LIB_PUBLIC void blake2s(unsigned char *hash, const unsigned char *in, size_t inlen);
LIB_PUBLIC void blake2s_keyed(unsigned char *hash, const unsigned char *in, size_t inlen, const unsigned char *key, size_t keylen);

LIB_PUBLIC int blake2s_startup(void);

#if defined(UTILITIES)
void blake2s_fuzz(void);
void blake2s_bench(void);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* BLAKE2S_H */

