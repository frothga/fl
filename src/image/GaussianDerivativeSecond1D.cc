#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class GaussianDerivativeSecond1D -------------------------------------------

GaussianDerivativeSecond1D::GaussianDerivativeSecond1D (double sigma, const BorderMode mode, const PixelFormat & format, const Direction direction)
: ConvolutionDiscrete1D (mode, GrayDouble, direction)
{
  double sigma2 = sigma * sigma;
  double C = 1.0 / (sqrt (2.0 * PI) * sigma * sigma2);

  int h = (int) rint (Gaussian2D::cutoff * sigma);
  width = 2 * h + 1;
  height = 1;
  buffer.grow (width * GrayDouble.depth);

  for (int i = 0; i <= h; i++)
  {
	double x2 = i * i;
	double value = C * exp (- x2 / (2 * sigma2)) * (x2 / sigma2 - 1);
	((double *) buffer)[h + i] = value;
	((double *) buffer)[h - i] = value;
  }

  *this *= format;
  normalFloats ();
}
