SOURCE=DMC-XMLUTILS-A-03-00-00-00A-040A-D_EN-CA.XML

MAN_PREFIX=/usr/local/share/man/man1

all: xml-highlight.1

../README.md: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github -s -o ../README.md

xml-highlight.1: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s -o xml-highlight.1

install: xml-highlight.1
	cp xml-highlight.1 $(MAN_PREFIX)/

clean:
	rm -f xml-highlight.1
