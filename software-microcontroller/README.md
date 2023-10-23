## IoT Benchmark
This folder contains all benchmark code for the Eevee paper run the IoT device (ARM Cortex M4).
The experiment/benchmark measures the number of cycles to encrypt a message of given length with a given mode of operation.
It is set up as follows (where we use `umbreon_forkskinny_64_192` as example mode)
  1. The binary for the primitive is built.
  2. The binary for the primitive is flashed to the microcontroller (here: `umbreon_benchmark_64_192.bin`). Flashing depends on the exact model and manufacturer of the M4. We used [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html).
  3. Test vectors are generated using `./generate.x umbreon_forkskinny_64_192 100 1000 > vectors_umbreon_forkskinny_64_192` (here: creates 1000 vectors of 100 byte messages).
  4. The microcontroller is connected to the PC, and the experiment script is started `python3 run-experiment.py--samples vectors_umbreon_forkskinny_64_192 -o cycles_umbreon_forkskinny_64_192`.
     The script synchronizes with the microcontroller, sends a key, nonce and message to encrypt and receives the ciphertext, tag and cyclecount. The script checks that the ciphertext/tag is correct and writes the cyclecount into the output file `cycles_umbreon_forkskinny_64_192`. This is repeated for all vectors in the input file. (Note that the device mount point `/dev/ttyUSB0` may differ depending on the setup. This must be changed in `run-experiment.py`.)

The process described above can be repeated for different message lengths and modes of operation. In the following, we describe how the board is set up, how to compile the benchmark code and more.

### Board Setup
We use the STM32F407G-DISC1 board that runs the benchmark code. The timing results
(cycles) are communicated back to the PC via a Serial-to-USB converter (CP2102N-MINI-EK Rev 2.0).

The following pins are connected:

| Cortex-M4 (STM32F407G-DISC1) | Serial-to-USB (CP2102N-MINI-EK Rev 2.0) |
|------------------------------|-----------------------------------------|
| GND                          | GND                                     |
| PA2                          | RXD                                     |
| PA3                          | TXD                                     |

The Serial-to-USB converter is then connected via USB to the PC. If the pins of the microcontroller are different, this change has to be reflected in the benchmark source code, e.g., `umbreon_benchmark_64_192.c`.

### Build
Prerequisites
  - GCC ARM toolchain (e.g., `arm-none-eabi-gcc` and related packages)
  - libopencm3: Make sure the submodule is pulled (`git submodule update --init --recursive`), then `cd libopencm3 && make`

Code
  - To build `generate.x` on the host PC, run `make generate.x`
  - To build `*.bin` on the host PC (cross-compile), run `make -f cortex-m4.mk all`.
  You may need to run `make -f cortex-m4.mk clean` and compile again if the linker reports `file not recognized: file format not recognized`.

### Executables
- `generate.x` outputs test vectors for a selected mode/cipher of a given length.
	These are sent to the device with `run-experiment.py`.
	Example usage
	```
	$> ./generate.x
	./generate.x [--check] primitive (mlen samples)+
		--check:	 check if encryption produces a valid ciphertext
		supported primitives:
			umbreon_forkskinny_64_192
			umbreon_forkskinny_128_256
			jolteon_forkskinny_64_192
			jolteon_forkskinny_128_256
			espeon_forkskinny_128_256
			espeon_forkskinny_128_384
			htmac_skinny_128_256
			pmac_skinny_128_256
			htmac_mimc_128
			ppmac_mimc_128
			skinny_c_64_192
			skinny_c_128_256
			skinny_fk_64_192
			skinny_fk_128_256
			forkskinny_c_64_192
			forkskinny_c_128_256
			forkskinny_64_192
			forkskinny_128_256
			aes_gcm_siv_128
			aes_gcm_128
			jolteon_aes_128
			espeon_aes_128
	
		mlen samples: the number of testvectors to generate for each message length mlen
	```
	I.e., `./generate.x --check jolteon_forkskinny_64_192 55 123` outputs 123 samples of 55-byte long message/ciphertext pairs, encrypted by Jolteon-Forkskinny-64-192, in the following format: `<key> <nonce> <message> <ciphertext> <tag>` where bytes are encoded in hexadecimal. One sample per line. Note that `--check` is not implemented for all primitives.

- `run-experiment.py` Given a file with test vectors in the format of `generate.x`, sends a test vector (key, nonce, message) to the connected M4, receives ciphertext, tag and the cycle count. The script compares the received ciphertext and tag with the test vector to check for correctness and saves the cyclecount.
	```
	$> python run-experiment.py -h
	usage: run-experiment.py [-h] --samples SAMPLEPATH -o OUTPUT
	                         [--cyclecounts N_CYCLECOUNTS]
	Run experiment on connected microcontroller
	options:
	  -h, --help            show this help message and exit
	  --samples SAMPLEPATH  Path to file containing testvector samples
	  -o OUTPUT             Path to output file
	  --cyclecounts N_CYCLECOUNTS
	                        Nr of expected cycle counts returned by the
	                        controller. Defaults to 1
	```
	I.e., `python run-exeriment.py --samples results/jolteon_fk_64_192_m8 -o jolteon_fk_64_192_m8_cycles` reads the test vectors in `results/jolteon_fk_64_192_m8`, sends them to the device and writes the reported cycle count in `jolteon_fk_64_192_m8_cycles` (line by line).

