.PHONY: all clean

PREFIX	?= arm-none-eabi
CC		= $(PREFIX)-gcc
LD		= $(PREFIX)-gcc
OBJCOPY	= $(PREFIX)-objcopy
OBJDUMP	= $(PREFIX)-objdump
GDB		= $(PREFIX)-gdb

OPENCM3DIR = ./libopencm3
ARMNONEEABIDIR = /usr/arm-none-eabi
COMMONDIR = ./common

ARCH_FLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

LIB_OPT32=forkskinny-opt32/libforkskinnyopt32.a
LIB_BLAKE2s=blake2s-opt-bin/lib/blake2s.lib
#LIB_MIMC=mimc-gmp/libmimcgmp.a
#LIB_GMP=gmp-6.2.1-bin32/lib/libgmp.a
LIB_FORKSKINNYC=forkskinny-c/libforkskinnyc.a
LIB_SKINNYC=skinny-c/libskinnyc.a
LIB_MBED_CRYPTO=mbedtls/library/libmbedcrypto.a

all: umbreon_benchmark_64_192.bin \
	umbreon_benchmark_128_256.bin \
	htmac_skinny_benchmark_128_256.bin \
	pmac_skinny_benchmark_128_256.bin \
	jolteon_benchmark_64_192.bin \
	jolteon_benchmark_128_256.bin \
	espeon_benchmark_128_256.bin \
	espeon_benchmark_128_384.bin \
	skinny_benchmark_64_192.bin \
	skinny_benchmark_128_256.bin \
	forkskinny_benchmark_64_192.bin \
	forkskinny_benchmark_128_256.bin \
	aes_gcm_siv_benchmark_128.bin \
	aes_gcm_benchmark_128.bin \
	jolteon_aes_benchmark_128.bin \
	espeon_aes_benchmark_128.bin

%.elf: LDSCRIPT = $(COMMONDIR)/stm32f407x6.ld
%.elf: LDFLAGS += -lopencm3_stm32f4
%.elf: OBJS += $(COMMONDIR)/stm32f4_wrapper.o

CFLAGS		+= -std=c99 -O3 -fomit-frame-pointer\
		   -Wall -Wextra -Wimplicit-function-declaration \
		   -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes \
		   -Wundef -Wshadow \
		   -I$(ARMNONEEABIDIR)/include -I$(OPENCM3DIR)/include \
			 -Imbedtls/include \
			 -Iblake2s-opt-bin/include \
		   -fno-common $(ARCH_FLAGS) -MD \
			 -DSTM32F4 -DAES_ASSEMBLY
LDFLAGS		+= --static -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group \
		   -T$(LDSCRIPT) -nostartfiles -Wl,--gc-sections,--no-print-gc-sections \
		   $(ARCH_FLAGS) -L$(OPENCM3DIR)/lib

FORKSKINNY_BACKEND=EEVEE_FORKSKINNY_BACKEND_OPT32 #EEVEE_FORKSKINNY_BACKEND_C

OBJS=aes/aes.o aes/aes.s aes/ghash.o aes/polyval.o aes/aes_gcm.o aes/aes_gcm_siv.o \
	eevee-forkskinny/eevee_common.o eevee-forkskinny/umbreon.o eevee-forkskinny/espeon.o eevee-forkskinny/jolteon.o \
	skinny-modes/skinny_modes.o aes/aes_xex_fork.o aes/jolteon_aes.o aes/espeon_aes.o

LIBS=${LIB_BLAKE2s} ${LIB_OPT32} ${LIB_FORKSKINNYC} ${LIB_SKINNYC} ${LIB_MBED_CRYPTO} # ${LIB_MIMC} ${LIB_GMP}

eevee-forkskinny/umbreon.o: eevee-forkskinny/umbreon.h eevee-forkskinny/eevee_common.h eevee-forkskinny/umbreon_core.c eevee-forkskinny/umbreon.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o eevee-forkskinny/umbreon.o eevee-forkskinny/umbreon.c #-DEEVEE_VERBOSE

eevee-forkskinny/jolteon.o: eevee-forkskinny/jolteon.h eevee-forkskinny/eevee_common.h eevee-forkskinny/jolteon_core.c eevee-forkskinny/jolteon.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o eevee-forkskinny/jolteon.o eevee-forkskinny/jolteon.c

eevee-forkskinny/espeon.o: eevee-forkskinny/espeon.h eevee-forkskinny/eevee_common.h eevee-forkskinny/espeon.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o eevee-forkskinny/espeon.o eevee-forkskinny/espeon.c

skinny-modes/skinny_modes.o: ${LIB_BLAKE2s} skinny-modes/skinny_modes.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o skinny-modes/skinny_modes.o skinny-modes/skinny_modes.c

eevee-forkskinny/eevee_common.o: eevee-forkskinny/eevee_common.h eevee-forkskinny/eevee_common.c
	${CC} ${CFLAGS} -D${FORKSKINNY_BACKEND} -c -o eevee-forkskinny/eevee_common.o eevee-forkskinny/eevee_common.c

skinny_benchmark_128_256.o: CFLAGS += -D${FORKSKINNY_BACKEND}
skinny_benchmark_64_192.o: CFLAGS += -D${FORKSKINNY_BACKEND}
forkskinny_benchmark_64_192.o: CFLAGS += -D${FORKSKINNY_BACKEND}
forkskinny_benchmark_128_256.o: CFLAGS += -D${FORKSKINNY_BACKEND}

${LIB_OPT32}: $(wildcard forkskinny-opt32/*.h) $(wildcard forkskinny-opt32/*.c)
	cd forkskinny-opt32/ && CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) all

${LIB_BLAKE2s}: blake2s-opt/
	mkdir -p blake2s-opt-bin
	cd blake2s-opt && \
	CC=${CC} CFLAGS="${CFLAGS} -I../${OPENCM3DIR}/include" ./configure --prefix=../blake2s-opt-bin --generic && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) lib && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) install-lib

#${LIB_MIMC}: $(wildcard mimc-gmp/*.h) $(wildcard mimc-gmp/*.c)
#	cd mimc-gmp/ && CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) -f cortex-m4.mk all

#${LIB_GMP}: gmp-6.2.1-32bit/
#	cd mimc-gmp && CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) -f cortex-m4.mk ${LIB_GMP}

${LIB_FORKSKINNYC}:
	cd forkskinny-c/ && CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) all

${LIB_SKINNYC}:
	cd skinny-c/ && CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) all

${LIB_MBED_CRYPTO}: mbedtls/
	cd mbedtls && \
	CC=${CC} CFLAGS="${CFLAGS}" $(MAKE) library/libmbedcrypto.a

%.bin: %.elf
	$(OBJCOPY) -Obinary $^ $@

%.elf: %.o $(OBJS) $(COMMONDIR)/stm32f4_wrapper.o $(LDSCRIPT) ${LIBS}
	$(LD) -o $@ $< $(OBJS) $(LDFLAGS) ${LIBS}
%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm -f *.o *.d *.elf *.bin aes/*.o aes/*.d eevee-forkskinny/*.o eevee-forkskinny/*.d skinny-modes/*.o skinny-modes/*.d
	cd forkskinny-opt32 && $(MAKE) clean
	rm -rf blake2s-opt-bin/
	cd mimc-gmp && $(MAKE) -f cortex-m4.mk clean
	cd forkskinny-c && $(MAKE) clean
	cd skinny-c && $(MAKE) clean
	cd mbedtls && $(MAKE) clean

deepclean: clean
	rm -rf gmp-6.2.1-bin32/
	cd gmp-6.2.1-32bit && $(MAKE) clean
