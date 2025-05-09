.PHONY: docs clean maintainer-clean install uninstall

common_dir = ../common

sources = xml-transform.c $(common_dir)/xml-utils.c

ifeq ($(OS),Windows_NT)
  executable = xml-transform.exe
else
  executable = xml-transform
endif

warning_flags = -Wall -Werror -pedantic-errors

CFLAGS = $(warning_flags) -I $(common_dir) `pkg-config --cflags libxml-2.0 libxslt libexslt`

ifeq ($(debug),1)
  CFLAGS += -g
else
  CFLAGS += -O3
endif

LDFLAGS = `pkg-config --libs libxml-2.0 libxslt libexslt`

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

INSTALL = install -Ds

all: $(executable)

identity.h: identity.xsl
	xxd -i $< > $@

null-input.h: null-input.xml
	xxd -i $< > $@

$(executable): $(sources) identity.h null-input.h
	$(CC) $(CFLAGS) -o $@ $(sources) $(LDFLAGS)

docs:
	$(MAKE) -C doc

clean:
	$(RM) $(executable) identity.h null-input.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(executable)
	$(INSTALL) $(executable) $(bindir)/$(executable)
	$(MAKE) -C doc install

uninstall:
	$(RM) $(bindir)/$(executable)
	$(MAKE) -C doc uninstall
