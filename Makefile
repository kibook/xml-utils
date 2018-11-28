TARGETS=xml-merge xml-trimspace xml-highlight

all docs clean maintainer-clean install uninstall: $(TARGETS)

$(TARGETS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
