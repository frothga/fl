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


// class DifferenceOfGaussians ------------------------------------------------

DifferenceOfGaussians::DifferenceOfGaussians (double sigmaPlus, double sigmaMinus, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (format, mode)
{
  const double sigma2plus  = 2 * sigmaPlus  * sigmaPlus;
  const double sigma2minus = 2 * sigmaMinus * sigmaMinus;
  const double Cplus  = 1.0 / (M_PI * sigma2plus);
  const double Cminus = 1.0 / (M_PI * sigma2minus);

  int h = (int) roundp (Gaussian2D::cutoff * sigmaPlus);  // "half" = distance from middle until cell values become insignificant
  int s = 2 * h + 1;  // "size" of kernel

  ImageOf<double> temp (s, s, GrayDouble);
  for (int row = 0; row < s; row++)
  {
	for (int column = 0; column < s; column++)
	{
	  double x = column - h;
	  double y = row - h;
	  double r2 = x * x + y * y;
	  temp (column, row) = Cplus * exp (- r2 / sigma2plus) - Cminus * exp (- r2 / sigma2minus);
	}
  }

  *((Image *) this) = temp * format;
  normalFloats ();
  scale = crossover (sigmaPlus, sigmaMinus);
}

double
DifferenceOfGaussians::crossover (double a, double b)
{
  double a2 = a * a;
  double b2 = b * b;
  return sqrt (2 * log (b2 / a2) / (1 / a2 - 1 / b2));
}
