#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class GaussianDerivativeFirst ----------------------------------------------

GaussianDerivativeFirst::GaussianDerivativeFirst (int xy, double sigmaX, double sigmaY, double angle, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (mode, format)
{
  if (sigmaY < 0)
  {
	sigmaY = sigmaX;
  }

  const double C = 1.0 / (2 * PI * sigmaX * sigmaY);
  int half = (int) rint (Gaussian2D::cutoff * max (sigmaX, sigmaY));
  int size = 2 * half + 1;

  ImageOf<double> temp (size, size, GrayDouble);

  double s = sin (- angle);
  double c = cos (- angle);

  double sigmaX2 = sigmaX * sigmaX;
  double sigmaY2 = sigmaY * sigmaY;

  for (int row = 0; row < size; row++)
  {
	for (int column = 0; column < size; column++)
	{
	  double u = column - half;
	  double v = row - half;
	  double x = u * c - v * s;
	  double y = u * s + v * c;

	  double value = C * exp (-0.5 * (x * x / sigmaX2 + y * y / sigmaY2));
	  if (xy)  // Gy
	  {
		value *= - y / sigmaY2;
	  }
	  else  // Gx
	  {
		value *= - x / sigmaX2;
	  }
	  temp (column, row) = value;
	}
  }

  *this <<= temp * format;
  normalFloats ();
}
