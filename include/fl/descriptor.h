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

  /**
	 Finds characteristic scale of point.
  **/
  class DescriptorScale : public Descriptor
  {
  public:
	DescriptorScale (float firstScale = 1, float lastScale = 25, int interQuanta = 40, float quantum = 2);  // quantum is most meaningful as a prime number; 2 means "doubling" or "octaves"
	void initialize (float firstScale, float lastScale, float stepSize);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	std::vector<Laplacian> laplacians;
  };

  /**
	 Finds characteristic angle of point using a pair of large
	 derivative-of-Gaussian kernels.
   **/
  class DescriptorOrientation : public Descriptor
  {
  public:
	DescriptorOrientation (float supportRadial = 6, int supportPixel = 32, float kernelSize = 2.5f);
	void initialize (float supportRadial, int supportPixel, float kernelSize);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int supportPixel;  ///< Pixel radius of patch.  Patch size = 2 * supportPixel + 1.
	float supportRadial;  ///< Number of sigmas away from center to include in patch (where 1 sigma = size of characteristic scale).
	float kernelSize;  ///< Number of sigmas of the Gaussian kernel to cover the radius fo the patch.
	GaussianDerivativeFirst Gx;
	GaussianDerivativeFirst Gy;
  };

  /**
	 Finds characteristic angle of point using a histogram of gradient
	 directions.  Follows David Lowe's approach.
   **/
  class DescriptorOrientationHistogram : public Descriptor
  {
  public:
	DescriptorOrientationHistogram (int bins = 36, float supportRadial = 4.5f, int supportPixel = 16);

	void computeGradient (const Image & image);  ///< Same as DescriptorSIFT::computeGradient().  TODO: combine somehow.

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int bins;  ///< Number of orientation bins in histogram.
	int supportPixel;  ///< Pixel radius of patch, if needed.  Patch size = 2 * supportPixel.
	float supportRadial;  ///< Number of sigmas away from center to include in histogram.
	float cutoff;  ///< Ratio of maximum histogram value above which to accept secondary maxima.

	const Image * lastImage;  ///< For cacheing derivative images.
	ImageOf<float> I_x;
	ImageOf<float> I_y;
  };

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

	std::vector<ConvolutionDiscrete2D> filters;
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
	ConvolutionDiscrete2D G;
	ConvolutionDiscrete2D Gx;
	ConvolutionDiscrete2D Gy;
	ConvolutionDiscrete2D Gxx;
	ConvolutionDiscrete2D Gxy;
	ConvolutionDiscrete2D Gyy;
	ConvolutionDiscrete2D Gxxx;
	ConvolutionDiscrete2D Gxxy;
	ConvolutionDiscrete2D Gxyy;
	ConvolutionDiscrete2D Gyyy;
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

  /**
	 Implements David Lowe's SIFT descriptor.
  **/
  class DescriptorSIFT : public Descriptor
  {
  public:
	DescriptorSIFT (int width = 4, int angles = 8);

	void computeGradient (const Image & image);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int width;  ///< Number of horizontal or vertical positions.
	int angles;  ///< Number of orientation bins.
	float supportRadial;  ///< supportRadial * point.scale gives pixel distance from center to edge of bins when they overlay the image.  The pixel diameter of one bin is 2 * supportPixel * point.scale / width.
	int supportPixel;  ///< Number of pixels in normalized form of affine-invariant patch, if used.
	float sigmaWeight;  ///< Size of Gaussian that weights the entries in the bins.
	float maxValue;  ///< Largest permissible entry in one bin.

	const Image * lastImage;  ///< The image most recently processed to find gradient vectors.  Allows some caching.
	ImageOf<float> I_x;  ///< x component of gradient vectors
	ImageOf<float> I_y;  ///< y component of gradient vectors
  };

  /**
	 Form a 2D color histogram of the UV components in a YUV patch.
  **/
  class DescriptorColorHistogram2D : public Descriptor
  {
  public:
	DescriptorColorHistogram2D (int width = 4, float supportRadial = 4.2f);
	void initialize ();

	// The following are subroutines used by value(), and also available to client code if the programmer wishes to use a different criterion for selecting pixels to bin.
	void clear ();  ///< Zero out histogram in preparation for a round of binning.
	void addToHistogram (const Image & image, const int x, const int y);  ///< An inline used by both add() and value().  Contains the common code for incrementing color bins.
	void add (const Image & image, int x, int y);  ///< Add color of image(x,y) to histogram.
	Vector<float> finish ();  ///< Extract feature vector from the histogram.  Only returns values for bins that map to a valid RGB color.  See member "valid" below.

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int width;  ///< Number of bins in the U and V dimensions.
	float supportRadial;  ///< Multiple of characteristic scale to use when drawing off a normalized patch.

	Matrix<bool> valid;  ///< Stores true for every bin that translates to a valid RGB color.
	int validCount;  ///< Total number of true entries in valid.  Effectively the dimension of the vector recturned by value().

	Matrix<float> histogram;  ///< Working histogram.  Forces this Descriptor object to be single threaded.
  };

  /**
	 Form a 3D color histogram of the UV components in a YUV patch.
	 If the need arises to use other color spaces, this class could be
	 generalized.
  **/
  class DescriptorColorHistogram3D : public Descriptor
  {
  public:
	DescriptorColorHistogram3D (int width = 4, int height = -1, float supportRadial = 4.2f);  ///< height == -1 means use value of width
	~DescriptorColorHistogram3D ();
	void initialize ();

	// The following are subroutines used by value(), and also available to client code if the programmer wishes to use a different criterion for selecting pixels to bin.
	void clear ();  ///< Zero out histogram in preparation for a round of binning.
	void addToHistogram (const Image & image, const int x, const int y);  ///< An inline used by both add() and value().  Contains the common code for incrementing color bins.
	void add (const Image & image, int x, int y);  ///< Add color of image(x,y) to histogram.
	Vector<float> finish ();  ///< Extract feature vector from the histogram.  Only returns values for bins that map to a valid RGB color.  See member "valid" below.

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int width;  ///< Number of bins in the U and V dimensions.
	int height;  ///< Number of bins in the Y dimension.
	float supportRadial;  ///< Multiple of characteristic scale to use when drawing off a normalized patch.

	bool * valid;  ///< A 3D block of booleans that stores true for every bin that translates to a valid RGB color.
	int validCount;  ///< Total number of true entries in valid.  Effectively the dimension of the vector recturned by value().

	float * histogram;  ///< Working histogram.  Forces this Descriptor object to be single threaded.
  };
}


#endif
