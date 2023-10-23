/*
 * Copyright (C) 2017 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef FORKSKINNY_C_FORKSKINNY_INTERNAL_H
#define FORKSKINNY_C_FORKSKINNY_INTERNAL_H

#include <stdint.h>
#include <string.h>

/* Figure out how to inline functions using this C compiler */
#if defined(__STDC__) && __STDC_VERSION__ >= 199901L
#define STATIC_INLINE static inline
#elif defined(__GNUC__) || defined(__clang__)
#define STATIC_INLINE static __inline__
#else
#define STATIC_INLINE static
#endif

/* Define SKINNY_64BIT to 1 if the CPU is natively 64-bit */
#if defined(__WORDSIZE) && __WORDSIZE == 64
#define SKINNY_64BIT 1
#else
#define SKINNY_64BIT 0
#endif

/* Define SKINNY_UNALIGNED to 1 if the CPU supports byte-aligned word access */
#if defined(__x86_64) || defined(__x86_64__) || \
    defined(__i386) || defined(__i386__)
#define SKINNY_UNALIGNED 1
#else
#define SKINNY_UNALIGNED 0
#endif

/* Define SKINNY_LITTLE_ENDIAN to 1 if the CPU is little-endian */
#if defined(__x86_64) || defined(__x86_64__) || \
    defined(__i386) || defined(__i386__) || \
    defined(__arm) || defined(__arm__)
#define SKINNY_LITTLE_ENDIAN 1
#else
#define SKINNY_LITTLE_ENDIAN 0
#endif

/* Define SKINNY_VEC128_MATH to 1 if we have 128-bit SIMD Vector Extensions */
#if defined(__GNUC__) || defined(__clang__)
#if defined(__SSE2__) || defined(__ARM_NEON) || \
    defined(__ARM_NEON__) || defined(__ARM_NEON_FP)
#define SKINNY_VEC128_MATH 1
#else
#define SKINNY_VEC128_MATH 0
#endif
#else
#define SKINNY_VEC128_MATH 0
#endif

/* Define SKINNY_VEC256_MATH to 1 if we have 256-bit SIMD Vector Extensions */
#if defined(__GNUC__) || defined(__clang__)
#if defined(__AVX2__)
#define SKINNY_VEC256_MATH 1
#else
#define SKINNY_VEC256_MATH 0
#endif
#else
#define SKINNY_VEC256_MATH 0
#endif

/* Attribute for declaring a vector type with this compiler */
#if defined(__clang__)
#define SKINNY_VECTOR_ATTR(words, bytes) __attribute__((ext_vector_type(words)))
#define SKINNY_VECTORU_ATTR(words, bytes) __attribute__((ext_vector_type(words), aligned(1)))
#else
#define SKINNY_VECTOR_ATTR(words, bytes) __attribute__((vector_size(bytes)))
#define SKINNY_VECTORU_ATTR(words, bytes) __attribute__((vector_size(bytes), aligned(1)))
#endif

/* XOR two blocks together of arbitrary size and alignment */
STATIC_INLINE void skinny_xor
    (void *output, const void *input1, const void *input2, size_t size)
{
    while (size > 0) {
        --size;
        ((uint8_t *)output)[size] = ((const uint8_t *)input1)[size] ^
                                    ((const uint8_t *)input2)[size];
    }
}

/* XOR two 128-bit blocks together */
STATIC_INLINE void skinny128_xor
    (void *output, const void *input1, const void *input2)
{
#if SKINNY_UNALIGNED && SKINNY_64BIT
    ((uint64_t *)output)[0] = ((const uint64_t *)input1)[0] ^
                              ((const uint64_t *)input2)[0];
    ((uint64_t *)output)[1] = ((const uint64_t *)input1)[1] ^
                              ((const uint64_t *)input2)[1];
#elif SKINNY_UNALIGNED
    ((uint32_t *)output)[0] = ((const uint32_t *)input1)[0] ^
                              ((const uint32_t *)input2)[0];
    ((uint32_t *)output)[1] = ((const uint32_t *)input1)[1] ^
                              ((const uint32_t *)input2)[1];
    ((uint32_t *)output)[2] = ((const uint32_t *)input1)[2] ^
                              ((const uint32_t *)input2)[2];
    ((uint32_t *)output)[3] = ((const uint32_t *)input1)[3] ^
                              ((const uint32_t *)input2)[3];
#else
    unsigned posn;
    for (posn = 0; posn < 16; ++posn) {
        ((uint8_t *)output)[posn] = ((const uint8_t *)input1)[posn] ^
                                    ((const uint8_t *)input2)[posn];
    }
#endif
}

/* XOR two 64-bit blocks together */
STATIC_INLINE void skinny64_xor
    (void *output, const void *input1, const void *input2)
{
#if SKINNY_UNALIGNED && SKINNY_64BIT
    ((uint64_t *)output)[0] = ((const uint64_t *)input1)[0] ^
                              ((const uint64_t *)input2)[0];
#elif SKINNY_UNALIGNED
    ((uint32_t *)output)[0] = ((const uint32_t *)input1)[0] ^
                              ((const uint32_t *)input2)[0];
    ((uint32_t *)output)[1] = ((const uint32_t *)input1)[1] ^
                              ((const uint32_t *)input2)[1];
#else
    unsigned posn;
    for (posn = 0; posn < 8; ++posn) {
        ((uint8_t *)output)[posn] = ((const uint8_t *)input1)[posn] ^
                                    ((const uint8_t *)input2)[posn];
    }
#endif
}

#define READ_BYTE(ptr,offset) \
    ((uint32_t)(((const uint8_t *)(ptr))[(offset)]))

#define READ_WORD16(ptr,offset) \
    (((uint16_t)(((const uint8_t *)(ptr))[(offset)])) | \
    (((uint16_t)(((const uint8_t *)(ptr))[(offset) + 1])) << 8))

#define READ_WORD32(ptr,offset) \
    (((uint32_t)(((const uint8_t *)(ptr))[(offset)])) | \
    (((uint32_t)(((const uint8_t *)(ptr))[(offset) + 1])) << 8) | \
    (((uint32_t)(((const uint8_t *)(ptr))[(offset) + 2])) << 16) | \
    (((uint32_t)(((const uint8_t *)(ptr))[(offset) + 3])) << 24))

#define READ_WORD64(ptr,offset) \
    (((uint64_t)(((const uint8_t *)(ptr))[(offset)])) | \
    (((uint64_t)(((const uint8_t *)(ptr))[(offset) + 1])) << 8) | \
    (((uint64_t)(((const uint8_t *)(ptr))[(offset) + 2])) << 16) | \
    (((uint64_t)(((const uint8_t *)(ptr))[(offset) + 3])) << 24) | \
    (((uint64_t)(((const uint8_t *)(ptr))[(offset) + 4])) << 32) | \
    (((uint64_t)(((const uint8_t *)(ptr))[(offset) + 5])) << 40) | \
    (((uint64_t)(((const uint8_t *)(ptr))[(offset) + 6])) << 48) | \
    (((uint64_t)(((const uint8_t *)(ptr))[(offset) + 7])) << 56))

#define WRITE_WORD16(ptr,offset,value) \
    ((((uint8_t *)(ptr))[(offset)] = (uint8_t)(value)), \
     (((uint8_t *)(ptr))[(offset) + 1] = (uint8_t)((value) >> 8)))

#define WRITE_WORD32(ptr,offset,value) \
    ((((uint8_t *)(ptr))[(offset)] = (uint8_t)(value)), \
     (((uint8_t *)(ptr))[(offset) + 1] = (uint8_t)((value) >> 8)), \
     (((uint8_t *)(ptr))[(offset) + 2] = (uint8_t)((value) >> 16)), \
     (((uint8_t *)(ptr))[(offset) + 3] = (uint8_t)((value) >> 24)))

#define WRITE_WORD64(ptr,offset,value) \
    ((((uint8_t *)(ptr))[(offset)] = (uint8_t)(value)), \
     (((uint8_t *)(ptr))[(offset) + 1] = (uint8_t)((value) >> 8)), \
     (((uint8_t *)(ptr))[(offset) + 2] = (uint8_t)((value) >> 16)), \
     (((uint8_t *)(ptr))[(offset) + 3] = (uint8_t)((value) >> 24)), \
     (((uint8_t *)(ptr))[(offset) + 4] = (uint8_t)((value) >> 32)), \
     (((uint8_t *)(ptr))[(offset) + 5] = (uint8_t)((value) >> 40)), \
     (((uint8_t *)(ptr))[(offset) + 6] = (uint8_t)((value) >> 48)), \
     (((uint8_t *)(ptr))[(offset) + 7] = (uint8_t)((value) >> 56)))


static uint8_t const RC[87] = {
    0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7e, 0x7d,
    0x7b, 0x77, 0x6f, 0x5f, 0x3e, 0x7c, 0x79, 0x73,
    0x67, 0x4f, 0x1e, 0x3d, 0x7a, 0x75, 0x6b, 0x57,
    0x2e, 0x5c, 0x38, 0x70, 0x61, 0x43, 0x06, 0x0d,
    0x1b, 0x37, 0x6e, 0x5d, 0x3a, 0x74, 0x69, 0x53,
    0x26, 0x4c, 0x18, 0x31, 0x62, 0x45, 0x0a, 0x15,
    0x2b, 0x56, 0x2c, 0x58, 0x30, 0x60, 0x41, 0x02,
    0x05, 0x0b, 0x17, 0x2f, 0x5e, 0x3c, 0x78, 0x71,
    0x63, 0x47, 0x0e, 0x1d, 0x3b, 0x76, 0x6d, 0x5b,
    0x36, 0x6c, 0x59, 0x32, 0x64, 0x49, 0x12, 0x25,
    0x4a, 0x14, 0x29, 0x52, 0x24, 0x48, 0x10
};

#endif // FORKSKINNY_C_FORKSKINNY_INTERNAL_H
