#!bin/bash
set -e
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 MESSAGE_LEN SIMD"
	exit 1
fi
MESSAGE_LEN=$1
SIMD=$2
echo "--------------------------"
echo "MP-SPDZ must be compiled with USE_GF2N_LONG=0"
echo "--------------------------"
TARGETS="pmac_skinny128_256 htmac_skinny128_256 umbreon_forkskinny64_192 umbreon_forkskinny128_256 jolteon_forkskinny64_192 jolteon_forkskinny128_256 espeon_forkskinny128_384"
EXP_TARGETS=""
for target in $TARGETS; do
	EXP_TARGETS+=" eevee_benchmark-$target-$SIMD-$MESSAGE_LEN"
done
echo "Running these targets for message length $MESSAGE_LEN bytes and SIMD=$SIMD using run-experiment.py: $EXP_TARGETS"
python run-experiment.py $EXP_TARGETS