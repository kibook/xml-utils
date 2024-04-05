targets = utils/xml-*

all docs clean maintainer-clean install uninstall: $(targets)

$(targets)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
