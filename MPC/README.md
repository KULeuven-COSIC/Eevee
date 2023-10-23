## Eevee MPC experiments
The distributed decryption part of the Eevee paper was implemented in MP-SPDZ (version `f3d5cc4241ce6f4b364a258317f6e5aa225167a7`). However, this version no longer compiles due to a problematic dependency (see [here](https://github.com/data61/MP-SPDZ/issues/1171)). We therefore use the latest version of MP-SPDZ (version `d7f2c318041d943f230edb9ea2cd5f26f04577b3` as of writing). 

MP-SPDZ has two parts: a python-like frontend to describe the program and a backend ("virtual machine") in C++ that executes the actual MPC protocol.
We will first install MP-SPDZ to compile the "bytecode" for the experiments to be run in the virtual machine, and afterwards setup servers to compute the real MPC protocol.

### Setup
On your local PC,
1. Pull the MP-SPDZ submodule: `git submodule update --init --recursive`
2. Navigate into the `MP-SPDZ` folder and install all dependencies (cf [MP-SPDZ/REAMDE](MPC/MP-SPDZ/README.md) and [MP-SPDZ online documentation](https://mp-spdz.readthedocs.io/en/latest/index.html))
    - on our servers (Ubuntu 22.04.2 LTS), we installed `apt-get install automake build-essential cmake git libboost-dev libboost-thread-dev libntl-dev libsodium-dev libssl-dev libtool m4 python3 texinfo yasm htop python3-pip libgmp3-dev`
    - configure *USE_GF2N_LONG*: `echo "USE_GF2N_LONG = 0" > CONFIG.mine`
    - `make boost libote`
    - `make mascot-party.x`
3. Now we can compile the bytecode for the benchmark. All modes can be compiled through the `eevee_benchmark` file where the arguments give details about the mode, the SIMD factor (i.e., how many decryptions in parallel) and the message length.
    - Copy all files from `code/` into `MP-SDPZ/Programs/Source/`
    - Run the compiler (in `MP-SPDZ`): `./compile.py eevee_benchmark <circuit> <simd> <message length in byte>`
    - For example, 
        ```
        $> ./compile.py eevee_benchmark jolteon_forkaes64 10 32
        Default bit length: 64
        Default security parameter: 40
        Compiling file Programs/Source/eevee_benchmark.mpc
        Compiling jolteon_forkaes64, SIMD=10, message length=32 bytes
        Compiled 100000 lines at Mon Oct 23 10:35:15 2023
        Compiled 200000 lines at Mon Oct 23 10:35:18 2023
        Compiled 300000 lines at Mon Oct 23 10:35:21 2023
        Compiled 400000 lines at Mon Oct 23 10:35:23 2023
        Writing to Programs/Schedules/eevee_benchmark-jolteon_forkaes64-10-32.sch
        Writing to Programs/Bytecode/eevee_benchmark-jolteon_forkaes64-10-32-0.bc
        Hash: 2a36876a140045892927c71afbc6ba11dd9882e4e7ed99a5814d1952dbd2fad9
        Program requires at most:
               66560 gf2n bits
               31220 gf2n triples
                 146 virtual machine rounds`
        ```

        This creates `Programs/Schedules/eevee_benchmark-jolteon_forkaes64-10-32.sch` and `Programs/Bytecode/eevee_benchmark-jolteon_forkaes64-10-32-0.bc`
    - The following circuits are available
        - AES-GCM: `aes_gcm64`
        - AES-GCM-SIV: `aes_gcm_siv64`
        - Jolten-AES: `jolteon_forkaes64`
        - Espeon-AES: `espeon_forkaes64`
        - Umbreon-Forkskinny-64-192: `umbreon_forkskinny64_192`
        - Umbreon-Forkskinny-128-256: `umbreon_forkskinny128_256`
        - Jolteon-Forkskinny-64-192: `jolteon_forkskinny64_192`
        - Jolteon-Forkskinny-128-256: `jolteon_forkskinny128_256`
        - Espeon-Forkskinny-128-384: `espeon_forkskinny128_384`
        - CTR-t-PMAC-SKINNY-128-256: `pmac_skinny128_256`
        - CTR-t-HtMAC-SKINNY-128-256: `htmac_skinny128_256`

    Note that the benchmark just decrypts a random (or zero) ciphertext with a random/zero key and we don't fail if the tag doesn't verify.
5. To test functionality, the MPC protocol can be run locally, e.g., each MPC party runs on the local PC and communicates via localhost. MP-SPDZ already has useful scripts for this.

    (In the `MP-SPDZ` directory) Use `Scripts/mascot.sh -v eevee_benchmark-jolteon_forkaes64-10-32` to run the previously compiled benchmark. Note that the arguments (`jolteon_forkaes64`, `simd=10`, `message length=32`) become part of the "name" of the final bytecode. The option `-v` gives more detailed information about the execution and reports timing data. Note that `-v` did not report timing data when the experiments were conducted for the paper, so the online and offline phase was measured separately, see [online-only benchmarking](https://mp-spdz.readthedocs.io/en/latest/readme.html#online-only-benchmarking) and [benchmarking the MASCOT offline phase](https://mp-spdz.readthedocs.io/en/latest/readme.html#benchmarking-the-mascot-or-spdz2k-offline-phase). This is no longer required.

   For example,
   ```
   $> Scripts/mascot.sh -v eevee_benchmark-jolteon_forkskinny64_192-10-32
    Running /MPC/MP-SPDZ/Scripts/../mascot-party.x 0 -v eevee_benchmark-jolteon_forkskinny64_192-10-32 -pn 12271 -h localhost -N 2
    Running /MPC/MP-SPDZ/Scripts/../mascot-party.x 1 -v eevee_benchmark-jolteon_forkskinny64_192-10-32 -pn 12271 -h localhost -N 2
    Using security parameter 40
    No modulus found in Player-Data//2-p-128/Params-Data, generating 128-bit prime
    Starting timer 1 at 0 (0 MB, 0 rounds) after 2.3655e-05
    Stopped timer 1 at 1.0487 (53.9952 MB, 1886 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 1.0487 (53.9952 MB, 1886 rounds) after 0.000749577
    Stopped timer 1 at 1.99283 (103.817 MB, 3703 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 1.99283 (103.817 MB, 3703 rounds) after 0.000966699
    Stopped timer 1 at 2.79567 (153.64 MB, 5520 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 2.79567 (153.64 MB, 5520 rounds) after 0.000636224
    Stopped timer 1 at 3.48371 (199.348 MB, 7315 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 3.48371 (199.348 MB, 7315 rounds) after 0.00061867
    Stopped timer 1 at 4.2837 (249.152 MB, 9125 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 4.2837 (249.152 MB, 9125 rounds) after 0.000747805
    Stopped timer 1 at 5.05574 (298.974 MB, 10942 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 5.05574 (298.974 MB, 10942 rounds) after 0.000746452
    Stopped timer 1 at 5.77274 (344.683 MB, 12737 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 5.77274 (344.683 MB, 12737 rounds) after 0.000573846
    Stopped timer 1 at 6.58291 (394.505 MB, 14554 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 6.58291 (394.505 MB, 14554 rounds) after 0.000422359
    Stopped timer 1 at 7.35667 (444.309 MB, 16364 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Starting timer 1 at 7.35667 (444.309 MB, 16364 rounds) after 0.000712027
    Stopped timer 1 at 8.07157 (490.017 MB, 18159 rounds)
    840100421 800108000 800100021 800108001 40000401 800100021 840108400 40008421 840108401 1 800108001 8021 800108001 1 8021 800100021 40008420 40008420 800100020 800100020 40008420 800108001 800108001 840108400 800108001 840108401 840108400 840100421 840100421 40008420 8021 40000400 840100420 40008421 800100021 8020 8020 800100021 1 8021 40000401 800100021 840108401 840108401 40000401 40000401 40008420 800108000 8021 800108000 40008421 800108001 1 840108400 40008421 800100021 840108400 40000401 800108000 840108400 40008420 8020 8021 840108400 
    Compiler: ./compile.py eevee_benchmark jolteon_forkskinny64_192 10 32
    	130 triples of SPDZ gf2n_ left
    	480 bits of SPDZ gf2n_ left
    	120 triples of SPDZ gf2n_ left
    	120 bits of SPDZ gf2n_ left
    2 threads spent a total of 1.09675 seconds (2.32743 MB, 14751 rounds) on the online phase, 7.98352 seconds (487.736 MB, 3509 rounds) on the preprocessing/offline phase, and 7.0803 seconds idling.
    Communication details (rounds in parallel threads counted double):
    Broadcasting 0.23032 MB in 9575 rounds, taking 0.429541 seconds
    Exchanging one-to-one 304.276 MB in 843 rounds, taking 0.554302 seconds
    Receiving directly 3.07095 MB in 3417 rounds, taking 0.185206 seconds
    Receiving one-to-one 182.486 MB in 504 rounds, taking 0 seconds
    Sending directly 3.07095 MB in 3417 rounds, taking 0.085419 seconds
    Sending one-to-one 182.486 MB in 504 rounds, taking 0 seconds
    CPU time = 7.91471 (overall core time)
    The following benchmarks are including preprocessing (offline phase).
    Time = 8.08369 seconds 
    Time1 = 8.07157 seconds (490.017 MB, 18159 rounds)
    Data sent = 490.063 MB in ~18260 rounds (party 0 only)
    Global data sent = 980.127 MB (all parties)
    Actual cost of program:
      Type gf2n
            117750        Triples
            118400           Bits
    Coordination took 0.240535 seconds
    Command line: /MPC/MP-SPDZ/Scripts/../mascot-party.x 0 -v eevee_benchmark-jolteon_forkskinny64_192-10-32 -pn 12271 -h localhost -N 2
    ```

    **NOTE THAT FOR AES-based CIRCUITS**: `USE_GF2N_LONG = 1` must be set in `CONFIG.mine` (and the virtual machine needs to be recompiled `make clean && make mascot-party.x`). Otherwise, the MPC protocol uses GF(2^40) and the GHASH computation an embeddings are incorrect and the circuit produces wrong results.


We hope these information are sufficient to show functionality.


### Benchmarking Environment to Reproduce
The benchmarks were run on up to 5 `pcgen05` machines in [imec's iLab.t Virtual Wall 2](https://doc.ilabt.imec.be/ilabt/virtualwall/hardware.html#virtual-wall-2).

Machine spec

- 4 core E3-1220v3 (3.1GHz)
- 16GB RAM
- 250GB harddisk
- 1 Gbit/s connection with < 1ms latency

On each server, MP-SPDZ has to be installed as described before (points 1 and 2 in **Setup**). Then
1. Let IP1, IP2 etc. denote the IP addresses of servers 1, 2, ..., then on each server, the IP addresses are configured in `MP-SPDZ/HOSTS` where line 1 specifies IP1, line 2 specifies IP2, etc. E.g., `echo "IP1\nIP2\n...IP5\n" > MP-SPDZ/HOSTS`
2. Either add the files from `code` to `MP-SPDZ/Programs/Source` as described before and recompile or copy the content of `MP-SPDZ/Programs/Bytecode` and `MP-SPDZ/Programs/Schedules` from the local PC into the respective folder of the MP-SPDZ installation of each server.
3. Introduce latency artifically using `tc`
    - Add delay: `tc qdisc add dev eno2 root netem delay 20ms` (for 40ms RTT)
    - Remove delay: `tc qdisc del dev eno2 root netem`
    
    where `eno2` is the network interface
4. On each server, run the MASCOT MPC protocol, e.g., `./mascot-party.x -v -p <player id> -N <total number of players> -ip HOSTS <program name>` where
    - `player id` is the id of the server starting from 0. For a three-party protocol run, the first server puts `-p 0`, the second `-p 1` etc.
    - `total number of players` for a three-party protocol is `-N 3`
    - `program name` is e.g. `eevee_benchmark-jolteon_forkskinny64_192-10-32`

While earlier benchmarks were run "by hand" like described above, the AES-based circuits were benchmarked using the script `run-experiment.py` (together with `run-both.sh`). This script automatically copies all bytecode/schedule files from the top-level directory to the correct directory in `MP-SPDZ/`, runs the benchmark for one or more targets and `tar`s the logfiles afterwards.
First, on each server, save the corresponding party id in `MP-SPDZ/playerid`, i.e., on server 1 `echo "0" > MP-SPDZ/playerid`, on server 2 `echo "1" > MP-SPDZ/playerid`, etc. and then run the script on each server
```
$> python run-experiment.py -h
usage: run-experiment.py [-h] [--mp-spdz MPSPDZHOME] [--both BOTH] [--skip]
                         [--detach]
                         target [target ...]

Run experiment

positional arguments:
  target                Targets to benchmark

options:
  -h, --help            show this help message and exit
  --mp-spdz MPSPDZHOME  Path to root directory of MP-SPDZ (default: MP-SPDZ)
  --both BOTH           Path to both script (default: run-both.sh)
  --skip                Skip setup and moving target files (default: False)
  --detach              Start benchmark detached (default: False)
```
For example, `python run-experiment.py eevee_benchmark-jolteon_forkskinny64_192-10-32 eevee_benchmark-jolteon_forkskinny128_256-10-32` first runs the benchmark for Jolteon-Forkskinny-64-192 and afterwards for Jolteon-Forkskinny-128-256. After the benchmarks complete, the script creates a tar file with all relevant log files named `res<i>.tar` for player `i`.

### (Old) Experiments
Experiments are defined in the [Espec](https://jfed.ilabt.imec.be/espec/) format and can be uploaded and run easily using [jFed](https://jfed.ilabt.imec.be/).

In each folder `experiments/<nr>-<experiment name>`, the following files can be found

- `experiment-specification.yml`: Describes the experiment setup, number of parties to run and which code to execute.
- `nodes.rspec`: Defines the details of the sliver and cluster. Obtained via jFed (Experiment >> RSpec Viewer >> Manifest RSpec).
- `run-experiment.py` is the script that is executed to start the experiment, it will
  1. load the code (bytecode and schedule files) of all requested benchmarks, or fail otherwise
  2. detach a `tmux` session that runs for each requested benchmark
      - `run-prep.sh` (generate pre-processing data)
      - `run-online.sh` (run online phase)
      - `run-both.sh` (runs the virtual machine of MP-SPDZ in the "normal" mode where offline and online phase run intertwined)
      - collect all log files and write them to a folder named after the requested benchmark

After all benchmarks ran successfully, the script creates a tar file with all relevant log files named `res<i>.tar` for player `i`.

### (Old) Data Analysis
For each experiment, we downloaded the `res<i>.tar` file into the corresponding experiment folder and unpacked the log files. Inside the experiment folder and benchmark folder, e.g., `46-espeon-lat/espeon_parallel-128-500`, the logfiles can be found

- `run-prep<i>.log` (from `run-prep.sh`)
- `run-online<i>.log` (from `run-online.sh`)
- `run-both<i>.log` (from `run-both.sh`)
To correctly parse the reported timing and memory data, the number of iterations, i.e., how many instances of the mode were executed in parallel, has to be specified. This is done by placing a file `iters` containing the number of iterations
into each benchmark folder.
Note that for some older experiments, this is not strictly followed and thus the structure is inconsistent (sorry).

The script `collect.py` is used to parse the raw log files and compute the relevant measurements. At the bottom, three functions can be called

- `prepare_data_lat(data_2players, data_lat)` which parses the experiments run in a network with latency and places the parsed data into the `lat` folder
- `plot_online_offline_2players(data_2players)` which parses the experiments and collects all measurements for two parties for varying message lengths. Results are placed in the `2players` folder
- `plot_nplayers(128, data_2players, data_3players, data_4players, data_5players)` which parses the experiments and collectes all measurements for varying numbers of players and 128 byte messages. The results are placed in the `nplayers` folder.

The script `collect_aes.py` parses the AES-based experiments in a similar fashion and saves the result to `results.csv`.
