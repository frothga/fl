# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# Revisions 1.4 and 1.6   Copyright 2005 Sandia Corporation.
# Revisions 1.8 thru 1.11 Copyright 2007 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.
#
#
#------------------------------------------------------------------------------
# $Log$
# Revision 1.11  2007/03/23 12:15:10  Fred
# Use CVS Log to generate revision history.
#
# Revision 1.10  2006/02/17 17:59:00  Fred
# Add -lpthread to test program.  Without this, it will link successfully on
# Linux, but then core dump at runtime when starting a service thread.
# 
# Revision 1.9  2006/02/14 05:55:27  Fred
# Rename socket.cc to SocketStream.cc.  Add Listener.cc, which implements a
# basic TCP server.
# 
# Revision 1.8  2006/02/03 14:30:18  Fred
# Change build system to handle shared libraries as well as static archives.
# 
# Add distclean rule.  They key difference is that now clean won't remove the
# Dependencies file.
# 
# Revision 1.7  2005/10/13 03:28:02  Fred
# Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.
# 
# Revision 1.6  2005/10/13 03:24:15  Fred
# Add detail to revision history.
# 
# Revision 1.5  2005/04/23 19:37:04  Fred
# Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
# 11/21.
# 
# Revision 1.4  2005/01/22 21:00:03  Fred
# MSVC compilability fix:  Clean intermediate files.
# 
# Revision 1.3  2004/09/08 17:14:33  rothgang
# Remove Dependencies for clean.  (Dependencies are not really needed for a
# clean build, because all .o files are missing.  Not having Dependencies makes
# distribution easier for user.)
# 
# Revision 1.2  2004/07/22 15:37:49  rothgang
# Add special linking for test.
# 
# Don't delete Dependencies when make clean.
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

