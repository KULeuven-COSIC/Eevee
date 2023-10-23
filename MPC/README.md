## Eevee MPC experiments
The distributed decryption part of the Eevee paper was implemented in MP-SPDZ (version f3d5cc4241ce6f4b364a258317f6e5aa225167a7). MP-SPDZ has two parts: a python-like frontend to describe the program and a backend ("virtual machine") in C++ that executed the actual MPC protocol.
We will first install MP-SPDZ to compile the "bytecode" for the experiments to be run in the virtual machine, and afterwards setup servers to compute the real MPC protocol.

### Setup
On your local PC,
1. Pull the MP-SPDZ submodule commit f3d5cc4241ce6f4b364a258317f6e5aa225167a7, `git submodule update --init --recursive`: git submodule add -b f3d5cc4241ce6f4b364a258317f6e5aa225167a7 https://github.com/data61/MP-SPDZ.git
2. Navigate into the `MP-SPDZ` folder and install all dependencies (cf [MP-SPDZ/REAMDE](MPC/MP-SPDZ/README.md) and [MP-SPDZ online documentation](https://mp-spdz.readthedocs.io/en/latest/index.html))
    - on our servers (Ubuntu X), we installed `apt-get install automake build-essential cmake git libboost-dev libboost-thread-dev libntl-dev libsodium-dev libssl-dev libtool m4 python3 texinfo yasm htop python3-pip`
    - configure *USE_GF2N_LONG*: `echo "USE_GF2N_LONG = 0" > CONFIG.mine`
    - `make boost mpir libote`
    - `make mascot-party.x`
3. Now we can compile the bytecode for the benchmark. All modes can be compiled through the `eevee_benchmark` file where the arguments give details about the mode, the SIMD factor (i.e., how many decryptions in parallel) and the message length.
    - `./compile.py eevee_benchmark <circuit> <simd> <message length in byte>`
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

### Setup & Environment
The benchmarks were run on up to 5 `pcgen05` machines in [imec's iLab.t Virtual Wall 2](https://doc.ilabt.imec.be/ilabt/virtualwall/hardware.html#virtual-wall-2).

Machine spec

- 4 core E3-1220v3 (3.1GHz)
- 16GB RAM
- 250GB harddisk
- 1 Gbit/s connection with < 1ms latency

### MPC Code
The folder `code` contains the source code files for MP-SPDZ. Copy the content into `Programs/Source` in [MP-SPDZ](https://github.com/data61/MP-SPDZ). Compile the desired function and execute as usual in MP-SPDZ

### Experiments
Experiments are defined in the [Espec](https://jfed.ilabt.imec.be/espec/) format and can be uploaded and run easily using [jFed](https://jfed.ilabt.imec.be/).

In each folder `<nr>-<experiment name>`, the following files can be found

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

### Experiments with Latency
Latency was introduced artificially using `tc`

- Add delay: `tc qdisc add dev eno2 root netem delay 20ms`
- Remove delay: `tc qdisc del dev eno2 root netem`
where `eno2` is the network interface

### Data Analysis
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