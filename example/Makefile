XML_HIGHLIGHT=xml-highlight
XSLTPROC=xsltproc
S1KD2PDF=s1kd2pdf

all: s1000d.pdf docbook.pdf

s1000d.pdf: s1000d.xml
	$(XML_HIGHLIGHT) s1000d.xml | $(S1KD2PDF) --out s1000d.pdf -

docbook.pdf: docbook.xml docbook.xsl
	$(XML_HIGHLIGHT) docbook.xml | $(XSLTPROC) docbook.xsl - | fop - docbook.pdf

clean:
	rm -f s1000d.pdf docbook.pdf
