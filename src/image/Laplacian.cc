#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class Laplacian ------------------------------------------------------------

Laplacian::Laplacian (double sigma, const PixelFormat & format, const BorderMode mode)
: ConvolutionDiscrete2D (format, mode)
{
  // This constructs a Laplacian that is strictly circular.  It would not be
  // hard to make a version that separates sigmaX and sigmaY

  this->sigma = sigma;

  int half = (int) rint (Gaussian2D::cutoff * sigma);
  const int size = 2 * half + 1;

  ImageOf<double> temp (size, size, GrayDouble);

  double sigma2 = sigma * sigma;
  double sigma4 = sigma2 * sigma2;
  double C = 2.0 * PI * sigma2;
  for (int row = 0; row < size; row++)
  {
	for (int column = 0; column < size; column++)
	{
	  double x = column - half;
	  double y = row - half;
	  double x2 = x * x;
	  double y2 = y * y;
	  double value = (1 / C) * exp (- (x2 + y2) / (2 * sigma2)) * ((x2 + y2) / sigma4 - 2 / sigma2);
	  temp (column, row) = value;
	}
  }

  *this <<= temp * format;
}
