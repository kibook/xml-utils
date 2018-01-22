SOURCE=DMC-XMLUTILS-A-02-00-00-00A-040A-D_EN-CA.XML
OUTPUT=xml-trimspace.1

MAN_PREFIX=/usr/local/share/man/man1

$(OUTPUT): $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s > $(OUTPUT)

install: $(OUTPUT)
	cp $(OUTPUT) $(MAN_PREFIX)/

clean:
	rm -f $(OUTPUT)
