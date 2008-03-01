# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# Revisions 1.6 thru 1.9 Copyright 2007 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.
#
#
#------------------------------------------------------------------------------
# $Log$
# Revision 1.9  2007/03/23 12:07:20  Fred
# Use CVS Log to generate revision history.
#
# Revision 1.8  2006/11/12 15:08:39  Fred
# Clean out Cygwin core files.
# 
# Revision 1.7  2006/02/15 14:04:37  Fred
# Use $(OBJS) when building test program.  Delete frile using $(EXESUF).
# 
# Revision 1.6  2006/02/03 14:29:00  Fred
# Change build system to handle shared libraries as well as static archives.
# 
# Add distclean rule.  They key difference is that now clean won't remove the
# Dependencies file.
# 
# Revision 1.5  2005/10/09 04:04:47  Fred
# Put UIUC license in the file LICENSE rather than LICENSE-UIUC.
# 
# Revision 1.4  2005/04/23 19:38:35  Fred
# Add UIUC copyright notice.
# 
# Revision 1.3  2004/09/08 17:14:00  rothgang
# Remove Dependencies for clean.  (Dependencies are not really needed for a
# clean build, because all .o files are missing.  Not having Dependencies makes
# distribution easier for user.)
# 
# Revision 1.2  2004/07/22 15:11:09  rothgang
# Don't kill Dependencies file during make clean.
# 
# Revision 1.1  2003/07/08 23:19:47  rothgang
# branches:  1.1.1;
# Initial revision
# 
# Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
# Imported sources
#------------------------------------------------------------------------------


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
