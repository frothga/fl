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

Revision 1.2  2003/09/07 22:00:25  rothgang
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


// class DifferenceOfGaussians ------------------------------------------------

DifferenceOfGaussians::DifferenceOfGaussians (double sigmaPlus, double sigmaMinus, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (format, mode)
{
  Gaussian2D plus (sigmaPlus, mode, GrayDouble);
  Gaussian2D minus (sigmaMinus, mode, GrayDouble);
  Image temp = plus - minus;

  if (format == GrayChar)
  {
	ImageOf<unsigned char> tempC = temp * GrayChar;

	int top    = tempC.height - 1;
	int bottom = 0;
	int left   = tempC.width - 1;
	int right  = 0;

	for (int x = 0; x < width; x++)
	{
	  for (int y = 0; y < height; y++)
	  {
		if (tempC (x, y) != 128)  // 128 is the standard bias for char images.
		{
		  top    = min (top,    y);
		  bottom = max (bottom, y);
		  left   = min (left,   x);
		  right  = max (right,  x);
		}
	  }
	}

	int w = right - left + 1;
	int h = bottom - top + 1;
	bitblt (tempC, 0, 0, left, top, w, h);
  }
  else
  {
	*this <<= temp * format;
	normalFloats ();
  }
}
