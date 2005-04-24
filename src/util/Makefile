# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
# for details.
#
#
# 12/2004 Revised by Fred Rothganger


SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


FL_LIB = -L$(SOURCE_LIB) -lflBase -lflImage -lflNumeric -lflX


VPATH = $(SOURCE_BIN)

PROGRAMS := $(patsubst %.cc,%$(EXESUF),$(wildcard *.cc))

default: all
all: $(PROGRAMS)

%$(EXESUF): %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@

crypt$(EXESUF): crypt.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ -lcrypt

invert$(EXESUF): invert.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ $(LAPACK_LIB)

vread$(EXESUF): vread.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ $(FL_LIB) $(MPEG_LIB) $(MPEG_INC) $(X_LIB) $(JPEG_LIB)


clean:
	rm -f *~ *.o core core.* Dependencies $(patsubst %,$(SOURCE_BIN)/%,$(PROGRAMS))

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies