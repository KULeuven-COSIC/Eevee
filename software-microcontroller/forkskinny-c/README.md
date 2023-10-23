# forkskinny-c
This repository provides a C implementation of Forkskinny-64-192, Forkskinny-128-256 and Forkskinny-128-384 ([Forkcipher paper](https://eprint.iacr.org/2019/1004.pdf)).

The implementation is based on [SKINNY-C](https://github.com/rweather/skinny-c) by Rhys Weatherley for the SKINNY round function, and on the 32-bit [ForkAE](https://github.com/ArneDeprez1/ForkAE-SW/tree/master/32_bit) implementation by Arne Deprez.

This is a constant-time implementation, i.e. no key-dependent branching or table lookups.

On a 32-bit platform, this implementation should be faster than ForkAE.

## Build
Run `make`

## Usage
See `demo.c` for examples how to use the code.

## Implementation details
- The tweakey schedule computation is separate for each tweakey TK1, TK2, TK3. This requires more memory but speeds up the primitive when used in a mode where only parts of the tweakey change.
- Round constants are integrated into the key schedule
- In the implementation, the forkcipher legs are swapped with respect to the formal definition in the paper, i.e. the left leg in the paper corresponds to the right leg in the code, etc.

## License
This code is licensed under the MIT license.
