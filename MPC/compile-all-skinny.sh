set -e
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 MESSAGE_LEN SIMD"
	exit 1
fi
MESSAGE_LEN=$1
SIMD=$2
cd MP-SPDZ/
TARGETS="pmac_skinny128_256 htmac_skinny128_256 umbreon_forkskinny64_192 umbreon_forkskinny128_256 jolteon_forkskinny64_192 jolteon_forkskinny128_256 espeon_forkskinny128_384"
echo "Compiling these targets for message length $MESSAGE_LEN bytes and SIMD=$SIMD: $TARGETS"
for target in $TARGETS; do
	echo "./compile.py eevee_benchmark $target $SIMD $MESSAGE_LEN"
	python ./compile.py eevee_benchmark $target $SIMD $MESSAGE_LEN
done
echo "Done :)"