# Eevee
This repository contains information about the Eevee family of AEAD modes for IoT-friendly encryption and MPC-friendly decryption.
The Eevee family stems from the paper "Let's Go Eevee! A Friendly and Suitable Family of AEAD Modes for IoT-to-Cloud Secure Computation" to appear in CCS 2023 ([eprint](https://eprint.iacr.org/2023/1361), [conference version](https://doi.org/10.1145/3576915.3623091)).

If content of this repository has been useful to you for academic work, please consider citing
```
@misc{cryptoeprint:2023/1361,
      author = {Amit Singh Bhati and Erik Pohle and Aysajan Abidin and Elena Andreeva and Bart Preneel},
      title = {Let's Go Eevee! A Friendly and Suitable Family of AEAD Modes for IoT-to-Cloud Secure Computation},
      howpublished = {Cryptology ePrint Archive, Paper 2023/1361},
      year = {2023},
      doi = {10.1145/3576915.3623091},
      note = {\url{https://eprint.iacr.org/2023/1361}},
      url = {https://eprint.iacr.org/2023/1361}
}
```

## Artifact Evaluation
We provide two implementations, one that implements encryption on the IoT microcontroller and one that implements distributed decryption in MPC (on top of the MP-SPDZ framework).

### 1. Encryption on IoT microcontroller
**Hardware needed: ARM Cortex M4 (STM32F407G-DISC1)**

The [software-microcontroller/README](software-microcontroller/README.md) provides information on the board setup, executables, build instructions and benchmark scripts

### 2. Distributed Decryption in MPC
**Hardware needed: 1 normal PC (to verify functionality) and 2-3 servers connected in a network**

The [MPC/README](MPC/README.md) provides detailed information how to install, configure and run all software for the experiments.
