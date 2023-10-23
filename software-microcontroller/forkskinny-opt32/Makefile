HEADER=$(wildcard *.h)
OBJS=aead-common.o internal-forkskinny.o skinny.o

all: ${OBJS}
	$(AR) -rcs libforkskinnyopt32.a ${OBJS}

%.o: %.c ${HEADER}
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f *.o *.a
