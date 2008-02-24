# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# Revisions 1.6 and 1.8    Copyright 2005 Sandia Corporation.
# Revisions 1.10 thru 1.21 Copyright 2008 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.
#
#
#------------------------------------------------------------------------------
# $Log$
# Revision 1.21  2008/02/24 14:15:37  Fred
# Change link order for JPEG lib.
#
# Revision 1.20  2007/08/26 12:18:48  Fred
# Fix rules for test and junk to link the library file rather than all the
# object files separately.  The individual objects form seems not to link
# correctly under the current version of Cygwin.
#
# Revision 1.19  2007/03/23 12:13:18  Fred
# Use CVS Log to generate revision history.
#
# Revision 1.18  2006/11/12 15:23:23  Fred
# Reorder libraries to deal with dependency of tiff on jpeg.
# 
# Revision 1.17  2006/03/20 05:35:16  Fred
# Fix link order for geotiff.
# 
# Revision 1.16  2006/03/16 03:37:00  Fred
# Fix order of references to tiff lib and geotiff lib to avoid errors when
# linking statically.
# 
# Revision 1.15  2006/03/01 04:03:52  Fred
# Basic support for GeoTiff.  Add ImageFileFormatNITF.
# 
# Rename test2 to junk.  Remove Cygwin core files.
# 
# Revision 1.14  2006/02/26 03:11:02  Fred
# Rename test2 to junk.
# 
# Revision 1.13  2006/02/25 22:42:25  Fred
# Change image structure by encapsulating storage format in a new PixelBuffer
# class.
# 
# Make "test.cc" into a real regression test, with self-verification.  Rename
# junk test program to test2.cc
# 
# Revision 1.12  2006/02/15 14:11:09  Fred
# Delete test program using $(EXESUF).
# 
# Revision 1.11  2006/02/03 14:29:54  Fred
# Change build system to handle shared libraries as well as static archives.
# 
# Add distclean rule.  They key difference is that now clean won't remove the
# Dependencies file.
# 
# Revision 1.10  2006/01/06 18:04:22  Fred
# Move PixelFormat classes into separate file.  Actually, PixelFormat is much
# more important than Pixel.  The main reason for the move is symmetry with a
# soon-to-be-added set of classes called PixelBuffer.
# 
# Revision 1.9  2005/10/13 03:22:35  Fred
# Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.
# 
# Revision 1.8  2005/10/09 05:39:48  Fred
# Add ImageCache and use freetype 2 library.
# 
# Update revision history and add Sandia copyright notice.
# 
# Revision 1.7  2005/04/23 19:36:46  Fred
# Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
# 11/21.
# 
# Revision 1.6  2005/01/22 20:59:47  Fred
# MSVC compilability fix:  Clean intermediate files.
# 
# Revision 1.5  2004/09/08 17:13:44  rothgang
# Remove Dependencies for clean.  (Dependencies are not really needed for a
# clean build, because all .o files are missing.  Not having Dependencies makes
# distribution easier for user.)
# 
# Revision 1.4  2004/07/22 15:34:20  rothgang
# Get rid of extra spin image classes.  Add several new filters and
# descriptors.  Add KLT class for tracking points in video.
# 
# Revision 1.3  2003/12/30 16:19:58  rothgang
# More descriptors; new interest operator; new image file format.
# 
# Revision 1.2  2003/09/07 22:17:56  rothgang
# Add several new classes, mainly to support InterestHessian.
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

MODULE = Image
CPPFLAGS += $(X_INC) $(MPEG_INC) $(FT_INC) $(GTIFF_INC)
VPATH = $(SOURCE_LIB)
LOADLIBS = -L$(SOURCE_LIB) -l$(NS)Base -l$(NS)Numeric $(MPEG_LIB) $(JPEG_LIB) $(FT_LIB) $(GTIFF_LIB) $(TIFF_LIB)


IMAGE_O = \
	Image.o \
	Pixel.o \
	PixelFormat.o \
	PixelBuffer.o \
	Point.o \
	ImageCache.o

