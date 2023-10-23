#if defined(__GNUC__)
#include "gcc_driver.inc"
#else
;.if 0
%include "yasm_driver.inc"
;.endif
#endif

INCLUDE_IF_SSE2_64BIT "blake2s/blake2s_sse2-64.inc"
INCLUDE_IF_SSE2_64BIT "blake2s/blake2s_ssse3-64.inc"
INCLUDE_IF_AVX_64BIT "blake2s/blake2s_avx-64.inc"
INCLUDE_IF_XOP_64BIT "blake2s/blake2s_xop-64.inc"

INCLUDE_IF_X86_32BIT "blake2s/blake2s_x86-32.inc"
INCLUDE_IF_SSE2_32BIT "blake2s/blake2s_sse2-32.inc"
INCLUDE_IF_SSSE3_32BIT "blake2s/blake2s_ssse3-32.inc"
INCLUDE_IF_AVX_32BIT "blake2s/blake2s_avx-32.inc"
INCLUDE_IF_XOP_32BIT "blake2s/blake2s_xop-32.inc"

#if defined(HAVE_ARMv6)
#include "blake2s/blake2s_armv6-32.inc"
#endif

