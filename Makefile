SOURCE=s1kd-highlight.c
OUTPUT=s1kd-highlight

CFLAGS=-Wall -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

INSTALL_PREFIX=/usr/local/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) languages.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

languages.h: syntax.xml classes.xml
	xxd -i classes.xml > languages.h
	xxd -i syntax.xml >> languages.h

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

clean:
	rm -f $(OUTPUT) languages.h
	$(MAKE) -C doc clean
