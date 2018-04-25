SOURCE=xml-trimspace.c
OUTPUT=xml-trimspace

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) normalize.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

normalize.h: normalize.xsl
	xxd -i normalize.xsl > normalize.h

clean:
	rm -f $(OUTPUT)
	rm -f normalize.h
	$(MAKE) -C doc clean

install:
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install
