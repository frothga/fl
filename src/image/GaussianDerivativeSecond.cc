#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class GaussianDerivativeSecond ---------------------------------------------

GaussianDerivativeSecond::GaussianDerivativeSecond (int xy1, int xy2, double sigmaX, double sigmaY, double angle, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (mode, format)
{
  if (sigmaY < 0)
  {
	sigmaY = sigmaX;
  }

  const double C = 2 * PI * sigmaX * sigmaY;
  int half = (int) (Gaussian2D::cutoff * max (sigmaX, sigmaY));
  int size = 2 * half + 1;

  ImageOf<double> temp (size, size, GrayDouble);

  double s = sin (- angle);
  double c = cos (- angle);

  double sigmaX2 = sigmaX * sigmaX;
  double sigmaY2 = sigmaY * sigmaY;
  double sigmaX4 = sigmaX2 * sigmaX2;
  double sigmaY4 = sigmaY2 * sigmaY2;

  for (int row = 0; row < size; row++)
  {
	for (int column = 0; column < size; column++)
	{
	  double u = column - half;
	  double v = row - half;
	  double x = u * c - v * s;
	  double y = u * s + v * c;

	  double value = (1 / C) * exp (-0.5 * (x * x / sigmaX2 + y * y / sigmaY2));
	  if (! xy1  &&  ! xy2)  // Gxx
	  {
		value *= x * x / sigmaX4 - 1 / sigmaX2;
	  }
	  else if (xy1  &&  xy2)  // Gyy
	  {
		value *= y * y / sigmaY4 - 1 / sigmaY2;
	  }
	  else  // Gxy = Gyx
	  {
		value *= x * y / (sigmaX2 * sigmaY2);
	  }
	  temp (column, row) = value;
	}
  }

  // When (if) adding code for GrayChar, be sure to keep size at least 3 * sigma.

  *this <<= temp * format;
  normalFloats ();
}
