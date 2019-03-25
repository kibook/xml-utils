TARGETS=utils/xml-*

all docs clean maintainer-clean install uninstall: $(TARGETS)

$(TARGETS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
