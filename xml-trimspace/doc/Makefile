SOURCE=DMC-XMLUTILS-A-02-00-00-00A-040A-D_EN-CA.XML
OUTPUT=xml-trimspace.1

PREFIX=/usr/local
MAN_PREFIX=$(PREFIX)/share/man/man1

all: $(OUTPUT) ../README.md

$(OUTPUT): $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s > $(OUTPUT)

../README.md: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github > ../README.md

install: $(OUTPUT)
	cp $(OUTPUT) $(MAN_PREFIX)/

clean:
	rm -f $(OUTPUT)
