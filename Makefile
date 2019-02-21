TARGETS=xml-format xml-highlight xml-merge xml-trimspace

all docs clean maintainer-clean install uninstall: $(TARGETS)

$(TARGETS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
