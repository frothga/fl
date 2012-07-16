/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Gaussian2D -----------------------------------------------------------

double Gaussian2D::cutoff = 4.0;

Gaussian2D::Gaussian2D (double sigma, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (mode, format)
{
  const double sigma2 = sigma * sigma;

  const double C = 1.0 / (TWOPI * sigma2);
  int h = (int) roundp (cutoff * sigma);  // "half" = distance from middle until cell values become insignificant
  int s = 2 * h + 1;  // "size" of kernel

  ImageOf<double> temp (s, s, GrayDouble);
  for (int row = 0; row < s; row++)
  {
	for (int column = 0; column < s; column++)
	{
	  double x = column - h;
	  double y = row - h;
	  temp (column, row) = C * exp (- (x * x + y * y) / (2 * sigma2));
	}
  }

  *((Image *) this) = temp * format;
  normalFloats ();
}
