/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


01/2005 Fred Rothganger -- Update comments and defaults on some classes.  Add
        ChiSquared::preprocess().  Guarantee orientations are returnd in
        descending order by strength.
09/2005 Fred Rothganger -- Commit to using ImageCache.  Efficiency improvements
        to DescriptorSIFT.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file
LICENSE for details.
*/


#ifndef fl_descriptor_h
#define fl_descriptor_h


#include "fl/convolve.h"
#include "fl/matrix.h"
#include "fl/point.h"
#include "fl/canvas.h"
#include "fl/imagecache.h"

#include <iostream>
#include <vector>


namespace fl
{
  // Comparison ---------------------------------------------------------------

  /**
	 Takes two feature vectors and returns a value in [0,1].  This could be
	 interpreted as a probability, but there is no guarantee that for any
	 given data it actually predicts the likelihood of a correct match.
	 What is guaranteed is the 1 means perfect match, and 0 means no
	 chance whatsoever of a match.

	 The application should remap [0,1] to an actual probability if that is
	 what it requires.  At some point, it may make sense to add a Function
	 object that we run on behalf of the application to reshape the result.
  **/
  class Comparison
  {
  public:
	virtual ~Comparison ();  ///< Establishes that destructor is virtual, but doesn't do anything else.

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const = 0;

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);
	static void addProducts ();  ///< Registers with the Factory all basic Comparison classes other than ComparisonCombo.
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
	ComparisonCombo (std::istream & stream);
	virtual ~ComparisonCombo ();  ///< Delete all comparisons we are holding.

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;
	virtual float value (int index, const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;  ///< Compares one specific feature vector from the set.
	Vector<float> extract (int index, const Vector<float> & value) const;  ///< Returns one specific feature vector from the set.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

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
	NormalizedCorrelation (bool subtractMean = true);
	NormalizedCorrelation (std::istream & stream);

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	bool subtractMean;  ///< Indicates that during normalization, subtract the mean of the elements in the vector.
  };

  /**
	 Uses the stardard Euclidean distance between two points.
	 Maps zero distance to 1 and infinite (or alternately, maximum) distance to
	 probability zero.
  **/
  class MetricEuclidean : public Comparison
  {
  public:
	MetricEuclidean (float upperBound = INFINITY);
	MetricEuclidean (std::istream & stream);

	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	float upperBound;  ///< The largest possible distance, if known.  Infinity if not known.  Determines whether to use linear function or hyperbolic squashing function to map distance to resulting value.
  };

  /**
	 Counts number of similar entries in a pair of histograms.  Measures
	 "similarity" as the ratio of the smaller entry to the larger entry.
	 Scales count by the number entries in one of the histograms.
   **/
  class HistogramIntersection : public Comparison
  {
  public:
	HistogramIntersection () {}
	HistogramIntersection (std::istream & stream);
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;
  };

  /**
	 Sum up the measure 1 - (a - b)^2 / (a + b) over all the elements of the
	 two vectors.
   **/
  class ChiSquared : public Comparison
  {
  public:
	ChiSquared () {}
	ChiSquared (std::istream & stream);

