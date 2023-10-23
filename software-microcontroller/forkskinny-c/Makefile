
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

HEADER=$(wildcard *.h)
OBJS = \
	forkskinny128-cipher.o \
	forkskinny64-cipher.o

all: ${OBJS}
	$(AR) -rcs libforkskinnyc.a ${OBJS}

%.o: %.c ${HEADER}
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f *.o *.a
