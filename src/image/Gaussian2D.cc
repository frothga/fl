#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class Gaussian2D -----------------------------------------------------------

double Gaussian2D::cutoff = 4.0;

Gaussian2D::Gaussian2D (double sigma, const PixelFormat & format, const BorderMode mode)
: ConvolutionDiscrete2D (format, mode)
{
  double sigma2 = sigma * sigma;

  const double C = 2 * PI * sigma2;
  int h = (int) rint (cutoff * sigma);  // "half" = distance from middle until cell values become insignificant
  int s = 2 * h + 1;  // "size" of kernel

  ImageOf<double> temp (s, s, GrayDouble);
  for (int row = 0; row < s; row++)
  {
	for (int column = 0; column < s; column++)
	{
	  double x = column - h;
	  double y = row - h;
	  temp (column, row) = (1 / C) * exp (- (x * x + y * y) / (2 * sigma2));
	}
  }

  *this <<= temp * format;
}