	virtual Vector<float> preprocess (const Vector<float> & value) const;
	virtual float value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed = false) const;
  };


  // Descriptor ---------------------------------------------------------------

  class Descriptor
  {
  public:
	Descriptor ();
	virtual ~Descriptor ();

	virtual Vector<float> value (const Image & image, const PointAffine & point) = 0;  ///< Returns a vector of floats that describe the image patch near the interest point.
	virtual Vector<float> value (const Image & image);  ///< Describe entire region that has non-zero alpha values.  Descriptor may treat all non-zero alpha values the same, or use them to weight the pixels.  This method is only available in Descriptors that don't require a specific point of reference.  IE: a spin image must have a central point, so it can't implement this method.
	virtual Image patch (const Vector<float> & value) = 0;  ///< Return a graphical representation of the descriptor.  Preferrably an image patch that would stimulate this descriptor to return the given value.
	virtual Comparison * comparison ();  ///< Return an instance of the recommended Comparison for feature vectors from this type of Descriptor.  Caller is responsible to destroy instance.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	bool monochrome;  ///< True if this descriptor works only on intensity values.  False if this descriptor uses color channels in some way.
	int dimension;  ///< Number of elements in result of value().  0 if dimension can change from one call to the next.
	float supportRadial;  ///< Number of sigmas away from center to include in patch (where 1 sigma = size of characteristic scale).  0 means this descriptor does not depend on characteristic scale.
  };

  /**
	 Applies several descriptors to a patch at once and returns the
	 concatenation of all their feature vectors.
   **/
  class DescriptorCombo : public Descriptor
  {
  public:
	DescriptorCombo ();
	DescriptorCombo (std::istream & stream);
	virtual ~DescriptorCombo ();  ///< Delete all descriptors we are holding.

	void add (Descriptor * descriptor);  ///< Append another descriptor to the list.  This object takes responsibility for the pointer.
	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Vector<float> value (const Image & image);
	virtual Image patch (const Vector<float> & value);
	Image patch (int index, const Vector<float> & value);  ///< Returns a visualization of one specific feature vector in the set.
	virtual Comparison * comparison ();
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	std::vector<Descriptor *> descriptors;

	void * lastBuffer;  ///< For detecting change in cached image.
	double lastTime;  ///< For detecting change in cached image.
	Image grayImage;
  };

  /**
	 Finds characteristic scale of point.
  **/
  class DescriptorScale : public Descriptor
  {
  public:
	DescriptorScale (float firstScale = 1, float lastScale = 25, int interQuanta = 40, float quantum = 2);  ///< quantum is most meaningful as a prime number; 2 means "doubling" or "octaves"
	DescriptorScale (std::istream & stream);
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
	DescriptorOrientation (std::istream & stream);
	void initialize (float supportRadial, int supportPixel, float kernelSize);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int supportPixel;  ///< Pixel radius of patch.  Patch size = 2 * supportPixel + 1.
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
	DescriptorOrientationHistogram (std::istream & stream);

	void computeGradient (const Image & image);

	virtual Vector<float> value (const Image & image, const PointAffine & point);  ///< Returns one or more angle hypotheses, listed in order of descending strength.  That is, the strongest angle hypothesis will be in row 0.
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int supportPixel;  ///< Pixel radius of patch, if needed.  Patch size = 2 * supportPixel.
	float kernelSize;  ///< Similar to DescriptorOrientation::kernelSize, except that this class achieves the same effect by raising blur to the appropriate level.  Only applies to patches with shape change.
	int bins;  ///< Number of orientation bins in histogram.
	float cutoff;  ///< Ratio of maximum histogram value above which to accept secondary maxima.

	void * lastBuffer;  ///< For detecting change in cached image.
	double lastTime;  ///< For detecting change in cached image.
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
	DescriptorContrast (std::istream & stream);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int supportPixel;  ///< Pixel radius of patch.  Patch size = 2 * supportPixel.
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
	DescriptorFiltersTexton (std::istream & stream) : DescriptorFilters (stream) {}
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
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int width;
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
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int   binsRadial;
	int   binsIntensity;
	float supportIntensity;  ///< Number of standard deviations away from avarge intensity.
  };

  /**
	 Implements David Lowe's SIFT descriptor.
	 Note on supportRadial: supportRadial * point.scale gives pixel distance from center to edge of bins when they overlay the image.  The pixel diameter of one bin is 2 * supportRadial * point.scale / width.
  **/
  class DescriptorSIFT : public Descriptor
  {
  public:
	DescriptorSIFT (int width = 4, int angles = 8);
	DescriptorSIFT (std::istream & stream);
	~DescriptorSIFT ();

	void init ();  ///< Computes certain working data based on current values of parameters.
	float * getKernel (int size);  ///< Generates/caches Gaussian weighting kernels for various sizes of rectified patch.

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Image patch (const Vector<float> & value);
	void patch (const std::string & fileName, const Vector<float> & value);  ///< Write a visualization of the descriptor to a postscript file.
	void patch (Canvas * canvas, const Vector<float> & value, int size);  ///< Subroutine used by other patch() methods.
	virtual Comparison * comparison ();  ///< Return a MetricEuclidean, rather than the default (NormalizedCorrelation).
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	// Parameters
	int width;  ///< Number of horizontal or vertical positions.
	int angles;  ///< Number of orientation bins.
	int supportPixel;  ///< Pixel radius of normalized form of affine-invariant patch, if used.
	float sigmaWeight;  ///< Size of Gaussian that weights the entries in the bins.
	float maxValue;  ///< Largest permissible entry in one bin.

	// Values derived from parameters by init().
	float angleStep;

	// Storage used for calculating individual descriptor values.  These are
	// here mainly to avoid repeatedly constructing certain objects.
	std::map<int, ImageOf<float> *> kernels;  ///< Gaussian weighting kernels for various sizes of rectified patch.
	ImageOf<float> I_x;  ///< x component of gradient vectors
	ImageOf<float> I_y;  ///< y component of gradient vectors
	FiniteDifferenceX fdX;
	FiniteDifferenceY fdY;
  };

  /**
	 Form a 2D color histogram of the UV components in a YUV patch.
	 Note on dimension: it is the total number of true entries in valid.
  **/
  class DescriptorColorHistogram2D : public Descriptor
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

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Vector<float> value (const Image & image);
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int width;  ///< Number of bins in the U and V dimensions.
	Matrix<bool> valid;  ///< Stores true for every bin that translates to a valid RGB color.
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
	DescriptorColorHistogram3D (int width = 5, int height = -1, float supportRadial = 4.2f);  ///< height == -1 means use value of width
	DescriptorColorHistogram3D (std::istream & stream);
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
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

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
  class DescriptorTextonScale : public Descriptor
  {
  public:
	DescriptorTextonScale (int angles = 4, float firstScale = 1.0f, float lastScale = 4.0f, int extraSteps = 3);
	DescriptorTextonScale (std::istream & stream);
	void initialize ();

	void preprocess (const Image & image);

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Vector<float> value (const Image & image);
	virtual Image patch (const Vector<float> & value);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	void * lastBuffer;  ///< For detecting change in cached image.
	double lastTime;  ///< For detecting change in cached image.
	std::vector< ImageOf<float> > responses;  ///< Responses to each filter in the bank over the entire input image.

	int angles;  ///< Number of discrete orientations in the filter bank.
	float firstScale;  ///< Delimits lower end of scale space.
	float lastScale;  ///< Delimits upper end of scale space.
	int steps;  ///< Number of discrete scale levels in one octave.

	int bankSize;  ///< Number of filters at a given scale level.
	float scaleRatio;  ///< Ratio between two adjacent scale levels.
	std::vector<ConvolutionDiscrete2D> filters;
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
  class DescriptorLBP : public Descriptor
  {
  public:
	DescriptorLBP (int P = 8, float R = 1.0f, float supportRadial = 4.2f, int supportPixel = 32);
	DescriptorLBP (std::istream & stream);
	void initialize ();

	void preprocess (const Image & image);  ///< Prepare gray version of image.  Subroutine of value().
	void add (const int x, const int y, Vector<float> & result);  ///< Does the actual LBP calculation for one pixel.  Subroutine of value().

	virtual Vector<float> value (const Image & image, const PointAffine & point);
	virtual Vector<float> value (const Image & image);
	virtual Image patch (const Vector<float> & value);
	virtual Comparison * comparison ();
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = true);

	int P;  ///< Number of evenly spaced sample points around center.
	float R;  ///< Radius of circle of sample points.
	int supportPixel;  ///< Radius of patch to draw off if point specifies a shape change.

	void * lastBuffer;  ///< For detecting change in cached image.
	double lastTime;  ///< For detecting change in cached image.
	ImageOf<unsigned char> categoryImage;  ///< Cached LBP categories for each pixel.  P is limited to 254.

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
