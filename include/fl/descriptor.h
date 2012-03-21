/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_descriptor_h
#define fl_descriptor_h


#include "fl/convolve.h"
#include "fl/matrix.h"
#include "fl/point.h"
#include "fl/canvas.h"
#include "fl/metric.h"
#include "fl/archive.h"
#include "fl/imagecache.h"

#include <iostream>
#include <vector>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flImage_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  // Comparison ---------------------------------------------------------------

  /**
	 A Metric that returns a value in [0,1] and that may preprocess the
	 two input vectors to normalize them in some way.
  **/
  class SHARED Comparison : public Metric
  {
  public:
	Comparison ();

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2) const = 0;

	void serialize (Archive & archive, uint32_t version);

	bool needPreprocess;  ///< Indicates that any data passed to the value() function should be preprocessed.  Default (set by constructor) is true.  If you compare values multiple times, it is more efficient to preprocess them all once and then set this flag to false.
  };

  /**
	 Handles comparisons between feature vectors that are composed of several
	 smaller feature vectors from various descriptors.
   **/
  class SHARED ComparisonCombo : public Comparison
  {
  public:
	ComparisonCombo ();
	virtual ~ComparisonCombo ();  ///< Delete all comparisons we are holding.

	void clear ();
	void add (Comparison * comparison, int dimension);

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2) const;
	virtual float value (int index, const Vector<float> & value1, const Vector<float> & value2) const;  ///< Compares one specific feature vector from the set.
	Vector<float> extract (int index, const Vector<float> & value) const;  ///< Returns one specific feature vector from the set.

	void serialize (Archive & archive, uint32_t version);

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
  class SHARED NormalizedCorrelation : public Comparison
  {
  public:
	NormalizedCorrelation (bool subtractMean = true);

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2) const;

	void serialize (Archive & archive, uint32_t version);

	bool subtractMean;  ///< Indicates that during normalization, subtract the mean of the elements in the vector.
  };

  /**
	 Uses the stardard Euclidean distance between two points.
	 Maps zero distance to 0 and infinite (or alternately, maximum) distance to
	 1.
  **/
  class SHARED MetricEuclidean : public Comparison
  {
  public:
	MetricEuclidean (float upperBound = INFINITY);

	virtual float value (const Vector<float> & value1, const Vector<float> & value2) const;

	void serialize (Archive & archive, uint32_t version);

	float upperBound;  ///< The largest possible distance, if known.  Infinity if not known.  Determines whether to use linear function or hyperbolic squashing function to map distance to resulting value.
  };

  /**
	 Counts number of similar entries in a pair of histograms.  Measures
	 "similarity" as the ratio of the smaller entry to the larger entry.
	 Scales count by the number entries in one of the histograms.
   **/
  class SHARED HistogramIntersection : public Comparison
  {
  public:
	HistogramIntersection () {}
	virtual float value (const Vector<float> & value1, const Vector<float> & value2) const;
  };

  /**
	 Sum up the measure 1 - (a - b)^2 / (a + b) over all the elements of the
	 two vectors.
   **/
  class SHARED ChiSquared : public Comparison
  {
  public:
	ChiSquared () {}

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2) const;
  };


  // Descriptor ---------------------------------------------------------------

  class SHARED Descriptor
  {
  public:
	Descriptor ();
	virtual ~Descriptor ();

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point) = 0;  ///< Returns a vector of floats that describe the image patch near the interest point.
	virtual Vector<float> value (ImageCache & cache);  ///< Describe entire region that has non-zero alpha values.  Descriptor may treat all non-zero alpha values the same, or use them to weight the pixels.  This method is only available in Descriptors that don't require a specific point of reference.  IE: a spin image must have a central point, so it can't implement this method.
	Vector<float> value (const Image & image, const PointAffine & point);
	Vector<float> value (const Image & image);
	virtual Image patch (const Vector<float> & value) = 0;  ///< Return a graphical representation of the descriptor.  Preferrably an image patch that would stimulate this descriptor to return the given value.
	virtual Comparison * comparison ();  ///< Return an instance of the recommended Comparison for feature vectors from this type of Descriptor.  Caller is responsible to destroy instance.

	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;

	bool monochrome;  ///< True if this descriptor works only on intensity values.  False if this descriptor uses color channels in some way.
	int dimension;  ///< Number of elements in result of value().  0 if dimension can change from one call to the next.
	float supportRadial;  ///< Number of sigmas away from center to include in patch (where 1 sigma = size of characteristic scale).  0 means this descriptor does not depend on characteristic scale.
  };

  /**
	 Applies several descriptors to a patch at once and returns the
	 concatenation of all their feature vectors.
   **/
  class SHARED DescriptorCombo : public Descriptor
  {
  public:
	DescriptorCombo ();
	virtual ~DescriptorCombo ();  ///< Delete all descriptors we are holding.

	void add (Descriptor * descriptor);  ///< Append another descriptor to the list.  This object takes responsibility for the pointer.
	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	virtual Vector<float> value (ImageCache & cache);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	Image patch (int index, const Vector<float> & value);  ///< Returns a visualization of one specific feature vector in the set.
	virtual Comparison * comparison ();
	void serialize (Archive & archive, uint32_t version);

	std::vector<Descriptor *> descriptors;
  };

  /**
	 Finds characteristic scale of point.
  **/
  class SHARED DescriptorScale : public Descriptor
  {
  public:
	DescriptorScale (float firstScale = 1, float lastScale = 25, int interQuanta = 40, float quantum = 2);  ///< quantum is most meaningful as a prime number; 2 means "doubling" or "octaves"
	virtual ~DescriptorScale ();
	void initialize ();

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);

	void serialize (Archive & archive, uint32_t version);

	float firstScale;
	float lastScale;
	float stepSize;
	std::vector<Laplacian *> laplacians;
  };

  /**
	 Finds characteristic angle of point using a pair of large
	 derivative-of-Gaussian kernels.
   **/
  class SHARED DescriptorOrientation : public Descriptor
  {
  public:
	DescriptorOrientation (float supportRadial = 6.0f, int supportPixel = 32, float kernelSize = 2.5f);
	void initialize ();

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	void serialize (Archive & archive, uint32_t version);

	int supportPixel;  ///< Pixel radius of patch.  Patch size = 2 * supportPixel + 1.
	float kernelSize;  ///< Number of sigmas of the Gaussian kernel to cover the radius fo the patch.  Similar semantics to supportRadial, except applies to the derivation kernels.
	GaussianDerivativeFirst Gx;
	GaussianDerivativeFirst Gy;
  };

  /**
	 Finds characteristic angle of point using a histogram of gradient
	 directions.  Follows David Lowe's approach.
   **/
  class SHARED DescriptorOrientationHistogram : public Descriptor
  {
  public:
	DescriptorOrientationHistogram (float supportRadial = 4.5f, int supportPixel = 16, float kernelSize = 2.5f, int bins = 36);

	void computeGradient (const Image & image);

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	void serialize (Archive & archive, uint32_t version);

	int supportPixel;  ///< Pixel radius of patch, if needed.  Patch size = 2 * supportPixel.
	float kernelSize;  ///< Similar to DescriptorOrientation::kernelSize, except that this class achieves the same effect by raising blur to the appropriate level.  Only applies to patches with shape change.
	int bins;  ///< Number of orientation bins in histogram.
	float cutoff;  ///< Ratio of maximum histogram value above which to accept secondary maxima.
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
  class SHARED DescriptorContrast : public Descriptor
  {
  public:
	DescriptorContrast (float supportRadial = 6.0f, int supportPixel = 32);

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	void serialize (Archive & archive, uint32_t version);

	int supportPixel;  ///< Pixel radius of patch.  Patch size = 2 * supportPixel.
  };

  class SHARED DescriptorFilters : public Descriptor
  {
  public:
	DescriptorFilters ();
	virtual ~DescriptorFilters ();  

	void prepareFilterMatrix ();

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	void serialize (Archive & archive, uint32_t version);

	std::vector<ConvolutionDiscrete2D> filters;
	Matrix<float> filterMatrix;
	int patchWidth;
	int patchHeight;
  };

  class SHARED DescriptorFiltersTexton : public DescriptorFilters
  {
  public:
	DescriptorFiltersTexton (int angles = 6, int scales = 4, float firstScale = -1, float scaleStep = -1);
  };

  class SHARED DescriptorPatch : public Descriptor
  {
  public:
	DescriptorPatch (int width = 10, float supportRadial = 0);
	DescriptorPatch (std::istream & stream);
	virtual ~DescriptorPatch ();

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	void serialize (Archive & archive, uint32_t version);

	int width;
  };

  class SHARED DescriptorSchmidScale : public Descriptor
  {
  public:
	DescriptorSchmidScale (float sigma = 1.0);
	DescriptorSchmidScale (std::istream & stream);
	void initialize ();
	virtual ~DescriptorSchmidScale ();

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	void serialize (Archive & archive, uint32_t version);

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

  class SHARED DescriptorSchmid : public Descriptor
  {
  public:
	DescriptorSchmid (int scaleCount = 8, float scaleStep = -1);
	DescriptorSchmid (std::istream & stream);
	void initialize (int scaleCount);
	virtual ~DescriptorSchmid ();

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	void serialize (Archive & archive, uint32_t version);

	DescriptorSchmidScale * findScale (float sigma);

	float scaleStep;
	std::vector<DescriptorSchmidScale *> descriptors;
  };

  class SHARED DescriptorSpin : public Descriptor
  {
  public:
	DescriptorSpin (int binsRadial = 5, int binsIntensity = 6, float supportRadial = 3, float supportIntensity = 3);
	DescriptorSpin (std::istream & stream);

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	void serialize (Archive & archive, uint32_t version);

	int   binsRadial;
	int   binsIntensity;
	float supportIntensity;  ///< Number of standard deviations away from avarge intensity.
  };

  /**
	 Implements David Lowe's SIFT descriptor.
	 Note on supportRadial: supportRadial * point.scale gives pixel distance from center to edge of bins when they overlay the image.  The pixel diameter of one bin is 2 * supportRadial * point.scale / width.
  **/
  class SHARED DescriptorSIFT : public Descriptor
  {
  public:
	DescriptorSIFT (int width = 4, int angles = 8);
	DescriptorSIFT (std::istream & stream);
	~DescriptorSIFT ();

	void init ();  ///< Computes certain working data based on current values of parameters.
	float * getKernel (int size);  ///< Generates/caches Gaussian weighting kernels for various sizes of rectified patch.

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	void patch (const std::string & fileName, const Vector<float> & value);  ///< Write a visualization of the descriptor to a postscript file.
	void patch (Canvas * canvas, const Vector<float> & value, int size);  ///< Subroutine used by other patch() methods.
	virtual Comparison * comparison ();  ///< Return a MetricEuclidean, rather than the default (NormalizedCorrelation).
	void serialize (Archive & archive, uint32_t version);

	// Parameters
	int width;  ///< Number of horizontal or vertical positions.
	int angles;  ///< Number of orientation bins.
	float angleRange;  ///< Default is 2*PI.  If set to PI instead, then ignore sign of gradient.
	int supportPixel;  ///< Pixel radius of normalized form of affine-invariant patch, if used.
	float sigmaWeight;  ///< Size of Gaussian that weights the entries in the bins.
	float maxValue;  ///< Largest permissible entry in one bin.

	// Values derived from parameters by init().
	float angleStep;

	// Storage used for calculating individual descriptor values.  These are
	// here mainly to avoid repeatedly constructing certain objects.
	std::map<int, ImageOf<float> *> kernels;  ///< Gaussian weighting kernels for various sizes of rectified patch.
	FiniteDifference fdX;
	FiniteDifference fdY;
  };

  /**
	 Form a 2D color histogram of the UV components in a YUV patch.
	 Note on dimension: it is the total number of true entries in valid.
  **/
  class SHARED DescriptorColorHistogram2D : public Descriptor
  {
  public:
	DescriptorColorHistogram2D (int width = 5, float supportRadial = 4.2f);
	DescriptorColorHistogram2D (std::istream & stream);
	void initialize ();

	// The following are subroutines used by value(), and also available to client code if the programmer wishes to use a different criterion for selecting pixels to bin.
	void clear ();  ///< Zero out histogram in preparation for a round of binning.
	void addToHistogram (const Image & image, const int x, const int y);  ///< An inline used by both add() and value().  Contains the common code for incrementing color bins.
	void add (const Image & image, int x, int y);  ///< Add color of image(x,y) to histogram.
	Vector<float> finish ();  ///< Extract feature vector from the histogram.  Only returns values for bins that map to a valid RGB color.  See member "valid" below.

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	virtual Vector<float> value (ImageCache & cache);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	void serialize (Archive & archive, uint32_t version);

	int width;  ///< Number of bins in the U and V dimensions.
	Matrix<bool> valid;  ///< Stores true for every bin that translates to a valid RGB color.
	Matrix<float> histogram;  ///< Working histogram.  Forces this Descriptor object to be single threaded.
  };

  /**
	 Form a 3D color histogram of the UV components in a YUV patch.
	 If the need arises to use other color spaces, this class could be
	 generalized.
  **/
  class SHARED DescriptorColorHistogram3D : public Descriptor
  {
  public:
	DescriptorColorHistogram3D (int width = 5, int height = -1, float supportRadial = 4.2f);  ///< height == -1 means use value of width
	DescriptorColorHistogram3D (std::istream & stream);
	~DescriptorColorHistogram3D ();
	void initialize ();

	// The following are subroutines used by value(), and also available to client code if the programmer wishes to use a different criterion for selecting pixels to bin.
	void clear ();  ///< Zero out histogram in preparation for a round of binning.
	void addToHistogram (const Image & image, const int x, const int y);  ///< An inline used by both add() and value().  Contains the common code for incrementing color bins.
	void add (const Image & image, int x, int y);  ///< Add color of image(x,y) to histogram.
	Vector<float> finish ();  ///< Extract feature vector from the histogram.  Only returns values for bins that map to a valid RGB color.  See member "valid" below.

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	virtual Vector<float> value (ImageCache & cache);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	void serialize (Archive & archive, uint32_t version);

	int width;  ///< Number of bins in the U and V dimensions.
	int height;  ///< Number of bins in the Y dimension.
	bool * valid;  ///< A 3D block of booleans that stores true for every bin that translates to a valid RGB color.
	float * histogram;  ///< Working histogram.  Forces this Descriptor object to be single threaded.
  };

  /**
	 Gathers statistics on responses to a filter bank in an image region.
	 The bank is replicated at several scale levels, and this descriptor
	 chooses the appropriate scale level for each individual pixel.
   **/
  class SHARED DescriptorTextonScale : public Descriptor
  {
  public:
	DescriptorTextonScale (int angles = 4, float firstScale = 1.0f, float lastScale = 4.0f, int extraSteps = 3);
	~DescriptorTextonScale ();
	void clear ();
	void initialize ();

	void processPixel (Image & image, ImageOf<float> & scaleImage, std::vector<ImageOf<float> > & dogs, std::vector<ImageOf<float> > & responses, int x, int y);
	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	virtual Vector<float> value (ImageCache & cache);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	void serialize (Archive & archive, uint32_t version);

	int angles;  ///< Number of discrete orientations in the filter bank.
	float firstScale;  ///< Delimits lower end of scale space.
	float lastScale;  ///< Delimits upper end of scale space.
	int steps;  ///< Number of discrete scale levels in one octave.

	int bankSize;  ///< Number of filters at a given scale level.
	float scaleRatio;  ///< Ratio between two adjacent scale levels.
	std::vector<ConvolutionDiscrete2D *> filters;
	std::vector<float> scales;
  };

  /**
	 "Local Binary Patterns": histogram counts of various patterns the appear
	 in the binarized itensity along a circle around a point.  The idea is
	 to take a circle at a certain radius from the center point, and binarize
	 the intensity at regular intervals along the circle with reference to
	 the intensity of the center point.  Characterize the resulting string
	 of 0s and 1s according to two measures: 1) how many lo-hi or hi-lo
	 transitions there are.  2) How many 1s there are.  If there are no more
	 than 2 transitions, then the LBP value for the point is the count of
	 1s.  If there are more than 2 transitions, then the LBP value is
	 "miscellaneous".  Finally, histogram the LBP values over the specified
	 region.
  **/
  class SHARED DescriptorLBP : public Descriptor
  {
  public:
	DescriptorLBP (int P = 8, float R = 1.0f, float supportRadial = 4.2f, int supportPixel = 32);
	DescriptorLBP (std::istream & stream);
	void initialize ();

	void add (const int x, const int y, Vector<float> & result);  ///< Does the actual LBP calculation for one pixel.  Subroutine of value().

	virtual Vector<float> value (ImageCache & cache, const PointAffine & point);
	virtual Vector<float> value (ImageCache & cache);
	using Descriptor::value;
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	void serialize (Archive & archive, uint32_t version);

	int P;  ///< Number of evenly spaced sample points around center.
	float R;  ///< Radius of circle of sample points.
	int supportPixel;  ///< Radius of patch to draw off if point specifies a shape change.

	/**
	   A structure for storing bilinear interpolation parameters.
	 **/
	struct Interpolate
	{
	  int xl;
	  int yl;
	  int xh;
	  int yh;
	  float wll;
	  float wlh;
	  float whl;
	  float whh;
	  bool exact;  ///< If true, then use pixel value at (xl,yl) and ignore all other data in this record.
	};
	std::vector<Interpolate> interpolates;  ///< Cached data for doing bilinear interpolation of pixel values along circle.
  };
}


#endif
