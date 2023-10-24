#!bin/bash
set -e
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 MESSAGE_LEN SIMD"
	exit 1
fi
MESSAGE_LEN=$1
SIMD=$2
echo "--------------------------"
echo "MP-SPDZ must be compiled with USE_GF2N_LONG=1"
echo "--------------------------"
TARGETS="jolteon_forkaes64 aes_gcm64 espeon_forkaes64 aes_gcm_siv64"
EXP_TARGETS=""
for target in $TARGETS; do
	EXP_TARGETS+=" eevee_benchmark-$target-$SIMD-$MESSAGE_LEN"
done
echo "Running these targets for message length $MESSAGE_LEN bytes and SIMD=$SIMD using run-experiment.py: $EXP_TARGETS"
python run-experiment.py $EXP_TARGETS