CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Wextra -Werror -fomit-frame-pointer -Iblake2s-opt-bin/include  -Imbedtls/include -O3
LIB_OPT32=forkskinny-opt32/libforkskinnyopt32.a
LIB_BLAKE2s=blake2s-opt-bin/lib/blake2s.lib
LIB_MIMC=mimc-gmp/libmimcgmp.a
LIB_GMP=gmp-6.2.1-bin64/lib/libgmp.a
LIB_FORKSKINNYC=forkskinny-c/libforkskinnyc.a
LIB_SKINNYC=skinny-c/skinnyc.a
LIB_MBED_CRYPTO=mbedtls/library/libmbedcrypto.a

LDFLAGS= -L forkskinny-opt32/ -l forkskinnyopt32 \
	-L mimc-gmp/ -l mimcgmp \
	-L gmp-6.2.1-bin/lib/ -l gmp \
	-L forkskinny-c/ -l forkskinnyc \
	-L skinny-c/ -l skinnyc \
	-L mbedtls/library/ -l mbedcrypto

OBJS=aes/aes.o aes/ghash.o aes/polyval.o aes/aes_gcm.o aes/aes_gcm_siv.o \
	eevee-forkskinny/eevee_common.o eevee-forkskinny/umbreon.o eevee-forkskinny/espeon.o eevee-forkskinny/jolteon.o \
	skinny-modes/skinny_modes.o aes/aes_xex_fork.o aes/jolteon_aes.o aes/espeon_aes.o
LIBS=${LIB_BLAKE2s} ${LIB_REF} ${LIB_OPT32} ${LIB_MIMC} ${LIB_FORKSKINNYC} ${LIB_SKINNYC} ${LIB_MBED_CRYPTO}

FORKSKINNY_BACKEND=EEVEE_FORKSKINNY_BACKEND_C #EEVEE_FORKSKINNY_BACKEND_OPT32

.PHONY: clean deepclean ${LIB_FORKSKINNYC}

all: generate.x

generate.x: generate.c ${OBJS} ${LIBS}
	${CC} ${CFLAGS} -o generate.x generate.c ${GENERATE_X_OBJS} ${OBJS} -lm ${LDFLAGS}  ${LIB_BLAKE2s}

eevee-forkskinny/umbreon.o: eevee-forkskinny/umbreon.h eevee-forkskinny/eevee_common.h eevee-forkskinny/umbreon_core.c eevee-forkskinny/umbreon.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o eevee-forkskinny/umbreon.o eevee-forkskinny/umbreon.c

eevee-forkskinny/jolteon.o: eevee-forkskinny/jolteon.h eevee-forkskinny/eevee_common.h eevee-forkskinny/jolteon_core.c eevee-forkskinny/jolteon.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o eevee-forkskinny/jolteon.o eevee-forkskinny/jolteon.c

eevee-forkskinny/espeon.o: eevee-forkskinny/espeon.h eevee-forkskinny/eevee_common.h eevee-forkskinny/espeon.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o eevee-forkskinny/espeon.o eevee-forkskinny/espeon.c

skinny-modes/skinny_modes.o: ${LIB_BLAKE2s} skinny-modes/skinny_modes.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o skinny-modes/skinny_modes.o skinny-modes/skinny_modes.c

eevee-forkskinny/eevee_common.o: eevee-forkskinny/eevee_common.h eevee-forkskinny/eevee_common.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o eevee-forkskinny/eevee_common.o eevee-forkskinny/eevee_common.c

${LIB_OPT32}: $(wildcard forkskinny-opt32/*.h) $(wildcard forkskinny-opt32/*.c)
	cd forkskinny-opt32/ && CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) all

${LIB_FORKSKINNYC}:
	cd forkskinny-c/ && CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) all

${LIB_BLAKE2s}: blake2s-opt/
	mkdir -p blake2s-opt-bin
	cd blake2s-opt && \
	CC=${CC} CFLAGS="${CFLAGS}" ./configure --prefix=../blake2s-opt-bin --generic && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) lib && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) install-lib

${LIB_MIMC}: $(wildcard mimc-gmp/*.h) $(wildcard mimc-gmp/*.c)
	cd mimc-gmp && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) all

${LIB_GMP}: gmp-6.2.1-64bit/
	cd mimc-gmp && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) ${LIB_GMP}

${LIB_SKINNYC}:
	cd skinny-c/ && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) all

${LIB_MBED_CRYPTO}: mbedtls/
	cd mbedtls && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) library/libmbedcrypto.a

clean:
	rm -f *.o *.x aes/*.o eevee-forkskinny/*.o skinny-modes/*.o
	rm -rf blake2s-opt-bin/
	cd forkskinny-opt32 && $(MAKE) clean
	cd mimc-gmp && $(MAKE) clean
	cd forkskinny-c && $(MAKE) clean
	cd skinny-c && $(MAKE) clean
	cd mbedtls && $(MAKE) clean

deepclean: clean
	rm -rf gmp-6.2.1-bin64/
	cd gmp-6.2.1-64bit && $(MAKE) clean
