#ifndef convolve_h
#define convolve_h


#include "fl/image.h"
#include "fl/matrix.h"
//#include "fl/search.h"
#include "fl/point.h"


// Coordinate system convention: For efficiency's sake, we follow the standard
// coordinate system for pixel rasters.  The origin is in the upper left.
// Positive x axis goes toward the right, and positive y axis goes downward.
// If you like to imagine that you are looking down at the plane where the
// pixels live, and that the positive z-axis comes out of the plane towards
// you, then this forms a "left-handed" coordinate system, which is universally
// condemned by purists.  However, it is better to imagine that the positive
// z-axis is heading away from you, in which case this is a proper right-handed
// system.  It is even better to realize that there is no z-axis whatsoever and
// accept the coordinate system in its own terms.

// All float-valued pixel coordinates follow the convention for
// Point (integral numbers refer to center of pixel).  Integer-valued
// pixel coordinates refer to either the entire pixel or to its center,
// depending on context.


namespace fl
{
  // 2D Convolutions ----------------------------------------------------------

  enum BorderMode  // What to do with resulting pixels when the kernel exceeds the border of the input image.
  {
	Crop,  // Resulting image is smaller than input image, and only contains "good" pixels.
	ZeroFill,  // Resulting image is full size, but "bad" pixels are set to zero.
	Boost,  // Treat the portion of the kernel that overlaps the image as if it were a full kernel.  This involves boosting the weights of the sub-kernel so the resulting pixel values are consistent with the "good" pixels.  Only appropriate for symmetric kernels that sum to 1.
	UseZeros  // Treat pixels beyond the border of source image as having value of zero and convolve with full kernel.
  };

  class Convolution2D : public Filter, public Image
  {
	// Stores the kernel as a discrete set of points (a raster).

  public:
	Convolution2D (const PixelFormat & format = GrayFloat,
				   const BorderMode mode = Crop);
	Convolution2D (const Image & image, const BorderMode mode = Crop);

	virtual Image filter (const Image & image);
	virtual double response (const Image & image, const Point & p) const;  // Strength of response of filter to image at input pixel (x, y).  Crop mode is treated as if ZeroFill mode, ie: no shift between input and output coordinate system.

	BorderMode mode;
  };

  class Gaussian2D : public Convolution2D
  {
  public:
	Gaussian2D (double sigma = 1.0,
				const PixelFormat & format = GrayFloat,
				const BorderMode mode = Crop);

	static double cutoff;  // Minimum number of standard deviations to include in a Gaussian kernel
  };

  class DifferenceOfGaussians : public Convolution2D
  {
  public:
	DifferenceOfGaussians (double sigmaPlus,
						   double sigmaMinus,
						   const PixelFormat & format = GrayFloat,
						   const BorderMode mode = Crop);
  };

  class GaussianDerivativeFirst : public Convolution2D
  {
  public:
	GaussianDerivativeFirst (int xy = 0,  // xy == 0 means x-derivative; xy != 0 means y-derivative
							 double sigmaX = 1.0,
							 double sigmaY = -1.0,
							 double angle = 0,
							 const PixelFormat & format = GrayFloat,
							 const BorderMode mode = Crop);
  };

  class GaussianDerivativeSecond : public Convolution2D
  {
  public:
	GaussianDerivativeSecond (int xy1 = 0,
							  int xy2 = 0,
							  double sigmaX = 1.0,
							  double sigmaY = -1.0,
							  double angle = 0,
							  const PixelFormat & format = GrayFloat,
							  const BorderMode mode = Crop);
  };

  class GaussianDerivativeThird : public Convolution2D
  {
  public:
	GaussianDerivativeThird (int xy1 = 0,
							 int xy2 = 0,
							 int xy3 = 0,
							 double sigmaX = 1.0,
							 double sigmaY = -1.0,
							 double angle = 0,
							 const PixelFormat & format = GrayFloat,
							 const BorderMode mode = Crop);
  };

  class Laplacian : public Convolution2D
  {
  public:
	Laplacian (double sigma = 1.0,
			   const PixelFormat & format = GrayFloat,
			   const BorderMode mode = Crop);

	double sigma;
  };


  // 1D Convolutions ----------------------------------------------------------

  enum Direction
  {
	Vertical,
	Horizontal
  };

  class Convolution1D : public Filter, public Image
  {
  public:
	Convolution1D (const PixelFormat & format = GrayFloat,
				   const Direction direction = Horizontal,
				   const BorderMode mode = Crop);
	Convolution1D (const Image & image,
				   const Direction direction = Horizontal,
				   const BorderMode mode = Crop);

	virtual Image filter (const Image & image);
	virtual double response (const Image & image, const Point & p) const;  // Strength of response of filter to image at pixel (x, y).

