# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# 12/2004 Fred Rothganger -- Clean MS Visual Studio files
# 08/2005 Fred Rothganger -- Put quotes around path names for sake of Cygwin
# Revisions Copyright 2005 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.
#
#
# 02/2006 Fred Rothganger -- Add distclean.


SOURCE_DIR := $(CURDIR)
-include Config


# Some programs in util require the libraries to be built first.
PACKAGES = base net numeric image X util

default: all
all: $(PACKAGES)

.DEFAULT:
	$(MAKE) -C src/$@


.PHONY: default all doc clean install depend tar changens

doc:
	doxygen

topclean:
	rm -f *~ include/$(NS)/*~ $(NS).tgz changens.sed
	rm -f *.ncb *.suo lib/*.lib
	rm -rf html man

clean: topclean
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE clean; done

distclean: topclean
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE distclean; done

depend:
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE depend; done

install-headers:
	mkdir -p "$(INSTALL_INC)"/$(NS)
	cp -Ruv "$(SOURCE_INC)"/$(NS)/*.h "$(INSTALL_INC)"/$(NS)

install-libs:
	mkdir -p "$(INSTALL_LIB)"
	cp -Ruv "$(SOURCE_LIB)"/* "$(INSTALL_LIB)"

install-bin:
	mkdir -p "$(INSTALL_BIN)"
	cp -Ruv "$(SOURCE_BIN)"/* "$(INSTALL_BIN)"
	find "$(SOURCE_DIR)"/script -maxdepth 1 -not \( -name "*~" -or -type d \) -exec cp -v \{\} "$(INSTALL_BIN)" \;

install-doc:
	mkdir -p "$(INSTALL_HTML)"
	cp -Ruv "$(SOURCE_HTML)"/* "$(INSTALL_HTML)"
	mkdir -p "$(INSTALL_MAN)"
	cp -Ruv "$(SOURCE_MAN)"/* "$(INSTALL_MAN)"

install: all install-headers install-libs install-bin #install-doc

tar: distclean
	tar -czvf $(NS).tgz --exclude=*.tgz --exclude=CVS --exclude=private --exclude=mswin --no-anchored -C .. $(NS)

# If you prefer to place the library in a different namespace,
# set OLDNS and NS in Config appropriately, then do "make changens".
changens: distclean
	sed -e s/oldns/$(OLDNS)/g -e s/ns/$(NS)/g < changens.template > changens.sed
	for PACKAGE in $(PACKAGES); do cd $(SOURCE_DIR)/src/$$PACKAGE; ../../changens.sh; done
	(cd $(SOURCE_DIR)/include; mv $(OLDNS) $(NS); exit 0)
	cd $(SOURCE_DIR)/include/$(NS);	../../changens.sh
	for PACKAGE in $(PACKAGES); do $(MAKE) -C src/$$PACKAGE depend; done
