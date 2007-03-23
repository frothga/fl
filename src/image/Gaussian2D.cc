/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

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


// class Gaussian2D -----------------------------------------------------------

double Gaussian2D::cutoff = 4.0;

Gaussian2D::Gaussian2D (double sigma, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (mode, format)
{
  const double sigma2 = sigma * sigma;

  const double C = 1.0 / (2 * PI * sigma2);
  int h = (int) rint (cutoff * sigma);  // "half" = distance from middle until cell values become insignificant
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

  *this <<= temp * format;
  normalFloats ();
}
