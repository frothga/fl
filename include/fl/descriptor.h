#ifndef descriptor_h
#define descriptor_h


#include "fl/convolve.h"
#include "fl/matrix.h"
#include "fl/point.h"

#include <iostream>
#include <vector>


namespace fl
{
  // Generic Descriptor interface ---------------------------------------------

  class Descriptor
  {
  public:
	static Descriptor * factory (std::istream & stream);  // Construct a new Descriptor from the stream.  The exact class depends on the data in the stream.

	virtual ~Descriptor ();

	virtual Vector<float> value (const Image & image, const Point & point);  // Calls PointAffine version with default values for scale, angle and shape.
	virtual Vector<float> value (const Image & image, const PointInterest & point);  // Calls PointAffine version with default values for angle and shape.
	virtual Vector<float> value (const Image & image, const PointAffine & point) = 0;  // Returns a vector of floats that describe the image patch near the interest point.
	virtual Image patch (const Vector<float> & value) = 0;  // Return a graphical representation of the descriptor.  Preferrably an image patch that would stimulate this descriptor to return the given value.

	virtual void read (std::istream & stream) = 0;
	virtual void write (std::ostream & stream, bool withName = true) = 0;
  };


  // Specific Descriptors -----------------------------------------------------

  class DescriptorFilters : public Descriptor
  {
  public:
	DescriptorFilters ();
	DescriptorFilters (std::istream & stream);
	virtual ~DescriptorFilters ();  

	void prepareFilterMatrix ();

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	std::vector<Convolution2D> filters;
	Matrix<float> filterMatrix;
	int patchWidth;
	int patchHeight;
  };

  class DescriptorFiltersTexton : public DescriptorFilters
  {
  public:
	DescriptorFiltersTexton (int angles = 6, int scales = 4, float firstScale = -1, float scaleStep = -1);
  };

  class DescriptorPatch : public Descriptor
  {
  public:
	DescriptorPatch (int width = 10, float supportRadial = 4.2);
	DescriptorPatch (std::istream & stream);
	virtual ~DescriptorPatch ();

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int width;
	float supportRadial;
  };

  class DescriptorSchmidScale : public Descriptor
  {
  public:
	DescriptorSchmidScale (float sigma = 1.0);
	DescriptorSchmidScale (std::istream & stream);
	void initialize ();
	virtual ~DescriptorSchmidScale ();

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	float sigma;
	Convolution2D G;
	Convolution2D Gx;
	Convolution2D Gy;
	Convolution2D Gxx;
	Convolution2D Gxy;
	Convolution2D Gyy;
	Convolution2D Gxxx;
	Convolution2D Gxxy;
	Convolution2D Gxyy;
	Convolution2D Gyyy;
  };

  class DescriptorSchmid : public Descriptor
  {
  public:
	DescriptorSchmid (int scaleCount = 8, float scaleStep = -1);
	DescriptorSchmid (std::istream & stream);
	void initialize (int scaleCount);
	virtual ~DescriptorSchmid ();

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	DescriptorSchmidScale * findScale (float sigma);

	float scaleStep;
	std::vector<DescriptorSchmidScale *> descriptors;
  };

  class DescriptorSpin : public Descriptor
  {
  public:
	DescriptorSpin (int binsRadial = 5, int binsIntensity = 6, float supportRadial = 3, float supportIntensity = 3);
	DescriptorSpin (std::istream & stream);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	// Subroutines for value ()
	virtual void rangeMinMax (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float & minIntensity, float & quantum);  // Finds intensity range based on simple min and max pixel values
	virtual void rangeMeanDeviation (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float & minIntensity, float & quantum);  // Finds intensity range using mean and standard deviation of pixel values
	virtual void doBinning (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float minIntensity, float quantum, float binRadius, Vector<float> & result);  // Perform the actually binning process

	int   binsRadial;
	int   binsIntensity;
	float supportRadial;
	float supportIntensity;  // Number of standard deviations away from avarge intensity.
  };

  class DescriptorSpinSimple : public DescriptorSpin
  {
  public:
	DescriptorSpinSimple (int binsRadial = 5, int binsIntensity = 6, float supportRadial = 3, float supportIntensity = 3);

	virtual void rangeMinMax (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float & minIntensity, float & quantum);
	virtual void rangeMeanDeviation (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float & minIntensity, float & quantum);
	virtual void doBinning (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float minIntensity, float quantum, float binRadius, Vector<float> & result);
  };

  class DescriptorSpinExact : public DescriptorSpin
  {
  public:
	DescriptorSpinExact (int binsRadial = 5, int binsIntensity = 6, float supportRadial = 3, float supportIntensity = 3);

	virtual void doBinning (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float minIntensity, float quantum, float binRadius, Vector<float> & result);
  };
}


#endif
