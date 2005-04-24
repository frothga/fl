/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class Laplacian ------------------------------------------------------------

Laplacian::Laplacian (double sigma, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (mode, format)
{
  // This constructs a Laplacian that is strictly circular.  It would not be
  // hard to make a version that separates sigmaX and sigmaY

  this->sigma = sigma;

  int half = (int) rint (Gaussian2D::cutoff * sigma);
  const int size = 2 * half + 1;

  ImageOf<double> temp (size, size, GrayDouble);

  double sigma2 = sigma * sigma;
  double sigma4 = sigma2 * sigma2;
  double C = 1.0 / (2.0 * PI * sigma2);
  for (int row = 0; row < size; row++)
  {
	for (int column = 0; column < size; column++)
	{
	  double x = column - half;
	  double y = row - half;
	  double x2 = x * x;
	  double y2 = y * y;
	  double value = C * exp (- (x2 + y2) / (2 * sigma2)) * ((x2 + y2) / sigma4 - 2 / sigma2);
	  temp (column, row) = value;
	}
  }

  *this <<= temp * format;
  normalFloats ();
}
