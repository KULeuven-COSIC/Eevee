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
cd MP-SPDZ/
TARGETS="jolteon_forkaes64 aes_gcm64 espeon_forkaes64 aes_gcm_siv64"
echo "Running these targets for message length $MESSAGE_LEN bytes and SIMD=$SIMD using Scripts/mascot.sh -v: $TARGETS"
for target in $TARGETS; do
	echo "Scripts/mascot.sh -v eevee_benchmark-$target-$SIMD-$MESSAGE_LEN > ../log-$target-$SIMD-$MESSAGE_LEN"
	bash Scripts/mascot.sh -v eevee_benchmark-$target-$SIMD-$MESSAGE_LEN > ../log-$target-$SIMD-$MESSAGE_LEN
done
echo "Done :)"