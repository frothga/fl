#ifndef fl_descriptor_h
#define fl_descriptor_h


#include "fl/convolve.h"
#include "fl/matrix.h"
#include "fl/point.h"

#include <iostream>
#include <vector>

// for debugging
#include "fl/slideshow.h"


namespace fl
{
  // Comparison ---------------------------------------------------------------

  /**
	 Takes two feature vectors and returns the probability that they match.
	 Probability values are in [0,1].
  **/
  class Comparison
  {
  public:
	virtual ~Comparison ();  ///< Establishes that destructor is virtual, but doesn't do anything else.

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const = 0;
  };

  class Descriptor;

  /**
	 Handles comparisons between feature vectors that are composed of several
	 smaller feature vectors from various descriptors.
   **/
  class ComparisonCombo : public Comparison
  {
  public:
	ComparisonCombo (std::vector<Descriptor *> & descriptors);
	virtual ~ComparisonCombo ();  ///< Delete all comparisons we are holding.

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;

	std::vector<Comparison *> comparisons;
	std::vector<int> dimensions;
	int totalDimension;
  };

  /**
	 Uses the correlation value, in range [-1,1], after normalizing each
	 vector.  Returns positive correlations directly, and clips negative
	 correlations to zero probability.  Normalization process is 1) subtract
	 mean of elements in vector, and 2) scale vector to unit norm.

	 May add other modes.  Two possibilities are:
	 * Affinely map [-1,1] onto [0,1].
	 * Let probability = absolute value of correlation
  **/
  class NormalizedCorrelation : public Comparison
  {
  public:
	NormalizedCorrelation (bool subtractMean = true, float gamma = 1.0f);
	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;

	bool subtractMean;  ///< Indicates that during normalization, subtract the mean of the elements in the vector.
	float gamma;  ///< Exponent of gamma function that maps correlations to probabilities.  Semantics are slightly different than gamma value in graphics.  Here we have y = x^gamma, rather than y = x^(1/gamma).
  };

  /**
	 Uses the stardard Euclidean distance between two points.
	 Maps zero distance to probability 1 and infinite distance to
	 probability zero.
  **/
  class MetricEuclidean : public Comparison
  {
  public:
	MetricEuclidean (float scale = 1.0f, float offset = 0.0f);
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;

	float scale;  ///< Factor by which to multiply the Euclidean distance before squashing into a probability value.
	float offset;  ///< Term to add to Euclidean distance before squashing into a probability value.
  };

  /**
	 Counts number of similar entries in a pair of histograms.  Measures
	 "similarity" as the ratio of the smaller entry to the larger entry.
	 Scales count by the number entries in one of the histograms.
   **/
  class HistogramIntersection : public Comparison
  {
  public:
	HistogramIntersection (float gamma = 1.0f);

	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;

	float gamma;  ///< Map intersection value i to probability p as: p = i^gamma.
  };

  /**
	 Sum up the measure 1 - (a - b)^2 / (a + b) over all the elements of the
	 two vectors.
   **/
  class ChiSquared : public Comparison
  {
  public:
	ChiSquared (float gamma = 1.0f);

	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;

	float gamma;  ///< Exponent of gamma function that maps chi^2 to probability.  See NormalizedCorrelation::gamma.
  };


  // Descriptor ---------------------------------------------------------------

  class Descriptor
  {
  public:
	virtual ~Descriptor ();

	virtual Vector<float> value (const Image & image, const Point & point);  ///< Calls PointAffine version with default values for scale, angle and shape.
	virtual Vector<float> value (const Image & image, const PointInterest & point);  ///< Calls PointAffine version with default values for angle and shape.
	virtual Vector<float> value (const Image & image, const PointAffine & point) = 0;  ///< Returns a vector of floats that describe the image patch near the interest point.
	virtual Vector<float> value (const Image & image);  ///< Describe entire region that has non-zero alpha values.  Descriptor may treat all non-zero alpha values the same, or use them to weight the pixels.  This method is only available in Descriptors that don't require a specific point of reference.  IE: a spin image must have a central point, so it can't implement this method.
	virtual Image patch (const Vector<float> & value) = 0;  ///< Return a graphical representation of the descriptor.  Preferrably an image patch that would stimulate this descriptor to return the given value.
	virtual Comparison * comparison ();  ///< Return an instance of the recommended Comparison for feature vectors from this type of Descriptor.  Caller is responsible to destroy instance.
	virtual bool isMonochrome ();  ///< Returns true if this descriptor works only on intensity values.  Returns false if this descriptor uses color channels in some way.
	virtual int dimension ();  ///< Number of elements in result of value().  Returns 0 if dimension can change from one call to the next.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);
  };

  /**
	 Applies several descriptors to a patch at once and returns the
	 concatenation of all their feature vectors.
   **/
  class DescriptorCombo : public Descriptor
  {
  public:
	DescriptorCombo ();
	virtual ~DescriptorCombo ();  ///< Delete all descriptors we are holding.

	void add (Descriptor * descriptor);  ///< Append another descriptor to the list.  This object takes responsibility for the pointer.
	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	virtual bool isMonochrome ();
	virtual int dimension ();

	std::vector<Descriptor *> descriptors;
	int dimension_;
	bool monochrome;

	const Image * lastImage;
	void *        lastBuffer;
	Image grayImage;
  };

