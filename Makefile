TARGETS=xml-merge xml-trimspace

all clean install uninstall: $(TARGETS)

$(TARGETS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
