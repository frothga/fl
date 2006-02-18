# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# 12/2004 Fred Rothganger -- Clean MSVC intermediate files.
# 02/2006 Fred Rothganger -- Build shared libraries.  Add distclean.
#         Rename SocketStream and add Listener


SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config

MODULE = Net
VPATH = $(SOURCE_LIB)
LOADLIBS =

OBJS = SocketStream.o Listener.o

$(LIBPREF)$(MODULE)$(LIBSUFF): $(OBJS)
	$(LIBCOMMAND)


test: test.o $(OBJS)
	g++ -o $@ $^ -lpthread


clean:
	rm -f *~ *.o core core.* $(SOURCE_LIB)/$(LIBPREF)$(MODULE)*
	rm -rf Debug Release
	rm -f test$(EXESUF)

distclean: clean
	rm -f Dependencies

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies