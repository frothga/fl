SOURCE_DIR := $(CURDIR)
-include Config


# Some programs in util require the libraries to be built first.
PACKAGES = base net image X numeric util

default: all
all: $(PACKAGES)

.DEFAULT:
	$(MAKE) -C src/$@


.PHONY: default all doc clean install depend tar changens

doc:
	doxygen

clean:
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE clean; done
	rm -f *~ include/$(NS)/*~ $(NS).tgz changens.sed
	rm -f *.ncb lib/*.lib
	rm -rf html man

depend:
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE depend; done

install-headers:
	mkdir -p $(INSTALL_INC)/$(NS)
	cp -Ruv $(SOURCE_INC)/$(NS)/*.h $(INSTALL_INC)/$(NS)

install-libs:
	mkdir -p $(INSTALL_LIB)
	cp -Ruv $(SOURCE_LIB)/* $(INSTALL_LIB)

install-bin:
	mkdir -p $(INSTALL_BIN)
	cp -Ruv $(SOURCE_BIN)/* $(INSTALL_BIN)
	find $(SOURCE_DIR)/script -not \( -name "*~" -or -type d \) -exec cp -v \{\} $(INSTALL_BIN) \;

install-doc:
	mkdir -p $(INSTALL_HTML)
	cp -Ruv $(SOURCE_HTML)/* $(INSTALL_HTML)
	mkdir -p $(INSTALL_MAN)
	cp -Ruv $(SOURCE_MAN)/* $(INSTALL_MAN)

install: all install-headers install-libs install-bin #install-doc

tar: clean
	tar -czvf $(NS).tgz -C .. $(notdir $(SOURCE_DIR)) --exclude=*.tgz --exclude=ideas --exclude=CVS --no-anchored

# If you prefer to place the library in a different namespace,
# set OLDNS and NS in Config appropriately, then do "make changens".
changens: clean
	sed -e s/oldns/$(OLDNS)/g -e s/ns/$(NS)/g < changens.template > changens.sed
	for PACKAGE in $(PACKAGES); do cd $(SOURCE_DIR)/src/$$PACKAGE; ../../changens.sh; done
	(cd $(SOURCE_DIR)/include; mv $(OLDNS) $(NS); exit 0)
	cd $(SOURCE_DIR)/include/$(NS);	../../changens.sh
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE depend; done
