# ABOUT #

This is a portable, performant implementation of [BLAKE2s](https://blake2.net/) using optimized block compression functions. The compression functions are tree/parallel mode compatible, although only serial mode (singled threaded, the common use-case) is currently implemented.

BLAKE2s is a 256 bit hash, i.e. the hashes produced are 32 bytes long.

All assembler is PIC safe.

# INITIALIZING #

The library can be initialized, i.e. the most optimized implementation that passes internal tests will be automatically selected, in two ways, **neither of which are thread safe**:

1. `int blake2s_startup(void);` explicitly initializes the library, and returns a non-zero value if no suitable implementation is found that passes internal tests

2. Do nothing and use the library like normal. It will auto-initialize itself when needed, and hard exit if no suitable implementation is found.

# CALLING #

Common assumptions:

* When using the incremental functions, the `blake2s_state` struct is assumed to be word aligned, if necessary, for the system in use.

## ONE SHOT ##

`in` is assumed to be word aligned. Incremental support has no alignment requirements, but will obviously slow down if non word-aligned pointers are passed.

`void blake2s(unsigned char *hash, const unsigned char *in, const size_t inlen);`

Hashes `inlen` bytes from `in` and stores the result in `hash`.

`void blake2s_keyed(unsigned char *hash, const unsigned char *in, const size_t inlen, const unsigned char *key, size_t keylen);`

Hashes `inlen` bytes from `in` in keyed mode using `key`, and and stores the result in `hash`. `keylen` must be <= 32.

## INCREMENTAL ##

Incremental `in` buffers are *not* required to be word aligned. Unaligned buffers will require copying to aligned buffers however, which will obviously incur a speed penalty.

`void blake2s_init(blake2s_state *S)`

Initializes `S` to the default state.

`void blake2s_keyed_init(blake2s_state *S, const unsigned char *key, size_t keylen)`

Initializes `S` in keyed mode with `key`. `keylen` must be <= 32.

`void blake2s_update(blake2s_state *S, const unsigned char *in, size_t inlen)`

Updates the state `S` with `inlen` bytes from `in` in.

`void blake2s_final(blake2s_state *S, unsigned char *hash)`

Performs the final pass on state `S` and stores the result in to `hash`.

# Examples #

## HASHING DATA WITH ONE CALL ##

    size_t bytes = ...;
    unsigned char data[...] = {...};
    unsigned char hash[32];

    blake2s(hash, data, bytes);

## HASHING INCREMENTALLY ##

Hashing incrementally, i.e. with multiple calls to update the hash state. 

    size_t bytes = ...;
    unsigned char data[...] = {...};
    unsigned char hash[32];
    blake2s_state state;
    size_t i;

    blake2s_init(&state);
    /* add one byte at a time, extremely inefficient */
    for (i = 0; i < bytes; i++) {
        blake2s_update(&state, data + i, 1);
    }
    blake2s_final(&state, hash);

# VERSIONS #

## Reference ##

There are 3 reference versions, specialized for increasingly capable systems from 8 bit only operations (with the world's most inefficient portable carries, you really don't want to use this unless nothing else runs) to unrolled 32 bit.

* Generic 8-bit: [blake2s\_ref](app/extensions/blake2s/blake2s_ref-8.inc)
* Generic 16-bit: [blake2s\_ref](app/extensions/blake2s/blake2s_ref-16.inc)
* Generic 32-bit: [blake2s\_ref](app/extensions/blake2s/blake2s_ref-32.inc)

## x86 (32 bit) ##

* 386 compatible: [blake2s\_x86](app/extensions/blake2s/blake2s_x86-32.inc)
* SSE2: [blake2s\_sse2](app/extensions/blake2s/blake2s_sse2-32.inc)
* SSSE3: [blake2s\_ssse3](app/extensions/blake2s/blake2s_ssse3-32.inc)
* AVX: [blake2s\_avx](app/extensions/blake2s/blake2s_avx-32.inc)
* XOP: [blake2s\_xop](app/extensions/blake2s/blake2s_xop-32.inc)

## x86-64 ##

* SSE2: [blake2s\_sse2](app/extensions/blake2s/blake2s_sse2-64.inc)
* SSSE3: [blake2s\_ssse3](app/extensions/blake2s/blake2s_ssse3-64.inc)
* AVX: [blake2s\_avx](app/extensions/blake2s/blake2s_avx-64.inc)
* XOP: [blake2s\_xop](app/extensions/blake2s/blake2s_xop-64.inc)
* AVX2: [blake2s\_avx2](app/extensions/blake2s/blake2s_avx2-64.inc)

From what I've seen, the x86-64 compatible version is usually slower than the optimized SIMD version for that platform, so it is not included.

## ARM ##

* ARMv6: [blake2s\_armv6](app/extensions/blake2s/blake2s_armv6-32.inc)

I attempted a NEON version, but it is almost entirely serial NEON, and the NEON latencies were too high to overcome the ARMv6 version.

# BUILDING #

See [asm-opt#configuring](https://github.com/floodyberry/asm-opt#configuring) for full configure options.

If you would like to use Yasm with a gcc-compatible compiler, pass `--yasm` to configure.

The Visual Studio projects are generated assuming Yasm is available. You will need to have [Yasm.exe](http://yasm.tortall.net/Download.html) somewhere in your path to build them.

## STATIC LIBRARY ##

    ./configure
    make lib

and `make install-lib` OR copy `bin/blake2s.lib` and `app/include/blake2s.h` to your desired location.

## SHARED LIBRARY ##

    ./configure --pic
    make shared
    make install-shared

## UTILITIES / TESTING ##

    ./configure
    make util
    bin/chacha-util [bench|fuzz]

### BENCHMARK / TESTING ###

Benchmarking will implicitly test every available version. If any fail, it will exit with an error indicating which versions did not pass. Features tested include:

* One-shot hashing
* Incremental hashing
* Counter handling when the 32-bit low half overflows to the upper half

### FUZZING ###

Fuzzing tests every available implementation for the current CPU against the reference implementation. Features tested are:

* Arbitrary starting state
* Arbitrary starting counter

# BENCHMARKS #

## [E5200](http://ark.intel.com/products/37212/) ##

Only the top 3 benchmarks per mode will be shown. Anything past 3 or so is pretty irrelevant to the current architecture.

<table>
<thead><tr><th>Implemenation</th><th>1 byte</th><th>576 bytes</th><th>8192 bytes</th></tr></thead>
<tbody>
<tr> <td>SSSE3-64  </td> <td> 433</td> <td>  6.10</td> <td>  5.91</td> </tr>
<tr> <td>SSSE3-32  </td> <td> 500</td> <td>  6.27</td> <td>  5.89</td> </tr>
<tr> <td>SSE2-64   </td> <td> 505</td> <td>  7.18</td> <td>  7.04</td> </tr>
<tr> <td>SSE2-32   </td> <td> 575</td> <td>  7.42</td> <td>  7.05</td> </tr>
<tr> <td>x86-32    </td> <td> 754</td> <td> 10.15</td> <td>  9.87</td> </tr>
</tbody>
</table>

## [i7-4770K](http://ark.intel.com/products/75123) ##

Timings are with Turbo Boost and Hyperthreading, so their accuracy is not concrete. 
For reference, OpenSSL and Crypto++ give ~0.8cpb for AES-128-CTR and ~1.1cpb for AES-256-CTR, ~7.4cpb for SHA-512, and ~4.5cpb for MD5.

<table>
<thead><tr><th>Implemenation</th><th>1 byte</th><th>576 bytes</th><th>8192 bytes</th></tr></thead>
<tbody>
<tr> <td>AVX-64    </td> <td> 355</td> <td>  5.10</td> <td>  5.02</td> </tr>
<tr> <td>SSSE3-64  </td> <td> 356</td> <td>  5.10</td> <td>  5.03</td> </tr>
<tr> <td>SSSE3-32  </td> <td> 384</td> <td>  5.17</td> <td>  5.05</td> </tr>
<tr> <td>AVX-32    </td> <td> 390</td> <td>  5.24</td> <td>  5.11</td> </tr>
<tr> <td>SSE2-64   </td> <td> 411</td> <td>  5.92</td> <td>  5.88</td> </tr>
<tr> <td>SSE2-32   </td> <td> 437</td> <td>  6.03</td> <td>  5.91</td> </tr>
</tbody>
</table>

## AMD FX-8120 ##

Timings are with Turbo on, so accuracy is not concrete. I'm not sure how to adjust for it either, 
and depending on clock speed (3.1ghz vs 4.0ghz), OpenSSL gives between 0.73cpb - 0.94cpb for AES-128-CTR, 
1.03cpb - 1.33cpb for AES-256-CTR, 10.96cpb - 14.1cpb for SHA-512, and 4.7cpb - 5.16cpb for MD5.

<table>
<thead><tr><th>Implemenation</th><th>1 byte</th><th>576 bytes</th><th>8192 bytes</th></tr></thead>
<tbody>
<tr> <td>XOP-64   </td> <td> 512</td> <td>  6.80</td> <td>  6.61</td> </tr>
<tr> <td>XOP-32   </td> <td> 523</td> <td>  6.90</td> <td>  6.66</td> </tr>
<tr> <td>AVX-64   </td> <td> 611</td> <td>  8.68</td> <td>  8.54</td> </tr>
<tr> <td>SSSE3-64 </td> <td> 604</td> <td>  8.69</td> <td>  8.56</td> </tr>
<tr> <td>SSSE3-32 </td> <td> 646</td> <td>  8.86</td> <td>  8.59</td> </tr>
<tr> <td>AVX-32   </td> <td> 664</td> <td>  9.03</td> <td>  8.80</td> </tr>

</tbody>
</table>


## ZedBoard (Cortex-A9) ##

I don't have access to the cycle counter yet, so cycles are computed by taking the microseconds times the clock speed (666mhz) divided by 1 million. For comparison, on long messages, OpenSSL 1.0.0e gives 52.3 cpb for AES-128-CBC (woof), ~123cpb for SHA-512 (really woof), ~49.11cpb for SHA-256, ~16.38 for SHA-1, and ~9.6cpb for MD5.

<table>
<thead><tr><th>Implemenation</th><th>1 byte</th><th>576 bytes</th><th>8192 bytes</th></tr></thead>
<tbody>
<tr> <td>ARMv6-32         </td> <td> 1014</td> <td> 13.26</td> <td> 12.87</td> </tr>
<tr> <td>Generic-32       </td> <td> 1806</td> <td> 22.06</td> <td> 21.20</td> </tr>
</tbody>
</table>

# LICENSE #

Public Domain, or MIT
