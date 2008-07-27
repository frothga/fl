# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# Copyright 2005, 2008 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.


SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


FL_LIB = -L$(SOURCE_LIB) -l$(NS)Base -l$(NS)Image -l$(NS)Numeric -l$(NS)X


VPATH = $(SOURCE_BIN)

PROGRAMS := $(patsubst %.cc,%$(EXESUF),$(wildcard *.cc))

default: all
all: $(PROGRAMS)

%$(EXESUF): %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@

crypt$(EXESUF): crypt.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ -lcrypt

invert$(EXESUF): invert.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ $(FL_LIB) $(LAPACK_LIB)

psnr$(EXESUF): psnr.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ $(FL_LIB) $(MPEG_LIB) $(MPEG_INC)

vread$(EXESUF): vread.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ $(FL_LIB) $(MPEG_LIB) $(MPEG_INC) $(X_LIB) $(JPEG_LIB) $(X_INC)

xv$(EXESUF): xv.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ $(FL_LIB) $(X_LIB) $(GTIFF_LIB) $(TIFF_LIB) $(JPEG_LIB) $(PNG_LIB) $(LAPACK_LIB) $(X_INC)


clean:
	rm -f *~ *.o core core.* $(patsubst %,$(SOURCE_BIN)/%,$(PROGRAMS))

distclean: clean
	rm -f Dependencies

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies
