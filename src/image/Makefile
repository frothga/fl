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


SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


CPPFLAGS += $(X_INC) $(MPEG_INC) $(FT_INC)
VPATH = $(SOURCE_LIB)


IMAGE_O = \
	Image.o \
	Pixel.o \
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

libflImage.a: $(IMAGE_O) $(IMAGE_FILE_FORMAT_O) $(CANVAS_O) $(CONVOLVE_O) $(DESCRIPTOR_O) $(INTEREST_O) $(VIDEO_O)
	ar rcsv $(SOURCE_LIB)/$@ $^


test: test.o $(IMAGE_O) $(IMAGE_FILE_FORMAT_O) $(INTEREST_O) $(CONVOLVE_O) $(VIDEO_O) $(CANVAS_O) $(DESCRIPTOR_O)
	g++ -o $@ $^ -L../../lib -lflBase -lflNumeric -lflX $(X_LIB) -lpthread $(JPEG_LIB) $(TIFF_LIB) $(LAPACK_LIB) $(MPEG_LIB) $(FT_LIB)


clean:
	rm -f *~ *.o core core.* test Dependencies $(SOURCE_LIB)/libflImage.a
	rm -rf Debug Release

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies