#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class GaussianDerivativeThird ----------------------------------------------

GaussianDerivativeThird::GaussianDerivativeThird (int xy1, int xy2, int xy3, double sigmaX, double sigmaY, double angle, const PixelFormat & format, const BorderMode mode)
: ConvolutionDiscrete2D (format, mode)
{
  // Count how many of each kind of derivative we have been given.
  int dxcount = 0;
  int dycount = 0;
  if (xy1)
  {
	dycount++;
  }
  else
  {
	dxcount++;
  }
  if (xy2)
  {
	dycount++;
  }
  else
  {
	dxcount++;
  }
  if (xy3)
  {
	dycount++;
  }
  else
  {
	dxcount++;
  }

  if (sigmaY < 0)
  {
	sigmaY = sigmaX;
  }

  const double C = 2 * PI * sigmaX * sigmaY;
  int half = (int) (Gaussian2D::cutoff * (sigmaX >? sigmaY));
  int size = 2 * half + 1;

  ImageOf<double> temp (size, size, GrayDouble);

  double s = sin (- angle);
  double c = cos (- angle);

  double sigmaX2 = sigmaX * sigmaX;
  double sigmaY2 = sigmaY * sigmaY;
  double sigmaX4 = sigmaX2 * sigmaX2;
  double sigmaY4 = sigmaY2 * sigmaY2;
  double sigmaX6 = sigmaX4 * sigmaX2;
  double sigmaY6 = sigmaY4 * sigmaY2;

  for (int row = 0; row < size; row++)
  {
	for (int column = 0; column < size; column++)
	{
	  double u = column - half;
	  double v = row - half;
	  double x = u * c - v * s;
	  double y = u * s + v * c;

	  double value = (1 / C) * exp (-0.5 * (x * x / sigmaX2 + y * y / sigmaY2));
	  if (dxcount == 2)  // Gxxy = Gxyx = Gyxx
	  {
		value *= (x * x / sigmaX4 - 1 / sigmaX2) * (- y / sigmaY2);
	  }
	  else if (dycount == 2)  // Gyyx = Gyxy = Gxyy
	  {
		value *= (y * y / sigmaY4 - 1 / sigmaY2) * (- x / sigmaX2);
	  }
	  else if (dxcount == 3)  // Gxxx
	  {
		value *= - pow (x, 3) / sigmaX6 + 3 * x / sigmaX4;
	  }
	  else  // Gyyy
	  {
		value *= - pow (y, 3) / sigmaY6 + 3 * y / sigmaY4;
	  }
	  temp (column, row) = value;
	}
  }

  *this <<= temp * format;
}
