# Eevee
This repository contains information about the Eevee family of AEAD modes for IoT-friendly encryption and MPC-friendly decryption.

## Artifact Evaluation
We provide two implementations, one that implements encryption on the IoT microcontroller and one that implements distributed decryption in MPC (on top of the MP-SPDZ framework).

### 1. Encryption on IoT microcontroller
**Hardware needed: ARM Cortex M4 (STM32F407G-DISC1)**

The [README](software-microcontroller/README.md) provides information on the board setup, executables, build instructions and benchmark scripts