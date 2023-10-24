#!bin/bash
set -e
export SKINNY_TARGET_FIELD=f128
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 MESSAGE_LEN SIMD"
	exit 1
fi
MESSAGE_LEN=$1
SIMD=$2
cd MP-SPDZ/
TARGETS="jolteon_forkaes64 aes_gcm64 espeon_forkaes64 aes_gcm_siv64"
echo "Compiling these targets for message length $MESSAGE_LEN bytes and SIMD=$SIMD: $TARGETS"
for target in $TARGETS; do
	echo "./compile.py eevee_benchmark $target $SIMD $MESSAGE_LEN"
	python ./compile.py eevee_benchmark $target $SIMD $MESSAGE_LEN
done
echo "Done :)"