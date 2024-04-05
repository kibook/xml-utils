.PHONY: docs clean maintainer-clean install uninstall

common_dir = ../common

sources = xml-validate.c $(common_dir)/xml-utils.c

ifeq ($(OS),Windows_NT)
  executable = xml-validate.exe
else
  executable = xml-validate
endif

warning_flags = -Wall -Werror -pedantic-errors

CFLAGS = $(warning_flags) -I $(common_dir) `pkg-config --cflags libxml-2.0`

ifeq ($(debug),1)
  CFLAGS += -g
else
  CFLAGS += -O3
endif

LDFLAGS = `pkg-config --libs libxml-2.0`

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

INSTALL = install -Ds

all: $(executable)

$(executable): $(sources)
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

docs:
	$(MAKE) -C doc

clean:
	$(RM) $(executable)

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(executable)
	$(INSTALL) $(executable) $(bindir)/$(executable)
	$(MAKE) -C doc install

uninstall:
	$(RM) $(bindir)/$(executable)
	$(MAKE) -C doc uninstall
