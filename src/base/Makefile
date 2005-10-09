# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# 12/2004 Fred Rothganger -- Clean MSVC intermediate files.


SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


VPATH = $(SOURCE_LIB)

libflBase.a: random.o Parameters.o
	ar rcsv $(SOURCE_LIB)/$@ $^

clean:
	rm -f *~ *.o core core.* Dependencies $(SOURCE_LIB)/libflBase.a
	rm -rf Debug Release

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies