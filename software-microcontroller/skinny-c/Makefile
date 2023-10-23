
# CC=gcc
# CFLAGS=-std=c99 -pedantic -Wall -Wextra -Werror -O3 -fomit-frame-pointer

.PHONY: all clean

# LIBRARY = libskinny.a

# OBJS = \
# 	forkskinny128-cipher.o \
# 	forkskinny64-cipher.o \
# 	demo.o
#
# all: $(LIBRARY)
#
# $(LIBRARY): $(OBJS)
# 	rm -f $(LIBRARY)
# 	$(AR) rc $(LIBRARY) $(OBJS)
#
# demo.x: $(OBJS)
# 	$(CC) $(CFLAGS) -o demo.x $(OBJS)
# clean:
# 	rm -f $(OBJS) $(LIBRARY)
#
# # Plain C core source files.
# forkskinny128-cipher.o: forkskinny-internal.h forkskinny128-cipher.h forkskinny128-cipher.c
# forkskinny64-cipher.o: forkskinny-internal.h forkskinny64-cipher.h forkskinny64-cipher.c
# demo.o: demo.c
CFLAGS += -Iinclude
HEADER=$(wildcard include/*.h)
OBJS = \
	src/skinny128-cipher.o \
	src/skinny64-cipher.o

all: ${OBJS}
	$(AR) -rcs libskinnyc.a ${OBJS}

%.o: %.c ${HEADER}
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f src/*.o *.a