IMAGE_FILE_FORMAT_O = \
	ImageFileFormat.o \
	ImageFileFormatPGM.o \
	ImageFileFormatEPS.o \
	ImageFileFormatJPEG.o \
	ImageFileFormatTIFF.o \
	ImageFileFormatMatlab.o \
	ImageFileFormatNITF.o

CANVAS_O = \
	Canvas.o \
	CanvasImage.o \
	CanvasPS.o

CONVOLVE_O = \
	Convolution2D.o \
	Gaussian2D.o \
	DifferenceOfGaussians.o \
	GaussianDerivativeFirst.o \
	GaussianDerivativeSecond.o \
	GaussianDerivativeThird.o \
	Laplacian.o \
	Convolution1D.o \
	Gaussian1D.o \
	GaussianDerivative1D.o \
	GaussianDerivativeSecond1D.o \
	FilterHarris.o \
	FilterHarrisEigen.o \
	FilterHessian.o \
	FiniteDifference.o \
	NonMaxSuppress.o \
	IntensityAverage.o \
	IntensityDeviation.o \
	IntensityHistogram.o \
	Normalize.o \
	AbsoluteValue.o \
	Rescale.o \
	Rotate90.o \
	Rotate180.o \
	Transform.o \
	TransformGauss.o \
	Zoom.o \
	ClearAlpha.o

DESCRIPTOR_O = \
	Comparison.o \
	Descriptor.o \
	DescriptorCombo.o \
	DescriptorScale.o \
	DescriptorOrientation.o \
	DescriptorOrientationHistogram.o \
	DescriptorContrast.o \
	DescriptorFilters.o \
	DescriptorFiltersTexton.o \
	DescriptorPatch.o \
	DescriptorSchmidScale.o \
	DescriptorSchmid.o \
	DescriptorSpin.o \
	DescriptorSIFT.o \
	DescriptorColorHistogram2D.o \
	DescriptorColorHistogram3D.o \
	DescriptorTextonScale.o \
	DescriptorLBP.o

INTEREST_O = \
	Interest.o \
	InterestHarris.o \
	InterestHarrisLaplacian.o \
	InterestLaplacian.o \
	InterestHessian.o \
	InterestDOG.o \
	InterestMSER.o

VIDEO_O = \
	Video.o \
	VideoFileFormatFFMPEG.o \
	KLT.o

OBJS = $(IMAGE_O) $(IMAGE_FILE_FORMAT_O) $(CANVAS_O) $(CONVOLVE_O) $(DESCRIPTOR_O) $(INTEREST_O) $(VIDEO_O)

$(LIBPREF)$(MODULE)$(LIBSUFF): $(OBJS)
	$(LIBCOMMAND)


test: test.o $(LIBPREF)$(MODULE)$(LIBSUFF)
	g++ -o $@ -L$(SOURCE_LIB) $^ -l$(NS)Base -l$(NS)Image -l$(NS)Numeric -l$(NS)X $(X_LIB) -lpthread $(JPEG_LIB) $(LAPACK_LIB) $(MPEG_LIB) $(FT_LIB) $(GTIFF_LIB) $(TIFF_LIB)

junk: junk.o $(LIBPREF)$(MODULE)$(LIBSUFF)
	g++ -o $@ -L$(SOURCE_LIB) $^ -l$(NS)Base -l$(NS)Image -l$(NS)Numeric -l$(NS)X $(X_LIB) -lpthread $(JPEG_LIB) $(LAPACK_LIB) $(MPEG_LIB) $(FT_LIB) $(GTIFF_LIB) $(TIFF_LIB)


clean:
	rm -f *~ *.o core core.* *.core $(SOURCE_LIB)/$(LIBPREF)$(MODULE)*
	rm -rf Debug Release
	rm -f test$(EXESUF) junk$(EXESUF)

distclean: clean
	rm -f Dependencies

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

