SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


CPPFLAGS += $(X_INC) $(MPEG_INC) # $(CGAL_INC)
VPATH = $(SOURCE_LIB)


IMAGE_O = \
	Image.o \
	Pixel.o \
	Point.o

IMAGE_FILE_FORMAT_O = \
	ImageFileFormat.o \
	ImageFileFormatPGM.o \
	ImageFileFormatEPS.o \
	ImageFileFormatJPEG.o \
	ImageFileFormatTIFF.o

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
	FilterHarris.o \
	FilterHarrisEigen.o \
	NonMaxSuppress.o \
	Normalize.o \
	AbsoluteValue.o \
	Rescale.o \
	Rotate180.o \
	Transform.o \
	TransformGauss.o \
	ClearAlpha.o

DESCRIPTOR_O = \
	Descriptor.o \
	DescriptorFilters.o \
	DescriptorFiltersTexton.o \
	DescriptorPatch.o \
	DescriptorSchmidScale.o \
	DescriptorSchmid.o \
	DescriptorSpin.o \
	DescriptorSpinSimple.o \
	DescriptorSpinExact.o

INTEREST_O = \
	Interest.o \
	InterestHarris.o \
	InterestHarrisLaplacian.o \
	InterestLaplacian.o 

VIDEO_O = \
	Video.o \
	VideoFileFormatFFMPEG.o

libflImage.a: $(IMAGE_O) $(IMAGE_FILE_FORMAT_O) $(CANVAS_O) $(CONVOLVE_O) $(DESCRIPTOR_O) $(INTEREST_O) $(VIDEO_O)
	ar rcsv $(SOURCE_LIB)/$@ $^

DescriptorSpin.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $*.cc -ffloat-store


test: test.o $(IMAGE_O) $(IMAGE_FILE_FORMAT_O) $(CONVOLVE_O)
	g++ -o $@ $^ -L../../lib -lflNumeric -lflX $(X_LIB) -lpthread $(JPEG_LIB) $(TIFF_LIB) $(LAPACK_LIB) $(MPEG_LIB) # -lefence


clean:
	rm -f *~ *.o core core.* Dependencies test $(SOURCE_LIB)/libflImage.a

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies