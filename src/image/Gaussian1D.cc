/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Gaussian1D -----------------------------------------------------------

Gaussian1D::Gaussian1D (double sigma, const BorderMode mode, const PixelFormat & format, const Direction direction)
: ConvolutionDiscrete1D (mode, GrayDouble, direction)
{
  double a = 1.0 / (sqrt (2.0) * sigma);
  double C = 1.0 / sqrt (4.0);

  int h = (int) roundp (Gaussian2D::cutoff * sigma);
  h = max (h, 1);
  resize (2 * h + 1, 1);

  double * kernel = (double *) ((PixelBufferPacked *) buffer)->base ();
  double last = erf (0.5 * a);
  kernel[h] = C * 2.0 * last;
  for (int i = 1; i <= h; i++)
  {
	double next = erf ((i + 0.5) * a);
	double value = C * (next - last);
	last = next;
	kernel[h + i] = value;
	kernel[h - i] = value;
  }

  *this *= format;
  normalFloats ();
}
