.PHONY: docs clean maintainer-clean install uninstall

COMMON=../common

SOURCE=xml-validate.c $(COMMON)/xml-utils.c

ifeq ($(OS),Windows_NT)
  OUTPUT=xml-validate.exe
else
  OUTPUT=xml-validate
endif

WARNING_FLAGS=-Wall -Werror -pedantic-errors
CFLAGS=$(WARNING_FLAGS) -I $(COMMON) `pkg-config --cflags libxml-2.0`

ifeq ($(DEBUG),1)
  CFLAGS+=-g
else
  CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -Ds

all: $(OUTPUT)

$(OUTPUT): $(SOURCE)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT)

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
