SOURCE=xml-trimspace.c
OUTPUT=xml-trimspace

CFLAGS=-Wall -Werror -pedantic-errors -g `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

INSTALL_PREFIX=/usr/local/bin

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
