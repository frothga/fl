/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.6 thru 1.7 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.6  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.5  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.3  2004/05/03 20:14:12  rothgang
Rearrange parameters so border mode comes before format.  Add function to zero
out subnormal floats in kernel (improves speed).

Revision 1.2  2003/09/07 22:03:23  rothgang
Rename convolution base classes to allow for other methods of computation
besides discrete kernel.

Revision 1.1  2003/07/30 14:10:27  rothgang
Added a 1D second Gaussian derivative for use by FilterHessian.
-------------------------------------------------------------------------------
*/


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
  resize (2 * h + 1, 1);

  double * kernel = (double *) ((PixelBufferPacked *) buffer)->memory;
  for (int i = 0; i <= h; i++)
  {
	double x2 = i * i;
	double value = C * exp (- x2 / (2 * sigma2)) * (x2 / sigma2 - 1);
	kernel[h + i] = value;
	kernel[h - i] = value;
  }

  *this *= format;
  normalFloats ();
}
