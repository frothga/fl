#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class GaussianDerivative1D -------------------------------------------------

GaussianDerivative1D::GaussianDerivative1D (double sigma, const PixelFormat & format, const Direction direction, const BorderMode mode)
: Convolution1D (GrayDouble, direction, mode)
{
  double sigma2 = sigma * sigma;
  double C = sqrt (2.0 * PI) * sigma;

  int h = (int) rint (Gaussian2D::cutoff * sigma);
  width = 2 * h + 1;
  height = 1;
  buffer.grow (width * GrayDouble.depth);

  ((double *) buffer)[h] = 0;
  for (int i = 1; i <= h; i++)
  {
	double x = i;
	double value = (1 / C) * exp (- x * x / (2 * sigma2)) * (- x / sigma2);
	((double *) buffer)[h + i] = value;
	((double *) buffer)[h - i] = -value;
  }

  *this *= format;
}
