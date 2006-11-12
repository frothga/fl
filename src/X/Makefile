# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# 02/2006 Fred Rothganger -- Build shared libraries.  Add distclean.


SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config

MODULE = X
CPPFLAGS += $(X_INC)
VPATH = $(SOURCE_LIB)
LOADLIBS = -L$(SOURCE_LIB) -l$(NS)Image $(X_LIB)

OBJS = X.o glx.o SlideShow.o GLShow.o

$(LIBPREF)$(MODULE)$(LIBSUFF): $(OBJS)
	$(LIBCOMMAND)


test.o: test.cc
	g++ -c test.cc -g $(CPPFLAGS)

test: test.o $(OBJS)
	g++ -o $@ $^ -L../../lib -lflImage -lflNumeric $(X_LIB) -lpthread -lefence


clean:
	rm -f *~ *.o core core.* *$(EXESUF).core $(SOURCE_LIB)/$(LIBPREF)$(MODULE)*
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