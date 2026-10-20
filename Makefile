CC ?= cc
CFLAGS ?= -O2
PREFIX ?= /usr/local
DESTDIR ?=

.PHONY: all test install
all: decompile46

decompile46: decompiler.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 -Wall -Wextra -Werror $< $(LDFLAGS) -o $@

test:
	CC="$(CC)" CFLAGS="$(CFLAGS)" python3 -m unittest -v test_static_bounds.py

install: decompile46
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 decompile46 "$(DESTDIR)$(PREFIX)/bin/decompile46"
