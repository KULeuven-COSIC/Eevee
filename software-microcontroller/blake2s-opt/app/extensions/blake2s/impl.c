#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpuid.h"
#include "blake2s.h"

enum blake2s_constants {
	BLAKE2S_BLOCKBYTES = 64,
	BLAKE2S_STRIDE = BLAKE2S_BLOCKBYTES,
	BLAKE2S_STRIDE_NONE = 0,
	BLAKE2S_HASHBYTES = 32,
	BLAKE2S_KEYBYTES = 32,
};

typedef struct blake2s_state_internal_t {
	unsigned char h[32];
	unsigned char t[8];
	unsigned char f[8];
	size_t leftover;
	unsigned char buffer[BLAKE2S_BLOCKBYTES];
} blake2s_state_internal;

typedef struct blake2s_impl_t {
	unsigned long cpu_flags;
	const char *desc;

	void (*blake2s_blocks)(blake2s_state_internal *state, const unsigned char *in, size_t bytes, size_t stride);
} blake2s_impl_t;

#define BLAKE2S_DECLARE(ext) \
	void blake2s_blocks_##ext(blake2s_state_internal *state, const unsigned char *in, size_t bytes, size_t stride);

#define BLAKE2S_IMPL(cpuflags, desc, ext) \
	{(cpuflags), desc, blake2s_blocks_##ext}

#if defined(ARCH_X86)
	/* 32 bit only implementations */
	#if defined(CPU_32BITS)
		BLAKE2S_DECLARE(x86)
		#define BLAKE2S_X86 BLAKE2S_IMPL(CPUID_X86, "x86", x86)
	#endif

	/* 64 bit only implementations */
	#if defined(CPU_64BITS)
	#endif

	/* both 32 and 64 bits */
	#if defined(HAVE_SSE2)
		BLAKE2S_DECLARE(sse2)
		#define BLAKE2S_SSE2 BLAKE2S_IMPL(CPUID_X86, "sse2", sse2)
	#endif

	#if defined(HAVE_SSSE3)
		BLAKE2S_DECLARE(ssse3)
		#define BLAKE2S_SSSE3 BLAKE2S_IMPL(CPUID_X86, "ssse3", ssse3)
	#endif


	#if defined(HAVE_AVX)
		BLAKE2S_DECLARE(avx)
		#define BLAKE2S_AVX BLAKE2S_IMPL(CPUID_AVX, "avx", avx)
	#endif

	#if defined(HAVE_XOP)
		BLAKE2S_DECLARE(xop)
		#define BLAKE2S_XOP BLAKE2S_IMPL(CPUID_XOP, "xop", xop)
	#endif
#endif

#if defined(ARCH_ARM)
	#if defined(HAVE_ARMv6)
		BLAKE2S_DECLARE(armv6)
		#define BLAKE2S_ARMv6 BLAKE2S_IMPL(CPUID_ARMv6, "armv6", armv6)
	#endif
#endif

/* the "always runs" version */
#if defined(HAVE_INT32)
	#define BLAKE2S_GENERIC BLAKE2S_IMPL(CPUID_GENERIC, "generic/32", ref)
	#include "blake2s/blake2s_ref-32.inc"
#elif defined(HAVE_INT16)
	#define BLAKE2S_GENERIC BLAKE2S_IMPL(CPUID_GENERIC, "generic/16", ref)
	#include "blake2s/blake2s_ref-16.inc"
#else
	#define BLAKE2S_GENERIC BLAKE2S_IMPL(CPUID_GENERIC, "generic/8", ref)
	#include "blake2s/blake2s_ref-8.inc"
#endif

/* list implemenations from most optimized to least, with generic as the last entry */
static const blake2s_impl_t blake2s_list[] = {
	/* x86 */
	#if defined(BLAKE2S_AVX2)
		BLAKE2S_AVX2,
	#endif
	#if defined(BLAKE2S_XOP)
		BLAKE2S_XOP,
	#endif
	#if defined(BLAKE2S_AVX)
		BLAKE2S_AVX,
	#endif
	#if defined(BLAKE2S_SSSE3)
		BLAKE2S_SSSE3,
	#endif
	#if defined(BLAKE2S_SSE2)
		BLAKE2S_SSE2,
	#endif
	#if defined(BLAKE2S_X86)
		BLAKE2S_X86,
	#endif

	/* arm */
	#if defined(BLAKE2S_ARMv6)
		BLAKE2S_ARMv6,
	#endif

	BLAKE2S_GENERIC
};

BLAKE2S_DECLARE(bootup)

static const blake2s_impl_t blake2s_bootup_impl = {
	CPUID_GENERIC,
	NULL,
	blake2s_blocks_bootup
};

static const blake2s_impl_t *blake2s_opt = &blake2s_bootup_impl;


/* is the pointer not aligned on a word boundary? */
static int
blake2s_not_aligned(const void *p) {
#if !defined(CPU_8BITS)
	return ((size_t)p & (sizeof(size_t) - 1)) != 0;
#else
	return 0;
#endif
}

static const union endian_test_t {
	unsigned char b[2];
	unsigned short s;
} blake2s_endian_test = {{1, 0}};

/* copy the hash from the internal state */
static void
blake2s_store_hash(blake2s_state_internal *state, unsigned char *hash) {
	if (blake2s_endian_test.s == 0x0001) {
		memcpy(hash, state->h, BLAKE2S_HASHBYTES);
	} else {
		size_t i, j;
		for (i = 0; i < 8; i++, hash += 4) {
			for (j = 0; j < 4; j++)
				hash[3 - j] = state->h[(i * 4) + j];
		}
	}
}

/*
	Blake2s initialization constants for serial mode:

	0x6a09e667 ^ 0x01010020
	0xbb67ae85
	0x3c6ef372
	0xa54ff53a
	0x510e527f
	0x9b05688c
	0x1f83d9ab
	0x5be0cd19
*/

static const unsigned char blake2s_init_le[32] = {
	0x67^0x20,0xe6^0x00,0x09^0x01,0x6a^0x01,
	0x85,0xae,0x67,0xbb,
	0x72,0xf3,0x6e,0x3c,
	0x3a,0xf5,0x4f,0xa5,
	0x7f,0x52,0x0e,0x51,
	0x8c,0x68,0x05,0x9b,
	0xab,0xd9,0x83,0x1f,
	0x19,0xcd,0xe0,0x5b
};

/* initialize the state in serial mode */
LIB_PUBLIC void
blake2s_init(blake2s_state *S) {
	blake2s_state_internal *state = (blake2s_state_internal *)S;
	/* assume state is fully little endian for now */
	memcpy(state, blake2s_init_le, sizeof(blake2s_init_le));
	memset(state->t, 0, sizeof(state->t) + sizeof(state->f) + sizeof(state->leftover));
}

/* initialized the state in serial-key'd mode */
LIB_PUBLIC void
blake2s_keyed_init(blake2s_state *S, const unsigned char *key, size_t keylen) {
	unsigned char k[BLAKE2S_BLOCKBYTES] = {0};
	if (keylen > BLAKE2S_KEYBYTES) {
		fprintf(stderr, "key size larger than %u passed to blake2s_keyed_init", BLAKE2S_KEYBYTES);
		exit(-1);
	} else {
		memcpy(k, key, keylen);
	}
	blake2s_init(S);
	blake2s_update(S, k, BLAKE2S_BLOCKBYTES);
}

/* hash inlen bytes from in, which may or may not be word aligned, returns the number of bytes used */
static size_t
blake2s_consume_blocks(blake2s_state_internal *state, const unsigned char *in, size_t inlen) {
	/* always need to leave at least BLAKE2S_BLOCKBYTES in case this is the final block */
	if (inlen <= BLAKE2S_BLOCKBYTES)
		return 0;

	inlen = ((inlen - 1) & ~(BLAKE2S_BLOCKBYTES - 1));
	if (blake2s_not_aligned(in)) {
		/* copy the unaligned data to an aligned buffer and process in chunks */
		unsigned char buffer[16 * BLAKE2S_BLOCKBYTES];
		size_t left = inlen;
		while (left) {
			const size_t bytes = (left > sizeof(buffer)) ? sizeof(buffer) : left;
			memcpy(buffer, in, bytes);
			blake2s_opt->blake2s_blocks(state, buffer, bytes, BLAKE2S_STRIDE);
			in += bytes;
			left -= bytes;
		}
	} else {
		/* word aligned, handle directly */
		blake2s_opt->blake2s_blocks(state, in, inlen, BLAKE2S_STRIDE);
	}

	return inlen;
}

/* update the hash state with inlen bytes from in */
LIB_PUBLIC void
blake2s_update(blake2s_state *S, const unsigned char *in, size_t inlen) {
	blake2s_state_internal *state = (blake2s_state_internal *)S;
	size_t bytes;

	/* blake2s processes the final <=BLOCKBYTES bytes raw, so we can only update if there are at least BLOCKBYTES+1 bytes available */
	if ((state->leftover + inlen) > BLAKE2S_BLOCKBYTES) {
		/* handle the previous data, we know there is enough for at least one block */
		if (state->leftover) {
			bytes = (BLAKE2S_BLOCKBYTES - state->leftover);
			memcpy(state->buffer + state->leftover, in, bytes);
			in += bytes;
			inlen -= bytes;
			state->leftover = 0;
			blake2s_opt->blake2s_blocks(state, state->buffer, BLAKE2S_BLOCKBYTES, BLAKE2S_STRIDE_NONE);
		}

		/* handle the direct data (if any) */
		bytes = blake2s_consume_blocks(state, in, inlen);
		inlen -= bytes;
		in += bytes;
	}

	/* handle leftover data */
	memcpy(state->buffer + state->leftover, in, inlen);
	state->leftover += inlen;
}

/* finalize the hash */
LIB_PUBLIC void
blake2s_final(blake2s_state *S, unsigned char *hash) {
	blake2s_state_internal *state = (blake2s_state_internal *)S;
	memset(&state->f[0], 0xff, 4);
	blake2s_opt->blake2s_blocks(state, state->buffer, state->leftover, BLAKE2S_STRIDE_NONE);
	blake2s_store_hash(state, hash);
}

/* one-shot hash inlen bytes from in */
LIB_PUBLIC void
blake2s(unsigned char *hash, const unsigned char *in, size_t inlen) {
	blake2s_state S;
	blake2s_state_internal *state = (blake2s_state_internal *)&S;
	size_t bytes;

	blake2s_init(&S);

	/* hash until <= 64 bytes left */
	bytes = blake2s_consume_blocks(state, in, inlen);
	in += bytes;
	inlen -= bytes;

	/* final block */
	memset(&state->f[0], 0xff, 4);
	blake2s_opt->blake2s_blocks(state, in, inlen, BLAKE2S_STRIDE_NONE);
	blake2s_store_hash(state, hash);
}

LIB_PUBLIC void
blake2s_keyed(unsigned char *hash, const unsigned char *in, size_t inlen, const unsigned char *key, size_t keylen) {
	blake2s_state S;
	blake2s_keyed_init(&S, key, keylen);
	blake2s_update(&S, in, inlen);
	blake2s_final(&S, hash);
}



/* initialize the state in serial mode, setting the counter to 0xffffffff */
static void
blake2s_init_test(blake2s_state *S) {
	blake2s_state_internal *state = (blake2s_state_internal *)S;
	/*memcpy(state, (blake2s_endian_test.s == 1) ? blake2s_init_le : blake2s_init_be, 64);*/
	memcpy(state, blake2s_init_le, sizeof(blake2s_init_le));
	memset(state->t, 0xff, sizeof(state->t));
	memset(state->f, 0x00, sizeof(state->f));
	state->leftover = 0;
}

/* hashes with the counter starting at 0xffffffff */
static void
blake2s_test(unsigned char *hash, const unsigned char *in, size_t inlen) {
	blake2s_state S;
	blake2s_state_internal *state = (blake2s_state_internal *)&S;
	size_t bytes;

	blake2s_init_test(&S);

	/* hash until <= 64 bytes left */
	bytes = blake2s_consume_blocks(state, in, inlen);
	in += bytes;
	inlen -= bytes;

	/* final block */
	memset(&state->f[0], 0xff, 4);
	blake2s_opt->blake2s_blocks(state, in, inlen, BLAKE2S_STRIDE_NONE);
	blake2s_store_hash(state, hash);
}

/* hash the hashes of [],[0],[0,1],[0,1,2]..[0,1,..255] with the counter starting at 0xffffffff */
static int
blake2s_test_impl(const void *impl) {
	static const unsigned char expected[BLAKE2S_HASHBYTES] = {
		0x41,0xab,0xaf,0x0b,0x80,0x27,0x72,0x63,0xbb,0x7c,0xef,0x59,0xe7,0x9e,0x1c,0x39,
		0x4f,0x25,0x36,0x3d,0xf8,0x28,0xf0,0xa0,0x0a,0x9f,0x93,0x11,0xab,0xfe,0xbf,0x34
	};
	static size_t len = 256;
	blake2s_state st;
	unsigned char buf[256], hash[BLAKE2S_HASHBYTES];
	size_t i;

	blake2s_opt = (blake2s_impl_t *)impl;

	for(i = 0; i < len; i++)
		buf[i] = (unsigned char)i;

	blake2s_init_test(&st);
	for(i = 0; i <= len; i++) {
		blake2s_test(hash, buf, i);
		//printf("%u %02x%02x%02x%02x\n", (unsigned int)i, hash[0], hash[1], hash[2], hash[3]);
		blake2s_update(&st, hash, BLAKE2S_HASHBYTES);
	}
	blake2s_final(&st, hash);
	return memcmp(expected, hash, BLAKE2S_HASHBYTES);
}

LIB_PUBLIC int
blake2s_startup(void) {
	const void *opt = LOCAL_PREFIX(cpu_select)(blake2s_list, sizeof(blake2s_impl_t), blake2s_test_impl);
	if (opt) {
		blake2s_opt = (const blake2s_impl_t *)opt;
		return 0;
	} else {
		return 1;
	}
}

void
blake2s_blocks_bootup(blake2s_state_internal *state, const unsigned char *in, size_t bytes, size_t stride) {
	if (blake2s_startup() == 0) {
		blake2s_opt->blake2s_blocks(state, in, bytes, stride);
	} else {
		fprintf(stderr, "blake2s failed to startup\n");
		exit(1);
	}
}


#if defined(UTILITIES)

#include <stdio.h>
#include <string.h>
#include "fuzz.h"
#include "bench.h"

static const fuzz_variable_t fuzz_inputs[] = {
	{"state", FUZZ_ARRAY, 32},
	{"counter", FUZZ_ARRAY, 8},
	{"input", FUZZ_RANDOM_LENGTH_ARRAY0, 2048},
	{0, FUZZ_DONE, 0}
};

static const fuzz_variable_t fuzz_outputs[] = {
	{"hash", FUZZ_ARRAY, BLAKE2S_HASHBYTES},
	{0, FUZZ_DONE, 0}
};


/* process the input with the given implementation and write it to the output */
static void
blake2s_fuzz_impl(const void *impl, const unsigned char *in, const size_t *random_sizes, unsigned char *out) {
	const unsigned char *state = in;
	const unsigned char *counter = in + 32;
	const unsigned char *input = in + 40;
	blake2s_state st;

	/* set the current implementation to impl */
	blake2s_opt = (const blake2s_impl_t *)impl;

	/* use random data for the state and counter */
	memset(&st, 0, sizeof(st));
	memcpy(((blake2s_state_internal *)&st)->h, state, 32);
	memcpy(((blake2s_state_internal *)&st)->t, counter, 8);

	blake2s_update(&st, input, random_sizes[0]);
	blake2s_final(&st, out);
}

/* run the fuzzer on blake2s */
void
blake2s_fuzz(void) {
	fuzz_init();
	fuzz(blake2s_list, sizeof(blake2s_impl_t), fuzz_inputs, fuzz_outputs, blake2s_fuzz_impl);
}



static unsigned char *bench_arr = NULL;
static unsigned char bench_hash[BLAKE2S_HASHBYTES];
static size_t bench_len = 0;

static void
blake2s_bench_impl(const void *impl) {
	blake2s_opt = (const blake2s_impl_t *)impl;
	blake2s(bench_hash, bench_arr, bench_len);
}

void
blake2s_bench(void) {
	static const size_t lengths[] = {1, 128, 576, 8192, 0};
	size_t i;
	bench_arr = bench_get_buffer();

	for (i = 0; lengths[i]; i++) {
		bench_len = lengths[i];
		bench(blake2s_list, sizeof(blake2s_impl_t), blake2s_test_impl, blake2s_bench_impl, bench_len, "byte");
	}

}

#endif /* defined(UTILITIES) */