	Direction direction;
	BorderMode mode;
  };

  class Gaussian1D : public Convolution1D
  {
  public:
	Gaussian1D (double sigma = 1.0,
				const PixelFormat & format = GrayFloat,
				const Direction direction = Horizontal,
				const BorderMode mode = Crop);

	//virtual Image filter (const Image & image);
  };

  class GaussianDerivative1D : public Convolution1D
  {
  public:
	GaussianDerivative1D (double sigma = 1.0,
						  const PixelFormat & format = GrayFloat,
						  const Direction direction = Horizontal,
						  const BorderMode mode = Crop);
  };

  class GaussianDerivativeSecond1D : public Convolution1D
  {
  public:
	GaussianDerivativeSecond1D (double sigma = 1.0,
								const PixelFormat & format = GrayFloat,
								const Direction direction = Horizontal,
								const BorderMode mode = Crop);
  };


  // Interest operators -------------------------------------------------------

  class FilterHarris : public Filter
  {
  public:
	FilterHarris (double sigmaD = 1.0, double sigmaI = 1.4, const PixelFormat & format = GrayFloat);

	virtual Image filter (const Image & image);  // Relies on preprocess, process, and response to do all the work.
	virtual void preprocess (const Image & image);  // Extracts the autocorrelation matrix from the image.  Stores in xx, xy, and yy.
	virtual Image process ();  // Collects responses into an Image.
	virtual double response (const int x, const int y) const;  // For one pixel: sums autocorrelation matrix using G_I and then determines Harris response.  Position (x,y) is relative to border that is shifted by "offset" from original image border.

	double sigmaD;  // Derivation scale
	double sigmaI;  // Integration scale
	Gaussian2D           G_I;   // Gaussian for integration
	Gaussian1D           G1_I;  // seperated Gaussian for integration
	Gaussian1D           G1_D;  // seperated Gaussian for derivation (blurring pass)
	GaussianDerivative1D dG_D;  // seperated Gaussian for derivation
	Image xx;  // Components of the autocorrelation matrix.
	Image xy;  // These are built by preprocess ().
	Image yy;
	int offset;   // Total amount of one image border removed
	int offsetI;  // Border removed by integration
	int offsetD;  // Border removed by differentiation

	static const double alpha;

  protected:
	// If the blurring part of the separable Gaussian derivative kernel has
	// a larger radius, then the difference in pixels is stored in offset1.
	// If the derivative part has a larger radius, then the difference is kept
	// in offset2.  These help align the x and y derivative images correctly.
	int offset1;
	int offset2;
  };

  // Like FilterHarris, but uses a modified form of the response function that
  // returns the absolute value of the product of the eigenvectors.  The
  // regular FilterHarris only approximates this and at a different scale.
  class FilterHarrisEigen : public FilterHarris
  {
  public:
	FilterHarrisEigen (double sigmaD = 1.0, double sigmaI = 1.4, const PixelFormat & format = GrayFloat) : FilterHarris (sigmaD, sigmaI, format) {}

	virtual Image process ();
	virtual double response (const int x, const int y) const;
  };

  // Also similar to FilterHarris, but computes L_xx + L_yy instead.
  class FilterHessian : public Filter
  {
  public:
	FilterHessian (double sigmaD = 1.0, double sigmaI = 1.4, const PixelFormat & format = GrayFloat);

	virtual Image filter (const Image & image);
	virtual void preprocess (const Image & image);
	virtual Image process ();
	virtual double response (const int x, const int y) const;

	double sigmaD;  // Derivation scale
	double sigmaI;  // Integration scale
	Gaussian2D                 G_I;
	Gaussian1D                 G1_I;
	Gaussian1D                 G1_D;
	GaussianDerivativeSecond1D dG_D;
	Image trace;  // Trace of Hessian image
	int offset;
	int offsetI;
	int offsetD;

  protected:
	int offset1;
	int offset2;
  };


  // Misc -----------------------------------------------------------------------

  class NonMaxSuppress : public Filter
  {
  public:
	NonMaxSuppress (int half = 1);

	virtual Image filter (const Image & image);

	int half;  // Number of pixels away from center to check for local maxima.
	float maximum;  // Largest value found during last run of filter.
	float average;  // Average value found during last run of filter.
  };

  // An information gathering filter.  Finds average of intensity values.
  // Returns the image unaltered and stores the average in object's state.
  class IntensityAverage : public Filter
  {
  public:
	IntensityAverage (bool ignoreZeros = false);

	virtual Image filter (const Image & image);

	float average;  // The computed value
	bool ignoreZeros;  // Don't include black pixels in count for determining average.
  };

