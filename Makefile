all:
	$(MAKE) -C xml-merge
	$(MAKE) -C xml-trimspace

install:
	$(MAKE) -C xml-merge install
	$(MAKE) -C xml-trimspace install

clean:
	$(MAKE) -C xml-merge clean
	$(MAKE) -C xml-trimspace clean
