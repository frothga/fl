SOURCE_DIR := $(CURDIR)
-include Config


PACKAGES = util base net image X numeric

default: all
all: depend $(PACKAGES)

.DEFAULT:
	$(MAKE) -C src/$@


.PHONY: default all clean install depend tar changens

clean:
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE clean; done
	rm -f *~ include/$(NS)/*~ $(NS).tgz changens.sed

depend:
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE depend; done

install-files:
	mkdir -p $(INSTALL_BIN)
	cp -Rv $(SOURCE_BIN)/* $(INSTALL_BIN)
	find $(SOURCE_DIR)/script -not \( -name "*~" -or -type d \) -exec cp -v \{\} $(INSTALL_BIN) \;
	mkdir -p $(INSTALL_LIB)
	cp -Rv $(SOURCE_LIB)/* $(INSTALL_LIB)
	mkdir -p $(INSTALL_INC)/$(NS)
	cp -Rv $(SOURCE_INC)/$(NS)/*.h $(INSTALL_INC)/$(NS)

install: all install-files

tar: clean
	tar -czvf $(NS).tgz -C .. $(notdir $(SOURCE_DIR))

# If you prefer to place the library in a different namespace,
# set OLDNS and NS in Config appropriately, then do "make changens".
changens: clean
	sed -e s/oldns/$(OLDNS)/g -e s/ns/$(NS)/g < changens.template > changens.sed
	for PACKAGE in $(PACKAGES); do cd $(SOURCE_DIR)/src/$$PACKAGE; ../../changens.sh; done
	(cd $(SOURCE_DIR)/include; mv $(OLDNS) $(NS); exit 0)
	cd $(SOURCE_DIR)/include/$(NS);	../../changens.sh
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE depend; done