  // An information gathering filter.  Finds standard deviation of intensity
  // values based on a given average value.
  class IntensityDeviation : public Filter
  {
  public:
	IntensityDeviation (float average, bool ignoreZeros = false);

	virtual Image filter (const Image & image);

	float average;  // The given value
	float deviation;  // The computed value
	bool ignoreZeros;  // Don't include black pixels in count for determining deviation.
  };

  // Treating the image as a vector in high-dimensional space, normalize to given Euclidean length.
  class Normalize : public Filter
  {
  public:
	Normalize (double length = 1);

	virtual Image filter (const Image & image);

	double length;
  };

  // For floating point images, convert all values v to fabs(v).
  class AbsoluteValue : public Filter
  {
  public:
	virtual Image filter (const Image & image);
  };

  // If image format is floating point, then determine and apply an affine
  // transformation x <== ax + b to the pixel values.  If image format is
  // integer, it is passed thru unmodified.
  class Rescale : public Filter
  {
  public:
	Rescale (double a = 1.0, double b = 0);
	Rescale (const Image & image);  // Determines a transformation that pulls the pixel values into the range [0,1].
	virtual Image filter (const Image & image);

	double a;
	double b;
  };

  class Transform : public Filter
  {
  public:
	Transform (const Matrix<double> & A, bool inverse = false);  // Prefer double format for better inversion (when needed).
	Transform (const Matrix<double> & A, const double scale);  // Assumes A is inverse.  Divides first two columns by scale before using A.  Just a convenience method.
	Transform (double angle);
	Transform (double scaleX, double scaleY);
	void initialize (const Matrix<double> & A, bool inverse = false);  // A should be at least 2x2

	virtual Image filter (const Image & image);

	void setPeg (float centerX = -1, float centerY = -1, int width = -1, int height = -1);  // Set up viewport (of resulting image) so its center hits at a specified point in source image.  centerX == -1 means use center of original image.  centerY == -1 is similar.  width == -1 means use width of original image.  height == -1 is similar.
	void setWindow (float centerX, float centerY, int width = -1, int height = -1);  // Set up viewport so its center hits a specified point in what would otherwise be the resulting image.
	void prepareResult (const Image & image, Image & result, Matrix3x3<float> & H);  // Subroutine of filter ().  Finalizes parameters that control fit between source and destination images.
	Transform operator * (const Transform & that) const;

	//Matrix3x3<float> A;  // Maps coordinates from input image to output image.
	//Matrix3x3<float> IA;  // Inverse of A.  Maps coordinates from output image back to input image.
	//bool inverse;  // Indicates whether the matrix given to the constructor was A or IA.

	Matrix2x2<float> IA;  // Inverse of A.  Maps coordinates from output image back to input image.
	float translateX;
	float translateY;

	bool peg;  // Indicates that (centerX, centerY) refers to source image rather than resulting image.
	bool defaultViewport;  // Indicates that viewport parameters are calculated rather than provided by user.
	float centerX;
	float centerY;
	int width;
	int height;
  };

  class TransformGauss : public Transform
  {
  public:
	TransformGauss (const Matrix<double> & A, bool inverse = false) : Transform (A, inverse), G (GrayFloat) {needG = true;}
	TransformGauss (const Matrix<double> & A, const double scale) : Transform (A, scale), G (GrayFloat) {needG = true;}
	TransformGauss (double angle) : Transform (angle), G (GrayFloat) {needG = true;}
	TransformGauss (double scaleX, double scaleY) : Transform (scaleX, scaleY), G (GrayFloat) {needG = true;}
	TransformGauss (const Transform & that) : Transform (that), G (GrayFloat) {needG = true;}
	void prepareG ();

	virtual Image filter (const Image & image);

	ImageOf<float> G;  // Gaussian used to calculate pixel values
	int Gshw;  // Width of half of Gaussian in source pixels
	int Gshh;  // Ditto for height
	int GstepX;  // Number of cells in Gaussian per one source pixel
	int GstepY;
	float sigmaX;  // Scale of Gaussian in source pixels
	float sigmaY;
	bool needG;  // Flag for lazy generation of G
  };

  class Rotate180 : public Filter
  {
  public:
	virtual Image filter (const Image & image);
  };

  class Rotate90 : public Filter
  {
  public:
	Rotate90 (bool clockwise = false);  // clockwise in terms of image coordinate system, not in terms of displayed image on screen.  clockwise == true is same as rotation by -90 degrees in image coordinate system.

	virtual Image filter (const Image & image);

	bool clockwise;
  };

  class ClearAlpha : public Filter
  {
  public:
	ClearAlpha (unsigned int color = 0x0);

	virtual Image filter (const Image & image);

	unsigned int color;
  };
}


#endif
