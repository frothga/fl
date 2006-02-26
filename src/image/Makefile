# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# 12/2004 Fred Rothganger -- Clean MSVC intermediate files.
# 02/2005 Fred Rothganger -- Add ImageCache.
# 06/2005 Fred Rothganger -- Add reference to Freetype2.  Remove reference to
#         CGAL.  Add InterestMSER.
# Revisions Copyright 2005 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.
#
#
# 01/2006 Fred Rothganger -- Move PixelFormat clases to separate file.
# 02/2006 Fred Rothganger -- Build shared libraries.  Add distclean.
#         Change Image structure.


SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config

MODULE = Image
CPPFLAGS += $(X_INC) $(MPEG_INC) $(FT_INC)
VPATH = $(SOURCE_LIB)
LOADLIBS = -L$(SOURCE_LIB) -l$(NS)Base -l$(NS)Numeric $(MPEG_LIB) $(TIFF_LIB) $(JPEG_LIB) $(FT_LIB)


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
	ImageFileFormatMatlab.o

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


test: test.o $(OBJS)
	g++ -o $@ $^ -L$(SOURCE_LIB) -l$(NS)Base -l$(NS)Numeric -l$(NS)X $(X_LIB) -lpthread $(JPEG_LIB) $(TIFF_LIB) $(LAPACK_LIB) $(MPEG_LIB) $(FT_LIB)

test2: test2.o $(OBJS)
	g++ -o $@ $^ -L$(SOURCE_LIB) -l$(NS)Base -l$(NS)Numeric -l$(NS)X $(X_LIB) -lpthread $(JPEG_LIB) $(TIFF_LIB) $(LAPACK_LIB) $(MPEG_LIB) $(FT_LIB)


clean:
	rm -f *~ *.o core core.* $(SOURCE_LIB)/$(LIBPREF)$(MODULE)*
	rm -rf Debug Release
	rm -f test$(EXESUF) test2$(EXESUF)

distclean: clean
	rm -f Dependencies

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies