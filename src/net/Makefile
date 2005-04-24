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


VPATH = $(SOURCE_LIB)

libflNet.a: socket.o
	ar rcsv $(SOURCE_LIB)/$@ $^


test: test.o socket.o
	g++ -o $@ $^


clean:
	rm -f *~ *.o core core.* test Dependencies $(SOURCE_LIB)/libflNet.a
	rm -rf Debug Release

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies