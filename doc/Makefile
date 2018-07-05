SOURCE=DMC-XMLUTILS-A-03-00-00-00A-040A-D_EN-CA.XML
OUTPUT=xml-highlight.1
README=../README.md

PREFIX=/usr/local
MAN_PREFIX=$(PREFIX)/share/man/man1
INSTALL=install

all: $(OUTPUT) $(README)

$(README): $(SOURCE)
	s1kd2db $^ | pandoc -f docbook -t markdown_github -s -o $@

$(OUTPUT): $(SOURCE)
	s1kd2db $^ | pandoc -f docbook -t man -s -o $@

.PHONY: clean install uninstall

clean:
	rm -f $(OUTPUT) $(README)

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(MAN_PREFIX)

uninstall:
	rm -f $(MAN_PREFIX)/$(OUTPUT)