  /**
	 Finds characteristic scale of point.
  **/
  class DescriptorScale : public Descriptor
  {
  public:
	DescriptorScale (float firstScale = 1, float lastScale = 25, int interQuanta = 40, float quantum = 2);  ///< quantum is most meaningful as a prime number; 2 means "doubling" or "octaves"
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
	DescriptorOrientation (float supportRadial = 6.0f, int supportPixel = 32, float kernelSize = 2.5f);
	void initialize (float supportRadial, int supportPixel, float kernelSize);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int supportPixel;  ///< Pixel radius of patch.  Patch size = 2 * supportPixel + 1.
	float supportRadial;  ///< Number of sigmas away from center to include in patch (where 1 sigma = size of characteristic scale).
	float kernelSize;  ///< Number of sigmas of the Gaussian kernel to cover the radius fo the patch.  Similar semantics to supportRadial, except applies to the derivation kernels.
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
	DescriptorOrientationHistogram (float supportRadial = 4.5f, int supportPixel = 16, float kernelSize = 2.5f, int bins = 36);

	void computeGradient (const Image & image);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int supportPixel;  ///< Pixel radius of patch, if needed.  Patch size = 2 * supportPixel.
	float supportRadial;  ///< Number of sigmas away from center to include in histogram.
	float kernelSize;  ///< Similar to DescriptorOrientation::kernelSize, except that this class achieves the same effect by raising blur to the appropriate level.  Only applies to patches with shape change.
	int bins;  ///< Number of orientation bins in histogram.
	float cutoff;  ///< Ratio of maximum histogram value above which to accept secondary maxima.

	const Image * lastImage;  ///< For cacheing derivative images.
	void *        lastBuffer;  ///< For cacheing derivative images.  Allows finer granularity in detecting change.
	ImageOf<float> I_x;
	ImageOf<float> I_y;
  };

  /**
	 Measures the degree of intensity variation in a patch.  The formula
	 for the resulting value is
	 \f[
	 \sum_{p\in I}|\nabla I(p)|^2 \over{|I|}
	 \f],
	 that is, the average squared gradient length.

	 The scale at which the gradient is measured directly impacts the meaning
	 of the resulting value.  If you measure gradient at a large scale
	 relative to the patch, you effectively measure overall orientation
	 strength.  If you measure at smaller scales, you effectively measure
	 the descriptiveness of the graylevel texture.  You can control the
	 scale level by manipulating the ratio of supportRadial to supportPixel.
   **/
  class DescriptorContrast : public Descriptor
  {
  public:
	DescriptorContrast (float supportRadial = 6.0f, int supportPixel = 32);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	virtual int dimension ();
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int supportPixel;  ///< Pixel radius of patch.  Patch size = 2 * supportPixel.
	float supportRadial;  ///< Number of sigmas away from center to include in patch (where 1 sigma = size of characteristic scale).

	SlideShow * window;  ///< Temporary for debugging.
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
	virtual Comparison * comparison ();
	virtual int dimension ();
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
	virtual Comparison * comparison ();
	virtual int dimension ();
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int   binsRadial;
	int   binsIntensity;
	float supportRadial;
	float supportIntensity;  ///< Number of standard deviations away from avarge intensity.
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
	virtual Comparison * comparison ();  ///< Return a MetricEuclidean, rather than the default (NormalizedCorrelation).
	virtual int dimension ();
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int width;  ///< Number of horizontal or vertical positions.
	int angles;  ///< Number of orientation bins.
	float supportRadial;  ///< supportRadial * point.scale gives pixel distance from center to edge of bins when they overlay the image.  The pixel diameter of one bin is 2 * supportPixel * point.scale / width.
	int supportPixel;  ///< Number of pixels in normalized form of affine-invariant patch, if used.
	float sigmaWeight;  ///< Size of Gaussian that weights the entries in the bins.
	float maxValue;  ///< Largest permissible entry in one bin.

	const Image * lastImage;  ///< For cacheing derivative images.
	void *        lastBuffer;  ///< For cacheing derivative images.  Allows finer granularity in detecting change.
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
	virtual Vector<float> value (const Image & image);
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	virtual bool isMonochrome ();
	virtual int dimension ();
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
	virtual Vector<float> value (const Image & image);
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	virtual bool isMonochrome ();
	virtual int dimension ();
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int width;  ///< Number of bins in the U and V dimensions.
	int height;  ///< Number of bins in the Y dimension.
	float supportRadial;  ///< Multiple of characteristic scale to use when drawing off a normalized patch.

	bool * valid;  ///< A 3D block of booleans that stores true for every bin that translates to a valid RGB color.
	int validCount;  ///< Total number of true entries in valid.  Effectively the dimension of the vector recturned by value().

	float * histogram;  ///< Working histogram.  Forces this Descriptor object to be single threaded.
  };

  /**
	 Gathers statistics on responses to a filter bank in an image region.
	 The bank is replicated at several scale levels, and this descriptor
	 chooses the appropriate scale level for each individual pixel.
   **/
  class DescriptorTextonScale : Descriptor
  {
  public:
	DescriptorTextonScale (int angles = 4, float firstScale = 1.0f, float lastScale = 4.0f, int extraSteps = 3);
	void initialize ();

	void preprocess (const Image & image);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Vector<float> value (const Image & image);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	const Image * lastImage;  ///< Pointer to the currently cached input image.
	void *        lastBuffer;  ///< Pointer to buffer of currently cached input image.  Allows finer granularity in detecting change.
	std::vector< ImageOf<float> > responses;  ///< Responses to each filter in the bank over the entire input image.

	int angles;  ///< Number of discrete orientations in the filter bank.
	float firstScale;  ///< Delimits lower end of scale space.
	float lastScale;  ///< Delimits upper end of scale space.
	int steps;  ///< Number of discrete scale levels in one octave.
	float supportRadial;  ///< Multiple of scale to use when selecting image region specified by PointAffine.

	int bankSize;  ///< Number of filters at a given scale level.
	float scaleRatio;  ///< Ratio between two adjacent scale levels.
	std::vector<ConvolutionDiscrete2D> filters;
  };
}


#endif
