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


#ifndef convolve_h
#define convolve_h


#include "fl/image.h"
#include "fl/matrix.h"
#include "fl/point.h"
#include "fl/serialize.h"

#include <ostream>

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
  enum BorderMode  ///< What to do with resulting pixels when the kernel exceeds the border of the input image.
  {
	Crop,      ///< Resulting image is smaller than input image, and only contains the well-defined pixels.
	ZeroFill,  ///< Resulting image is full size, but border pixels are set to zero.
	Boost,     ///< Treat the portion of the kernel that overlaps the image as if it were a full kernel.  This involves boosting the weights of the sub-kernel so the resulting pixel values are consistent with the well-defined pixels.  Only appropriate for symmetric kernels that sum to 1.
	UseZeros,  ///< Treat pixels beyond the border of source image as having value of zero and convolve with full kernel.
	Copy,      ///< Resulting image is full size, and border pixels are copied from source image.
	Undefined  ///< Border pixels are not computed, but resulting image is same size as input.  Does exactly the same amount of computation at Crop.
  };

  /**
	 A Convolution is a type of Filter which computes the convolution of the
	 image with some function.

	 Coordinate system convention: For efficiency's sake, we follow the standard
	 coordinate system for pixel rasters.  The origin is in the upper left.
	 Positive x axis goes toward the right, and positive y axis goes downward.

	 Note about handedness:
	 If you like to imagine that you are looking down at the plane where the
	 pixels live, and that the positive z-axis comes out of the plane towards
	 you, then this forms a "left-handed" coordinate system, which is universally
	 condemned by purists.  However, it is better to imagine that the positive
	 z-axis is heading away from you, in which case this is a proper right-handed
	 system.  It is even better to realize that there is no z-axis whatsoever and
	 that a 2D plane has no intrinsic handedness.  (It is possible to define a
	 handedness by choosing three non-colinear points as a basis.  Since we have
	 a coordiate system, we have implicitly already made this choice.)

	 All float-valued pixel coordinates follow the convention for
	 Point (integral numbers refer to center of pixel).  Integer-valued
	 pixel coordinates refer to either the entire pixel or to its center,
	 depending on context.
  **/
  class SHARED Convolution : public Filter
  {
  public:
	virtual Image filter (const Image & image) = 0;  ///< Convolve the entire image with the kernel contained in this object.
	virtual double response (const Image & image, const Point & p) const = 0;  ///< Strength of response of filter to image at pixel (x, y).

	BorderMode mode;
  };


  // 2D Convolutions ----------------------------------------------------------

  /**
	 Stores the kernel as a discrete set of points (a raster).
  **/
  class SHARED ConvolutionDiscrete2D : public Convolution, public Image
  {
  public:
	ConvolutionDiscrete2D (const BorderMode mode = Crop,
						   const PixelFormat & format = GrayFloat);
	ConvolutionDiscrete2D (const Image & image, const BorderMode mode = Crop);

	virtual Image filter (const Image & image);
	virtual double response (const Image & image, const Point & p) const;  ///< Strength of response of filter to image at input pixel (x, y).  Crop mode is treated as if ZeroFill mode, ie: no shift between input and output coordinate system.

	virtual void serialize (Archive & archive, uint32_t version);

	void normalFloats ();  ///< Zero out any sub-normal floats in kernel, because they cause numerical exceptions that really drag down performance.

	static uint32_t serializeVersion;
  };

  class SHARED Gaussian2D : public ConvolutionDiscrete2D
  {
  public:
	Gaussian2D (double sigma = 1.0,
				const BorderMode mode = Crop,
				const PixelFormat & format = GrayFloat);

	static double cutoff;  ///< Minimum number of standard deviations to include in a Gaussian kernel
  };

  class SHARED DifferenceOfGaussians : public ConvolutionDiscrete2D
  {
  public:
	DifferenceOfGaussians (double sigmaPlus,
						   double sigmaMinus,
						   const BorderMode mode = Crop,
						   const PixelFormat & format = GrayFloat);
  };

  class SHARED GaussianDerivativeFirst : public ConvolutionDiscrete2D
  {
  public:
	GaussianDerivativeFirst (int xy = 0,  ///< xy == 0 means x-derivative; xy != 0 means y-derivative
							 double sigmaX = 1.0,
							 double sigmaY = -1.0,
							 double angle = 0,
							 const BorderMode mode = Crop,
							 const PixelFormat & format = GrayFloat);
  };

  class SHARED GaussianDerivativeSecond : public ConvolutionDiscrete2D
  {
  public:
	GaussianDerivativeSecond (int xy1 = 0,
							  int xy2 = 0,
							  double sigmaX = 1.0,
							  double sigmaY = -1.0,
							  double angle = 0,
							  const BorderMode mode = Crop,
							  const PixelFormat & format = GrayFloat);
  };

  class SHARED GaussianDerivativeThird : public ConvolutionDiscrete2D
  {
  public:
	GaussianDerivativeThird (int xy1 = 0,
							 int xy2 = 0,
							 int xy3 = 0,
							 double sigmaX = 1.0,
							 double sigmaY = -1.0,
							 double angle = 0,
							 const BorderMode mode = Crop,
							 const PixelFormat & format = GrayFloat);
  };

  class SHARED Laplacian : public ConvolutionDiscrete2D
  {
  public:
	Laplacian (double sigma = 1.0,
			   const BorderMode mode = Crop,
			   const PixelFormat & format = GrayFloat);

	virtual void serialize (Archive & archive, uint32_t version);

	double sigma;
  };


  // 1D Convolutions ----------------------------------------------------------

  enum Direction
  {
	Vertical,
	Horizontal
  };

  class SHARED Convolution1D : public Convolution
  {
  public:
	Direction direction;
  };

  class SHARED ConvolutionDiscrete1D : public Convolution1D, public Image
  {
  public:
	ConvolutionDiscrete1D (const BorderMode mode = Crop,
						   const PixelFormat & format = GrayFloat,
						   const Direction direction = Horizontal);
	ConvolutionDiscrete1D (const Image & image,
						   const BorderMode mode = Crop,
						   const Direction direction = Horizontal);

	virtual Image filter (const Image & image);
	virtual double response (const Image & image, const Point & p) const;  ///< Strength of response of filter to image at pixel (x, y).

	void normalFloats ();  ///< Zero out any sub-normal floats in kernel, because they cause numerical exceptions that really drag down performance.
  };

  class SHARED Gaussian1D : public ConvolutionDiscrete1D
  {
  public:
	Gaussian1D (double sigma = 1.0,
				const BorderMode mode = Crop,
				const PixelFormat & format = GrayFloat,
				const Direction direction = Horizontal);
  };

  class SHARED GaussianDerivative1D : public ConvolutionDiscrete1D
  {
  public:
	GaussianDerivative1D (double sigma = 1.0,
						  const BorderMode mode = Crop,
						  const PixelFormat & format = GrayFloat,
						  const Direction direction = Horizontal);
  };

  class SHARED GaussianDerivativeSecond1D : public ConvolutionDiscrete1D
  {
  public:
	GaussianDerivativeSecond1D (double sigma = 1.0,
								const BorderMode mode = Crop,
								const PixelFormat & format = GrayFloat,
								const Direction direction = Horizontal);
  };

  /**
	 Uses recursive Gaussian approach.
	 This is a direct adaptation of Krystian's implementation.
	 The "kernels" are only in double format, and the only BorderMode is
	 (sort of like) Boost.
   **/
  class SHARED ConvolutionRecursive1D : public Convolution1D
  {
  public:
	virtual Image filter (const Image & image);
	virtual double response (const Image & image, const Point & p) const;

	void set_nii_and_dii (double sigma,
						  double a0, double a1,
						  double b0, double b1,
						  double c0, double c1,
						  double o0, double o1);

	// Coefficients:
	double n00p;
	double n11p;
	double n22p;
	double n33p;
	double n11m;
	double n22m;
	double n33m;
	double n44m;
	double d11p;
	double d22p;
	double d33p;
	double d44p;
	double d11m;
	double d22m;
	double d33m;
	double d44m;
	double scale;
  };

  class SHARED GaussianRecursive1D : public ConvolutionRecursive1D
  {
  public:
	GaussianRecursive1D (double sigma = 1.0,
						 const Direction direction = Horizontal);
  };

  class SHARED GaussianDerivativeRecursive1D : public ConvolutionRecursive1D
  {
  public:
	GaussianDerivativeRecursive1D (double sigma = 1.0,
								   const Direction direction = Horizontal);
  };

  class SHARED GaussianDerivativeSecondRecursive1D : public ConvolutionRecursive1D
  {
  public:
	GaussianDerivativeSecondRecursive1D (double sigma = 1.0,
										 const Direction direction = Horizontal);
  };


  // Interest operators -------------------------------------------------------

  class SHARED FilterHarris : public Filter
  {
  public:
	FilterHarris (double sigmaD = 1.0, double sigmaI = 1.4, const PixelFormat & format = GrayFloat);

	virtual Image filter (const Image & image);  ///< Relies on preprocess, process, and response to do all the work.
	virtual void preprocess (const Image & image);  ///< Extracts the square gradient matrices from the image.  Stores in xx, xy, and yy.
	virtual Image process ();  ///< Collects responses into an Image.
	virtual double response (int x, int y) const;  ///< Returns Harris function value (of mose recently filtered image) at (x,y).  The location uses the same coordinates as the result of filter().  Uses squareGradient().
	virtual void gradientSquared (int x, int y, Matrix<double> & result) const;  ///< Finds the autocorrelation matrix (of the most recently filtered image) at (x,y).  The location uses the same coordinates as the result of filter().

	double sigmaD;  ///< Derivation scale
	double sigmaI;  ///< Integration scale
	Gaussian2D           G_I;   ///< Gaussian for integration
	Gaussian1D           G1_I;  ///< seperated Gaussian for integration
	Gaussian1D           G1_D;  ///< seperated Gaussian for derivation (blurring pass)
	GaussianDerivative1D dG_D;  ///< seperated Gaussian for derivation
	Image xx;  ///< Components of the autocorrelation matrix.
	Image xy;  ///< These are built by preprocess ().
	Image yy;
	int offset;   ///< Total amount of one image border removed
	int offsetI;  ///< Border removed by integration
	int offsetD;  ///< Border removed by differentiation

	static const double alpha;

  protected:
	/**
	   If the blurring part of the separable Gaussian derivative kernel has
	   a larger radius, then the difference in pixels is stored in offset1.
	   If the derivative part has a larger radius, then the difference is kept
	   in offset2.  These help align the x and y derivative images correctly.
	**/
	int offset1;
	int offset2;
  };

  /**
	 Like FilterHarris, but uses a modified form of the response function that
	 returns the absolute value of the product of the eigenvectors.  The
	 regular FilterHarris only approximates this and at a different scale.
  **/
  class SHARED FilterHarrisEigen : public FilterHarris
  {
  public:
	FilterHarrisEigen (double sigmaD = 1.0, double sigmaI = 1.4, const PixelFormat & format = GrayFloat) : FilterHarris (sigmaD, sigmaI, format) {}

	virtual Image process ();
	virtual double response (int x, int y) const;
  };

  /**
	 Also similar to FilterHarris, but computes L_xx + L_yy instead.
  **/
  class SHARED FilterHessian : public Filter
  {
  public:
	FilterHessian (double sigma = 1.0, const PixelFormat & format = GrayFloat);

	virtual Image filter (const Image & image);

	double sigma;  ///< scale
	Gaussian1D                 G;
	GaussianDerivativeSecond1D dG;
	int offset;  ///< number of pixels removed from border

  protected:
	int offset1;
	int offset2;
  };


  // Misc -----------------------------------------------------------------------

  /**
	 Similar to convolving with the kernel [1,0,-1], but written to run more
	 efficiently than the general ConvolutionDiscrete1D.  Handles borders
	 with a modified form of the kernel: [1,-1]*2.  In general, the entire
	 result is effectively scaled by 2, since we avoid the extra
	 division in the (much more repetitive) non-border case, and instead
	 multiply by 2 in border case.
   **/
  class SHARED FiniteDifferenceX : public Filter
  {
  public:
	virtual Image filter (const Image & image);	
  };

  /**
	 Same as FiniteDifferenceX, but in the Y direction.
   **/
  class SHARED FiniteDifferenceY : public Filter
  {
  public:
	virtual Image filter (const Image & image);	
  };

  class SHARED NonMaxSuppress : public Filter
  {
  public:
	NonMaxSuppress (int half = 1, BorderMode mode = UseZeros);  ///< Only recognizes UseZeros and ZeroFill.  Other modes are mapped to closest equivalent.

	virtual Image filter (const Image & image);

	int half;  ///< Number of pixels away from center to check for local maxima.
	BorderMode mode;
	float maximum;  ///< Largest value found during last run of filter.
	float minimum;  ///< Smallest value found during last run of filter.
	float average;  ///< Average value found during last run of filter.
	int count;  ///< Number of pixels that passed last run of filter.
  };

  class SHARED Median : public Filter
  {
  public:
	Median (int radius = 2, float order = 0.5f);

	virtual Image filter (const Image & image);

	void split  (int width, int height,                      uint8_t * inBuffer, int inStrideH, int inStrideV, uint8_t * outBuffer, int outStrideH, int outStrideV);
	void filter (int width, int height, int left, int right, uint8_t * inBuffer, int inStrideH, int inStrideV, uint8_t * outBuffer, int outStrideH, int outStrideV);

	int radius;  ///< Radius of region on which to compute ordered list.  Region has width = 2 * radius + 1.  That is, the region is always odd-sized.
	float order;   ///< Position in list (ordered from smallest to largest) from which to get resulting value, given as a fraction, where 0 means smallest and 1 means largest entry.
	int cacheSize;  ///< if non-zero, then split() will break problem into columns small enough to fit in cache.
  };

  /**
	 Finds standard deviation, average, minimum and maximum of intensity values.
	 An information gathering filter.  Returns the image unaltered and stores
	 the results in this object's state.
  **/
  class SHARED IntensityStatistics : public Filter
  {
  public:
	IntensityStatistics (bool ignoreZeros = false);

	virtual Image filter (const Image & image);

	double deviation (double average = NAN);  ///< Computes standard deviation around given average.  If average is not specified, then uses actual average inensity in image.  This is a convenience function for accessing results.

	double average;        ///< Average intensity value.
	double averageSquare;  ///< Average of squared intensity value.
	double minimum;        ///< Smallest intensity value.
	double maximum;        ///< Largest intensity value.
	int count;             ///< Number of pixels included in average.
	bool ignoreZeros;      ///< Don't include black pixels in count.
  };

  /**
	 An information gathering filter.  Finds standard deviation of intensity
	 values based on a given average value.
  **/
  class SHARED IntensityHistogram : public Filter
  {
  public:
	IntensityHistogram (const std::vector<float> & ranges);
	IntensityHistogram (float minimum, float maximum, int bins);

	virtual Image filter (const Image & image);

	int total () const;  ///< Add up all the counts.
	void dump (std::ostream & stream, bool center = false, bool percent = false) const;  ///< For each bin, print: {start of range | center of range} {count | percent of total}

	std::vector<float> ranges;  ///< The interval for bin n is [ranges[n], ranges[n+1]).  However, the last bin is a fully closed interval.  Bins are numbered from zero.  ranges has one more entry than counts.
	std::vector<int>   counts;  ///< The counts for each of the bins.
  };

  /**
	 Treating the image as a vector in high-dimensional space, normalize to given Euclidean length.
  **/
  class SHARED Normalize : public Filter
  {
  public:
	Normalize (double length = 1);

	virtual Image filter (const Image & image);

	double length;
  };

  /**
	 For floating point images, convert all values v to fabs(v).
  **/
  class SHARED AbsoluteValue : public Filter
  {
  public:
	virtual Image filter (const Image & image);
  };

  /**
	 If image format is floating point, then determine and apply an affine
	 transformation x <== ax + b to the pixel values.  If image format is
	 integer, it is passed thru unmodified.
  **/
  class SHARED Rescale : public Filter
  {
  public:
	Rescale (double a = 1.0, double b = 0);
	Rescale (const Image & image, bool useFullRange = true);  ///< Determines a transformation that pulls the pixel values into the range [0,1].  useFullRange indicates that min intensity should be 0 and max should be 1 after rescaling.
	virtual Image filter (const Image & image);

	double a;
	double b;
  };

  class SHARED Transform : public Filter
  {
  public:
	Transform (const Matrix<double> & A, bool inverse = false);  ///< Prefer double format for better inversion (when needed).
	Transform (const Matrix<double> & IA, const double scale);  ///< Divides first two columns by scale before using IA.  Just a convenience method.
	Transform (double angle);
	Transform (double scaleX, double scaleY);
	void initialize (const Matrix<double> & A, bool inverse = false);  ///< A should be at least 2x2

	virtual Image filter (const Image & image);

	void setPeg (float centerX = NAN, float centerY = NAN, int width = -1, int height = -1);
	void setWindow (float centerX, float centerY, int width = -1, int height = -1);
	void setWindowEdges (int left, int top, int right, int bottom);
	void twistCorner (const double inx, const double iny, double & l, double & r, double & t, double & b);  ///< Subroutine of prepare.
	void clip (const double dx0, const double dy0, const double dx1, const double dy1,
			   const double sx0, const double sy0, const double sx1, const double sy1,
			   const bool open,
			   double & dLo, double & dHi, bool & openHi, bool & openLo);
	void prepareResult (const Image & image, int & w, int & h, MatrixFixed<double,3,3> & H, int & lo, int & hi);  ///< Subroutine of filter ().  Finalizes parameters that control fit between source and destination images.
	Transform operator * (const Transform & that) const;

	MatrixFixed<double,3,3> A;  ///< Maps coordinates from input image to output image.
	MatrixFixed<double,3,3> IA;  ///< Inverse of A.  Maps coordinates from output image back to input image.
	bool inverse;  ///< Indicates whether the matrix given to the constructor was A or IA.

	bool peg;  ///< Indicates that (centerX, centerY) refers to source image rather than resulting image.
	bool defaultViewport;  ///< Indicates that viewport parameters are calculated rather than provided by user.
	float centerX;
	float centerY;
	int width;
	int height;
  };

  class SHARED TransformGauss : public Transform
  {
  public:
	TransformGauss (const Matrix<double> & A, bool inverse = false) : Transform (A, inverse),     G (GrayFloat) {needG = true; sigma = 0.5f;}
	TransformGauss (const Matrix<double> & A, const double scale)   : Transform (A, scale),       G (GrayFloat) {needG = true; sigma = 0.5f;}
	TransformGauss (double angle)                                   : Transform (angle),          G (GrayFloat) {needG = true; sigma = 0.5f;}
	TransformGauss (double scaleX, double scaleY)                   : Transform (scaleX, scaleY), G (GrayFloat) {needG = true; sigma = 0.5f;}
	TransformGauss (const Transform & that)                         : Transform (that),           G (GrayFloat) {needG = true; sigma = 0.5f;}
	void prepareG ();

	virtual Image filter (const Image & image);

	/** Desired blur of resampling kernel in terms of destination image.
		Default is 0.5.  It is impossible avoid adding blur to an image using
		Gaussian resampling, because the kernel must have some extent.
		Effective blur in the destination image is sqrt(oldBlur^2 + sigma^2),
		where oldBlur is the blur level of the source image projected through
		the transformation A.  In general, the effective blur is non-isotropic.
	**/
	double sigma;

	ImageOf<float> G;  ///< Gaussian used to calculate pixel values
	int Gshw;  ///< Width of half of Gaussian in source pixels
	int Gshh;  ///< Ditto for height
	int GstepX;  ///< Number of cells in Gaussian per one source pixel
	int GstepY;
	double sigmaX;  ///< Scale of Gaussian in source pixels
	double sigmaY;
	bool needG;  ///< Flag for lazy generation of G
  };

  /**
	 Scales an image up by an integer amount.  Basically, this is a stripped
	 down version of Transform that avoids blurring the image.
   **/
  class SHARED Zoom : public Filter
  {
  public:
	Zoom (int scaleX, int scaleY);

	virtual Image filter (const Image & image);

	int scaleX;
	int scaleY;
  };

  class SHARED Rotate180 : public Filter
  {
  public:
	virtual Image filter (const Image & image);
  };

  class SHARED Rotate90 : public Filter
  {
  public:
	Rotate90 (bool clockwise = false);  ///< clockwise in terms of image coordinate system, not in terms of displayed image on screen.  clockwise == true is same as rotation by -90 degrees in image coordinate system.

	virtual Image filter (const Image & image);

	bool clockwise;
  };

  class SHARED ClearAlpha : public Filter
  {
  public:
	ClearAlpha (unsigned int color = 0x0);

	virtual Image filter (const Image & image);

	unsigned int color;
  };
}


#endif