- `*.bin` are executables that can be flashed to the M4 to run the indicated mode, e.g., `jolteon_benchmark_64_192.bin` runs the benchmark that encrypts the received key, nonce and message with Jolteon-Forkskinny-64-192.

### Implementations
- Primitives
  - **SKINNY-128-256** (constant time)  in two variants
    1. from [SKINNY-C](https://github.com/rweather/skinny-c) by Rhys Weatherley in `skinny-c/*`
    2. adapted from Arne Depreze's [ForkAE](https://github.com/ArneDeprez1/ForkAE-SW) implementation in `forkskinny-opt32/skinny.h`
  - **Forkskinny-64-192**, **Forkskinny-128-256** and **Forkskinny-128-384** (all constant time) in two variants:
    1. adapted from [ForkAE](https://github.com/ArneDeprez1/ForkAE-SW) in `forkskinny-opt32/internal-forkskinny.h`
    2. adapted from [SKINNY-C](https://github.com/rweather/skinny-c) in `forkskinny-c/forkskinny64-cipher.h, forkskinny-c/forkskinny128-cipher.h`
  - **MiMC-128** [MiMC](https://eprint.iacr.org/2016/492) instantiated with a 128-bit prime `0x800000000000000000000000001b8001` in `mimc-gmp/mimc128_gmp.h, mimc-gmp/mimc128_gmp.c` using GMP
  - **AES** (constant time) fully-fixedsliced implementation from [Alexandre Adomnicai and Thomas Peyrin](https://github.com/aadomn/aes/) in `aes/aes.h, aes/aes.c` and a constant time M4 assembly version by [Peter Schwabe and Ko Stoffelen](https://github.com/Ko-/aes-armcortexm) also in `aes/aes.s`
  - **GHASH** (constant time) from [Mbed-TLS](https://github.com/Mbed-TLS/mbedtls) (mbedtls/library/gcm.c) in `aes/ghash.h, aes/ghash.c`
  - **Polyval** (constant time) as defined in RFC8452 ported from [RustCrypto](https://github.com/RustCrypto/universal-hashes/blob/master/polyval/src/backend/soft32.rs) in `aes/polyval.h, aes/polyval.c`
  - **"ForkAES"** a trivial instantiation of a forkcipher from two parallel tweakable block ciphers in `aes/aes_xex_fork.h, aes/aes_xex_fork.c`
- Modes
  - **Umbreon** with instantiations `Umbreon[Forkskinny-64-192]` and `Umbreon[Forkskinny-128-256]` in `eevee/umbreon.h, eevee/umbreon.c`
  - **Espeon** with instantiations `Espeon[Forkskinny-128-256]` and `Espeon[Forkskinny-128-384]` in `eevee/espeon.h, eevee/espeon.c` and `Espeon[AES]` in `aes/espeon_aes.h, aes/espeon_aes.c`
  - **Jolteon** with instantiations `Jolteon[Forkskinny-64-192]` and `Jolteon[Forkskinny-128-256]` in `eevee/jolteon.h, eevee/jolteon.c` and `Jolteon[AES]` in `aes/jolteon_aes.h`, `aes/jolteon_aes.c`
  - **CTR-then-Hash-then-MAC** with instantiations
    - `SKINNY-128-256` as SIV-like counter in tweak and `blake2s` as hash function in `skinny-modes/skinny_modes.h, skinny-modes/skinny_modes.c`
    - `MiMC-128` as block cipher in [CTR-HtMAC](https://eprint.iacr.org/2017/496) and `blake2s` as hash function in `mimc-gmp/htmac128_gmp.h, mimc-gmp/htmac128_gmp.c`
  - **CTR-then-PMAC1** with instantiations
    - `SKINNY-128-256` in `skinny-modes/skinny_modes.h, skinny-modes/skinny_modes.c`
    - `MiMC-128` in `mimc-gmp/ppmac128_gmp.h, mimc-gmp/ppmac128_gmp.c`
  - **PMAC1-then-MultiCTR** with instantiation `SKINNY-128-256` in `skinny-modes/skinny_modes.h, skinny-modes/skinny_modes.c`
  - **AES-GCM** with instantiation `AES-128` in `aes/aes_gcm.h, aes/aes_gcm.c`
  - **AES-GCM-SIV** with instantiation `AES-128` in `aes/aes_gcm_siv.h, aes/aes_gcm_siv.c`

### Benchmark results
The folder `results` contains the queried test vectors and the corresponding reported cycle count. We tested 1000 samples each of lengths 8, 16, 128, 500 and 1500 bytes.

E.g., `jolteon_fk_64_192_m8` contains the 1000 test vectors for 8 byte messages while `jolteon_fk_64_192_m8_cycles` contains the corresponding cycle counts.

`results.csv` contains the parsed results, created by `plot-data.py`.
