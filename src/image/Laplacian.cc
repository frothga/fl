/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/23 02:32:04  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.3  2004/05/03 20:14:12  rothgang
Rearrange parameters so border mode comes before format.  Add function to zero
out subnormal floats in kernel (improves speed).

Revision 1.2  2003/09/07 22:16:32  rothgang
Rename convolution base classes to allow for other methods of computation
besides discrete kernel.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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
