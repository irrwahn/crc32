# crc32 Makefile

LDFLAGS=
CFLAGS=-std=gnu99 -W -Wall -Wextra -O3
# -DDEBUG
STRIP=strip
RM=rm -f

BINARY=crc32
SRC=*.c
PREFIX?=/usr/local

.PHONY: all clean install

all: $(BINARY)

clean:
	$(RM) $(BINARY)

$(BINARY): $(SRC)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	$(STRIP) $(BINARY)

install:
	mkdir -p ${PREFIX}/bin
	cp -v $(BINARY) ${PREFIX}/bin/

# EOF
