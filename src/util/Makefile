# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# Revisions 1.4, 1.6 thru 1.10 Copyright 2005 Sandia Corporation.
# Revisions 1.12 and 1.13      Copyright 2007 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.
#
#
#------------------------------------------------------------------------------
# $Log$
# Revision 1.13  2007/03/23 12:18:52  Fred
# Use CVS Log to generate revision history.
#
# Revision 1.12  2006/02/03 14:33:37  Fred
# Add distclean rule.  They key difference is that now clean won't remove the
# Dependencies file.
# 
# Use the NS variable rather than an explicit "fl".  This is a cleaner
# approach, and makes changing the namespace simpler.
# 
# Revision 1.11  2005/10/14 17:00:43  Fred
# Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.
# 
# Revision 1.10  2005/10/14 16:50:01  Fred
# Add Sandia distribution terms.
# 
# Revision 1.9  2005/09/17 16:13:13  Fred
# Link invert to FL_LIB, thanks to new arrangement of lapack in numeric code. 
# Add Sandia copyright notice, since the lapack change is no compilability
# stuff.  Must add license before release.
# 
# Revision 1.8  2005/09/17 16:06:24  Fred
# Add detail to revision history.
# 
# Revision 1.7  2005/08/06 16:21:58  Fred
# Add header comments about psnr and xv.
# 
# Revision 1.6  2005/08/06 16:19:33  Fred
# Add psnr and xv.
# 
# Revision 1.5  2005/04/23 19:37:40  Fred
# Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
# 11/21.
# 
# Revision 1.4  2005/01/12 05:25:53  rothgang
# cygwin compilation fix: Add .exe suffix to programs when appropriate.
# 
# Revision 1.3  2004/09/08 17:15:19  rothgang
# Remove Dependencies for clean.  (Dependencies are not really needed for a
# clean build, because all .o files are missing.  Not having Dependencies makes
# distribution easier for user.)
# 
# Revision 1.2  2004/07/22 15:19:24  rothgang
# Add vread().
# 
# Don't remove Dependencies file when make clean.
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
