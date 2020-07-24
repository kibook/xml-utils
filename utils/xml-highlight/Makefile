.PHONY: docs clean maintainer-clean install uninstall

COMMON=../common

SOURCE=xml-highlight.c $(COMMON)/xml-utils.c

ifeq ($(OS),Windows_NT)
  OUTPUT=xml-highlight.exe
else
  OUTPUT=xml-highlight
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

$(OUTPUT): $(SOURCE) languages.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

languages.h: syntax.xml classes.xml
	> $@ && for f in $+; do xxd -i $$f >> $@; done

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT) languages.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
