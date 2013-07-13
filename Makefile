# crc32 Makefile

LDFLAGS=
CFLAGS=-std=gnu99 -W -Wall -Wextra -O3
# -DDEBUG
STRIP=strip
RM=rm -f
INSTALL=install -D

BINARY=crc32
SRC=*.c
DESTDIR=

.PHONY: all clean install

all: $(BINARY)

clean:
	$(RM) $(BINARY)

$(BINARY): $(SRC)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	$(STRIP) $(BINARY)

install:
	$(INSTALL) $(BINARY) ${DESTDIR}/usr/local/bin

# EOF
