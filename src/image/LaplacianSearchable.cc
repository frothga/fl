#include "fl/convolve.h"
#include "fl/pi.h"


namespace fl
{
  // This Laplacian computes directly every time, rather than storing the
  // convolution as an Image.  This makes it more efficient in the case where
  // you need to construct a Laplacian of arbitrary size and use it only once,
  // and you need to do this repeatedly.
  // Needs to be updated to be a Searchable
  class LaplacianSearchable
  {
  public:
	LaplacianSearchable (const Image & image, const Point & center);

	virtual double value (Vector<double> point);

	Image image;
	float centerX;
	float centerY;
  };
}


using namespace std;
using namespace fl;


// class LaplacianSearchable --------------------------------------------------

// This class needs to be updated to match current interface of Searchable

LaplacianSearchable::LaplacianSearchable (const Image & image, const Point & center)
: image (image)
{
  centerX = center.x;
  centerY = center.y;
}

double
LaplacianSearchable::value (Vector<double> point)
{
  double sigma = point[0];
  double sigma2 = sigma * sigma;
  double sigma4 = sigma2 * sigma2;
  double C = 2.0 * PI * sigma2;

  double half = sigma * (Gaussian2D::cutoff >? 4.0);  // cutoff must be at least 4 to make values at different scales form a smooth curve
  int hl = 0 >? (int) rint (centerX - half);
  int hh = (image.width - 1) <? (int) rint (centerX + half);
  int vl = 0 >? (int) rint (centerY - half);
  int vh = (image.height - 1) <? (int) rint (centerY + half);

  if (*image.format == GrayFloat)
  {
	ImageOf<float> temp (image);
	float result = 0;
	for (int row = vl; row <= vh; row++)
	{
	  for (int column = hl; column <= hh; column++)
	  {
		float x = column - centerX;
		float y = row - centerY;
		float x2 = x * x;
		float y2 = y * y;
		float value = (1 / C) * expf (- (x2 + y2) / (2 * sigma2)) * ((x2 + y2) / sigma4 - 2 / sigma2);
		result += temp (column, row) * value;
	  }
	}
	// Values of a Searchable must be comparable across the domain.  The domain
	// in this case is scale, so we must do scale normalization.
	return fabsf (result * sigma2);
  }
  else if (*image.format == GrayDouble)
  {
	ImageOf<double> temp (image);
	double result = 0;
	for (int row = vl; row <= vh; row++)
	{
	  for (int column = hl; column <= hh; column++)
	  {
		double x = column - centerX;
		double y = row - centerY;
		double x2 = x * x;
		double y2 = y * y;
		double value = (1 / C) * exp (- (x2 + y2) / (2 * sigma2)) * ((x2 + y2) / sigma4 - 2 / sigma2);
		result += temp (column, row) * value;
	  }
	}
	return fabs (result * sigma2);
  }
  else
  {
	throw "LaplacianSearchable: unimplemented format";
  }
}
