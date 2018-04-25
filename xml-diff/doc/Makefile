SOURCE=DMC-XMLUTILS-A-04-00-00-00A-040A-D_EN-CA.XML

MAN_PREFIX=/usr/local/share/man/man1

all: xml-diff.1

../README.md: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github -s -o ../README.md

xml-diff.1: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s -o xml-diff.1

install: xml-diff.1
	cp xml-diff.1 $(MAN_PREFIX)/

clean:
	rm -f xml-diff.1
