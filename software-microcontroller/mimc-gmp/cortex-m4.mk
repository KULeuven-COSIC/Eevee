HEADER=$(wildcard *.h)
OBJS=gmp_utils.o htmac128_gmp.o mimc128_gmp.o mini-gmp.o ppmac128_gmp.o
INCLUDE += -I../blake2s-opt-bin/include -I../gmp-6.2.1-bin32/include

LIB_GMP=gmp-6.2.1-bin32/lib/libgmp.a
CFLAGS_GMP=-nostartfiles --specs=nosys.specs -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

all: ${OBJS}
	$(AR) -rcs libmimcgmp.a ${OBJS}

mini-gmp.o: mini-gmp.c mini-gmp.h
	# suppress some warnings that occurr in mini-gmp
	${CC} ${CFLAGS} -Wno-unused-parameter -Wno-sign-compare -c -o $@ $<

%.o: %.c ${HEADER} ${LIB_GMP}
	${CC} ${CFLAGS} ${INCLUDE} -c -o $@ $<

${LIB_GMP}: $(wildcard ../gmp-6.2.1-32bit/**/*.h) $(wildcard ../gmp-6.2.1-32bit/**/*.c)
	cd ../gmp-6.2.1-32bit && \
	./configure CC=arm-none-eabi-gcc CFLAGS="${CFLAGS} ${CFLAGS_GMP}" \
  --host=arm-none-eabi --disable-assembly --prefix=$(PWD)/../gmp-6.2.1-bin32  && \
	$(MAKE) && \
	$(MAKE) install

clean:
	rm -f *.o *.a *.d
